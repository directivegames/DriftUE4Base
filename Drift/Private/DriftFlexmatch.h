// Copyright 2021 Directive Games Limited - All Rights Reserved

#pragma once

#include "IDriftMatchmaker.h"
#include "DriftMessageQueue.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDriftMatchmaking, Log, All);

enum class EMatchmakingEvent : uint8
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

	FConnectionInfo ConnectionInfo() const override;

	FMatchmakingStartedDelegate& OnMatchmakingStarted() override;
	FMatchmakingSearchingDelegate& OnMatchmakingSearching() override;
	FMatchmakingStoppedDelegate& OnMatchmakingStopped() override;
	FMatchmakingCancelledDelegate& OnMatchmakingCancelled() override;
	FMatchmakingFailedDelegate& OnMatchmakingFailed() override;
	FPotentialMatchCreatedDelegate& OnPotentialMatchCreated() override;
	FAcceptMatchDelegate& OnAcceptMatch() override;
	FMatchmakingSuccessDelegate& OnMatchmakingSuccess() override;

private:
	void HandleMatchmakingEvent(const FMessageQueueEntry& Message);
	void ReportLatencies();
	void SetStatusFromString(const FString& StatusString);
	FString GetStatusString() const;
	void InitializeLocalState();
	static EMatchmakingEvent ParseEvent(const FString& EventName);

	TSharedPtr<JsonRequestManager> RequestManager;
	TSharedPtr<IDriftMessageQueue> MessageQueue;
	FString FlexmatchURL;
	int32 PlayerId;

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
	const FString PingUrlTemplate = TEXT("https://gamelift.{0}.amazonaws.com");
	const TArray<FString> PingRegions{"eu-west-1"};

	// Current state
	bool IsInitialized = false;
	EMatchmakingTicketStatus Status = EMatchmakingTicketStatus::None;
	FString TicketId;
	FString ConnectionString;
	FString ConnectionOptions;
};
