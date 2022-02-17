// Copyright 2022 Directive Games Limited - All Rights Reserved

#pragma once

enum class EDriftMatchPlacementStatus : uint8
{
    Unknown,
    Issued,
    Fulfilled,
    Cancelled,
    TimedOut,
    Failed,
};

class IDriftMatchPlacement
{
public:
    virtual FString GetMatchPlacementId() const = 0;
    virtual FString GetMapName() const = 0;
    virtual int32 GetMaxPlayers() const = 0;
    virtual int32 GetPlayerId() const = 0;
    virtual EDriftMatchPlacementStatus GetMatchPlacementStatus() const = 0;
    virtual FString GetCustomData() const = 0;
    virtual FString GetConnectionString() const = 0;
    virtual FString GetConnectionOptions() const = 0;

    virtual ~IDriftMatchPlacement() = default;
};

struct FDriftMatchPlacementProperties
{
	FString QueueName;
	FString MapName;
	TOptional<FString> Identifier;
	TOptional<int32> MaxPlayers;
	TOptional<FString> CustomData;

	FString ToString() const
	{
		FString Ret = FString::Printf(TEXT("Queue name: '%s' | Map name: '%s'"), *QueueName, *MapName);

		if (Identifier.IsSet())
		{
			Ret += FString::Printf(TEXT(" | Identifier: '%s'"), *Identifier.GetValue());
		}

		if (MaxPlayers.IsSet())
		{
			Ret += FString::Printf(TEXT(" | Max players: '%d'"), MaxPlayers.GetValue());
		}

		if (CustomData.IsSet())
		{
			Ret += FString::Printf(TEXT(" | Custom data: '%s'"), *CustomData.GetValue());
		}

		return Ret;
	}
};

DECLARE_DELEGATE_ThreeParams(FQueryMatchPlacementCompletedDelegate, bool /* bSuccess */, const FString& /* MatchPlacementId */, const FString& /* ErrorMessage */);
DECLARE_DELEGATE_ThreeParams(FCreateMatchPlacementCompletedDelegate, bool /* bSuccess */, const FString& /* MatchPlacementId */, const FString& /* ErrorMessage */);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMatchPlacementStatusChangedDelegate, const FString& /* MatchPlacementId */, EDriftMatchPlacementStatus /* Status */);

class IDriftMatchPlacementManager
{
public:
    /* Get the cached match placement if any */
    virtual TSharedPtr<IDriftMatchPlacement> GetCachedMatchPlacement() const = 0;

    /* Get information about the current match placement from the server */
    virtual bool QueryMatchPlacement(FQueryMatchPlacementCompletedDelegate Delegate) = 0;

	/* Create a new match placement */
	virtual bool CreateMatchPlacement(FDriftMatchPlacementProperties MatchPlacementProperties, FCreateMatchPlacementCompletedDelegate Delegate) = 0;

	/* Raised when the match placement status changes */
	virtual FOnMatchPlacementStatusChangedDelegate& OnMatchPlacementStatusChanged() = 0;

	virtual ~IDriftMatchPlacementManager() = default;
};
