// Copyright 2020 Directive Games Limited - All Rights Reserved

#include "DriftPartyManager.h"


#include "Details/UrlHelper.h"
#include "Json/Public/Serialization/JsonSerializerMacros.h"


DEFINE_LOG_CATEGORY(LogDriftParties);


struct FDriftPartyInviteMessage : FJsonSerializable
{
	BEGIN_JSON_SERIALIZER;
		JSON_SERIALIZE("invite_id", InviteId);
		JSON_SERIALIZE("invite_url", InviteUrl);
		JSON_SERIALIZE("inviting_player_id", InvitingPlayerId);
		JSON_SERIALIZE("inviting_player_url", InvitingPlayerUrl);
		END_JSON_SERIALIZER;

	int32 InviteId;
	FString InviteUrl;
	int32 InvitingPlayerId;
	FString InvitingPlayerUrl;
};


struct FDriftPlayerJoinedPartyMessage : FJsonSerializable
{
	BEGIN_JSON_SERIALIZER;
		JSON_SERIALIZE("party_id", PartyId);
		JSON_SERIALIZE("party_url", PartyUrl);
		JSON_SERIALIZE("player_id", PlayerId);
		JSON_SERIALIZE("member_url", MemberUrl);
		JSON_SERIALIZE("player_url", PlayerUrl);
		JSON_SERIALIZE("inviting_player_id", InvitingPlayerId);
		JSON_SERIALIZE("inviting_player_url", InvitingPlayerUrl);
		END_JSON_SERIALIZER;

	int32 PartyId;
	FString PartyUrl;
	int32 PlayerId;
	FString MemberUrl;
	FString PlayerUrl;
	int32 InvitingPlayerId;
	FString InvitingPlayerUrl;
};


struct FDriftPlayerLeftPartyMessage : FJsonSerializable
{
	BEGIN_JSON_SERIALIZER;
		JSON_SERIALIZE("party_id", PartyId);
		JSON_SERIALIZE("party_url", PartyUrl);
		JSON_SERIALIZE("player_id", PlayerId);
		JSON_SERIALIZE("player_url", PlayerUrl);
		END_JSON_SERIALIZER;

	int32 PartyId;
	FString PartyUrl;
	int32 PlayerId;
	FString PlayerUrl;
};


struct FDriftPartyDisbandedMessage : FJsonSerializable
{
	BEGIN_JSON_SERIALIZER;
		JSON_SERIALIZE("party_id", PartyId);
		JSON_SERIALIZE("party_url", PartyUrl);
		END_JSON_SERIALIZER;

	int32 PartyId;
	FString PartyUrl;
};


struct FDriftPartyCreatedMessage : FJsonSerializable
{
	BEGIN_JSON_SERIALIZER;
	JSON_SERIALIZE("party_id", PartyId);
	JSON_SERIALIZE("party_url", PartyUrl);
	END_JSON_SERIALIZER;

	int32 PartyId;
	FString PartyUrl;
};


struct FDriftPartyInviteAcceptedMessage : FJsonSerializable
{
	BEGIN_JSON_SERIALIZER;
	JSON_SERIALIZE("player_id", PlayerId);
	JSON_SERIALIZE("player_url", PlayerUrl);
	END_JSON_SERIALIZER;

	int32 PlayerId;
	FString PlayerUrl;
};


struct FDriftPartyInviteDeclinedMessage : FJsonSerializable
{
	BEGIN_JSON_SERIALIZER;
	JSON_SERIALIZE("player_id", PlayerId);
	JSON_SERIALIZE("player_url", PlayerUrl);
	END_JSON_SERIALIZER;

	int32 PlayerId;
	FString PlayerUrl;
};


struct FDriftPartyInviteCanceledMessage : FJsonSerializable
{
	BEGIN_JSON_SERIALIZER;
	JSON_SERIALIZE("invite_id", InviteId);
	JSON_SERIALIZE("inviting_player_id", InvitingPlayerId);
	JSON_SERIALIZE("inviting_player_url", InvitingPlayerUrl);
	END_JSON_SERIALIZER;

	int32 InviteId;
	int32 InvitingPlayerId;
	FString InvitingPlayerUrl;
};


