// Copyright 2015-2018 Directive Games Limited - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FJsonValue;
class FJsonObject;

namespace rapidjson
{
	enum Type
	{
		kNullType = 0,      //!< null
		kFalseType = 1,     //!< false
		kTrueType = 2,      //!< true
		kObjectType = 3,    //!< object
		kArrayType = 4,     //!< array
		kStringType = 5,    //!< string
		kNumberType = 6     //!< number
	};
}


class JSONARCHIVE_API JsonValue
{
public:
	JsonValue() = default;
	JsonValue(rapidjson::Type Type);
	JsonValue(const TSharedPtr<FJsonValue>& InValue);
	
	FString ToString() const;
	
	bool IsNull() const;
	bool IsObject() const;
	bool IsString() const;
	bool IsUint() const { return IsNumber(); }
	bool IsInt64() const { return IsNumber(); }
	bool IsUint64() const { return IsNumber(); }
	bool IsDouble() const { return IsNumber(); }
	bool IsInt() const { return IsNumber(); }
	bool IsBool() const;
	bool IsArray() const;
	
	FString GetString() const;
	uint GetUint() const;
	int64 GetInt64() const;
	uint64 GetUint64() const;
	double GetDouble() const;
	int GetInt() const;
	bool GetBool() const;
	
	void SetString(const FString& Value);
	void SetUint(uint Value);
	void SetInt64(int64 Value);
	void SetUint64(uint64 Value);
	void SetDouble(double Value);
	void SetBool(bool Value);
	void SetInt(int Value);
	void SetArray();
	
	TArray<JsonValue> GetArray() const;
	
	void PushBack(const JsonValue& Value);
	
	void SetObject();
	
	JsonValue FindField(const FString& Name) const;
	
	operator bool() const;
	
	JsonValue operator[](const FString& Name) const;
	JsonValue operator[](const TCHAR* Name) const;
	
	void CopyFrom(const JsonValue& Other);
	
	bool HasField(const FString& Name) const;
	void SetField(const FString& Name, float Value);
	void SetField(const FString& Name, double Value);
	void SetField(const FString& Name, int Value);
	void SetField(const FString& Name, unsigned Value);
	void SetField(const FString& Name, int64_t Value);
	void SetField(const FString& Name, uint64_t Value);
	void SetField(const FString& Name, const JsonValue& Value);
	void SetField(const FString& Name, const FString& Value);
	void SetField(const JsonValue& Name, const JsonValue& Value);
	
	TMap<FString, JsonValue> GetObject() const;
	
	int MemberCount() const;
	
	const TSharedPtr<FJsonValue>& GetInternalValue() const { return InternalValue; }
	
protected:
	TSharedPtr<FJsonObject> AsObject() const;
	bool IsNumber() const;
	void SetNumber(double Number);
	void SetNumberField(const FString& Name, double Value);
	
protected:
	TSharedPtr<FJsonValue> InternalValue;
};


class JSONARCHIVE_API JsonDocument : public JsonValue
{
public:
	void Parse(const FString& JsonString);
	bool HasParseError();
	
	int GetErrorOffset() const { return 0; }
	int GetParseError() const { return 0; }
};


class JSONARCHIVE_API JsonValueWrapper
{
public:
	JsonValue value;
	
	JsonValueWrapper() = default;
	JsonValueWrapper(const JsonValueWrapper& other);
	JsonValueWrapper(JsonValueWrapper&& other);
	
	JsonValueWrapper(const JsonValue& other);
	JsonValueWrapper(JsonValue&& other);
	
	JsonValueWrapper& operator=(const JsonValueWrapper& other);
	JsonValueWrapper& operator=(JsonValueWrapper&& other);
};
