// Copyright 2020 Directive Games Limited - All Rights Reserved

#pragma once


class IDriftPartyPlayer
{
public:
	virtual FString GetPlayerName() const = 0;
	virtual int GetPlayerID() const = 0;

	virtual ~IDriftPartyPlayer() = default;
};


class IDriftPartyInvite
{
public:
	virtual int GetInvitedPlayerID() const = 0;

	virtual ~IDriftPartyInvite() = default;
};


class IDriftParty
{
public:
	virtual int GetPartyID() const = 0;
	virtual TArray<TSharedPtr<IDriftPartyPlayer>> GetPlayers() const = 0;

	virtual ~IDriftParty() = default;
};


DECLARE_DELEGATE_TwoParams(FInvitePlayerToPartyCompletedDelegate, bool, int);
DECLARE_DELEGATE_TwoParams(FAcceptPartyInviteCompletedDelegate, bool, int);
DECLARE_DELEGATE_TwoParams(FCancelPartyIniviteCompletedDelegate, bool, int);
DECLARE_DELEGATE_TwoParams(FDeclinePartyIniviteCompletedDelegate, bool, int);
DECLARE_DELEGATE_TwoParams(FLeavePartyCompletedDelegate, bool, int);


class IDriftPartyManager
{
public:
	virtual TSharedPtr<IDriftParty> GetParty() const = 0;
	virtual bool LeaveParty(int PartyID, FLeavePartyCompletedDelegate callback = {}) = 0;

	virtual bool InvitePlayerToParty(int PlayerID, FInvitePlayerToPartyCompletedDelegate callback = {}) = 0;
	virtual TArray<TSharedPtr<IDriftPartyInvite>> GetPendingPartyInvites() const = 0;

	virtual bool AcceptPartyInvite(int PartyInviteID, FAcceptPartyInviteCompletedDelegate callback = {}) = 0;
	virtual bool CancelPartyInvite(int PartyInviteID, FCancelPartyIniviteCompletedDelegate callback = {}) = 0;
	virtual bool DeclinePartyInvite(int PartyInviteID, FDeclinePartyIniviteCompletedDelegate callback = {}) = 0;

	virtual ~IDriftPartyManager() = default;
};
