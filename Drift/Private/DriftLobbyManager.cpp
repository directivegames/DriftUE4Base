// Copyright 2021 Directive Games Limited - All Rights Reserved

#include "DriftLobbyManager.h"


DEFINE_LOG_CATEGORY(LogDriftLobby);

static const FString LobbyMessageQueue(TEXT("lobby"));

FDriftLobbyManager::FDriftLobbyManager(TSharedPtr<IDriftMessageQueue> InMessageQueue)
	: MessageQueue{MoveTemp(InMessageQueue)}
{
	MessageQueue->OnMessageQueueMessage(LobbyMessageQueue).AddRaw(this, &FDriftLobbyManager::HandleLobbyEvent);

	ResetCurrentLobby();
}

FDriftLobbyManager::~FDriftLobbyManager()
{
	MessageQueue->OnMessageQueueMessage(LobbyMessageQueue).RemoveAll(this);
}

void FDriftLobbyManager::SetRequestManager(TSharedPtr<JsonRequestManager> RootRequestManager)
{
	RequestManager = RootRequestManager;
}

void FDriftLobbyManager::ConfigureSession(const FDriftEndpointsResponse& DriftEndpoints, int32 InPlayerId)
{
	PlayerId = InPlayerId;

	LobbiesURL = DriftEndpoints.lobbies;
	CurrentLobbyURL = DriftEndpoints.my_lobby;
	CurrentLobbyMembersURL = DriftEndpoints.my_lobby_members;
	CurrentLobbyMemberURL = DriftEndpoints.my_lobby_member;

	if (HasSession())
	{
		QueryLobby({});
	}
}

bool FDriftLobbyManager::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (FParse::Command(&Cmd, TEXT("Drift.Lobby")))
	{
		if (PlayerId == INDEX_NONE || !RequestManager)
		{
			UE_LOG(LogDriftLobby, Verbose, TEXT("FDriftLobbyManager::Exec - Lobby manager potentially not initialized. Ignoring command"));
			return false;
		}

		auto GetString = [](const TCHAR* Cmd)
		{
			return FParse::Token(Cmd, false);
		};

		auto GetInt32 = [](const TCHAR* Cmd)
		{
			return FCString::Atoi(*FParse::Token(Cmd, false));
		};

		if (FParse::Command(&Cmd, TEXT("Get")))
		{
			QueryLobby({});
		}
		else if (FParse::Command(&Cmd, TEXT("Create")))
		{
			FDriftLobbyProperties LobbyProperties{};
			LobbyProperties.TeamCapacity = 4;
			LobbyProperties.TeamNames = TArray<FString>{ "Team A", "Team B" };

			CreateLobby(MoveTemp(LobbyProperties), {});
		}
		else if (FParse::Command(&Cmd, TEXT("Join")))
		{
			JoinLobby(GetString(Cmd), {});
		}
		else if (FParse::Command(&Cmd, TEXT("Leave")))
		{
			LeaveLobby({});
		}
		else if (FParse::Command(&Cmd, TEXT("UpdateLobbyName")))
		{
			FDriftLobbyProperties LobbyProperties{};
			LobbyProperties.LobbyName = GetString(Cmd);

			UpdateLobby(LobbyProperties, {});
		}
		else if (FParse::Command(&Cmd, TEXT("UpdateLobbyMap")))
		{
			FDriftLobbyProperties LobbyProperties{};
			LobbyProperties.MapName = GetString(Cmd);

			UpdateLobby(LobbyProperties, {});
		}
		else if (FParse::Command(&Cmd, TEXT("UpdateLobbyTeamCapacity")))
		{
			FDriftLobbyProperties LobbyProperties{};
			LobbyProperties.TeamCapacity = GetInt32(Cmd);

			UpdateLobby(LobbyProperties, {});
		}
		else if (FParse::Command(&Cmd, TEXT("UpdateLobbyTeamNames")))
		{
			TArray<FString> TeamNames;
			GetString(Cmd).ParseIntoArray(TeamNames, TEXT(","));

			FDriftLobbyProperties LobbyProperties{};
			LobbyProperties.TeamNames = TeamNames;

			UpdateLobby(LobbyProperties, {});
		}
		else if (FParse::Command(&Cmd, TEXT("UpdatePlayerTeamName")))
		{
			FDriftLobbyMemberProperties MemberProperties{};
			MemberProperties.TeamName = GetString(Cmd);

			UpdatePlayer(MemberProperties, {});
		}
		else if (FParse::Command(&Cmd, TEXT("UpdatePlayerReady")))
		{
			FDriftLobbyMemberProperties MemberProperties{};
			MemberProperties.bReady = static_cast<bool>(GetInt32(Cmd));

			UpdatePlayer(MemberProperties, {});
		}
		else if (FParse::Command(&Cmd, TEXT("KickPlayer")))
		{
			KickLobbyMember(GetInt32(Cmd), {});
		}
		else if (FParse::Command(&Cmd, TEXT("StartMatch")))
		{
			StartLobbyMatch({});
		}
		else
		{
			UE_LOG(LogDriftLobby, Error, TEXT("Unknown lobby command: '%s'"), Cmd);
		}

		return true;
	}
