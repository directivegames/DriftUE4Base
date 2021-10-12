// Copyright 2021 Directive Games Limited - All Rights Reserved

#include "DriftLobbyManager.h"


DEFINE_LOG_CATEGORY(LogDriftLobby);

static const FString LobbyMessageQueue(TEXT("lobby"));

struct FDriftMatchPlacementResponse : FJsonSerializable
{
	BEGIN_JSON_SERIALIZER;
	JSON_SERIALIZE("placement_id", PlacementId);
	JSON_SERIALIZE("player_id", PlayerId);
	JSON_SERIALIZE("match_provider", MatchProvider);
	JSON_SERIALIZE("status", Status);
	JSON_SERIALIZE("lobby_id", LobbyId);
	JSON_SERIALIZE("match_placement_url", MatchPlacementURL);
	END_JSON_SERIALIZER;

	FString PlacementId = "";
	int32 PlayerId = 0;
	FString MatchProvider = "";
	FString Status = "";
	FString LobbyId = "";
	FString MatchPlacementURL = "";
};

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
	RequestManager = MoveTemp(RootRequestManager);
}

void FDriftLobbyManager::ConfigureSession(const FDriftEndpointsResponse& DriftEndpoints, int32 InPlayerId)
{
	PlayerId = InPlayerId;

	MatchPlacementsURL = DriftEndpoints.match_placements;
	LobbiesURL = DriftEndpoints.lobbies;
	TemplateLobbyMemberURL = DriftEndpoints.template_lobby_member;
	TemplateLobbyMembersURL = DriftEndpoints.template_lobby_members;

	if (HasSession())
	{
		InitializeLocalState();
	}
}

void FDriftLobbyManager::InitializeLocalState()
{
	UE_LOG(LogDriftLobby, Log, TEXT("Querying for initial lobby state"));

	const auto Request = RequestManager->Get(LobbiesURL);
	Request->OnResponse.BindLambda([this](ResponseContext& Context, JsonDocument& Doc)
	{
		UE_LOG(LogDriftLobby, Verbose, TEXT("InitializeLocalState response:'n'%s'"), *Doc.ToString());

		const auto Response = Doc.GetObject();
		if (Response.Num() == 0)
		{
			UE_LOG(LogDriftLobby, Warning, TEXT("No lobby found when querying for initial state. Should return 404, not '%d'"), Context.response->GetResponseCode());
			ResetCurrentLobby();
			return;
		}

		FDriftLobbyResponse LobbyResponse{};
		if (!LobbyResponse.FromJson(Doc.GetInternalValue()->AsObject()))
		{
			UE_LOG(LogDriftLobby, Error, TEXT("Failed to serialize get lobby response"));
			return;
		}

		const auto LobbyMember = LobbyResponse.Members.FindByPredicate([this](const FDriftLobbyResponseMember& Member)
		{
			return PlayerId == Member.PlayerId;
		});

		if (LobbyMember)
		{
			CacheLobby(LobbyResponse);
		}
		else
		{
			UE_LOG(LogDriftLobby, Error, TEXT("Found existing lobby but player is not a member"));
		}
	});
	Request->OnError.BindLambda([this](ResponseContext& Context)
	{
		if (Context.response->GetResponseCode() == EHttpResponseCodes::NotFound)
		{
			UE_LOG(LogDriftLobby, Log, TEXT("No existing lobby found"));
			Context.errorHandled = true;
		}
		else
		{
			FString Error;
			Context.errorHandled = GetResponseError(Context, Error);
			UE_LOG(LogDriftLobby, Error, TEXT("InitializeLocalState - Error fetching existing lobby, Response code %d, error: '%s'"), Context.responseCode, *Error);
		}

		ResetCurrentLobby();
	});

	Request->Dispatch();
}