struct FDriftSendPartyInviteResponse : FJsonSerializable
{
	BEGIN_JSON_SERIALIZER;
	JSON_SERIALIZE("id", InviteId);
	JSON_SERIALIZE("url", InviteUrl);
	END_JSON_SERIALIZER;

	int32 InviteId;
	FString InviteUrl;
};


struct FDriftAcceptPartyInviteResponse : FJsonSerializable
{
	BEGIN_JSON_SERIALIZER;
		JSON_SERIALIZE("party_id", PartyId);
		JSON_SERIALIZE("party_url", PartyUrl);
		JSON_SERIALIZE("player_id", PlayerId);
		JSON_SERIALIZE("player_url", PlayerUrl);
		END_JSON_SERIALIZER;

	int32 PartyId;
	FString PartyUrl;
	int32 PlayerId;
	FString PlayerUrl;
};


struct FDriftPartyMember : FJsonSerializable
{
	BEGIN_JSON_SERIALIZER;
		JSON_SERIALIZE("id", Id);
		JSON_SERIALIZE("url", Url);
		JSON_SERIALIZE("player_url", PlayerUrl);
		END_JSON_SERIALIZER;

	int32 Id;
	FString Url;
	FString PlayerUrl;
};


struct FDriftGetPartyResponse : FJsonSerializable
{
	BEGIN_JSON_SERIALIZER;
		JSON_SERIALIZE("id", Id);
		JSON_SERIALIZE("url", Url);
		JSON_SERIALIZE("invites_url", InvitesUrl);
		JSON_SERIALIZE("members_url", MembersUrl);
		JSON_SERIALIZE_ARRAY_SERIALIZABLE("members", Members, FDriftPartyMember);
		END_JSON_SERIALIZER;

	int32 Id;
	FString Url;
	FString InvitesUrl;
	FString MembersUrl;
	TArray<FDriftPartyMember> Members;
};


FDriftPartyManager::FDriftPartyManager(TSharedPtr<IDriftMessageQueue> MessageQueue)
	: MessageQueue_{MoveTemp(MessageQueue)}
{
	MessageQueue_->OnMessageQueueMessage(TEXT("party_notification")).AddRaw(
		this, &FDriftPartyManager::HandlePartyNotification);
}


FDriftPartyManager::~FDriftPartyManager()
{
	MessageQueue_->OnMessageQueueMessage(TEXT("party_notification")).RemoveAll(this);
}


TSharedPtr<IDriftParty> FDriftPartyManager::GetParty() const
{
	if (!HasSession())
	{
		UE_LOG(LogDriftParties, Error, TEXT("Trying to access player parties without a session"));

		return {};
	}
	return CurrentParty_;
}


bool FDriftPartyManager::LeaveParty(int PartyId, FLeavePartyCompletedDelegate Callback)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftParties, Error, TEXT("Trying to access player parties without a session"));

		return false;
	}
	if (!CurrentParty_.IsValid())
	{
		UE_LOG(LogDriftParties, Error, TEXT("Trying to leave a party without being in one"));

		return false;
	}
	auto Request = RequestManager_->Delete(CurrentMembershipUrl_, HttpStatusCodes::NoContent);
	Request->OnResponse.BindLambda([this, Callback, PartyId](ResponseContext& Context, JsonDocument& Doc)
	{
		CurrentPartyUrl_.Empty();
		CurrentPartyId_ = INDEX_NONE;
		CurrentParty_.Reset();
		CurrentMembershipUrl_.Empty();

		UE_LOG(LogDriftParties, Verbose, TEXT("Player left party"));

		(void)Callback.ExecuteIfBound(true, PartyId);
	});
	Request->OnError.BindLambda([this, Callback, PartyId](ResponseContext& Context)
	{
		(void)Callback.ExecuteIfBound(false, PartyId);
	});
	Request->Dispatch();
	return false;
}


