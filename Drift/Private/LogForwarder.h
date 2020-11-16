// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "JsonRequestManager.h"
#include "DriftSchemas.h"

#include "Tickable.h"


DECLARE_LOG_CATEGORY_EXTERN(LogDriftLogs, Log, All);


class FLogForwarder : public FOutputDevice, public FTickableGameObject, public FSelfRegisteringExec
{
public:
	FLogForwarder();
	~FLogForwarder();

	/**
	 * FOutputDevice overrides
	 */
	virtual void Serialize(const TCHAR* text, ELogVerbosity::Type level, const FName& category) override;

	/**
	 * FTickableGameObject overrides
	 */
    void Tick(float DeltaTime) override;
    bool IsTickable() const override { return true; }

    TStatId GetStatId() const override;

    /**
     * FSelfRegisteringExec overrides
     */
    virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

    /**
     * API
     */
    void FlushLogs();

    void SetRequestManager(TSharedPtr<JsonRequestManager> newRequestManager);
    void SetLogsUrl(const FString& newLogsUrl);
    void SetForwardedLogLevel(ELogVerbosity::Type Level);
    
private:
	void Log(const TCHAR* text, ELogVerbosity::Type level, const FName& category);
    const FString& GetLogLevelName(ELogVerbosity::Type level) const;

    TWeakPtr<JsonRequestManager> requestManager;
    FString logsUrl;

    TArray<FDriftLogMessage> pendingLogs;
    float flushLogsInSeconds = FLT_MAX;

    ELogVerbosity::Type minLogLevel = ELogVerbosity::Error;
};