bool FDriftLobbyManager::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
#if !UE_BUILD_SHIPPING
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
			StartLobbyMatch(GetString(Cmd), {});
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
		(void)Delegate.ExecuteIfBound(false, "", "No backend connection");
		return false;
	}

	UE_LOG(LogDriftLobby, Log, TEXT("Querying for current lobby"));

	const auto Request = RequestManager->Get(LobbiesURL);
	Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
	{
		UE_LOG(LogDriftLobby, Verbose, TEXT("QueryLobby response:'n'%s'"), *Doc.ToString());

		const auto Response = Doc.GetObject();
		if (Response.Num() == 0)
		{
			UE_LOG(LogDriftLobby, Log, TEXT("No lobby found"));

			const auto OldLobbyId = CurrentLobbyId;

			ResetCurrentLobby();

			if (!OldLobbyId.IsEmpty())
			{
				OnLobbyDeletedDelegate.Broadcast(OldLobbyId);
			}

			(void)Delegate.ExecuteIfBound(true, "", "");
			return;
		}

		FDriftLobbyResponse LobbyResponse{};
		if (!LobbyResponse.FromJson(Doc.GetInternalValue()->AsObject()))
		{
			UE_LOG(LogDriftLobby, Error, TEXT("Failed to serialize get lobby response"));
			return;
		}

		const auto LobbyMember = LobbyResponse.Members.FindByPredicate([this](const FDriftLobbyResponseMember& Member)
		{
			return PlayerId == Member.PlayerId;
		});

		if (LobbyMember)
		{
			CacheLobby(LobbyResponse);
			(void)Delegate.ExecuteIfBound(true, CurrentLobbyId, "");
		}
		else
		{
			UE_LOG(LogDriftLobby, Error, TEXT("Found existing lobby but player is not a member"));
			ResetCurrentLobby();
			(void)Delegate.ExecuteIfBound(false, "", "Lobby found, but you're not registered as a member of the lobby");
		}
	});
	Request->OnError.BindLambda([Delegate](ResponseContext& Context)
	{
		FString Error;
		Context.errorHandled = GetResponseError(Context, Error);
		(void)Delegate.ExecuteIfBound(false, "", Error);
	});

	return Request->Dispatch();
}

bool FDriftLobbyManager::JoinLobby(FString LobbyId, FJoinLobbyCompletedDelegate Delegate)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Trying to join a lobby without a session"));
		(void)Delegate.ExecuteIfBound(false, LobbyId, "No backend connection");
		return false;
	}

	UE_LOG(LogDriftLobby, Log, TEXT("Joining lobby %s"), *LobbyId);

	const auto URL = TemplateLobbyMembersURL.Replace(TEXT("{lobby_id}"), *LobbyId);

	const JsonValue Payload{rapidjson::kObjectType};
	const auto Request = RequestManager->Post(URL, Payload);
	Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
	{
		UE_LOG(LogDriftLobby, Verbose, TEXT("JoinLobby response:'n'%s'"), *Doc.ToString());

		FDriftLobbyResponse LobbyResponse{};
		if (!LobbyResponse.FromJson(Doc.GetInternalValue()->AsObject()))
		{
			UE_LOG(LogDriftLobby, Error, TEXT("Failed to serialize join lobby response"));
			return;
		}

		CacheLobby(LobbyResponse);

		UE_LOG(LogDriftLobby, Log, TEXT("Joined lobby '%s'"), *CurrentLobbyId);
		(void)Delegate.ExecuteIfBound(true, CurrentLobbyId, "");
	});
	Request->OnError.BindLambda([LobbyId, Delegate](ResponseContext& Context)
	{
		FString Error;
		Context.errorHandled = GetResponseError(Context, Error);
		(void)Delegate.ExecuteIfBound(false, LobbyId, Error);
	});

	return Request->Dispatch();
}

bool FDriftLobbyManager::LeaveLobby(FLeaveLobbyCompletedDelegate Delegate)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Trying to leave a lobby without a session"));
		(void)Delegate.ExecuteIfBound(false, CurrentLobbyId, "No backend connection");
		return false;
	}

	if (CurrentLobbyMemberURL.IsEmpty())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Trying to leave a lobby without having a locally cached lobby. Unable to determine which lobby to leave."));
		(void)Delegate.ExecuteIfBound(false, CurrentLobbyId, "No lobby found to leave");
		return false;
	}

	UE_LOG(LogDriftLobby, Log, TEXT("Leaving current lobby. Locally cached lobby: '%s'"), *CurrentLobbyId);

	const auto Request = RequestManager->Delete(CurrentLobbyMemberURL, HttpStatusCodes::NoContent);
	Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
	{
		UE_LOG(LogDriftLobby, Log, TEXT("Left lobby '%s''"), *CurrentLobbyId);

		const auto OldLobbyId = CurrentLobbyId;
		const auto bLobbyDeleted = !OldLobbyId.IsEmpty();

		ResetCurrentLobby();

		if (bLobbyDeleted)
		{
			OnLobbyDeletedDelegate.Broadcast(OldLobbyId);
		}

		(void)Delegate.ExecuteIfBound(true, CurrentLobbyId, "");
	});
	Request->OnError.BindLambda([this, Delegate](ResponseContext& Context)
	{
		FString Error;
		Context.errorHandled = GetResponseError(Context, Error);
		(void)Delegate.ExecuteIfBound(false, CurrentLobbyId, Error);
	});

	return Request->Dispatch();
}

