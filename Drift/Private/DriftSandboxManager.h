// Copyright 2022 Directive Games Limited - All Rights Reserved

#pragma once

#include "IDriftSandboxManager.h"
#include "DriftMessageQueue.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDriftSandbox, Log, All);

class FDriftSandboxManager : public IDriftSandboxManager
{
public:
	FDriftSandboxManager(TSharedPtr<IDriftMessageQueue> InMessageQueue);
	~FDriftSandboxManager() override;

    void SetRequestManager(TSharedPtr<JsonRequestManager> RootRequestManager);
    void ConfigureSession(const FDriftEndpointsResponse& DriftEndpoints, int32 InPlayerId);

    bool JoinSandbox(const int32 SandboxId, FJoinSandboxFinishedDelegate Delegate) override;
    FOnSandboxJoinStatusChangedDelegate& OnSandboxJoinStatusChanged() override { return OnSandboxJoinStatusChangedDelegate; }

private:
    void HandleSandboxEvent(const FMessageQueueEntry& Message);

    TSharedPtr<JsonRequestManager> RequestManager;
    TSharedPtr<IDriftMessageQueue> MessageQueue;
    int32 PlayerId = INDEX_NONE;
    FString SandboxURL;

    FOnSandboxJoinStatusChangedDelegate OnSandboxJoinStatusChangedDelegate;
};