#endif
	return false;
}

bool FDriftLobbyManager::QueryLobby(FQueryLobbyCompletedDelegate Delegate)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Trying to query lobby without a session"));
		(void)Delegate.ExecuteIfBound(false, "");
		return false;
	}

	UE_LOG(LogDriftLobby, Log, TEXT("Querying for current lobby"));

	const auto Request = RequestManager->Get(LobbiesURL);
	Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
	{
		const auto Response = Doc.GetObject();
		if (Response.Num() == 0)
		{
			UE_LOG(LogDriftLobby, Log, TEXT("No lobby found"));

			const auto OldLobbyId = CurrentLobbyId;
			const auto bLobbyDeleted = !OldLobbyId.IsEmpty();

			ResetCurrentLobby();

			if (bLobbyDeleted)
			{
				OnLobbyDeletedDelegate.Broadcast(OldLobbyId);
			}

			(void)Delegate.ExecuteIfBound(true, "");
			return;
		}

		FDriftLobbyResponse LobbyResponse{};
		if (!LobbyResponse.FromJson(Doc.GetInternalValue()->AsObject()))
		{
			UE_LOG(LogDriftLobby, Error, TEXT("Failed to serialize get lobby response"));
			return;
		}

		const auto LobbyMember = LobbyResponse.Members.FindByPredicate(
			[this](const FDriftLobbyResponseMember& Member)
			{
				return PlayerId == Member.PlayerId;
			});

		if (LobbyMember)
		{
			ExtractLobby(LobbyResponse);
			(void)Delegate.ExecuteIfBound(true, CurrentLobbyId);
		}
		else
		{
			UE_LOG(LogDriftLobby, Error, TEXT("Found existing lobby but player is not a member"));
			(void)Delegate.ExecuteIfBound(false, "");
		}
	});
	Request->OnError.BindLambda([Delegate](ResponseContext& Context)
	{
		Context.errorHandled = true;
		(void)Delegate.ExecuteIfBound(false, "");
	});

	return Request->Dispatch();
}

bool FDriftLobbyManager::JoinLobby(FString LobbyId, FJoinLobbyCompletedDelegate Delegate)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Trying to join a lobby without a session"));
		(void)Delegate.ExecuteIfBound(false, "");
		return false;
	}

	UE_LOG(LogDriftLobby, Log, TEXT("Joining lobby %s"), *LobbyId);

	const FString URL = FString::Printf(TEXT("%s%s/members"), *LobbiesURL, *LobbyId); // Manual URL constructing. Maybe figure out a better way to get the members resource of the lobby

	const JsonValue Payload{rapidjson::kObjectType};
	const auto Request = RequestManager->Post(URL, Payload);
	Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
	{
		FDriftLobbyResponse LobbyResponse{};
		if (!LobbyResponse.FromJson(Doc.GetInternalValue()->AsObject()))
		{
			UE_LOG(LogDriftLobby, Error, TEXT("Failed to serialize join lobby response"));
			return;
		}

		ExtractLobby(LobbyResponse);

		UE_LOG(LogDriftLobby, Log, TEXT("Joined lobby '%s'"), *CurrentLobbyId);
		(void)Delegate.ExecuteIfBound(true, CurrentLobbyId);
	});
	Request->OnError.BindLambda([Delegate](ResponseContext& Context)
	{
		Context.errorHandled = true;
		(void)Delegate.ExecuteIfBound(false, "");
	});

	return Request->Dispatch();
}