bool FDriftLobbyManager::CreateLobby(FDriftLobbyProperties LobbyProperties, FCreateLobbyCompletedDelegate Delegate)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Trying to create a lobby without a session"));
		(void)Delegate.ExecuteIfBound(false, "", "No backend connection");
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
		UE_LOG(LogDriftLobby, Log, TEXT("Lobby created"));

		UE_LOG(LogDriftLobby, Verbose, TEXT("CreateLobby response:'n'%s'"), *Doc.ToString());

		FDriftLobbyResponse LobbyResponse{};
		if (!LobbyResponse.FromJson(Doc.GetInternalValue()->AsObject()))
		{
			UE_LOG(LogDriftLobby, Error, TEXT("Failed to serialize create lobby response"));
			return;
		}

		CacheLobby(LobbyResponse);
		(void)Delegate.ExecuteIfBound(true, CurrentLobbyId, "");
	});
	Request->OnError.BindLambda([Delegate](ResponseContext& Context)
	{
		FString Error;
		Context.errorHandled = GetResponseError(Context, Error);
		(void)Delegate.ExecuteIfBound(false, "", Error);
	});

	return Request->Dispatch();
}

bool FDriftLobbyManager::UpdateLobby(FDriftLobbyProperties LobbyProperties, FUpdateLobbyCompletedDelegate Delegate)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Trying to update lobby properties without a session"));
		(void)Delegate.ExecuteIfBound(false, CurrentLobbyId, "No backend connection");
		return false;
	}

	if (!IsCurrentLobbyHost())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Only the lobby host can update the lobby properties"));
		(void)Delegate.ExecuteIfBound(false, CurrentLobbyId, "You are not the host. Only the host can update the lobby");
		return false;
	}

	UE_LOG(LogDriftLobby, Log, TEXT("Updating lobby with properties: '%s'"), *LobbyProperties.ToString());

	JsonValue Payload{rapidjson::kObjectType};

	if (LobbyProperties.LobbyName.IsSet())
	{
		JsonArchive::AddMember(Payload, TEXT("lobby_name"), *LobbyProperties.LobbyName.GetValue());
		CurrentLocalLobbyProperties.LobbyName = LobbyProperties.LobbyName.GetValue();
	}

	if (LobbyProperties.MapName.IsSet())
	{
		JsonArchive::AddMember(Payload, TEXT("map_name"), *LobbyProperties.MapName.GetValue());
		CurrentLocalLobbyProperties.MapName = LobbyProperties.MapName.GetValue();
	}

	if (LobbyProperties.TeamNames.IsSet())
	{
		JsonArchive::AddMember(Payload, TEXT("team_names"), LobbyProperties.TeamNames.GetValue());
		CurrentLocalLobbyProperties.TeamNames = LobbyProperties.TeamNames.GetValue();
	}

	if (LobbyProperties.TeamCapacity.IsSet())
	{
		JsonArchive::AddMember(Payload, TEXT("team_capacity"), LobbyProperties.TeamCapacity.GetValue());
		CurrentLocalLobbyProperties.TeamCapacity = LobbyProperties.TeamCapacity.GetValue();
	}

	if (LobbyProperties.CustomData.IsSet())
	{
		JsonArchive::AddMember(Payload, TEXT("custom_data"), LobbyProperties.CustomData.GetValue());
		CurrentLocalLobbyProperties.CustomData = LobbyProperties.CustomData.GetValue();
	}

	ApplyLobbyProperties(CurrentLocalLobbyProperties);
	OnLobbyUpdatedDelegate.Broadcast(CurrentLobbyId);

	const auto Request = RequestManager->Patch(CurrentLobbyURL, Payload);
	Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
	{
		UE_LOG(LogDriftLobby, Log, TEXT("Lobby updated"));

		// Update the current properties to local optimistic properties
		CurrentLobbyProperties = CurrentLocalLobbyProperties;

		(void)Delegate.ExecuteIfBound(true, CurrentLobbyId, "");
	});
	Request->OnError.BindLambda([this, Delegate](ResponseContext& Context)
	{
		// Revert the state prior to the request
		CurrentLobbyProperties = CurrentLocalLobbyProperties;
		ApplyLobbyProperties(CurrentLobbyProperties);
		OnLobbyUpdatedDelegate.Broadcast(CurrentLobbyId);

		FString Error;
		Context.errorHandled = GetResponseError(Context, Error);
		(void)Delegate.ExecuteIfBound(false, CurrentLobbyId, Error);
	});

	return Request->Dispatch();
}

