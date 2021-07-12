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

class FDriftFlexmatch : public IDriftMatchmaker, FTickableGameObject
{
public:
	FDriftFlexmatch(TSharedPtr<IDriftMessageQueue> InMessageQueue);
	~FDriftFlexmatch() override;

	void ConfigureSession(TSharedPtr<JsonRequestManager> RootRequestManager, const FString& MatchmakingUrl, int32 InPlayerId);

	// FTickableGameObject overrides
	void Tick(float DeltaTime) override;
	bool IsTickable() const override;
	TStatId GetStatId() const override;

	// IDriftMatchmaker overrides
	void StartLatencyReporting() override;
	void StopLatencyReporting() override;
	FLatencyMap GetLatencyAverages() override;

	void StartMatchmaking(const FString& MatchmakingConfiguration) override;
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
	void ReportLatencies();
	void SetStatusFromString(const FString& StatusString);
	FString GetStatusString() const;
	void InitializeLocalState();
	static EDriftMatchmakingEvent ParseEvent(const FString& EventName);

	TSharedPtr<JsonRequestManager> RequestManager;
	TSharedPtr<IDriftMessageQueue> MessageQueue;
	FString FlexmatchURL;
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
	bool DoPings = false;
	const float PingInterval = 3.0;
	float TimeToPing = 0.0;
	FLatencyMap AverageLatencyMap;
	// TODO: Fetch valid region->pingServer mapping from backend.
	const FString PingUrlTemplate = TEXT("https://gamelift.{0}.amazonaws.com");
	const TArray<FString> PingRegions{"eu-west-1"};

	// Current state
	bool IsInitialized = false;
	EMatchmakingTicketStatus Status = EMatchmakingTicketStatus::None;
	FString TicketId;
	FString ConnectionString;
	FString ConnectionOptions;
};
