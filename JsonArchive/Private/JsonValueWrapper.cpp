// Copyright 2015-2018 Directive Games Limited - All Rights Reserved.

#include "JsonValueWrapper.h"
#include "Json.h"


JsonValue::JsonValue(const TSharedPtr<FJsonValue>& InValue):
InternalValue(InValue)
{
}

JsonValue::JsonValue(rapidjson::Type Type)
{
	FString ValueString;
	
	switch(Type)
	{
		case rapidjson::kNullType:
			InternalValue = nullptr;
			break;
			
		case rapidjson::kFalseType:
			InternalValue = MakeShared<FJsonValueBoolean>(false);
			break;
			
		case rapidjson::kTrueType:
			InternalValue = MakeShared<FJsonValueBoolean>(true);
			break;
			
		case rapidjson::kObjectType:
			SetObject();
			break;
			
		case rapidjson::kArrayType:
			SetArray();
			break;
			
		case rapidjson::kStringType:
			InternalValue = MakeShared<FJsonValueString>(ValueString);
			break;
			
		case rapidjson::kNumberType:
			InternalValue = MakeShared<FJsonValueNumber>(0.0);
			break;
	}
}

FString JsonValue::ToString() const
{
	if (!InternalValue)
	{
		return {};
	}
	
	if (IsObject())
	{
		const auto& JsonObject = InternalValue->AsObject();
		FString JsonString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
		return MoveTemp(JsonString);
	}
	else
	{
		return GetString();
	}
}

bool JsonValue::IsNull() const
{
	return !InternalValue || InternalValue->Type == EJson::Null;
}

bool JsonValue::IsObject() const
{
	return InternalValue && InternalValue->Type == EJson::Object;
}

bool JsonValue::IsString() const
{
	return InternalValue && InternalValue->Type == EJson::String;
}

bool JsonValue::IsBool() const
{
	return InternalValue && InternalValue->Type == EJson::Boolean;
}

bool JsonValue::IsArray() const
{
	return InternalValue && InternalValue->Type == EJson::Array;
}

FString JsonValue::GetString() const
{
	if (IsString())
	{
		return InternalValue->AsString();
	}
	
	return {};
}

uint JsonValue::GetUint() const
{
	uint Number = 0;
	if (InternalValue)
	{
		InternalValue->TryGetNumber(Number);
	}
	return Number;
}

int64 JsonValue::GetInt64() const
{
	int64 Number = 0;
	if (InternalValue)
	{
		InternalValue->TryGetNumber(Number);
	}
	return Number;
}

uint64 JsonValue::GetUint64() const
{
	return (uint64)GetInt64();
}

double JsonValue::GetDouble() const
{
	double Number = 0;
	if (InternalValue)
	{
		InternalValue->TryGetNumber(Number);
	}
	return Number;
}

int JsonValue::GetInt() const
{
	int Number = 0;
	if (InternalValue)
	{
		InternalValue->TryGetNumber(Number);
	}
	return Number;
}

bool JsonValue::GetBool() const
{
	if (IsBool())
	{
		return InternalValue->AsBool();
	}
	return false;
}

void JsonValue::SetString(const FString& Value)
{
	InternalValue = MakeShared<FJsonValueString>(Value);
}

void JsonValue::SetUint(uint Value)
{
	SetNumber(Value);
}

void JsonValue::SetInt64(int64 Value)
{
	SetNumber(Value);
}

void JsonValue::SetUint64(uint64 Value)
{
	SetNumber(Value);
}

void JsonValue::SetDouble(double Value)
{
	SetNumber(Value);
}

void JsonValue::SetBool(bool Value)
{
	InternalValue = MakeShared<FJsonValueBoolean>(Value);
}

void JsonValue::SetInt(int Value)
{
	SetNumber(Value);
}

void JsonValue::SetArray()
{
	TArray<TSharedPtr<FJsonValue>> ValueArray;
	InternalValue = MakeShared<FJsonValueArray>(ValueArray);
}

TArray<JsonValue> JsonValue::GetArray() const
{
	if (IsArray())
	{
		TArray<JsonValue> ValueArray;
		for (const auto& Element : InternalValue->AsArray())
		{
			ValueArray.Add(JsonValue(Element));
		}
		return MoveTemp(ValueArray);
	}
	
	return {};
}

