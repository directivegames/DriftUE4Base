// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

#pragma once


class IDriftAPI;


class DRIFT_API FDriftWorldHelper
{
public:
    FDriftWorldHelper(UObject* worldContextObject);
    FDriftWorldHelper(UWorld* world);
    FDriftWorldHelper(FName context);

    IDriftAPI* GetInstance();

    void DestroyInstance();

private:
    UWorld* world_;
    FName context_;
};
