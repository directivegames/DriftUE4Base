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

#include "CoreMinimal.h"

#include "JsonValueWrapper.h"

#include <type_traits>

DEFINE_LOG_CATEGORY_STATIC(LogDriftJson, Log, All);


class JsonArchive;


class JSONARCHIVE_API SerializationContext
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
	bool SerializeProperty(const TCHAR* propertyName, T& property);
	
	template<typename T>
	bool SerializePropertyOptional(const TCHAR* propertyName, T& property, const T& defaultValue);
	
	template<typename T>
	bool SerializePropertyOptional(const TCHAR* propertyName, T& property);
	
	void SetVersion(int version);
	int GetVersion();
	
	template<typename T>
	bool SerializeOptionalProperty(const TCHAR* propertyName, T& property);
	
private:
	JsonArchive& archive;
	JsonValue& value;
};


/**
 * Serialize a named C++ property with the corresponding json value
 */
#define SERIALIZE_PROPERTY(context, propertyName) context.SerializeProperty(TEXT(#propertyName), propertyName)

// TODO: Legacy name, replace with SERIALIZE_OPTIONAL_PROPERTY below
#define SERIALIZE_PROPERTY_OPTIONAL(context, propertyName, defaultValue) context.SerializePropertyOptional(TEXT(#propertyName), propertyName, defaultValue)
#define SERIALIZE_PROPERTY_OPTIONAL_NODEFAULT(context, propertyName) context.SerializePropertyOptional(TEXT(#propertyName), propertyName)

#define SERIALIZE_OPTIONAL_PROPERTY(context, propertyName) context.SerializeOptionalProperty(TEXT(#propertyName), propertyName)


class JSONARCHIVE_API JsonArchive
{
public:
	/**
	 * Load a json string into a json document, return if the parsing succeeds
	 */
	static bool LoadDocument(const TCHAR* jsonString, JsonDocument& document)
	{
		document.Parse(jsonString);
		return !document.HasParseError();
	}
	
	/**
	 * Load a json string into a C++ object, return if the parsing succeeds
	 */
	template<class T>
	static bool LoadObject(const TCHAR* jsonString, T& object)
	{
		return LoadObject(jsonString, object, true);
	}
	
