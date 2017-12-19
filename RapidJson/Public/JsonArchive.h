/**
* This file is part of the Drift Unreal Engine Integration.
*
* Copyright (C) 2016-2017 Directive Games Limited. All Rights Reserved.
*
* Licensed under the MIT License (the "License");
*
* You may not use this file except in compliance with the License.
* You may obtain a copy of the license in the LICENSE file found at the top
* level directory of this module, and at https://mit-license.org/
*/

#pragma once

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/allocators.h"

#include <type_traits>


typedef rapidjson::GenericValue<rapidjson::UTF16<>, rapidjson::CrtAllocator> JsonValue;
typedef rapidjson::GenericDocument<rapidjson::UTF16<>, rapidjson::CrtAllocator> JsonDocument;
typedef rapidjson::GenericStringBuffer<rapidjson::UTF16<>, rapidjson::CrtAllocator> JsonStringBuffer;
typedef rapidjson::Writer<JsonStringBuffer, rapidjson::UTF16<>, rapidjson::UTF16<>> JsonWriter;


class JsonArchive;


DEFINE_LOG_CATEGORY_STATIC(LogDriftJson, Log, All);


class RAPIDJSON_API SerializationContext
{
public:
    SerializationContext(JsonArchive& a, JsonValue& v)
    : archive{ a }
    , value{ v }
    {
    }
    
    bool IsLoading() const;
    JsonValue& GetValue() const { return value; }
    
    template<typename T>
    bool SerializeProperty(const wchar_t* propertyName, T& property);
    
    template<typename T>
    bool SerializeOptionalProperty(const wchar_t* propertyName, T& property);

private:
    JsonArchive& archive;
    JsonValue& value;
};


/**
* Serialize a named C++ property with the corresponding json value
*/
#define SERIALIZE_PROPERTY(context, propertyName) context.SerializeProperty(L###propertyName, propertyName)
#define SERIALIZE_OPTIONAL_PROPERTY(context, propertyName) context.SerializeOptionalProperty(L###propertyName, propertyName)


class RAPIDJSON_API JsonArchive
{
public:
    /**
    * Load a json string into a json document, return if the parsing succeeds
    */
    static bool LoadDocument(const wchar_t* jsonString, JsonDocument& document)
    {
        document.Parse(jsonString);
        return !document.HasParseError();
    }

    /**
    * Load a json string into a C++ object, return if the parsing succeeds
    */
    template<class T>
    static bool LoadObject(const wchar_t* jsonString, T& object)
    {
        return LoadObject(jsonString, object, true);
    }

    template<class T>
    static bool LoadObject(const wchar_t* jsonString, T& object, bool logErrors)
    {
        JsonDocument doc;

        if (LoadDocument(jsonString, doc))
        {
            JsonArchive reader(true, logErrors);
            return reader.SerializeObject(doc, object);
        }

        return false;
    }

    /**
    * Load a json value into a C++ object, return if the parsing succeeds
    */
    template<class T>
    static bool LoadObject(const JsonValue& value, T& object)
    {
        JsonArchive reader(true);
        return reader.SerializeObject(const_cast<JsonValue&>(value), object);
    }

    static FString ToString(const JsonValue& jValue)
    {
        JsonStringBuffer buffer;
        JsonWriter writer(buffer);
        jValue.Accept(writer);
        return buffer.GetString();
    }

    /**
    * Serialize a C++ object into a json string, return if the operation succeeds
    */
    template<class T>
    static bool SaveObject(const T& object, FString& jsonString)
    {
        JsonValue jValue;
        if (SaveObject(object, jValue))
        {
            jsonString = ToString(jValue);
            return true;
        }

        return false;
    }

    template<class T>
    static bool SaveObject(const T& object, JsonValue& jValue)
    {
        JsonArchive writer(false);
        return writer.SerializeObject(jValue, const_cast<T&>(object));
    }

