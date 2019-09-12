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
	bool IsInt32() const { return IsNumber(); }
	bool IsUint32() const { return IsNumber(); }
	bool IsInt64() const { return IsNumber(); }
	bool IsUint64() const { return IsNumber(); }
	bool IsDouble() const { return IsNumber(); }
	bool IsBool() const;
	bool IsArray() const;

	FString GetString() const;
	int32 GetInt32() const;
	uint32 GetUint32() const;
	int64 GetInt64() const;
	uint64 GetUint64() const;
	double GetDouble() const;
	bool GetBool() const;

	void SetString(const FString& Value);
	void SetInt32(int32 Value);
	void SetUint32(uint32 Value);
	void SetInt64(int64 Value);
	void SetUint64(uint64 Value);
	void SetDouble(double Value);
	void SetBool(bool Value);
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
	void SetField(const FString& Name, int32 Value);
	void SetField(const FString& Name, uint32 Value);
	void SetField(const FString& Name, int64 Value);
	void SetField(const FString& Name, uint64 Value);
	void SetField(const FString& Name, const JsonValue& Value);
	void SetField(const FString& Name, const FString& Value);
	void SetField(const JsonValue& Name, const JsonValue& Value);

	TMap<FString, JsonValue> GetObject() const;

	int MemberCount() const;

	const TSharedPtr<FJsonValue>& GetInternalValue() const { return InternalValue; }

protected:
	TSharedPtr<FJsonObject> AsObject() const;
	TArray<TSharedPtr<FJsonValue>> AsArray() const;
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
