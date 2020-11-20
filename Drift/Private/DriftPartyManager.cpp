// Copyright 2020 Directive Games Limited - All Rights Reserved

#include "DriftPartyManager.h"


TSharedPtr<IDriftParty> FDriftPartyManager::GetParty() const
{
	return {};
}


bool FDriftPartyManager::LeaveParty(int PartyID, FLeavePartyCompletedDelegate callback)
{
	return false;
}


bool FDriftPartyManager::InvitePlayerToParty(int PlayerID, FInvitePlayerToPartyCompletedDelegate callback)
{
	return false;
}


TArray<TSharedPtr<IDriftPartyInvite>> FDriftPartyManager::GetPendingPartyInvites() const
{
	return {};
}


bool FDriftPartyManager::AcceptPartyInvite(int PartyInviteID, FAcceptPartyInviteCompletedDelegate callback)
{
	return false;
}


bool FDriftPartyManager::CancelPartyInvite(int PartyInviteID, FCancelPartyIniviteCompletedDelegate callback)
{
	return false;
}


bool FDriftPartyManager::DeclinePartyInvite(int PartyInviteID, FDeclinePartyIniviteCompletedDelegate callback)
{
	return false;
}