bool FDriftLobbyManager::LeaveLobby(FLeaveLobbyCompletedDelegate Delegate)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Trying to join a lobby without a session"));
		(void)Delegate.ExecuteIfBound(false, "");
		return false;
	}

	UE_LOG(LogDriftLobby, Log, TEXT("Leaving current lobby. Locally cached lobby: '%s'"), *CurrentLobbyId);

	const auto Request = RequestManager->Delete(LobbiesURL, HttpStatusCodes::NoContent);
	Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
	{
		const auto OldLobbyId = CurrentLobbyId;
		const auto bLobbyDeleted = !OldLobbyId.IsEmpty();

		ResetCurrentLobby();

		if (bLobbyDeleted)
		{
			OnLobbyDeletedDelegate.Broadcast(OldLobbyId);
		}

		(void)Delegate.ExecuteIfBound(true, "");
	});
	Request->OnError.BindLambda([Delegate](ResponseContext& Context)
	{
		Context.errorHandled = true;
		(void)Delegate.ExecuteIfBound(false, "");
	});

	return Request->Dispatch();
}

bool FDriftLobbyManager::CreateLobby(FDriftLobbyProperties LobbyProperties, FCreateLobbyCompletedDelegate Delegate)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Trying to join a lobby without a session"));
		(void)Delegate.ExecuteIfBound(false, "");
		return false;
	}

	UE_LOG(LogDriftLobby, Log, TEXT("Creating lobby with properties: '%s'"), *LobbyProperties.ToString());

	JsonValue Payload{rapidjson::kObjectType};

	if (LobbyProperties.LobbyName.IsSet())
	{
		JsonArchive::AddMember(Payload, TEXT("lobby_name"), *LobbyProperties.LobbyName.GetValue());
	}

	if (LobbyProperties.MapName.IsSet())
	{
		JsonArchive::AddMember(Payload, TEXT("map_name"), *LobbyProperties.MapName.GetValue());
	}

	if (LobbyProperties.TeamNames.IsSet())
	{
		JsonArchive::AddMember(Payload, TEXT("team_names"), LobbyProperties.TeamNames.GetValue());
	}

	if (LobbyProperties.TeamCapacity.IsSet())
	{
		JsonArchive::AddMember(Payload, TEXT("team_capacity"), LobbyProperties.TeamCapacity.GetValue());
	}

	const auto Request = RequestManager->Post(LobbiesURL, Payload);
	Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
	{
		FDriftLobbyResponse LobbyResponse{};
		if (!LobbyResponse.FromJson(Doc.GetInternalValue()->AsObject()))
		{
			UE_LOG(LogDriftLobby, Error, TEXT("Failed to serialize join lobby response"));
			return;
		}

		ExtractLobby(LobbyResponse);
		(void)Delegate.ExecuteIfBound(true, CurrentLobbyId);
	});
	Request->OnError.BindLambda([Delegate](ResponseContext& Context)
	{
		Context.errorHandled = true;
		(void)Delegate.ExecuteIfBound(false, "");
	});

	return Request->Dispatch();
}

bool FDriftLobbyManager::UpdateLobby(FDriftLobbyProperties LobbyProperties, FUpdateLobbyCompletedDelegate Delegate)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Trying to join a lobby without a session"));
		(void)Delegate.ExecuteIfBound(false, "");
		return false;
	}

	if (!IsCurrentLobbyHost())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Only the lobby host can update the lobby properties"));
		(void)Delegate.ExecuteIfBound(false, "");
		return false;
	}

	UE_LOG(LogDriftLobby, Log, TEXT("Updating lobby with properties: '%s'"), *LobbyProperties.ToString());

	JsonValue Payload{rapidjson::kObjectType};

	if (LobbyProperties.LobbyName.IsSet())
	{
		JsonArchive::AddMember(Payload, TEXT("lobby_name"), *LobbyProperties.LobbyName.GetValue());
		CurrentLobby->LobbyName = LobbyProperties.LobbyName.GetValue();
	}

	if (LobbyProperties.MapName.IsSet())
	{
		JsonArchive::AddMember(Payload, TEXT("map_name"), *LobbyProperties.MapName.GetValue());
		CurrentLobby->MapName = LobbyProperties.MapName.GetValue();
	}

	if (LobbyProperties.TeamNames.IsSet())
	{
		JsonArchive::AddMember(Payload, TEXT("team_names"), LobbyProperties.TeamNames.GetValue());
		CurrentLobby->TeamNames = LobbyProperties.TeamNames.GetValue();
	}

	if (LobbyProperties.TeamCapacity.IsSet())
	{
		JsonArchive::AddMember(Payload, TEXT("team_capacity"), LobbyProperties.TeamCapacity.GetValue());
		CurrentLobby->TeamCapacity = LobbyProperties.TeamCapacity.GetValue();
	}

	OnLobbyUpdatedDelegate.Broadcast(CurrentLobbyId);

	const auto Request = RequestManager->Patch(LobbiesURL, Payload);
	Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
	{
		(void)Delegate.ExecuteIfBound(true, CurrentLobbyId);
	});
	Request->OnError.BindLambda([Delegate](ResponseContext& Context)
	{
		Context.errorHandled = true;
		(void)Delegate.ExecuteIfBound(false, "");
	});

	return Request->Dispatch();
}

