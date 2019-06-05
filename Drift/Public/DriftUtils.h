// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#pragma once


class IDriftAPI;


class DRIFT_API FDriftWorldHelper
{
public:
    FDriftWorldHelper(UObject* worldContextObject);
    FDriftWorldHelper(UWorld* world);
    FDriftWorldHelper(FName context);

    IDriftAPI* GetInstance();
    IDriftAPI* GetInstance(const FString& config);

    void DestroyInstance();

    static void DestroyInstance(IDriftAPI* instance);

private:
    UWorld* world_;
    FName context_;
};
