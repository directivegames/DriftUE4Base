// Copyright 2022 Directive Games Limited - All Rights Reserved

#pragma once

DECLARE_DELEGATE_TwoParams(FJoinSandboxFinishedDelegate, bool /* bSuccess */, const FString& /* ErrorMessage */);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSandboxJoinStatusChangedDelegate, const FString& /* ConnectionString or Error */, bool /* Succeeded */);

class IDriftSandboxManager
{
public:
    virtual bool JoinSandbox(const int32 SandboxId, FJoinSandboxFinishedDelegate Delegate) = 0;

    virtual FOnSandboxJoinStatusChangedDelegate& OnSandboxJoinStatusChanged() = 0;
    virtual ~IDriftSandboxManager() = default;
};