void JsonValue::PushBack(const JsonValue& Value)
{
	if (IsArray())
	{
		const TArray<TSharedPtr<FJsonValue>>* ValueArray = nullptr;
		if (InternalValue->TryGetArray(ValueArray))
		{
			const_cast<TArray<TSharedPtr<FJsonValue>>*>(ValueArray)->Add(Value.GetInternalValue());
		}
	}
}

void JsonValue::SetObject()
{
	InternalValue = MakeShared<FJsonValueObject>(nullptr);
}

JsonValue JsonValue::FindField(const FString& Name) const
{
	if (auto JsonObject = AsObject())
	{
		return JsonValue(JsonObject->TryGetField(Name));
	}
	
	return {};
}

JsonValue::operator bool() const
{
	return !IsNull();
}

JsonValue JsonValue::operator[](const FString& Name) const
{
	return FindField(Name);
}

JsonValue JsonValue::operator[](const TCHAR* Name) const
{
	return (*this)[FString(Name)];
}

void JsonValue::CopyFrom(const JsonValue& Other)
{
	InternalValue = Other.GetInternalValue();
}

bool JsonValue::HasField(const FString& Name) const
{
	if (auto JsonObject = AsObject())
	{
		return JsonObject->HasField(Name);
	}
	
	return false;
}

void JsonValue::SetNumberField(const FString& Name, double Value)
{
	if (auto JsonObject = AsObject())
	{
		JsonObject->SetNumberField(Name, Value);
	}
}

void JsonValue::SetField(const FString& Name, float Value)
{
	SetNumberField(Name, Value);
}

void JsonValue::SetField(const FString& Name, double Value)
{
	SetNumberField(Name, Value);
}

void JsonValue::SetField(const FString& Name, int Value)
{
	SetNumberField(Name, Value);
}

void JsonValue::SetField(const FString& Name, unsigned Value)
{
	SetNumberField(Name, Value);
}

void JsonValue::SetField(const FString& Name, int64_t Value)
{
	SetNumberField(Name, Value);
}

void JsonValue::SetField(const FString& Name, uint64_t Value)
{
	SetNumberField(Name, Value);
}

void JsonValue::SetField(const FString& Name, const JsonValue& Value)
{
	if (auto JsonObject = AsObject())
	{
		JsonObject->SetField(Name, Value.GetInternalValue());
	}
}

void JsonValue::SetField(const FString& Name, const FString& Value)
{
	if (auto JsonObject = AsObject())
	{
		JsonObject->SetStringField(Name, Value);
	}
}

void JsonValue::SetField(const JsonValue& Name, const JsonValue& Value)
{
	if (auto JsonObject = AsObject())
	{
		JsonObject->SetField(Name.GetString(), Value.GetInternalValue());
	}
}

TMap<FString, JsonValue> JsonValue::GetObject() const
{
	TMap<FString, JsonValue> Values;
	
	if (auto JsonObject = AsObject())
	{
		for (const auto& Itr : JsonObject->Values)
		{
			Values.Add(Itr.Key, JsonValue(Itr.Value));
		}
	}
	
	return MoveTemp(Values);
}

int JsonValue::MemberCount() const
{
	if (auto JsonObject = AsObject())
	{
		return JsonObject->Values.Num();
	}
	
	return 0;
}

bool JsonValue::IsNumber() const
{
	return InternalValue && InternalValue->Type == EJson::Number;
}

TSharedPtr<FJsonObject> JsonValue::AsObject() const
{
	if (IsObject())
	{
		return InternalValue->AsObject();
	}
	
	return nullptr;
}

void JsonValue::SetNumber(double Number)
{
	InternalValue = MakeShared<FJsonValueNumber>(Number);
}

JsonValueWrapper::JsonValueWrapper(const JsonValueWrapper& other)
{
	value.CopyFrom(other.value);
}

JsonValueWrapper::JsonValueWrapper(JsonValueWrapper&& other) :
value(MoveTemp(other.value))
{
}

JsonValueWrapper::JsonValueWrapper(const JsonValue& other)
{
	value.CopyFrom(other);
}

JsonValueWrapper::JsonValueWrapper(JsonValue&& other) :
value(MoveTemp(other))
{
}

JsonValueWrapper& JsonValueWrapper::operator=(const JsonValueWrapper& other)
{
	if (this != &other)
	{
		value.CopyFrom(other.value);
	}
	
	return *this;
}

JsonValueWrapper& JsonValueWrapper::operator=(JsonValueWrapper&& other)
{
	if (this != &other)
	{
		value = MoveTemp(other.value);
	}
	
	return *this;
}