bool FDriftPartyManager::InvitePlayerToParty(int PlayerId, FInvitePlayerToPartyCompletedDelegate Callback)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftParties, Error, TEXT("Trying to access player parties without a session"));

		return false;
	}

	JsonValue Payload{rapidjson::kObjectType};
	JsonArchive::AddMember(Payload, TEXT("player_id"), PlayerId);
	auto Request = RequestManager_->Post(PartyInvitesUrl_, Payload, HttpStatusCodes::Created);
	Request->OnResponse.BindLambda([this, Callback, PlayerId](ResponseContext& Context, JsonDocument& Doc)
	{
		FDriftSendPartyInviteResponse Payload{};
        if (!Payload.FromJson(Doc.GetInternalValue()->AsObject()))
        {
            UE_LOG(LogDriftParties, Error, TEXT("Failed to serialize send party invite response"));

            (void)Callback.ExecuteIfBound(false, PlayerId);
            return;
        }

        OutgoingInvites_.Add(MakeShared<FDriftPartyInvite>(Payload.InviteUrl, Payload.InviteId, 0, PlayerId_));
		(void)Callback.ExecuteIfBound(true, PlayerId);
	});
	Request->OnError.BindLambda([this, Callback, PlayerId](ResponseContext& Context)
	{
		(void)Callback.ExecuteIfBound(false, PlayerId);
	});
	Request->Dispatch();
	return false;
}


TArray<TSharedPtr<IDriftPartyInvite>> FDriftPartyManager::GetOutgoingPartyInvites() const
{
	if (!HasSession())
	{
		UE_LOG(LogDriftParties, Error, TEXT("Trying to access player parties without a session"));

		return {};
	}
	return TArray<TSharedPtr<IDriftPartyInvite>>{OutgoingInvites_};
}


TArray<TSharedPtr<IDriftPartyInvite>> FDriftPartyManager::GetIncomingPartyInvites() const
{
	if (!HasSession())
	{
		UE_LOG(LogDriftParties, Error, TEXT("Trying to access player parties without a session"));

		return {};
	}
	return TArray<TSharedPtr<IDriftPartyInvite>>{IncomingInvites_};
}


bool FDriftPartyManager::AcceptPartyInvite(int PartyInviteId, FAcceptPartyInviteCompletedDelegate Callback)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftParties, Error, TEXT("Trying to access player parties without a session"));

		return false;
	}

	const auto Invite = IncomingInvites_.FindByPredicate([PartyInviteId](const TSharedPtr<FDriftPartyInvite>& Invite)
	{
		return Invite->InviteId == PartyInviteId;
	});
	if (!Invite)
	{
		UE_LOG(LogDriftParties, Error, TEXT("Trying to accept non-existing invite %d"), PartyInviteId);

		(void)Callback.ExecuteIfBound(false, PartyInviteId);
		return false;
	}

	JsonValue Payload{rapidjson::kObjectType};
	JsonArchive::AddMember(Payload, TEXT("inviter_id"), (*Invite)->InvitingPlayerId);
	auto Request = RequestManager_->Patch((*Invite)->InviteUrl, Payload, HttpStatusCodes::Ok);
	Request->OnResponse.BindLambda([this, Callback, PartyInviteId](ResponseContext& Context, JsonDocument& Doc)
	{
		FDriftAcceptPartyInviteResponse Payload{};
		if (!Payload.FromJson(Doc.GetInternalValue()->AsObject()))
		{
			UE_LOG(LogDriftParties, Error, TEXT("Failed to serialize aceept party invite response"));

			(void)Callback.ExecuteIfBound(false, PartyInviteId);
			return;
		}

		CurrentPartyUrl_ = Payload.PartyUrl;

		UE_LOG(LogDriftParties, Verbose, TEXT("Joined party %s"), *Payload.PartyUrl);

		(void)Callback.ExecuteIfBound(true, PartyInviteId);
	});
	Request->OnError.BindLambda([Callback, PartyInviteId](ResponseContext& Context)
	{
		(void)Callback.ExecuteIfBound(false, PartyInviteId);
	});
	Request->Dispatch();
	return true;
}


bool FDriftPartyManager::CancelPartyInvite(int PartyInviteId, FCancelPartyIniviteCompletedDelegate Callback)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftParties, Error, TEXT("Trying to access player parties without a session"));

		return false;
	}
	return false;
}


bool FDriftPartyManager::DeclinePartyInvite(int PartyInviteId, FDeclinePartyIniviteCompletedDelegate Callback)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftParties, Error, TEXT("Trying to access player parties without a session"));

		return false;
	}
	return false;
}