bool FDriftLobbyManager::UpdatePlayer(FDriftLobbyMemberProperties PlayerProperties, FUpdatePlayerCompletedDelegate Delegate)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Trying to update player properties without a session"));
		(void)Delegate.ExecuteIfBound(false, CurrentLobbyId, "No backend connection");
		return false;
	}

	if (!CurrentLobby.IsValid())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Trying to join update player properties while not being in a lobby"));
		(void)Delegate.ExecuteIfBound(false, CurrentLobbyId, "You are not in a lobby. Must be in a lobby to update your properties");
		return false;
	}

	UE_LOG(LogDriftLobby, Log, TEXT("Updating player properties with properties: '%s'"), *PlayerProperties.ToString());

	const auto OldPlayerProperties = CurrentPlayerProperties;

	auto TeamName = CurrentPlayerProperties.TeamName;
	auto bReady = CurrentPlayerProperties.bReady;

	JsonValue Payload{rapidjson::kObjectType};

	if (PlayerProperties.TeamName.IsSet())
	{
		TeamName = PlayerProperties.TeamName;
		CurrentLocalPlayerProperties.TeamName = PlayerProperties.TeamName.GetValue();
	}

	if (PlayerProperties.bReady.IsSet())
	{
		bReady = PlayerProperties.bReady;
		CurrentLocalPlayerProperties.bReady = PlayerProperties.bReady.GetValue();
	}

	JsonArchive::AddMember(Payload, TEXT("team_name"), TeamName.IsSet() ? *TeamName.GetValue() : nullptr);
	JsonArchive::AddMember(Payload, TEXT("ready"), bReady.IsSet() ? bReady.GetValue() : false);

	ApplyPlayerProperties(CurrentLocalPlayerProperties);
	OnLobbyUpdatedDelegate.Broadcast(CurrentLobbyId);

	const auto Request = RequestManager->Put(CurrentLobbyMemberURL, Payload);
	Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
	{
		UE_LOG(LogDriftLobby, Log, TEXT("Lobby player updated"));

		// Update the current properties to local optimistic properties
		CurrentPlayerProperties = CurrentLocalPlayerProperties;

		(void)Delegate.ExecuteIfBound(true, CurrentLobbyId, "");
	});
	Request->OnError.BindLambda([this, OldPlayerProperties, Delegate](ResponseContext& Context)
	{
		// Revert the state prior to the request
		CurrentLocalPlayerProperties = CurrentPlayerProperties;
		ApplyPlayerProperties(CurrentPlayerProperties);
		OnLobbyUpdatedDelegate.Broadcast(CurrentLobbyId);

		FString Error;
		Context.errorHandled = GetResponseError(Context, Error);
		(void)Delegate.ExecuteIfBound(false, CurrentLobbyId, Error);
	});

	return Request->Dispatch();
}