    /**
    * Serialize between a Json and C++ object
    */
    template<class T>
    typename std::enable_if<!std::is_enum<T>::value, bool>::type
    SerializeObject(JsonValue& jValue, T& cValue)
    {
        auto context = SerializationContext(*this, jValue);

        if (isLoading_)
        {
            if (!jValue.IsObject())
            {
                return false;
            }
        }
        else
        {
            jValue.SetObject();
        }

        return cValue.Serialize(context);
    }
    
    template<class T>
    typename std::enable_if<std::is_enum<T>::value, bool>::type
    SerializeObject(JsonValue& jValue, T& cEnum)
    {
        bool success = false;
        if (isLoading_)
        {
            if (jValue.IsInt())
            {
                cEnum = (T)jValue.GetInt();
                success = true;
            }
        }
        else
        {
            jValue.SetInt((int)cEnum);
            success = true;
        }
        
        return success;
    }
    
    template<class T>
    bool SerializeObject(JsonValue& jArray, TArray<T>& cValue)
    {
        bool success = false;
        
        if (isLoading_)
        {
            if (jArray.IsArray())
            {
                cValue.Empty();
                for (auto& element : jArray.GetArray())
                {
                    T elem;
                    if (SerializeObject(element, elem))
                    {
                        cValue.Add(MoveTemp(elem));
                    }
                    else
                    {
                        if (logErrors_)
                        {
                            UE_LOG(LogDriftJson, Warning, TEXT("Failed to parse array entry: %s"), *ToString(element));
                        }
                        return false;
                    }
                }
                success = true;
            }
        }
        else
        {
            // todo: fix this up:
            jArray.SetArray();
            
            for (auto& elem : cValue)
            {
                JsonValue jValue;
                if (SerializeObject(jValue, elem))
                {
                    jArray.PushBack(jValue, allocator_);
                }
            }
            
            success = true;
        }
        
        return success;
    }

    /**
     * Serialize between a Json and a TUniquePtr managed C++ object
     */
    template<class T>
    bool SerializeObject(JsonValue& jValue, TUniquePtr<T>& cValue)
    {
        auto context = SerializationContext(*this, jValue);
        
        if (isLoading_)
        {
            // Loading is not supported
            if (!jValue.IsObject())
            {
                return false;
            }
            return false;
        }
        else
        {
            check(cValue.Get());
            jValue.SetObject();
        }
        
        return (*cValue).Serialize(context);
    }

    /**
     * Serialize between a Json and a TMap<> object
     */
    template<class TKey, class TValue>
    bool SerializeObject(JsonValue& jValue, TMap<TKey, TValue>& cValue)
    {
        auto context = SerializationContext(*this, jValue);
        
        if (isLoading_)
        {
            if (!jValue.IsObject())
            {
                return false;
            }
            
            for (auto& member : jValue.GetObject())
            {
                TKey key;
                TValue value;
                if (SerializeObject(member.name, key) && SerializeObject(member.value, value))
                {
                    cValue[key] = value;
                }
            }
        }
        else
        {
            jValue.SetObject();
            for (const auto& itr : cValue)
            {
                JsonValue key, value;
                if (SerializeObject(key, itr.Key) && SerializeObject(value, itr.Value))
                {
                    jValue.AddMember(key, value, allocator_);
                }
            }
        }
        
        return true;
    }
    
    /**
    * Serialize between a json and C++ value
    * The json value is assumed to be named propName under the parent value
    */
    template<class T>
    bool SerializeProperty(JsonValue& parent, const wchar_t* propName, T& cValue)
    {
        bool success = false;
        
        if (isLoading_)
        {
            JsonValue& v = parent[propName];
            success = SerializeObject(v, cValue);
            if (!success && logErrors_)
            {
                UE_LOG(LogDriftJson, Warning, TEXT("Failed to serialize property: %s from: %s"), propName, *ToString(parent));
            }
        }
        else
        {
            JsonValue value;
            success = SerializeObject(value, cValue);
            
            if (success)
            {
                parent.AddMember(NewValue(propName), value, allocator_);
            }
        }
        
        return success;
    }

