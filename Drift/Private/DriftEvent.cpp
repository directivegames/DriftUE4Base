// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#include "DriftPrivatePCH.h"

#include "DriftEvent.h"
#include "JsonArchive.h"
#include "JsonUtils.h"


class DRIFT_API FDriftEvent : public IDriftEvent
{
public:
    FDriftEvent()
    : FDriftEvent(FString())
    {}

    FDriftEvent(const FString& name)
    : name_{ name }
    , timestamp_{ FDateTime::UtcNow() }
    , details_{ rapidjson::kObjectType }
    {}

    ~FDriftEvent() = default;

    bool Serialize(SerializationContext& context) override
    {
        context.SerializeProperty(TEXT("event_name"), name_);

        if (!context.IsLoading())
        {
            for (auto& member : details_.GetObject())
            {
                context.SerializeProperty(member.name.GetString(), member.value);
            }
            
            auto temp = timestamp_.ToIso8601();
            context.SerializeProperty(TEXT("timestamp"), temp);
        }
        
        return true;
    }
    
    void Add(const FString& name, int value) override
    {
        InternalAdd(name, value);
    }

    void Add(const FString& name, unsigned value) override
    {
        InternalAdd(name, value);
    }

    void Add(const FString& name, long long value) override
    {
        InternalAdd(name, value);
    }

    void Add(const FString& name, float value) override
    {
        InternalAdd(name, value);
    }

    void Add(const FString& name, double value) override
    {
        InternalAdd(name, value);
    }
    
    void Add(const FString& name, const TCHAR* value) override
    {
        InternalAdd(name, FString{ value });
    }

    void Add(const FString& name, const FString& value) override
    {
        InternalAdd(name, value);
    }

    void Add(const FString& name, bool value) override
    {
        InternalAdd(name, value);
    }

    void Add(const FString& name, TArray<TUniquePtr<IDriftEvent>> events) override
    {
        JsonValue array{ rapidjson::kArrayType };
        for (auto& e : events)
        {
            e->AddEvent([this, &array](FDriftEvent& event)
            {
                array.PushBack(event.details_, JsonArchive::Allocator());
            });
        }
        JsonArchive::AddMember(details_, name, array);
    }
    
    void Add(TUniquePtr<IDriftEvent> value) override
    {
        value->AddEvent([this](FDriftEvent& event)
        {
            JsonArchive::AddMember(details_, event.name_, event.details_);
        });
    }

    void AddEvent(std::function<void(FDriftEvent&)> event) override
    {
        event(*this);
    }

    FString GetName() const override
    {
        return name_;
    }
    
private:
    template <typename TValue>
    void InternalAdd(const FString& name, const TValue& value)
    {
        JsonValue temp;
        JsonArchive::SaveObject(value, temp);
        JsonArchive::AddMember(details_, name, temp);
    }

    FString name_;
    FDateTime timestamp_;
    JsonValue details_;
};


DRIFT_API TUniquePtr<IDriftEvent> MakeEvent(const FString& name)
{
    return MakeUnique<FDriftEvent>(name);
}


DRIFT_API TUniquePtr<IDriftEvent> MakeEvent()
{
    return MakeUnique<FDriftEvent>();
}
