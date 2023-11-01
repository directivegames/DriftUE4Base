// Copyright 2022 Directive Games Limited - All Rights Reserved

#pragma once

#include "IDriftMatchPlacementManager.h"
#include "DriftMessageQueue.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDriftMatchPlacement, Log, All);

struct FDriftMatchPlacement : IDriftMatchPlacement
{
	FDriftMatchPlacement(
		FString InMatchPlacementId,
		FString InMapName,
		int32 InPlayerId,
		int32 InMaxPlayers,
		EDriftMatchPlacementStatus InMatchPlacementStatus,
		FString InCustomData,
		FString InMatchPlacementURL)
		:
		MatchPlacementId{ InMatchPlacementId },
		MapName{ InMapName },
		PlayerId{ InPlayerId },
		MaxPlayers{ InMaxPlayers },
        MatchPlacementStatus { InMatchPlacementStatus },
		CustomData { InCustomData },
		MatchPlacementURL{ InMatchPlacementURL }
	{ }

    FString GetMatchPlacementId() const override { return MatchPlacementId; }
    FString GetMapName() const override { return MapName; }
    int32 GetMaxPlayers() const override { return MaxPlayers; }
    int32 GetPlayerId() const override { return PlayerId; }
    EDriftMatchPlacementStatus GetMatchPlacementStatus() const override { return MatchPlacementStatus; }
    FString GetCustomData() const override { return CustomData; }
    FString GetConnectionString() const override { return ConnectionString; }
    FString GetConnectionOptions() const override { return ConnectionOptions; }
    TArray<int32>& GetPlayerIds() override { return PlayerIds; }

    FString ToString() const override
	{
	    return FString::Printf(TEXT("MatchPlacementId: %s, MapName: %s, PlayerId: %d, MaxPlayers: %d, MatchPlacementStatus: %d, CustomData: %s, ConnectionString: %s, ConnectionOptions: %s"),
	        *MatchPlacementId, *MapName, PlayerId, MaxPlayers, MatchPlacementStatus, *CustomData, *ConnectionString, *ConnectionOptions);
	}

	FString MatchPlacementId;
	FString MapName;
	int32 PlayerId = 0;
	int32 MaxPlayers = 0;
	EDriftMatchPlacementStatus MatchPlacementStatus = EDriftMatchPlacementStatus::Unknown;
	FString CustomData;
    TArray<int32> PlayerIds;

	FString MatchPlacementURL;

	FString ConnectionString;
	FString ConnectionOptions;
};

struct FDriftMatchPlacementResponse : FJsonSerializable
{
    BEGIN_JSON_SERIALIZER;
    JSON_SERIALIZE("placement_id", PlacementId);
    JSON_SERIALIZE("player_id", PlayerId);
    JSON_SERIALIZE("match_provider", MatchProvider);
    JSON_SERIALIZE("status", Status);
    JSON_SERIALIZE("custom_data", CustomData);
    JSON_SERIALIZE("map_name", MapName);
    JSON_SERIALIZE("max_players", MaxPlayers);
    JSON_SERIALIZE("connection_string", ConnectionString);
    JSON_SERIALIZE("connection_options", ConnectionOptions);
    JSON_SERIALIZE("match_placement_url", MatchPlacementURL);
    END_JSON_SERIALIZER;

    FString PlacementId;
    int32 PlayerId = 0;
    FString MatchProvider;
    FString Status;
    FString CustomData;
    FString MapName;
    int32 MaxPlayers;
    FString ConnectionString;
    FString ConnectionOptions;
    FString MatchPlacementURL;

    FString ToString() const
    {
        return FString::Printf(TEXT("PlacementId: %s, PlayerId: %d, MatchProvider: %s, Status: %s, CustomData: %s, MapName: %s, MaxPlayers: %d, ConnectionString: %s, ConnectionOptions: %s, MatchPlacementURL: %s"),
            *PlacementId, PlayerId, *MatchProvider, *Status, *CustomData, *MapName, MaxPlayers, *ConnectionString, *ConnectionOptions, *MatchPlacementURL);
    }
};

class FDriftMatchPlacementManager : public IDriftMatchPlacementManager
{
public:
	FDriftMatchPlacementManager(TSharedPtr<IDriftMessageQueue> InMessageQueue);
	~FDriftMatchPlacementManager() override;

	void SetRequestManager(TSharedPtr<JsonRequestManager> RootRequestManager);
	void ConfigureSession(const FDriftEndpointsResponse& DriftEndpoints, int32 InPlayerId);

	// IDriftMatchPlacementManager overrides
    TSharedPtr<IDriftMatchPlacement> GetCachedMatchPlacement() const override { return CurrentMatchPlacement; }
    TArray< TSharedPtr<IDriftMatchPlacement> >& GetCachedPublicMatchPlacements() override { return PublicMatchPlacements; }

    bool QueryMatchPlacement(FQueryMatchPlacementCompletedDelegate Delegate) override;
    bool CreateMatchPlacement(FDriftMatchPlacementProperties MatchPlacementProperties, FCreateMatchPlacementCompletedDelegate Delegate) override;
    bool JoinMatchPlacement(const FString& MatchPlacementID, FJoinMatchPlacementCompletedDelegate Delegate) override;
    bool RejoinMatchPlacement(const FString& MatchPlacementID, FJoinMatchPlacementCompletedDelegate Delegate) override;
    bool FetchPublicMatchPlacements(FFetchPublicMatchPlacementsCompletedDelegate Delegate) override;

    FOnMatchPlacementStatusChangedDelegate& OnMatchPlacementStatusChanged() override { return OnMatchPlacementStatusChangedDelegate; }

private:
	void InitializeLocalState();

    bool GetPlacement(const FString& MatchPlacementId, FQueryMatchPlacementCompletedDelegate Delegate);
    bool GetConnectionString(const FString& MatchPlacementId, FQueryMatchPlacementCompletedDelegate Delegate);

	void HandleMatchPlacementEvent(const FMessageQueueEntry& Message);

    static EDriftMatchPlacementStatus ParseEvent(const FString& EventName);
    static EDriftMatchPlacementStatus ParseStatus(const FString& Status);

	bool HasSession() const;

    void CacheMatchPlacement(const JsonValue& MatchPlacementJsonValue);
	void CacheMatchPlacement(const FDriftMatchPlacementResponse& MatchPlacementResponse);
	void ResetCurrentMatchPlacement();

	static bool GetResponseError(const ResponseContext& Context, FString& Error);

	TSharedPtr<JsonRequestManager> RequestManager;
	TSharedPtr<IDriftMessageQueue> MessageQueue;

	FString MatchPlacementsURL;
    FString PublicPlacementsURL;
	FString CurrentMatchPlacementURL;
    FString RejoinConnectionString;
    FString RejoinConnectionOptions;;
	int32 PlayerId = INDEX_NONE;

	TSharedPtr<FDriftMatchPlacement> CurrentMatchPlacement;
    TArray< TSharedPtr<IDriftMatchPlacement> > PublicMatchPlacements;
	FString CurrentMatchPlacementId;

	FOnMatchPlacementStatusChangedDelegate OnMatchPlacementStatusChangedDelegate;
};