bool FDriftLobbyManager::UpdatePlayer(FDriftLobbyMemberProperties PlayerProperties, FUpdatePlayerCompletedDelegate Delegate)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Trying to join a lobby without a session"));
		(void)Delegate.ExecuteIfBound(false, "");
		return false;
	}

	if (!CurrentLobby.IsValid())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Trying to join update player properties while not being in a lobby"));
		(void)Delegate.ExecuteIfBound(false, "");
		return false;
	}

	UE_LOG(LogDriftLobby, Log, TEXT("Updating player properties with properties: '%s'"), *PlayerProperties.ToString());

	auto TeamName = CurrentPlayerProperties.TeamName;
	auto bReady = CurrentPlayerProperties.bReady;

	JsonValue Payload{rapidjson::kObjectType};

	if (PlayerProperties.TeamName.IsSet())
	{
		TeamName = PlayerProperties.TeamName;
		CurrentPlayerProperties.TeamName = PlayerProperties.TeamName.GetValue();
	}

	if (PlayerProperties.bReady.IsSet())
	{
		bReady = PlayerProperties.bReady;
		CurrentPlayerProperties.bReady = PlayerProperties.bReady.GetValue();
	}

	JsonArchive::AddMember(Payload, TEXT("team_name"), TeamName.IsSet() ? *TeamName.GetValue() : nullptr);
	JsonArchive::AddMember(Payload, TEXT("ready"), bReady.IsSet() ? bReady.GetValue() : false);

	ApplyCurrentPlayerProperties();
	OnLobbyUpdatedDelegate.Broadcast(CurrentLobbyId);

	const auto Request = RequestManager->Put(CurrentLobbyMemberURL, Payload);
	Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
	{
		(void)Delegate.ExecuteIfBound(true, CurrentLobbyId);
	});
	Request->OnError.BindLambda([Delegate](ResponseContext& Context)
	{
		Context.errorHandled = true;
		(void)Delegate.ExecuteIfBound(false, "");
	});

	return Request->Dispatch();
}