bool FDriftLobbyManager::KickLobbyMember(int32 MemberPlayerId, FKickMemberCompletedDelegate Delegate)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Trying to kick a lobby member without a session"));
		(void)Delegate.ExecuteIfBound(false, CurrentLobbyId, INDEX_NONE, "No backend connection");
		return false;
	}

	if (!IsCurrentLobbyHost())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Only the lobby host can kick lobby member"));
		(void)Delegate.ExecuteIfBound(false, CurrentLobbyId, INDEX_NONE, "You are not the host. Only the host can kick other lobby members");
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
		UE_LOG(LogDriftLobby, Warning, TEXT("Player '%d' not found in locally cached lobby. Maybe out of sync with server. Will query just in case"));
		QueryLobby({});
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
			UE_LOG(LogDriftLobby, Warning, TEXT("Player '%d' invalid in locally cached lobby. Maybe out of sync with server. Will query just in case"));
			QueryLobby({});
		}

		// Update local state for host now
		CurrentLobby->Members.RemoveAt(PlayerIndex);
		OnLobbyMemberKickedDelegate.Broadcast(CurrentLobbyId);
		OnLobbyUpdatedDelegate.Broadcast(CurrentLobbyId);
	}

	if (URL.IsEmpty())
	{
		// Use template URL as last resort
		URL = TemplateLobbyMemberURL.Replace(TEXT("{lobby_id}"), *CurrentLobbyId).Replace(TEXT("{lobby_member_id}"), *FString::FromInt(MemberPlayerId));
	}

	const auto Request = RequestManager->Delete(URL, HttpStatusCodes::NoContent);
	Request->OnResponse.BindLambda([this, MemberPlayerId, Delegate](ResponseContext& Context, JsonDocument& Doc)
	{
		UE_LOG(LogDriftLobby, Log, TEXT("Player '%d' kicked from lobby"), MemberPlayerId);

		(void)Delegate.ExecuteIfBound(true, "", MemberPlayerId, "");
	});
	Request->OnError.BindLambda([this, Delegate](ResponseContext& Context)
	{
		FString Error;
		Context.errorHandled = GetResponseError(Context, Error);
		(void)Delegate.ExecuteIfBound(false, CurrentLobbyId, INDEX_NONE, Error);
	});

	return Request->Dispatch();
}