FPartyInviteReceivedDelegate& FDriftPartyManager::OnPartyInviteReceived()
{
	return OnPartyInviteReceivedDelegate_;
}


FPartyInviteAcceptedDelegate& FDriftPartyManager::OnPartyInviteAccepted()
{
	return OnPartyInviteAcceptedDelegate_;
}


FPartyInviteDeclinedDelegate& FDriftPartyManager::OnPartyInviteDeclined()
{
	return OnPartyInviteDeclinedDelegate_;
}


FPartyInviteCanceledDelegate& FDriftPartyManager::OnPartyInviteCanceled()
{
	return OnPartyInviteCanceledDelegate_;
}


FPartyMemberJoinedDelegate& FDriftPartyManager::OnPartyMemberJoined()
{
	return OnPartyMemberJoinedDelegate_;
}


FPartyMemberLeftDelegate& FDriftPartyManager::OnPartyMemberLeft()
{
	return OnPartyMemberLeftDelegate_;
}


FPartyDisbandedDelegate& FDriftPartyManager::OnPartyDisbanded()
{
	return OnPartyDisbandedDelegate_;
}


void FDriftPartyManager::SetRequestManager(TSharedPtr<JsonRequestManager> RequestManager)
{
	RequestManager_ = RequestManager;
}


void FDriftPartyManager::ConfigureSession(int32 PlayerId, const FString& PartyInvitesUrl, const FString& PartiesUrl)
{
	PlayerId_ = PlayerId;
	PartyInvitesUrl_ = PartyInvitesUrl;
	PartiesUrl_ = PartiesUrl;
	TryGetCurrentParty();
}


void FDriftPartyManager::RaisePartyInviteReceived(int32 InviteId, int32 FromPlayerId)
{
	OnPartyInviteReceivedDelegate_.Broadcast(InviteId, FromPlayerId);
}


void FDriftPartyManager::RaisePartyInviteAccepted(int32 PlayerId)
{
	OnPartyInviteAcceptedDelegate_.Broadcast(PlayerId);
}


void FDriftPartyManager::RaisePartyInviteDeclined(int32 PlayerId)
{
	OnPartyInviteDeclinedDelegate_.Broadcast(PlayerId);
}


void FDriftPartyManager::RaisePartyInviteCanceled(int32 InviteId, int32 InvitingPlayerId)
{
	OnPartyInviteCanceledDelegate_.Broadcast(InviteId);
}


void FDriftPartyManager::RaisePartyMemberJoined(int32 PartyId, int32 PlayerId)
{
	OnPartyMemberJoinedDelegate_.Broadcast(PartyId, PlayerId);
}


void FDriftPartyManager::RaisePartyMemberLeft(int32 PartyId, int32 PlayerId)
{
	OnPartyMemberLeftDelegate_.Broadcast(PartyId, PlayerId);
}


void FDriftPartyManager::RaisePartyDisbanded(int32 PartyId)
{
	OnPartyDisbandedDelegate_.Broadcast(PartyId);
}


bool FDriftPartyManager::HasSession() const
{
	return !PartyInvitesUrl_.IsEmpty() && RequestManager_.IsValid();
}


void FDriftPartyManager::RemoveExistingInvitesFromPlayer(int32 InvitingPlayerId)
{
	IncomingInvites_.RemoveAll([InvitingPlayerId](const TSharedPtr<FDriftPartyInvite>& Invite)
	{
		return Invite->InvitingPlayerId == InvitingPlayerId;
	});
}


void FDriftPartyManager::RemoveInviteToPlayer(int32 InvitedPlayerId)
{
	OutgoingInvites_.RemoveAll([InvitedPlayerId](const TSharedPtr<FDriftPartyInvite>& Invite)
    {
        return Invite->InvitedPlayerId == InvitedPlayerId;
    });
}


