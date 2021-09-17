// Copyright 2020 Directive Games Limited - All Rights Reserved

#pragma once


class IDriftPartyMember
{
public:
	virtual FString GetPlayerName() const = 0;
	virtual int GetPlayerID() const = 0;

	virtual ~IDriftPartyMember() = default;
};


class IDriftPartyInvite
{
public:
	virtual int GetInviteID() const = 0;
	virtual int GetInvitingPlayerID() const = 0;
	virtual FString GetInvitingPlayerName() const = 0;
	virtual int GetInvitedPlayerID() const = 0;

	virtual ~IDriftPartyInvite() = default;
};


class IDriftParty
{
public:
	virtual int GetPartyId() const = 0;
	virtual TArray<TSharedPtr<IDriftPartyMember>> GetMembers() const = 0;

	virtual ~IDriftParty() = default;
};


DECLARE_DELEGATE_TwoParams(FQueryPartyCompletedDelegate, bool, int32);
DECLARE_DELEGATE_TwoParams(FInvitePlayerToPartyCompletedDelegate, bool, int32);
DECLARE_DELEGATE_FourParams(FAcceptPartyInviteCompletedDelegate, bool, int32, int32 /* Status Code */, const FString& /* Error description */);
DECLARE_DELEGATE_TwoParams(FCancelPartyIniviteCompletedDelegate, bool, int32);
DECLARE_DELEGATE_TwoParams(FDeclinePartyIniviteCompletedDelegate, bool, int32);
DECLARE_DELEGATE_TwoParams(FLeavePartyCompletedDelegate, bool, int32);


DECLARE_MULTICAST_DELEGATE_ThreeParams(FPartyInviteReceivedDelegate, int32 /* InviteId */, int32 /* FromPlayerId */, FString /* FromPlayerName */);
DECLARE_MULTICAST_DELEGATE_OneParam(FPartyInviteAcceptedDelegate, int32 /* AcceptingPlayerId */);
DECLARE_MULTICAST_DELEGATE_OneParam(FPartyInviteDeclinedDelegate, int32 /* DecliningPlayerId */);
DECLARE_MULTICAST_DELEGATE_OneParam(FPartyInviteCanceledDelegate, int32 /* DecliningPlayerId */);
DECLARE_MULTICAST_DELEGATE_TwoParams(FPartyMemberJoinedDelegate, int32 /* PartyId */, int32 /* JoiningPlayerId */);
DECLARE_MULTICAST_DELEGATE_TwoParams(FPartyMemberLeftDelegate, int32 /* PartyId */, int32 /* LeavingPlayerId */);
DECLARE_MULTICAST_DELEGATE_OneParam(FPartyDisbandedDelegate, int32 /* PartyId */);
DECLARE_MULTICAST_DELEGATE_OneParam(FPartyUpdatedDelegate, int32 /* PartyId */);


class IDriftPartyManager
{
public:
	/* Get cached information about the current party, if any */
	virtual TSharedPtr<IDriftParty> GetCachedParty() const = 0;

	/* Get information about the current party from the server */
	virtual bool QueryParty(FQueryPartyCompletedDelegate Callback) = 0;

	/* Leave the current party */
	virtual bool LeaveParty(int PartyID, FLeavePartyCompletedDelegate Callback) = 0;

	/* Send an invite to form or join a party to another player */
	virtual bool InvitePlayerToParty(int PlayerID, FInvitePlayerToPartyCompletedDelegate Callback) = 0;

	/* Get a list of sent party invites */
	virtual TArray<TSharedPtr<IDriftPartyInvite>> GetOutgoingPartyInvites() const = 0;

	/* Get a list of unanswered party invites */
	virtual TArray<TSharedPtr<IDriftPartyInvite>> GetIncomingPartyInvites() const = 0;

	/* Accept an invite to a party from another player */
	virtual bool AcceptPartyInvite(int PartyInviteID, FAcceptPartyInviteCompletedDelegate Callback) = 0;

	/* Cancel a party invite you have sent to another player */
	virtual bool CancelPartyInvite(int PartyInviteID, FCancelPartyIniviteCompletedDelegate Callback) = 0;

	/* Decline a party invite from another player */
	virtual bool DeclinePartyInvite(int PartyInviteID, FDeclinePartyIniviteCompletedDelegate Callback) = 0;

	/* Raised when you have received a new party invite from another player */
	virtual FPartyInviteReceivedDelegate& OnPartyInviteReceived() = 0;

	/* Raised when another player has accepted your party invite */
	virtual FPartyInviteAcceptedDelegate& OnPartyInviteAccepted() = 0;

	/* Raised when another player declines your party invite */
	virtual FPartyInviteDeclinedDelegate& OnPartyInviteDeclined() = 0;

	/* Raised when another player cancels your party invite */
	virtual FPartyInviteCanceledDelegate& OnPartyInviteCanceled() = 0;

	/* Raised when a player joins the party you're in */
	virtual FPartyMemberJoinedDelegate& OnPartyMemberJoined() = 0;

	/* Raised when a player leaves the party you're in */
	virtual FPartyMemberLeftDelegate& OnPartyMemberLeft() = 0;

	/* Raised when the party you're in has been disbanded */
	virtual FPartyDisbandedDelegate& OnPartyDisbanded() = 0;

	/* Raised when the party you're in has changed */
	virtual FPartyUpdatedDelegate& OnPartyUpdated() = 0;

	virtual ~IDriftPartyManager() = default;
};