bool FDriftLobbyManager::StartLobbyMatch(FString Queue, FStartLobbyMatchCompletedDelegate Delegate)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Trying to start the lobby match without a session"));
		(void)Delegate.ExecuteIfBound(false, CurrentLobbyId, "No backend connection");
		return false;
	}

	if (!IsCurrentLobbyHost())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Only the lobby host can start the match"));
		(void)Delegate.ExecuteIfBound(false, CurrentLobbyId, "You are not the host. Only the host can start the match");
		return false;
	}

	if (CurrentLobby->LobbyStatus == EDriftLobbyStatus::Starting)
	{
		UE_LOG(LogDriftLobby, Warning, TEXT("Lobby match is already starting, ignoring start lobby match request"));
		return true;
	}

	UE_LOG(LogDriftLobby, Log, TEXT("Starting the lobby match for lobby '%s'"), *CurrentLobbyId);

	// Update locally for host
	CurrentLobby->LobbyStatus = EDriftLobbyStatus::Starting;

	JsonValue Payload{rapidjson::kObjectType};
	JsonArchive::AddMember(Payload, TEXT("queue"), Queue);
	JsonArchive::AddMember(Payload, TEXT("lobby_id"), CurrentLobbyId);

	const auto Request = RequestManager->Post(MatchPlacementsURL, Payload, HttpStatusCodes::NoContent);
	Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
	{
		UE_LOG(LogDriftLobby, Log, TEXT("Lobby match start request accepted"));

		UE_LOG(LogDriftLobby, Verbose, TEXT("StartLobbyMatch response:'n'%s'"), *Doc.ToString());

		FDriftMatchPlacementResponse MatchPlacementResponse{};
		if (!MatchPlacementResponse.FromJson(Doc.GetInternalValue()->AsObject()))
		{
			UE_LOG(LogDriftLobby, Error, TEXT("Failed to serialize start lobby match response"));
			return;
		}

		CurrentLobby->LobbyMatchPlacementURL = MatchPlacementResponse.MatchPlacementURL;

		(void)Delegate.ExecuteIfBound(true, "", "");

		OnLobbyStatusChangedDelegate.Broadcast(CurrentLobbyId, CurrentLobby->LobbyStatus);
	});
	Request->OnError.BindLambda([this, Delegate](ResponseContext& Context)
	{
	    CurrentLobby->LobbyStatus = EDriftLobbyStatus::Failed;

		FString Error;
		Context.errorHandled = GetResponseError(Context, Error);
		(void)Delegate.ExecuteIfBound(false, CurrentLobbyId, Error);
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
		UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - Event data doesn't contain 'lobby_id'. Discarding the event. Current cached lobby id: '%s'. Querying for the current lobby to sync up just in case."), *CurrentLobbyId);
		QueryLobby({});
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
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - Failed to serialize LobbyUpdated event data. Syncing up the lobby state just in case."));
				QueryLobby({});
				return;
			}

			CacheLobby(LobbyResponse, false);
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
			if (CacheMembers(EventData))
			{
				OnLobbyMemberJoinedDelegate.Broadcast(CurrentLobbyId);
				return;
			}

			UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - Failed to serialize one or more members for LobbyMemberJoined event data. Syncing up the lobby state just in case."));
			QueryLobby({});
			break;
		}

		case EDriftLobbyEvent::LobbyMemberUpdated:
		{
			if (CacheMembers(EventData))
			{
				OnLobbyMemberUpdatedDelegate.Broadcast(CurrentLobbyId);
				return;
			}

			UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - Failed to serialize one or more members for LobbyMemberUpdated event data. Syncing up the lobby state just in case."));
			QueryLobby({});
			break;
		}

		case EDriftLobbyEvent::LobbyMemberLeft:
		{
			if (CacheMembers(EventData))
			{
				OnLobbyMemberLeftDelegate.Broadcast(CurrentLobbyId);
				return;
			}

			UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - Failed to serialize one or more members for LobbyMemberLeft event data. Syncing up the lobby state just in case."));
			QueryLobby({});
			break;
		}

		case EDriftLobbyEvent::LobbyMemberKicked:
		{
			if (CacheMembers(EventData))
			{
				OnLobbyMemberKickedDelegate.Broadcast(CurrentLobbyId);
				return;
			}

			UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - Failed to serialize one or more members for LobbyMemberKicked event data. Syncing up the lobby state just in case."));
			QueryLobby({});
			break;
		}

		case EDriftLobbyEvent::LobbyMatchStarting:
		{
			if (!EventData.HasField("status"))
			{
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - LobbyMatchStarting - Event data missing 'status' field. Syncing up the lobby state just in case."));
				QueryLobby({});
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
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - LobbyMatchStarted - Event data missing 'status' field. Syncing up the lobby state just in case."));
				QueryLobby({});
				return;
			}

			if (!EventData.HasField("connection_string"))
			{
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - LobbyMatchStarted - Event data missing 'connection_string' field. Syncing up the lobby state just in case."));
				QueryLobby({});
				return;
			}

			if (!EventData.HasField("connection_options"))
			{
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - LobbyMatchStarted - Event data missing 'connection_options' field. Syncing up the lobby state just in case."));
				QueryLobby({});
				return;
			}

			CurrentLobby->LobbyStatus = ParseStatus(EventData.FindField("status").GetString());
			CurrentLobby->ConnectionString = EventData.FindField("connection_string").GetString();
			CurrentLobby->ConnectionOptions = EventData.FindField("connection_options").GetString();

			OnLobbyStatusChangedDelegate.Broadcast(CurrentLobbyId, CurrentLobby->LobbyStatus);
			OnLobbyMatchStartedDelegate.Broadcast(CurrentLobbyId, CurrentLobby->ConnectionString, CurrentLobby->ConnectionOptions);
			break;
		}

		case EDriftLobbyEvent::LobbyMatchCancelled:
		{
			if (!EventData.HasField("status"))
			{
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - LobbyMatchCancelled - Event data missing 'status' field. Syncing up the lobby state just in case."));
				QueryLobby({});
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
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - LobbyMatchTimedOut - Event data missing 'status' field. Syncing up the lobby state just in case."));
				QueryLobby({});
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
				UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - LobbyMatchFailed - Event data missing 'status' field. Syncing up the lobby state just in case."));
				QueryLobby({});
				return;
			}

			const auto Status = ParseStatus(EventData.FindField("status").GetString());
			CurrentLobby->LobbyStatus = Status;
			OnLobbyStatusChangedDelegate.Broadcast(CurrentLobbyId, Status);
			break;
		}

		case EDriftLobbyEvent::Unknown:
		default:
			UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::HandleLobbyEvent - Unknown event '%s'. Syncing up the lobby state just in case."), *Event);
			QueryLobby({});
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
	return !LobbiesURL.IsEmpty() &&
		!MatchPlacementsURL.IsEmpty() &&
		!TemplateLobbyMemberURL.IsEmpty() &&
		!TemplateLobbyMembersURL.IsEmpty() &&
		RequestManager.IsValid();
}

