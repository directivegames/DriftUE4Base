// Copyright 2020 Directive Games Limited - All Rights Reserved

#pragma once


#include "IDriftPartyManager.h"


class FDriftPartyManager : public IDriftPartyManager
{
public:
	TSharedPtr<IDriftParty> GetParty() const override;
	bool LeaveParty(int PartyID, FLeavePartyCompletedDelegate callback) override;
	bool InvitePlayerToParty(int PlayerID, FInvitePlayerToPartyCompletedDelegate callback) override;
	TArray<TSharedPtr<IDriftPartyInvite>> GetPendingPartyInvites() const override;
	bool AcceptPartyInvite(int PartyInviteID, FAcceptPartyInviteCompletedDelegate callback) override;
	bool CancelPartyInvite(int PartyInviteID, FCancelPartyIniviteCompletedDelegate callback) override;
	bool DeclinePartyInvite(int PartyInviteID, FDeclinePartyIniviteCompletedDelegate callback) override;
};
