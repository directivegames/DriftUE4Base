// Copyright 2021 Directive Games Limited - All Rights Reserved

#pragma once

#include "IDriftMatchmaker.h"
#include "DriftMessageQueue.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDriftMatchmaking, Log, All);

enum class EDriftMatchmakingEvent : uint8
{
	Unknown,
	MatchmakingStarted,
	MatchmakingSearching,
	MatchmakingStopped,
	PotentialMatchCreated,
	MatchmakingSuccess,
	MatchmakingCancelled,
	AcceptMatch,
	MatchmakingFailed
};

class FDriftFlexmatch : public IDriftMatchmaker, public FTickableGameObject, public TSharedFromThis<FDriftFlexmatch>
{
public:
	FDriftFlexmatch(TSharedPtr<IDriftMessageQueue> InMessageQueue);
	~FDriftFlexmatch() override;

	void SetRequestManager(TSharedPtr<JsonRequestManager> RootRequestManager);
	void ConfigureSession(const FDriftEndpointsResponse& DriftEndpoints, int32 InPlayerId);

	// FTickableGameObject overrides
	void Tick(float DeltaTime) override;
	bool IsTickable() const override;
	TStatId GetStatId() const override;

	// IDriftMatchmaker overrides
	void StartLatencyReporting() override;
	void StopLatencyReporting() override;
	bool IsLatencyReporting() override { return bDoPings; }
	FLatencyMap GetLatencyAverages() override;

	void StartMatchmaking(const FString& MatchmakingConfiguration, const JsonValue& ExtraData = rapidjson::kObjectType) override;
	void StopMatchmaking() override;
	EMatchmakingTicketStatus GetMatchmakingStatus() override;

	void SetAcceptance(const FString& MatchId, bool Accepted) override;

	int32 GetLocalPlayerId() const override;

	FConnectionInfo ConnectionInfo() const override;

	FMatchmakingStartedDelegate& OnDriftMatchmakingStarted() override;
	FMatchmakingSearchingDelegate& OnDriftMatchmakingSearching() override;
	FMatchmakingStoppedDelegate& OnDriftMatchmakingStopped() override;
	FMatchmakingCancelledDelegate& OnDriftMatchmakingCancelled() override;
	FMatchmakingFailedDelegate& OnDriftMatchmakingFailed() override;
	FPotentialMatchCreatedDelegate& OnDriftPotentialMatchCreated() override;
	FAcceptMatchDelegate& OnDriftAcceptMatch() override;
	FMatchmakingSuccessDelegate& OnDriftMatchmakingSuccess() override;

private:
	void HandleMatchmakingEvent(const FMessageQueueEntry& Message);
	void MeasureLatencies();
	void ReportLatencies(const TSharedRef<TMap<FString, int>> LatenciesByRegion);
	void SetStatusFromString(const FString& StatusString);
	FString GetStatusString() const;
	void InitializeLocalState();
	static EDriftMatchmakingEvent ParseEvent(const FString& EventName);

	TSharedPtr<JsonRequestManager> RequestManager;
	TSharedPtr<IDriftMessageQueue> MessageQueue;
	FString FlexmatchLatencyURL;
	FString FlexmatchTicketsURL;
	int32 PlayerId = 0;

	FMatchmakingStartedDelegate OnMatchmakingStartedDelegate;
	FMatchmakingSearchingDelegate OnMatchmakingSearchingDelegate;
	FMatchmakingStoppedDelegate OnMatchmakingStoppedDelegate;
	FMatchmakingCancelledDelegate OnMatchmakingCancelledDelegate;
	FMatchmakingFailedDelegate OnMatchmakingFailedDelegate;
	FPotentialMatchCreatedDelegate OnPotentialMatchCreatedDelegate;
	FAcceptMatchDelegate OnAcceptMatchDelegate;
	FMatchmakingSuccessDelegate OnMatchmakingSuccessDelegate;

	// Latency measuring/reporting
	bool bDoPings = false;
	bool bIsPinging = false;
	float PingInterval = 2.0;
	const float MaxPingInterval = 15.0;
	float TimeToPing = 0.0;
	FLatencyMap AverageLatencyMap;
	// TODO: Fetch valid region->pingServer mapping from backend.
	const FString PingUrlTemplate = TEXT("gamelift.{0}.amazonaws.com");
	const TArray<FString> PingRegions{"eu-west-1"};

	// Current state
	bool bIsInitialized = false;
	EMatchmakingTicketStatus Status = EMatchmakingTicketStatus::None;
	FString CurrentTicketUrl;
	FString ConnectionString;
	FString ConnectionOptions;
};