void FDriftLobbyManager::CacheLobby(const FDriftLobbyResponse& LobbyResponse, bool bUpdateURLs)
{
	CurrentLobbyId = LobbyResponse.LobbyId;

	if (bUpdateURLs)
	{
		CurrentLobbyURL = LobbyResponse.LobbyURL;
		CurrentLobbyMembersURL = LobbyResponse.LobbyMembersURL;
		CurrentLobbyMemberURL = LobbyResponse.LobbyMemberURL;
	}

	bool bAllTeamMembersReady = true;
	TSharedPtr<FDriftLobbyMember> LocalMember;
	TArray<TSharedPtr<FDriftLobbyMember>> Members;
	for (const auto& ResponseMember : LobbyResponse.Members)
	{
		const auto Member = MakeShared<FDriftLobbyMember>(
			ResponseMember.PlayerId,
			ResponseMember.PlayerName,
			ResponseMember.TeamName,
			ResponseMember.bReady,
			ResponseMember.bHost,
			ResponseMember.PlayerId == PlayerId,
			ResponseMember.LobbyMemberURL
		);

		if (Member->bLocalPlayer)
		{
			LocalMember = Member;
		}

		if (!Member->bReady && Member->TeamName.IsSet() && !Member->TeamName->IsEmpty())
		{
			bAllTeamMembersReady = false;
		}

		Members.Emplace(Member);
	}

	CurrentLobby = MakeShared<FDriftLobby>(
		CurrentLobbyId,
		LobbyResponse.LobbyName,
		LobbyResponse.MapName,
		LobbyResponse.TeamNames,
		LobbyResponse.TeamCapacity,
		ParseStatus(LobbyResponse.LobbyStatus),
		MoveTemp(Members),
		LocalMember,
		bAllTeamMembersReady,
		LobbyResponse.CustomData,
		LobbyResponse.LobbyURL,
		LobbyResponse.LobbyMembersURL,
		LobbyResponse.LobbyMemberURL,
		LobbyResponse.LobbyMatchPlacementURL
	);

	if (!LobbyResponse.ConnectionString.IsEmpty())
	{
		CurrentLobby->ConnectionString = LobbyResponse.ConnectionString;
		CurrentLobby->ConnectionOptions = LobbyResponse.ConnectionOptions.IsEmpty() ? "SpectatorOnly=1" : LobbyResponse.ConnectionOptions;
	}

	UpdateCurrentPlayerProperties();

	UE_LOG(LogDriftLobby, Log, TEXT("Current lobby updated: '%s'"), *CurrentLobbyId);
	OnLobbyUpdatedDelegate.Broadcast(CurrentLobbyId);
}

bool FDriftLobbyManager::CacheMembers(const JsonValue& EventData)
{
	if (!CurrentLobby)
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Cannot cache members when no local lobby is present. EventData:\n'%s'"), *EventData.ToString());
		return false;
	}

	bool bAllTeamMembersReady = true;
	TSharedPtr<FDriftLobbyMember> LocalMember;
	TArray<TSharedPtr<FDriftLobbyMember>> Members;
	for (const auto& Elem : EventData.FindField("members").GetArray())
	{
		FDriftLobbyResponseMember LobbyResponseMember{};
		if (!LobbyResponseMember.FromJson(Elem.ToString()))
		{
			UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::CacheMembers - Failed to serialize member data. EventData:\n'%s'"), *EventData.ToString());
			return false;
		}

		const auto Member = MakeShared<FDriftLobbyMember>(
			LobbyResponseMember.PlayerId,
			LobbyResponseMember.PlayerName,
			LobbyResponseMember.TeamName,
			LobbyResponseMember.bReady,
			LobbyResponseMember.bHost,
			LobbyResponseMember.PlayerId == PlayerId,
			LobbyResponseMember.LobbyMemberURL
		);

		if (!LobbyResponseMember.bReady && !LobbyResponseMember.TeamName.IsEmpty())
		{
			bAllTeamMembersReady = false;
		}

		if (Member->bLocalPlayer)
		{
			LocalMember = Member;
		}

		Members.Emplace(Member);
	}

	CurrentLobby->Members = MoveTemp(Members);
	CurrentLobby->bAllTeamMembersReady = bAllTeamMembersReady;
	CurrentLobby->LocalPlayerMember = LocalMember;

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
	if (!CurrentLobby.IsValid())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::IsCurrentLobbyHost - No locally cached lobby"));
		return false;
	}

	if (!CurrentLobby->LocalPlayerMember.IsValid())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("FDriftLobbyManager::IsCurrentLobbyHost - Player isn't a member of the locally cached lobby"));
		return false;
	}

	UE_LOG(LogDriftLobby, Verbose, TEXT("FDriftLobbyManager::IsCurrentLobbyHost - Local player is host: '%s'"), CurrentLobby->LocalPlayerMember->bHost ? TEXT("Yes") : TEXT("No"));

	return CurrentLobby->LocalPlayerMember->bHost;
}

