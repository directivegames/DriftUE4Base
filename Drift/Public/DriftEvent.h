// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include <functional>


class SerializationContext;
class FDriftEvent;


class DRIFT_API IDriftEvent
{
public:
	virtual ~IDriftEvent() {}
    
    virtual void Add(const FString& name, int value) = 0;
    virtual void Add(const FString& name, unsigned value) = 0;
    virtual void Add(const FString& name, uint64 value) = 0;
    virtual void Add(const FString& name, long long value) = 0;
    virtual void Add(const FString& name, float value) = 0;
    virtual void Add(const FString& name, const TArray<float>& value) = 0;
    virtual void Add(const FString& name, double value) = 0;
    virtual void Add(const FString& name, const TCHAR* value) = 0;
    virtual void Add(const FString& name, const FString& value) = 0;
    virtual void Add(const FString& name, const TArray<FString>& value) = 0;
    virtual void Add(const FString& name, bool value) = 0;
    virtual void Add(const FString& name, TArray<TUniquePtr<IDriftEvent>> events) = 0;
    
    virtual void Add(TUniquePtr<IDriftEvent> event) = 0;
    
    virtual FString GetName() const = 0;
    
    virtual bool Serialize(SerializationContext& context) = 0;

    virtual void AddEvent(std::function<void(FDriftEvent&)> event) = 0;
};


/**
 * Use this for the root event data, and for sub-objects
 */
DRIFT_API TUniquePtr<IDriftEvent> MakeEvent(const FString& name);

/**
 * Use this for array elements
 */
DRIFT_API TUniquePtr<IDriftEvent> MakeEvent();