bool FDriftLobbyManager::KickLobbyMember(int32 MemberPlayerId, FKickMemberCompletedDelegate Delegate)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Trying to join a lobby without a session"));
		(void)Delegate.ExecuteIfBound(false, "", INDEX_NONE);
		return false;
	}

	if (!IsCurrentLobbyHost())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Only the lobby host can kick lobby member"));
		(void)Delegate.ExecuteIfBound(false, "", INDEX_NONE);
		return false;
	}

	UE_LOG(LogDriftLobby, Log, TEXT("Kicking player '%d' from lobby '%s'"), MemberPlayerId, *CurrentLobbyId);

	FString URL = "";

	const auto PlayerIndex = CurrentLobby->Members.IndexOfByPredicate([MemberPlayerId](const TSharedPtr<FDriftLobbyMember>& Member)
	{
		return Member->PlayerId == MemberPlayerId;
	});

	if (PlayerIndex == INDEX_NONE)
	{
		UE_LOG(LogDriftLobby, Warning, TEXT("Player '%d' not found in locally cached lobby. Maybe out of sync with server"));
	}
	else
	{
		const auto Member = CurrentLobby->Members[PlayerIndex];
		if (Member.IsValid())
		{
			URL = Member->LobbyMemberURL;
		}
		else
		{
			UE_LOG(LogDriftLobby, Warning, TEXT("Player '%d' invalid in locally cached lobby. Maybe out of sync with server"));
		}

		// Update local state for host now
		CurrentLobby->Members.RemoveAt(PlayerIndex);
		OnLobbyMemberKickedDelegate.Broadcast(CurrentLobbyId);
		OnLobbyUpdatedDelegate.Broadcast(CurrentLobbyId);
	}

	if (URL.IsEmpty())
	{
		URL = FString::Printf(TEXT("%s%s/members/%d"), *LobbiesURL, *CurrentLobbyId, MemberPlayerId); // Manual URL constructing as a last resort
	}

	const auto Request = RequestManager->Delete(URL, HttpStatusCodes::NoContent);
	Request->OnResponse.BindLambda([this, MemberPlayerId, Delegate](ResponseContext& Context, JsonDocument& Doc)
	{
		(void)Delegate.ExecuteIfBound(true, "", MemberPlayerId);
	});
	Request->OnError.BindLambda([Delegate](ResponseContext& Context)
	{
		Context.errorHandled = true;
		(void)Delegate.ExecuteIfBound(false, "", INDEX_NONE);
	});

	return Request->Dispatch();
}

bool FDriftLobbyManager::StartLobbyMatch(FStartLobbyMatchCompletedDelegate Delegate)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Trying to join a lobby without a session"));
		(void)Delegate.ExecuteIfBound(false, "");
		return false;
	}

	if (!IsCurrentLobbyHost())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Only the lobby host can start the match"));
		(void)Delegate.ExecuteIfBound(false, "");
		return false;
	}

	UE_LOG(LogDriftLobby, Log, TEXT("Starting the lobby match for lobby '%s'"), *CurrentLobbyId);

	// Update locally for host
	CurrentLobby->LobbyStatus = EDriftLobbyStatus::Starting;

	const JsonValue Payload{rapidjson::kObjectType};
	const auto Request = RequestManager->Post(CurrentLobbyURL, Payload, HttpStatusCodes::NoContent);
	Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
	{
		(void)Delegate.ExecuteIfBound(true, "");
	});
	Request->OnError.BindLambda([Delegate](ResponseContext& Context)
	{
		Context.errorHandled = true;
		(void)Delegate.ExecuteIfBound(false, "");
	});

	return Request->Dispatch();
}