bool FDriftLobbyManager::UpdateCurrentPlayerProperties()
{
	if (!CurrentLobby.IsValid())
	{
		return false;
	}

	if (!CurrentLobby->LocalPlayerMember.IsValid())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Failed to apply current player properties. Player member pointer is invalid"));
		return false;
	}

	CurrentPlayerProperties.TeamName = CurrentLobby->LocalPlayerMember->TeamName;
	CurrentPlayerProperties.bReady = CurrentLobby->LocalPlayerMember->bReady;

	return true;
}

bool FDriftLobbyManager::ApplyLobbyProperties(const FDriftLobbyProperties& LobbyProperties)
{
	if (!CurrentLobby.IsValid())
	{
		return false;
	}

	if (LobbyProperties.LobbyName.IsSet())
	{
		CurrentLobby->LobbyName = LobbyProperties.LobbyName.GetValue();
	}

	if (LobbyProperties.MapName.IsSet())
	{
		CurrentLobby->MapName = LobbyProperties.MapName.GetValue();
	}

	if (LobbyProperties.TeamNames.IsSet())
	{
		CurrentLobby->TeamNames = LobbyProperties.TeamNames.GetValue();
	}

	if (LobbyProperties.TeamCapacity.IsSet())
	{
		CurrentLobby->TeamCapacity = LobbyProperties.TeamCapacity.GetValue();
	}

	if (LobbyProperties.CustomData.IsSet())
	{
		CurrentLobby->CustomData = LobbyProperties.CustomData.GetValue();
	}

	return true;
}

bool FDriftLobbyManager::ApplyPlayerProperties(const FDriftLobbyMemberProperties& PlayerProperties)
{
	if (!CurrentLobby.IsValid())
	{
		return false;
	}

	if (!CurrentLobby->LocalPlayerMember.IsValid())
	{
		UE_LOG(LogDriftLobby, Error, TEXT("Failed to apply player properties. Player member pointer is invalid"));
		return false;
	}

	if (PlayerProperties.TeamName.IsSet())
	{
		CurrentLobby->LocalPlayerMember->TeamName = PlayerProperties.TeamName.GetValue();
	}

	if (PlayerProperties.bReady.IsSet())
	{
		CurrentLobby->LocalPlayerMember->bReady = PlayerProperties.bReady.GetValue();
	}

	// Local player isn't ready and in a team
	if (!CurrentLobby->LocalPlayerMember->bReady && !CurrentLobby->LocalPlayerMember->TeamName.Get("").IsEmpty())
	{
		CurrentLobby->bAllTeamMembersReady = false;
	}

	return true;
}

bool FDriftLobbyManager::GetResponseError(const ResponseContext& Context, FString& Error)
{
	Error = "Unknown error";

	if (!Context.response)
	{
		return false;
	}

	JsonDocument Doc;
	Doc.Parse(*Context.response->GetContentAsString());

	if (Doc.HasParseError())
	{
		return false;
	}

	// Check if there is a specific error message provided
	if (Doc.HasField(TEXT("error")))
	{
		const auto ErrorField = Doc[TEXT("error")].GetObject();
		if (const auto ErrorValuePtr = ErrorField.Find("description"))
		{
			Error = ErrorValuePtr->GetString();
			return true;
		}

	}

	// Fallback to the generic error message if provided
	if (Doc.HasField(TEXT("message")))
	{
		Error = Doc[TEXT("message")].GetString();
		return true;
	}

	return false;
}
