// Copyright 2021 Directive Games Limited - All Rights Reserved

#pragma once

#include "JsonValueWrapper.h"

enum class EMatchmakingTicketStatus : uint8
{
	None, // No Ticket
	Queued,  // Ticket issued, waiting on response
	Searching, // Ticket accepted, searching for match
	RequiresAcceptance, // Match found; waiting for players to accept
	Placing,  // Match found, players all accepted or acceptance wasn't required; waiting on servers to report ready
	Completed, // Matchmaking success; servers are ready and connection info available
	MatchCompleted, // Ticket refers to a completed match, i.e. the ticket is 'used'
	Cancelled, // Ticket was cancelled by player request
	TimedOut, // No match found in acceptable time
	Failed // Error in processing or potential match was rejected.
};

typedef TMap<FString, TArray<int32>> FPlayersByTeam;
typedef TArray<int32> FPlayersAccepted;
typedef TMap<FString, int32> FLatencyMap;

struct DRIFT_API FConnectionInfo
{
	FString ConnectionString;
	FString ConnectionOptions;
};

DECLARE_MULTICAST_DELEGATE(FMatchmakingStartedDelegate);
DECLARE_MULTICAST_DELEGATE(FMatchmakingSearchingDelegate);
DECLARE_MULTICAST_DELEGATE(FMatchmakingStoppedDelegate);
DECLARE_MULTICAST_DELEGATE(FMatchmakingCancelledDelegate);
DECLARE_MULTICAST_DELEGATE_OneParam(FMatchmakingFailedDelegate, FString /* reason */);
DECLARE_MULTICAST_DELEGATE_FourParams(FPotentialMatchCreatedDelegate, FPlayersByTeam /* PlayersByTeam */, FString /* MatchId */, bool /* AcceptanceRequired */, int32 /* TimeOut */ );
DECLARE_MULTICAST_DELEGATE_OneParam(FAcceptMatchDelegate, FPlayersAccepted /* accepted_players */);
DECLARE_MULTICAST_DELEGATE_OneParam(FMatchmakingSuccessDelegate, const FConnectionInfo& /* connection_string and options */);

class IDriftMatchmaker
{
public:
	/* Start latency measuring loop to configured regions and report them to the backend */
	virtual void StartLatencyReporting() = 0;

	/* Stop latency measuring loop */
	virtual void StopLatencyReporting() = 0;

	/* Is latency measuring loop active */
	virtual bool IsLatencyReporting() = 0;

	/* Get reported averages from the backend  */
	virtual FLatencyMap GetLatencyAverages() = 0;

	/* Start matchmaking for player/players party */
	virtual void StartMatchmaking(const FString& MatchmakingConfiguration, const JsonValue& ExtraData = rapidjson::kObjectType) = 0;

	/* Stop matchmaking for player/players party */
	virtual void StopMatchmaking() = 0;

	/* Get the matchmaking status from the backend */
	virtual EMatchmakingTicketStatus GetMatchmakingStatus() = 0;

	/* Update acceptance for player */
	virtual void SetAcceptance(const FString& MatchId, bool Accepted) = 0;

	/* Get cached connection info */
	virtual FConnectionInfo ConnectionInfo() const = 0;

	/* Returns the ID of the local player or 0 if we don't have a login */
	virtual int32 GetLocalPlayerId() const = 0;

	/* Issued when a ticket including player is queued */
	virtual FMatchmakingStartedDelegate& OnDriftMatchmakingStarted() = 0;

	/* Issued when a player ticket gets into searching state */
	virtual FMatchmakingSearchingDelegate& OnDriftMatchmakingSearching() = 0;

	/* Issued when a ticket including player is cancelled via a player request */
	virtual FMatchmakingStoppedDelegate& OnDriftMatchmakingStopped() = 0;

	/* Issued when a ticket including player is cancelled via any means, including a player request */
	virtual FMatchmakingCancelledDelegate& OnDriftMatchmakingCancelled() = 0;

	/* Issued when a ticket including player has failed for any reason, e.g. timeout, rejected matches etc */
	virtual FMatchmakingFailedDelegate& OnDriftMatchmakingFailed() = 0;

	/* Issued when a ticket including player is included in a potential match */
	virtual FPotentialMatchCreatedDelegate& OnDriftPotentialMatchCreated() = 0;

	/* Issued when a player in a potential match requiring acceptance accepts the match */
	virtual FAcceptMatchDelegate& OnDriftAcceptMatch() = 0;

	/* Issued when a match has been made which includes this player */
	virtual FMatchmakingSuccessDelegate& OnDriftMatchmakingSuccess() = 0;

	virtual ~IDriftMatchmaker() = default;
};