void FDriftLobbyManager::HandleLobbyEvent(const FMessageQueueEntry& Message)
{
	if (Message.sender_id != FDriftMessageQueue::SenderSystemID && Message.sender_id != PlayerId)
	{
		UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - Ignoring message from sender '%d'"), Message.sender_id);
		return;
	}

	const auto Event = Message.payload.FindField("event").GetString();
	const auto EventData = Message.payload.FindField("data");

	UE_LOG(LogDriftLobby, Verbose, TEXT("FDriftLobbyManager::HandleLobbyEvent - Incoming event '%s'"), *Event);

	if (!EventData.HasField("lobby_id"))
	{
		UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - Event data doesn't contain 'lobby_id'. Discarding the event. Current cached lobby id: '%s'"), *CurrentLobbyId);
		return;
	}

	const auto LobbyId = EventData.FindField("lobby_id").GetString();

	// Verify that this event is relevant to us
	if (LobbyId != CurrentLobbyId)
	{
		UE_LOG(LogDriftLobby, Warning, TEXT("FDriftLobbyManager::HandleLobbyEvent - Cached lobby '%s' does not match the event lobby '%s'. Will determine if this event is relevant to us by checking the lobby members."), *CurrentLobbyId, *LobbyId);

		if (!EventData.HasField("members"))
		{
			UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - Event data doesn't contain 'members'. Querying for the current lobby to sync up just in case."));
			QueryLobby({});
			return;
		}

		bool bRelevantEvent = false;
		for (const auto& Elem : EventData.FindField("members").GetArray())
		{
			if (!Elem.HasField("player_id"))
			{
				UE_LOG(LogDriftLobby, Warning, TEXT("FDriftLobbyManager::HandleLobbyEvent - Member has no 'player_id' field. This event is all kinds of messed up. Member: '%s'"), *Elem.ToString());
				continue;
			}

			if (Elem.FindField("player_id").GetInt32() == PlayerId)
			{
				bRelevantEvent = true;
				break;
			}
		}

		if (!bRelevantEvent)
		{
			UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - Player isn't a member of the lobby for this event. Why did we receive this event? Discarding and syncing up with server just in case."));
			QueryLobby({});
			return;
		}
	}

	switch (ParseEvent(Event))
	{
		case EDriftLobbyEvent::LobbyUpdated:
		{
			FDriftLobbyResponse LobbyResponse{};
			if (!LobbyResponse.FromJson(EventData.ToString()))
			{
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - Failed to serialize LobbyUpdated event data"));
				return;
			}

			ExtractLobby(LobbyResponse);
			OnLobbyUpdatedDelegate.Broadcast(CurrentLobbyId);
			break;
		}

		case EDriftLobbyEvent::LobbyDeleted:
		{
			ResetCurrentLobby();
			OnLobbyDeletedDelegate.Broadcast(LobbyId);
			break;
		}

		case EDriftLobbyEvent::LobbyMemberJoined:
		{
			if (ExtractMembers(EventData))
			{
				OnLobbyMemberJoinedDelegate.Broadcast(CurrentLobbyId);
			}
			else
			{
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - Failed to serialize one or more members for LobbyMemberUpdated event data"));
			}
			break;
		}

		case EDriftLobbyEvent::LobbyMemberUpdated:
		{
			if (ExtractMembers(EventData))
			{
				OnLobbyMemberUpdatedDelegate.Broadcast(CurrentLobbyId);
			}
			else
			{
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - Failed to serialize one or more members for LobbyMemberUpdated event data"));
			}
			break;
		}

		case EDriftLobbyEvent::LobbyMemberLeft:
		{
			if (ExtractMembers(EventData))
			{
				OnLobbyMemberLeftDelegate.Broadcast(CurrentLobbyId);
			}
			else
			{
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - Failed to serialize one or more members for LobbyMemberUpdated event data"));
			}
			break;
		}

		case EDriftLobbyEvent::LobbyMemberKicked:
		{
			if (ExtractMembers(EventData))
			{
				OnLobbyMemberKickedDelegate.Broadcast(CurrentLobbyId);
			}
			else
			{
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - Failed to serialize one or more members for LobbyMemberKicked event data"));
			}
			break;
		}

		case EDriftLobbyEvent::LobbyMatchStarting:
		{
			if (!EventData.HasField("status"))
			{
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - LobbyMatchStarting - Event data missing 'status' field"));
				return;
			}

			const auto Status = ParseStatus(EventData.FindField("status").GetString());
			CurrentLobby->LobbyStatus = Status;
			OnLobbyStatusChangedDelegate.Broadcast(CurrentLobbyId, Status);
			break;
		}

		case EDriftLobbyEvent::LobbyMatchStarted:
		{
			if (!EventData.HasField("status"))
			{
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - LobbyMatchStarted - Event data missing 'status' field"));
				return;
			}

			if (!EventData.HasField("connection_string"))
			{
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - LobbyMatchStarted - Event data missing 'connection_string' field"));
				return;
			}

			if (!EventData.HasField("connection_options"))
			{
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - LobbyMatchStarted - Event data missing 'connection_options' field"));
				return;
			}

			CurrentLobby->LobbyStatus = ParseStatus(EventData.FindField("status").GetString());
			CurrentLobby->ConnectionString = EventData.FindField("connection_string").GetString();
			CurrentLobby->ConnectionOptions = EventData.FindField("connection_options").GetString();

			OnLobbyStatusChangedDelegate.Broadcast(CurrentLobbyId, CurrentLobby->LobbyStatus);
			break;
		}

		case EDriftLobbyEvent::LobbyMatchCancelled:
		{
			if (!EventData.HasField("status"))
			{
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - LobbyMatchCancelled - Event data missing 'status' field"));
				return;
			}

			const auto Status = ParseStatus(EventData.FindField("status").GetString());
			CurrentLobby->LobbyStatus = Status;
			OnLobbyStatusChangedDelegate.Broadcast(CurrentLobbyId, Status);
			break;
		}

		case EDriftLobbyEvent::LobbyMatchTimedOut:
		{
			if (!EventData.HasField("status"))
			{
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - LobbyMatchTimedOut - Event data missing 'status' field"));
				return;
			}

			const auto Status = ParseStatus(EventData.FindField("status").GetString());
			CurrentLobby->LobbyStatus = Status;
			OnLobbyStatusChangedDelegate.Broadcast(CurrentLobbyId, Status);
			break;
		}

		case EDriftLobbyEvent::LobbyMatchFailed:
		{
			if (!EventData.HasField("status"))
			{
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - LobbyMatchFailed - Event data missing 'status' field"));
				return;
			}

			const auto Status = ParseStatus(EventData.FindField("status").GetString());
			CurrentLobby->LobbyStatus = Status;
			OnLobbyStatusChangedDelegate.Broadcast(CurrentLobbyId, Status);
			break;
		}

		case EDriftLobbyEvent::Unknown:
		default:
			UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - Unknown event '%s'"), *Event);
	}
}

