// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "Json.h"


class ERRORREPORTER_API IErrorReporter
{
public:
public:
    virtual void AddError(const FName& context, const FString& message) = 0;
    virtual void AddError(const FName& context, const FString& message, const TSharedPtr<FJsonObject>& extra) = 0;

    void AddError(const FName& context, const FString& message, const FString& extra)
    {
        auto details{ MakeShared<FJsonObject>() };
        details->SetStringField(TEXT("details"), extra);
        AddError(context, message, details);
    }

    virtual ~IErrorReporter() {}

public:
    static IErrorReporter* Get();
    static void Set(IErrorReporter* errorReporter);
};