void FDriftPartyManager::HandlePartyNotification(const FMessageQueueEntry& Message)
{
	const auto EventField = Message.payload.FindField(TEXT("event"));
	if (!EventField.IsString())
	{
		UE_LOG(LogDriftParties, Error, TEXT("Party notification message contains no event"));

		return;
	}

	const auto EventName = EventField.GetString();

	UE_LOG(LogDriftParties, Verbose, TEXT("Received party notification: %d "), *Message.message_id, *EventName);

	if (EventName == TEXT("invite"))
	{
		HandlePartyInviteNotification(Message);
	}
	else if (EventName == TEXT("invite_accepted"))
	{
		HandlePartyInviteAcceptedNotification(Message);
	}
	else if (EventName == TEXT("invite_declined"))
	{
		HandlePartyInviteDeclinedNotification(Message);
	}
	else if (EventName == TEXT("invite_canceled"))
	{
		HandlePartyInviteCanceledNotification(Message);
	}
	else if (EventName == TEXT("player_joined"))
	{
		HandlePartyPlayerJoinedNotification(Message);
	}
	else if (EventName == TEXT("player_left"))
	{
		HandlePartyPlayerLeftNotification(Message);
	}
	else if (EventName == TEXT("disbanded"))
	{
		HandlePartyDisbandedNotification(Message);
	}
}


void FDriftPartyManager::HandlePartyInviteNotification(const FMessageQueueEntry& Message)
{
	FDriftPartyInviteMessage Payload{};
	if (!Payload.FromJson(Message.payload.GetInternalValue()->AsObject()))
	{
		UE_LOG(LogDriftParties, Error, TEXT("Failed to serialize party invite message"));
		return;
	}

	RemoveExistingInvitesFromPlayer(Payload.InvitingPlayerId);

	IncomingInvites_.Add(
		MakeShared<FDriftPartyInvite>(Payload.InviteUrl, Payload.InviteId, Payload.InvitingPlayerId, 0));

	UE_LOG(LogDriftParties, Log, TEXT("Got a party invite from player %d"), Payload.InvitingPlayerId);

	RaisePartyInviteReceived(Payload.InviteId, Payload.InvitingPlayerId);
}


void FDriftPartyManager::HandlePartyInviteAcceptedNotification(const FMessageQueueEntry& Message)
{
	FDriftPartyInviteAcceptedMessage Payload{};
	if (!Payload.FromJson(Message.payload.GetInternalValue()->AsObject()))
	{
		UE_LOG(LogDriftParties, Error, TEXT("Failed to serialize party invite accepted message"));
		return;
	}

	// TODO: Refresh party
}


void FDriftPartyManager::HandlePartyInviteDeclinedNotification(const FMessageQueueEntry& Message)
{
	FDriftPartyInviteDeclinedMessage Payload{};
	if (!Payload.FromJson(Message.payload.GetInternalValue()->AsObject()))
	{
		UE_LOG(LogDriftParties, Error, TEXT("Failed to serialize party invite declined message"));
		return;
	}

	RemoveInviteToPlayer(Payload.PlayerId);
}


void FDriftPartyManager::HandlePartyInviteCanceledNotification(const FMessageQueueEntry& Message)
{
	FDriftPartyInviteCanceledMessage Payload{};
	if (!Payload.FromJson(Message.payload.GetInternalValue()->AsObject()))
	{
		UE_LOG(LogDriftParties, Error, TEXT("Failed to serialize party invite canceled message"));
		return;
	}

	RemoveExistingInvitesFromPlayer(Payload.InvitingPlayerId);

	UE_LOG(LogDriftParties, Log, TEXT("A party invite from player %d was canceled"), Payload.InvitingPlayerId);

	RaisePartyInviteCanceled(Payload.InviteId, Payload.InvitingPlayerId);
}


void FDriftPartyManager::HandlePartyPlayerJoinedNotification(const FMessageQueueEntry& Message)
{
	FDriftPlayerJoinedPartyMessage Payload{};
	if (!Payload.FromJson(Message.payload.GetInternalValue()->AsObject()))
	{
		UE_LOG(LogDriftParties, Error, TEXT("Failed to serialize player joined party message"));
		return;
	}

	if (CurrentPartyUrl_.IsEmpty())
	{
		PartyPlayers_.Empty();
		CurrentPartyUrl_ = Payload.PartyUrl;
	}
	else if (CurrentPartyUrl_ != Payload.PartyUrl)
	{
		UE_LOG(LogDriftParties, Error
			   , TEXT("Got notification about player joining a different party than the one you're in"));
	}
	PartyPlayers_.Add(Payload.PlayerId);

	UE_LOG(LogDriftParties, Log, TEXT("Player %d joined party %s"), Payload.PlayerId, *Payload.PartyUrl);

	RaisePartyMemberJoined(Payload.PartyId, Payload.PlayerId);
}