EDriftLobbyEvent FDriftLobbyManager::ParseEvent(const FString& EventName)
{
	if (EventName == TEXT("LobbyUpdated"))
		return EDriftLobbyEvent::LobbyUpdated;
	if (EventName == TEXT("LobbyDeleted"))
		return EDriftLobbyEvent::LobbyDeleted;
	if (EventName == TEXT("LobbyMemberJoined"))
		return EDriftLobbyEvent::LobbyMemberJoined;
	if (EventName == TEXT("LobbyMemberUpdated"))
		return EDriftLobbyEvent::LobbyMemberUpdated;
	if (EventName == TEXT("LobbyMemberLeft"))
		return EDriftLobbyEvent::LobbyMemberLeft;
	if (EventName == TEXT("LobbyMemberKicked"))
		return EDriftLobbyEvent::LobbyMemberKicked;
	if (EventName == TEXT("LobbyMatchStarting"))
		return EDriftLobbyEvent::LobbyMatchStarting;
	if (EventName == TEXT("LobbyMatchStarted"))
		return EDriftLobbyEvent::LobbyMatchStarted;
	if (EventName == TEXT("LobbyMatchCancelled"))
		return EDriftLobbyEvent::LobbyMatchCancelled;
	if (EventName == TEXT("LobbyMatchTimedOut"))
		return EDriftLobbyEvent::LobbyMatchTimedOut;
	if (EventName == TEXT("LobbyMatchFailed"))
		return EDriftLobbyEvent::LobbyMatchFailed;

	return EDriftLobbyEvent::Unknown;
}

EDriftLobbyStatus FDriftLobbyManager::ParseStatus(const FString& Status)
{
	if (Status == TEXT("idle"))
		return EDriftLobbyStatus::Idle;
	if (Status == TEXT("starting"))
		return EDriftLobbyStatus::Starting;
	if (Status == TEXT("started"))
		return EDriftLobbyStatus::Started;
	if (Status == TEXT("cancelled"))
		return EDriftLobbyStatus::Cancelled;
	if (Status == TEXT("timed_out"))
		return EDriftLobbyStatus::TimedOut;
	if (Status == TEXT("failed"))
		return EDriftLobbyStatus::Failed;

	return EDriftLobbyStatus::Unknown;
}

bool FDriftLobbyManager::HasSession() const
{
	return !LobbiesURL.IsEmpty() && RequestManager.IsValid();
}

void FDriftLobbyManager::ExtractLobby(const FDriftLobbyResponse& LobbyResponse)
{
	CurrentLobbyId = LobbyResponse.LobbyId;
	CurrentLobbyURL = LobbyResponse.LobbyURL;
	CurrentLobbyMembersURL = LobbyResponse.LobbyMembersURL;
	CurrentLobbyMemberURL = LobbyResponse.LobbyMemberURL;

	TArray<TSharedPtr<FDriftLobbyMember>> Members;
	for (const auto& Member : LobbyResponse.Members)
	{
		Members.Add(MakeShared<FDriftLobbyMember>(
			Member.PlayerId,
			Member.PlayerName,
			Member.TeamName,
			Member.bReady,
			Member.bHost,
			Member.LobbyMemberURL
		));
	}

	CurrentLobby = MakeShared<FDriftLobby>(
		CurrentLobbyId,
		LobbyResponse.LobbyName,
		LobbyResponse.MapName,
		LobbyResponse.TeamNames,
		LobbyResponse.TeamCapacity,
		ParseStatus(LobbyResponse.LobbyStatus),
		MoveTemp(Members),
		LobbyResponse.LobbyURL,
		LobbyResponse.LobbyMembersURL,
		LobbyResponse.LobbyMemberURL
	);

	UpdateCurrentPlayerProperties();

	UE_LOG(LogDriftLobby, Log, TEXT("Current lobby updated: '%s'"), *CurrentLobbyId);
	OnLobbyUpdatedDelegate.Broadcast(CurrentLobbyId);
}

