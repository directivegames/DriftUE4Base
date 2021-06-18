// Copyright 2021 Directive Games Limited - All Rights Reserved

#pragma once

enum class EMatchmakingState : uint8
{
	None, // No Ticket
	Queued,
	Searching,
	RequiresAcceptance,
	Placing,
	Completed,
	MatchCompleted,
	Cancelled,
	TimedOut,
	Failed
};

typedef TMap<FString, TArray<int32>> FPlayersByTeam;
typedef TArray<int32> FPlayersAccepted;
typedef TMap<FString, int32> FLatencyMap;

DECLARE_MULTICAST_DELEGATE(FMatchmakingStartedDelegate);
DECLARE_MULTICAST_DELEGATE(FMatchmakingStoppedDelegate);
DECLARE_MULTICAST_DELEGATE(FMatchmakingCancelledDelegate);
DECLARE_MULTICAST_DELEGATE_OneParam(FMatchmakingFailedDelegate, FString /* reason */);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FPotentialMatchCreatedDelegate, FPlayersByTeam /* PlayersByTeam */, FString /* MatchId */, bool /* acceptance_required */);
DECLARE_MULTICAST_DELEGATE_OneParam(FAcceptMatchDelegate, FPlayersAccepted /* accepted_players */);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMatchmakingSuccessDelegate, FString /* connection_string */, FString /* options */);

class IDriftMatchmaker
{
public:
	/* Start latency measuring loop to configured regions and report them to the backend  */
	virtual void StartLatencyReporting() = 0;

	/* Stop latency measuring loop  */
	virtual void StopLatencyReporting() = 0;

	/* Get reported averages from the backend  */
	virtual FLatencyMap GetLatencyAverages() = 0;

	/* Start matchmaking for player/players party */
	virtual void StartMatchmaking(const FString& MatchmakingConfiguration) = 0;

	/* Stop matchmaking for player/players party */
	virtual void StopMatchmaking() = 0;

	/* Get the matchmaking status from the backend */
	virtual EMatchmakingState MatchmakingStatus() = 0;

	/* Update acceptance for player */
	virtual void SetAcceptance(const FString& MatchId, bool Accepted) = 0;

	/* Issued when a ticket including player is queued */
	virtual FMatchmakingStartedDelegate& OnMatchmakingStarted() = 0;

	/* Issued when a ticket including player is cancelled via a player request */
	virtual FMatchmakingStoppedDelegate& OnMatchmakingStopped() = 0;

	/* Issued when a ticket including player is cancelled via any means, including a player request */
	virtual FMatchmakingCancelledDelegate& OnMatchmakingCancelled() = 0;

	/* Issued when a ticket including player has failed for any reason, e.g. timeout, rejected matches etc */
	virtual FMatchmakingFailedDelegate& OnMatchmakingFailed() = 0;

	/* Issued when a ticket including player is included in a potential match */
	virtual FPotentialMatchCreatedDelegate& OnPotentialMatchCreated() = 0;

	/* Issued when a player in a potential match requiring acceptance accepts the match */
	virtual FAcceptMatchDelegate& OnAcceptMatch() = 0;

	/* Issued when a match has been made which includes this player */
	virtual FMatchmakingSuccessDelegate& OnMatchmakingSuccess() = 0;

	virtual ~IDriftMatchmaker() = default;
};
