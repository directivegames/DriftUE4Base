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
    FString GetConnectionString() const override { return ConnectionString; };
    FString GetConnectionOptions() const override { return ConnectionOptions; };

	FString MatchPlacementId;
	FString MapName;
	int32 PlayerId = 0;
	int32 MaxPlayers = 0;
	EDriftMatchPlacementStatus MatchPlacementStatus = EDriftMatchPlacementStatus::Unknown;
	FString CustomData;

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
    bool QueryMatchPlacement(FQueryMatchPlacementCompletedDelegate Delegate) override;
    bool CreateMatchPlacement(FDriftMatchPlacementProperties MatchPlacementProperties, FCreateMatchPlacementCompletedDelegate Delegate) override;

    FOnMatchPlacementStatusChangedDelegate& OnMatchPlacementStatusChanged() override { return OnMatchPlacementStatusChangedDelegate; }

private:
	void InitializeLocalState();

	void HandleMatchPlacementEvent(const FMessageQueueEntry& Message);

	static EDriftMatchPlacementStatus ParseEvent(const FString& EventName);

	bool HasSession() const;

	void CacheMatchPlacement(const FDriftMatchPlacementResponse& MatchPlacementResponse);
	void ResetCurrenMatchPlacement();

	static bool GetResponseError(const ResponseContext& Context, FString& Error);

	TSharedPtr<JsonRequestManager> RequestManager;
	TSharedPtr<IDriftMessageQueue> MessageQueue;

	FString MatchPlacementsURL;
	FString CurrentMatchPlacementURL;
	int32 PlayerId = INDEX_NONE;

	TSharedPtr<FDriftMatchPlacement> CurrentMatchPlacement;
	FString CurrentMatchPlacementId;

	FOnMatchPlacementStatusChangedDelegate OnMatchPlacementStatusChangedDelegate;
};