void FDriftPartyManager::HandlePartyPlayerLeftNotification(const FMessageQueueEntry& Message)
{
	FDriftPlayerLeftPartyMessage Payload{};
	if (!Payload.FromJson(Message.payload.GetInternalValue()->AsObject()))
	{
		UE_LOG(LogDriftParties, Error, TEXT("Failed to serialize player left party message"));
		return;
	}

	PartyPlayers_.Remove(Payload.PlayerId);

	UE_LOG(LogDriftParties, Log, TEXT("Player %d left party %s"), Payload.PlayerId, *Payload.PartyUrl);

	RaisePartyMemberLeft(Payload.PartyId, Payload.PlayerId);
}


void FDriftPartyManager::HandlePartyDisbandedNotification(const FMessageQueueEntry& Message)
{
	FDriftPartyDisbandedMessage Payload{};
	if (!Payload.FromJson(Message.payload.GetInternalValue()->AsObject()))
	{
		UE_LOG(LogDriftParties, Error, TEXT("Failed to serialize party disbanded message"));
		return;
	}

	CurrentPartyUrl_.Empty();
	PartyPlayers_.Empty();

	UE_LOG(LogDriftParties, Log, TEXT("Party %s was disbanded"), *Payload.PartyUrl);

	RaisePartyDisbanded(Payload.PartyId);
}


void FDriftPartyManager::TryGetCurrentParty()
{
	if (HasSession())
	{
		FString Url = PartiesUrl_;
		internal::UrlHelper::AddUrlOption(Url, TEXT("player_id"), FString::Printf(TEXT("%d"), PlayerId_));
		auto Request = RequestManager_->Get(Url);
		Request->OnResponse.BindLambda([this](ResponseContext& Context, JsonDocument& Doc)
		{
			FDriftGetPartyResponse PartyResponse{};
			if (!PartyResponse.FromJson(Doc.GetInternalValue()->AsObject()))
			{
				UE_LOG(LogDriftParties, Error, TEXT("Failed to serialize aceept party invite response"));

				return;
			}

			const auto Membership = PartyResponse.Members.FindByPredicate(
				[PlayerId = PlayerId_](const FDriftPartyMember& Member)
				{
					return PlayerId == Member.Id;
				});
			if (Membership)
			{
				CurrentMembershipUrl_ = *Membership->Url;
				CurrentPartyId_ = PartyResponse.Id;
				CurrentPartyUrl_ = PartyResponse.Url;
				CurrentParty_ = MakeShared<FDriftParty>(CurrentPartyId_, TArray<TSharedPtr<IDriftPartyMember>>{});
				UE_LOG(LogDriftParties, Display, TEXT("Found existing party: %s"), *CurrentPartyUrl_);
			}
			else
			{
				UE_LOG(LogDriftParties, Error, TEXT("Found existing party but player is not a member"));
			}
		});
		Request->OnError.BindLambda([](ResponseContext& Context)
		{
			Context.errorHandled = true;
		});
		Request->Dispatch();
	}
}


bool FDriftPartyManager::Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (FParse::Command(&Cmd, TEXT("Party")))
	{
		ELogVerbosity::Type level = ELogVerbosity::Log;

		auto GetInt32 = [](const TCHAR* Cmd)
		{
			return FCString::Atoi(*FParse::Token(Cmd, false));
		};

		if (FParse::Command(&Cmd, TEXT("SendInvite")))
		{
			InvitePlayerToParty(GetInt32(Cmd), {});
		}
		else if (FParse::Command(&Cmd, TEXT("AcceptInvite")))
		{
			AcceptPartyInvite(GetInt32(Cmd), {});
		}
		else if (FParse::Command(&Cmd, TEXT("DeclineInvite")))
		{
			DeclinePartyInvite(GetInt32(Cmd), {});
		}
		else if (FParse::Command(&Cmd, TEXT("Leave")))
		{
			const auto PartyId = GetInt32(Cmd);
			if (PartyId != 0)
			{
				LeaveParty(PartyId, {});
			}
			else
			{
				LeaveParty(CurrentPartyId_, {});
			}
		}

		return true;
	}
#endif
	return false;
}