	template<class T>
	static bool LoadObject(const TCHAR* jsonString, T& object, bool logErrors)
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
		return jValue.ToString();
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
			if (jValue.IsInt32())
			{
				cEnum = (T)jValue.GetInt32();
				success = true;
			}
		}
		else
		{
			jValue.SetInt32((int)cEnum);
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
					jArray.PushBack(jValue);
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
				if (SerializeObject(member.Key, key) && SerializeObject(member.Value, value))
				{
					cValue[key] = value;
				}
			}
		}
		else
		{
			jValue.SetObject();
			for (auto& itr : cValue)
			{
				JsonValue key, value;
				if (SerializeObject(key, itr.Key) && SerializeObject(value, itr.Value))
				{
					jValue.SetField(key, value);
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
	bool SerializeProperty(JsonValue& parent, const TCHAR* propName, T& cValue)
	{
		bool success = false;
		
		if (isLoading_)
		{
			JsonValue v = parent[propName];
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
				parent.SetField(propName, value);
			}
		}
		
		return success;
	}
	
	bool IsLoading() const { return isLoading_; }
	
	template<typename TValue>
	static void AddMember(JsonValue& parent, const FString& name, const TValue& value)
	{
		JsonValue temp;
		if (SaveObject(value, temp))
		{
			AddMember(parent, name, MoveTemp(temp));
		}
	}
	
	static void AddMember(JsonValue& parent, const FString& name, float value)
	{
		parent.SetField(name, value);
	}
	
	static void AddMember(JsonValue& parent, const FString& name, double value)
	{
		parent.SetField(name, value);
	}
	
	static void AddMember(JsonValue& parent, const FString& name, int32 value)
	{
		parent.SetField(name, value);
	}
	
	static void AddMember(JsonValue& parent, const FString& name, uint32 value)
	{
		parent.SetField(name, value);
	}
	
	static void AddMember(JsonValue& parent, const FString& name, int64 value)
	{
		parent.SetField(name, value);
	}
	
	static void AddMember(JsonValue& parent, const FString& name, uint64 value)
	{
		parent.SetField(name, value);
	}
	
	static void AddMember(JsonValue& parent, const FString& name, JsonValue&& value)
	{
		parent.SetField(name, value);
	}
	
	static void AddMember(JsonValue& parent, const FString& name, const FString& value)
	{
		parent.SetField(name, value);
	}
	
	static void AddMember(JsonValue& parent, const TCHAR* name, const TCHAR* value)
	{
		parent.SetField(name, value);
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
	bool logErrors_;
};

template<>
JSONARCHIVE_API bool JsonArchive::SerializeObject<int>(JsonValue& jValue, int& cValue);

template<>
JSONARCHIVE_API bool JsonArchive::SerializeObject<uint8>(JsonValue& jValue, uint8& cValue);

template<>
JSONARCHIVE_API bool JsonArchive::SerializeObject<unsigned>(JsonValue& jValue, unsigned& cValue);

template<>
JSONARCHIVE_API bool JsonArchive::SerializeObject<long long>(JsonValue& jValue, long long& cValue);

template<>
JSONARCHIVE_API bool JsonArchive::SerializeObject<unsigned long long>(JsonValue& jValue, unsigned long long& cValue);

template<>
JSONARCHIVE_API bool JsonArchive::SerializeObject<long>(JsonValue& jValue, long& cValue);

template<>
JSONARCHIVE_API bool JsonArchive::SerializeObject<float>(JsonValue& jValue, float& cValue);

template<>
JSONARCHIVE_API bool JsonArchive::SerializeObject<double>(JsonValue& jValue, double& cValue);

template<>
JSONARCHIVE_API bool JsonArchive::SerializeObject<bool>(JsonValue& jValue, bool& cValue);

template<>
JSONARCHIVE_API bool JsonArchive::SerializeObject<FString>(JsonValue& jValue, FString& cValue);

template<>
JSONARCHIVE_API bool JsonArchive::SerializeObject<FName>(JsonValue& jValue, FName& cValue);

template<>
JSONARCHIVE_API bool JsonArchive::SerializeObject<FDateTime>(JsonValue& jValue, FDateTime& cValue);

template<>
JSONARCHIVE_API bool JsonArchive::SerializeObject<FTimespan>(JsonValue& jValue, FTimespan& cValue);

template<>
JSONARCHIVE_API bool JsonArchive::SerializeObject<JsonValue>(JsonValue& jValue, JsonValue& cValue);

template<>
JSONARCHIVE_API bool JsonArchive::SerializeObject<JsonValueWrapper>(JsonValue& jValue, JsonValueWrapper& cValue);

template<typename T>
bool SerializationContext::SerializeProperty(const TCHAR* propertyName, T& property)
{
	return archive.SerializeProperty(value, propertyName, property);
}

template<typename T>
bool SerializationContext::SerializePropertyOptional(const TCHAR* propertyName, T& property, const T& defaultValue)
{
	if (archive.IsLoading())
	{
		if (value.HasField(propertyName))
		{
			return archive.SerializeProperty(value, propertyName, property);
		}
		else
		{
			property = defaultValue;
			return true;
		}
	}
	else
	{
		if (property != defaultValue)
		{
			return archive.SerializeProperty(value, propertyName, property);
		}
		else
		{
			return true;
		}
	}
}

template<typename T>
bool SerializationContext::SerializePropertyOptional(const TCHAR* propertyName, T& property)
{
	if (archive.IsLoading())
	{
		if (value.HasField(propertyName))
		{
			return archive.SerializeProperty(value, propertyName, property);
		}
		else
		{
			return true;
		}
	}
	else
	{
		return archive.SerializeProperty(value, propertyName, property);
	}
}

template<typename T>
bool SerializationContext::SerializeOptionalProperty(const TCHAR* propertyName, T& property)
{
	if (archive.IsLoading())
	{
		if (value.HasField(propertyName))
		{
			return archive.SerializeProperty(value, propertyName, property);
		}
		return true;
	}
	return archive.SerializeProperty(value, propertyName, property);
}


