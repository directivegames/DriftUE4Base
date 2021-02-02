// Copyright 2020 Directive Games Limited - All Rights Reserved

#pragma once

#include "DriftMessageQueue.h"
#include "IDriftPartyManager.h"
#include "JsonRequestManager.h"
#include "OnlineSubsystemTypes.h"


DECLARE_LOG_CATEGORY_EXTERN(LogDriftParties, Log, All);


struct FDriftPartyInvite : IDriftPartyInvite
{
	FDriftPartyInvite(FString InviteUrl, int32 InviteId, int32 InvitingPlayerId, int32 InvitedPlayerId)
		: InviteUrl{ InviteUrl }
		, InviteId{ InviteId }
		, InvitingPlayerId{ InvitingPlayerId }
		, InvitedPlayerId{ InvitedPlayerId }
	{}

	int GetInvitingPlayerID() const override
	{
		return InvitingPlayerId;
	}

	int GetInvitedPlayerID() const override
	{
		return InvitedPlayerId;
	}
	
	FString InviteUrl;
	int32 InviteId;
	int32 InvitingPlayerId;
	int32 InvitedPlayerId;
};


struct FDriftPartyMember : IDriftPartyMember
{
	FDriftPartyMember(FString PlayerName, int PlayerId)
		: PlayerName{ PlayerName }
		, PlayerId{ PlayerId }
	{}
	
	FString GetPlayerName() const override
	{
		return PlayerName;
	}

	int GetPlayerID() const override
	{
		return PlayerId;
	}

	FString PlayerName;
	int PlayerId;
};


struct FDriftParty : IDriftParty
{
	FDriftParty(int32 PartyId, TArray<TSharedPtr<IDriftPartyMember>> Members)
		: PartyId{ PartyId }
		, Members{ MoveTemp(Members) }
	{}

	int GetPartyId() const override
	{
		return PartyId;
	}

	TArray<TSharedPtr<IDriftPartyMember>> GetMembers() const override
	{
		return Members;
	}

	int32 PartyId;
	TArray<TSharedPtr<IDriftPartyMember>> Members;
};


class FDriftPartyManager : public IDriftPartyManager, public FSelfRegisteringExec
{
public:
	FDriftPartyManager(TSharedPtr<IDriftMessageQueue> MessageQueue);
	~FDriftPartyManager();

	// IDriftPartyManager implementation

	TSharedPtr<IDriftParty> GetCachedParty() const override;
	bool QueryParty(FQueryPartyCompletedDelegate Callback) override;

	bool LeaveParty(int PartyId, FLeavePartyCompletedDelegate Callback) override;

	bool InvitePlayerToParty(int PlayerId, FInvitePlayerToPartyCompletedDelegate Callback) override;
	TArray<TSharedPtr<IDriftPartyInvite>> GetOutgoingPartyInvites() const override;
	TArray<TSharedPtr<IDriftPartyInvite>> GetIncomingPartyInvites() const override;

	bool AcceptPartyInvite(int PartyInviteId, FAcceptPartyInviteCompletedDelegate Callback) override;
	bool CancelPartyInvite(int PartyInviteId, FCancelPartyIniviteCompletedDelegate Callback) override;
	bool DeclinePartyInvite(int PartyInviteId, FDeclinePartyIniviteCompletedDelegate Callback) override;

	FPartyInviteReceivedDelegate& OnPartyInviteReceived() override;
	FPartyInviteAcceptedDelegate& OnPartyInviteAccepted() override;
	FPartyInviteDeclinedDelegate& OnPartyInviteDeclined() override;
	FPartyInviteCanceledDelegate& OnPartyInviteCanceled() override;
	FPartyMemberJoinedDelegate& OnPartyMemberJoined() override;
	FPartyMemberLeftDelegate& OnPartyMemberLeft() override;
	FPartyDisbandedDelegate& OnPartyDisbanded() override;
	FPartyUpdatedDelegate& OnPartyUpdated() override;

	// FSelfRegisteringExec overrides

	virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

	// Public API

	void SetRequestManager(TSharedPtr<JsonRequestManager> RequestManager);
	void ConfigureSession(int32 PlayerId, const FString& PartyInvitesUrl, const FString& PartiesUrl);

protected:
	void RaisePartyInviteReceived(int32 InviteId, int32 FromPlayerId);
	void RaisePartyInviteAccepted(int32 PlayerId);
	void RaisePartyInviteDeclined(int32 PlayerId);
	void RaisePartyInviteCanceled(int32 InviteId, int32 InvitingPlayerId);
	void RaisePartyMemberJoined(int32 PartyId, int32 PlayerId);
	void RaisePartyMemberLeft(int32 PartyId, int32 PlayerId);
	void RaisePartyDisbanded(int32 PartyId);
	void RaisePartyUpdated(int32 PartyId);

private:
	bool HasSession() const;
	void RemoveExistingInvitesFromPlayer(int32 InvitingPlayerId);
	void RemoveInviteToPlayer(int32 InvitedPlayerId);
	void HandlePartyNotification(const FMessageQueueEntry& Message);
	void HandlePartyInviteNotification(const FMessageQueueEntry& Message);
	void HandlePartyInviteAcceptedNotification(const FMessageQueueEntry& Message);
	void HandlePartyInviteDeclinedNotification(const FMessageQueueEntry& Message);
	void HandlePartyInviteCanceledNotification(const FMessageQueueEntry& Message);
	void HandlePartyPlayerJoinedNotification(const FMessageQueueEntry& Message);
	void HandlePartyPlayerLeftNotification(const FMessageQueueEntry& Message);
	void HandlePartyDisbandedNotification(const FMessageQueueEntry& Message);

	void TryGetCurrentParty();

	TSharedPtr<IDriftMessageQueue> MessageQueue_;

	TSharedPtr<JsonRequestManager> RequestManager_;
	FString PartyInvitesUrl_;
	FString PartiesUrl_;
	int32 PlayerId_;
	TArray<TSharedPtr<FDriftPartyInvite>> OutgoingInvites_;
	TArray<TSharedPtr<FDriftPartyInvite>> IncomingInvites_;
	TSharedPtr<FDriftParty> CurrentParty_;
	int32 CurrentPartyId_;
	FString CurrentPartyUrl_;
	FString CurrentMembershipUrl_;
	TArray<int32> PartyPlayers_;
	
	FPartyInviteReceivedDelegate OnPartyInviteReceivedDelegate_;
	FPartyInviteAcceptedDelegate OnPartyInviteAcceptedDelegate_;
	FPartyInviteDeclinedDelegate OnPartyInviteDeclinedDelegate_;
	FPartyInviteCanceledDelegate OnPartyInviteCanceledDelegate_;
	FPartyMemberJoinedDelegate OnPartyMemberJoinedDelegate_;
	FPartyMemberLeftDelegate OnPartyMemberLeftDelegate_;
	FPartyDisbandedDelegate OnPartyDisbandedDelegate_;
	FPartyUpdatedDelegate OnPartyUpdatedDelegate_;
};