    bool IsLoading() const { return isLoading_; }
    
    static rapidjson::CrtAllocator& Allocator() { return allocator_; }

    static JsonValue NewValue(const wchar_t* str)
    {
        return JsonValue(str, allocator_);
    }

    template<typename TValue>
    static void AddMember(JsonValue& parent, const FString& name, const TValue& value)
    {
        parent.AddMember(JsonValue(*name, name.Len(), allocator_), JsonValue(value, allocator_), allocator_);
    }

    static void AddMember(JsonValue& parent, const FString& name, float value)
    {
        parent.AddMember(JsonValue(*name, name.Len(), allocator_), JsonValue(value), allocator_);
    }
    
    static void AddMember(JsonValue& parent, const FString& name, double value)
    {
        parent.AddMember(JsonValue(*name, name.Len(), allocator_), JsonValue(value), allocator_);
    }
    
    static void AddMember(JsonValue& parent, const FString& name, int value)
    {
        parent.AddMember(JsonValue(*name, name.Len(), allocator_), JsonValue(value), allocator_);
    }
    
    static void AddMember(JsonValue& parent, const FString& name, JsonValue&& value)
    {
        parent.AddMember(JsonValue(*name, name.Len(), allocator_), Forward<JsonValue>(value), allocator_);
    }
    
//private:
    JsonArchive(bool loading)
        : isLoading_{ loading }
        , logErrors_{ true }
    {}

    JsonArchive(bool loading, bool logErrors)
        : isLoading_{ loading }
        , logErrors_{ logErrors }
    {}

private:
    bool isLoading_;
    static rapidjson::CrtAllocator allocator_;
    bool logErrors_;
};

template<>
RAPIDJSON_API bool JsonArchive::SerializeObject<int>(JsonValue& jValue, int& cValue);

template<>
RAPIDJSON_API bool JsonArchive::SerializeObject<uint8>(JsonValue& jValue, uint8& cValue);

template<>
RAPIDJSON_API bool JsonArchive::SerializeObject<unsigned>(JsonValue& jValue, unsigned& cValue);

template<>
RAPIDJSON_API bool JsonArchive::SerializeObject<long long>(JsonValue& jValue, long long& cValue);

template<>
RAPIDJSON_API bool JsonArchive::SerializeObject<float>(JsonValue& jValue, float& cValue);

template<>
RAPIDJSON_API bool JsonArchive::SerializeObject<double>(JsonValue& jValue, double& cValue);

template<>
RAPIDJSON_API bool JsonArchive::SerializeObject<bool>(JsonValue& jValue, bool& cValue);

template<>
RAPIDJSON_API bool JsonArchive::SerializeObject<FString>(JsonValue& jValue, FString& cValue);

template<>
RAPIDJSON_API bool JsonArchive::SerializeObject<FName>(JsonValue& jValue, FName& cValue);

template<>
RAPIDJSON_API bool JsonArchive::SerializeObject<FDateTime>(JsonValue& jValue, FDateTime& cValue);

template<>
RAPIDJSON_API bool JsonArchive::SerializeObject<FTimespan>(JsonValue& jValue, FTimespan& cValue);

template<>
RAPIDJSON_API bool JsonArchive::SerializeObject<JsonValue>(JsonValue& jValue, JsonValue& cValue);


template<typename T>
bool SerializationContext::SerializeProperty(const wchar_t* propertyName, T& property)
{
    return archive.SerializeProperty(value, propertyName, property);
}


template<typename T>
bool SerializationContext::SerializeOptionalProperty(const wchar_t* propertyName, T& property)
{
    if (archive.IsLoading())
    {
        if (value.HasMember(propertyName))
        {
            return archive.SerializeProperty(value, propertyName, property);
        }
        return true;
    }
    return archive.SerializeProperty(value, propertyName, property);
}