bool FDriftLobbyManager::ExtractMembers(const JsonValue& EventData)
{
	TArray<TSharedPtr<FDriftLobbyMember>> Members;

	for (const auto& Elem : EventData.FindField("members").GetArray())
	{
		FDriftLobbyResponseMember LobbyResponseMember{};
		if (!LobbyResponseMember.FromJson(Elem.ToString()))
		{
			UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::ExtractMembers - Failed to serialize member data"));
			return false;
		}

		Members.Add(MakeShared<FDriftLobbyMember>(
			LobbyResponseMember.PlayerId,
			LobbyResponseMember.PlayerName,
			LobbyResponseMember.TeamName,
			LobbyResponseMember.bReady,
			LobbyResponseMember.bHost,
			LobbyResponseMember.LobbyMemberURL
		));
	}

	CurrentLobby->Members = MoveTemp(Members);

	UpdateCurrentPlayerProperties();

	return true;
}

void FDriftLobbyManager::ResetCurrentLobby()
{
	CurrentLobby.Reset();
	CurrentLobbyId.Empty();
	CurrentLobbyURL.Empty();
	CurrentLobbyMembersURL.Empty();
	CurrentLobbyMemberURL.Empty();

	CurrentPlayerProperties.bReady = false;
	CurrentPlayerProperties.TeamName.Reset();

	UE_LOG(LogDriftLobby, Verbose, TEXT("Current lobby state reset"));
}

bool FDriftLobbyManager::IsCurrentLobbyHost() const
{
	if (CurrentLobby.IsValid())
	{
		const auto PlayerPtr = CurrentLobby->Members.FindByPredicate([this](const TSharedPtr<FDriftLobbyMember>& Member)
		{
			return Member->PlayerId == PlayerId;
		});

		if (!PlayerPtr)
		{
			UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::IsCurrentLobbyHost - Player isn't a member of the locally cached lobby"));
			return false;
		}

		return PlayerPtr->Get()->bHost;
	}

	return false;
}

bool FDriftLobbyManager::UpdateCurrentPlayerProperties()
{
	if (!CurrentLobby.IsValid())
	{
		return false;
	}

	const auto PlayerPtr = CurrentLobby->Members.FindByPredicate([this](const TSharedPtr<FDriftLobbyMember>& Member)
	{
		return Member->PlayerId == PlayerId;
	});

	if (!PlayerPtr)
	{
		UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::UpdateCurrentPlayerProperties - Player isn't a member of the locally cached lobby"));
		return false;
	}

	const auto Player = *PlayerPtr;
	if (!Player.IsValid())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Failed to update current player properties. Player member pointer is invalid"));
		return false;
	}

	CurrentPlayerProperties.TeamName = Player->TeamName;
	CurrentPlayerProperties.bReady = Player->bReady;

	return true;
}

bool FDriftLobbyManager::ApplyCurrentPlayerProperties()
{
	if (!CurrentLobby.IsValid())
	{
		return false;
	}

	const auto PlayerIndex = CurrentLobby->Members.IndexOfByPredicate([this](const TSharedPtr<FDriftLobbyMember>& Member)
	{
		return Member->PlayerId == PlayerId;
	});

	if (PlayerIndex == INDEX_NONE)
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Failed to apply current player properties. Player isn't a member of the locally cached lobby"));
		return false;
	}

	const auto Player = CurrentLobby->Members[PlayerIndex];
	if (!Player.IsValid())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Failed to apply current player properties. Player member pointer is invalid"));
		return false;
	}

	if (CurrentPlayerProperties.TeamName.IsSet())
	{
		Player->TeamName = CurrentPlayerProperties.TeamName.GetValue();
	}

	if (CurrentPlayerProperties.bReady.IsSet())
	{
		Player->bReady = CurrentPlayerProperties.bReady.GetValue();
	}

	return true;
}
