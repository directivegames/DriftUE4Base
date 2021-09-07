// Copyright 2021 Directive Games Limited - All Rights Reserved

#pragma once

#include "IDriftLobbyManager.h"
#include "DriftMessageQueue.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDriftLobby, Log, All);

enum class EDriftLobbyEvent : uint8
{
	Unknown,
	LobbyUpdated,
	LobbyDeleted,
	LobbyMemberJoined,
	LobbyMemberUpdated,
	LobbyMemberLeft,
	LobbyMemberKicked,
};

struct FDriftLobbyMember : IDriftLobbyMember
{
	FDriftLobbyMember(
		int32 InPlayerId,
		FString InPlayerName,
		TOptional<FString> InTeamName,
		bool InbReady,
		bool InbHost,
		FString InLobbyMemberURL)
		:
		PlayerId{ InPlayerId },
		PlayerName{ InPlayerName },
		TeamName{ InTeamName },
		bReady{ InbReady },
		bHost{ InbHost },
		LobbyMemberURL { InLobbyMemberURL }
	{ }

	int32 GetPlayerId() const override { return PlayerId; }
	FString GetPlayerName() const override { return PlayerName; }
	TOptional<FString> GetTeamName() const override { return TeamName; }
	bool IsReady() const override { return bHost; }
	bool IsHost() const override { return bHost; }

	int32 PlayerId;
	FString PlayerName;
	TOptional<FString> TeamName;
	bool bReady;
	bool bHost;

	FString LobbyMemberURL;
};

struct FDriftLobby : IDriftLobby
{
	FDriftLobby(
		FString InLobbyId,
		FString InLobbyName,
		FString InMapName,
		TArray<FString> InTeamNames,
		int32 InTeamCapacity,
		EDriftLobbyStatus InLobbyStatus,
		TArray<TSharedPtr<FDriftLobbyMember>> Members,
		FString InLobbyURL,
		FString InLobbyMembersURL,
		FString InLobbyMemberURL)
		:
		LobbyId{ InLobbyId },
		LobbyName{ InLobbyName },
		MapName{ InMapName },
		TeamNames{ InTeamNames },
		TeamCapacity{ InTeamCapacity },
		LobbyStatus{ InLobbyStatus },
		Members{ MoveTemp(Members) },
		LobbyURL{ InLobbyURL },
		LobbyMembersURL{ InLobbyMembersURL },
		LobbyMemberURL{ InLobbyMemberURL }
	{ }

	FString GetLobbyId() const override { return LobbyId; }
	FString GetLobbyName() const override { return LobbyName; }
	FString GetMapName() const override { return MapName; }
	TArray<FString> GetTeamNames() const override { return TeamNames; }
	int32 GetTeamCapacity() const override { return TeamCapacity; }
	EDriftLobbyStatus GetLobbyStatus() const override { return LobbyStatus; }
	TArray<TSharedPtr<IDriftLobbyMember>> GetMembers() const override { return static_cast<TArray<TSharedPtr<IDriftLobbyMember>>>(Members); }

	FString LobbyId;
	FString LobbyName;
	FString MapName;
	TArray<FString> TeamNames;
	int32 TeamCapacity;
	EDriftLobbyStatus LobbyStatus;
	TArray<TSharedPtr<FDriftLobbyMember>> Members;

	FString LobbyURL;
	FString LobbyMembersURL;
	FString LobbyMemberURL;
};

struct FDriftLobbyResponseMember : FJsonSerializable
{
	BEGIN_JSON_SERIALIZER;
	JSON_SERIALIZE("player_id", PlayerId);
	JSON_SERIALIZE("player_name", PlayerName);
	JSON_SERIALIZE("team_name", TeamName);
	JSON_SERIALIZE("ready", bReady);
	JSON_SERIALIZE("host", bHost);
	JSON_SERIALIZE("lobby_member_url", LobbyMemberURL);
	JSON_SERIALIZE("join_date", JoinDate);
	END_JSON_SERIALIZER;

	int32 PlayerId;
	FString PlayerName;
	FString TeamName;
	bool bReady;
	bool bHost;
	FString LobbyMemberURL;
	FDateTime JoinDate;
};

struct FDriftLobbyResponse : FJsonSerializable
{
	BEGIN_JSON_SERIALIZER;
	JSON_SERIALIZE("lobby_id", LobbyId);
	JSON_SERIALIZE("lobby_name", LobbyName);
	JSON_SERIALIZE("map_name", MapName);
	JSON_SERIALIZE_ARRAY("team_names", TeamNames);
	JSON_SERIALIZE("team_capacity", TeamCapacity);
	JSON_SERIALIZE("lobby_status", LobbyStatus);
	JSON_SERIALIZE_ARRAY_SERIALIZABLE("members", Members, FDriftLobbyResponseMember);
	JSON_SERIALIZE("create_date", CreateDate);
	JSON_SERIALIZE("start_date", StartDate);
	JSON_SERIALIZE("lobby_url", LobbyURL);
	JSON_SERIALIZE("lobby_members_url", LobbyMembersURL);
	JSON_SERIALIZE("lobby_member_url", LobbyMemberURL);
	END_JSON_SERIALIZER;

	FString LobbyId;
	FString LobbyName;
	FString MapName;
	TArray<FString> TeamNames;
	int32 TeamCapacity;
	FString LobbyStatus;
	TArray<FDriftLobbyResponseMember> Members;

	FDateTime CreateDate;
	FDateTime StartDate;

	FString LobbyURL;
	FString LobbyMembersURL;
	FString LobbyMemberURL;
};

class FDriftLobbyManager : public IDriftLobbyManager, public FSelfRegisteringExec
{
public:
	FDriftLobbyManager(TSharedPtr<IDriftMessageQueue> InMessageQueue);
	~FDriftLobbyManager() override;

	void SetRequestManager(TSharedPtr<JsonRequestManager> RootRequestManager);
	void ConfigureSession(const FDriftEndpointsResponse& DriftEndpoints, int32 InPlayerId);

	// FSelfRegisteringExec overrides

	bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

	// IDriftLobbyManager overrides
	TSharedPtr<IDriftLobby> GetCachedLobby() const override { return CurrentLobby; }
	bool QueryLobby(FQueryLobbyCompletedDelegate Delegate) override;
	bool JoinLobby(FString LobbyId, FJoinLobbyCompletedDelegate Delegate) override;
	bool LeaveLobby(FLeaveLobbyCompletedDelegate Delegate) override;
	bool CreateLobby(FDriftLobbyProperties LobbyProperties, FCreateLobbyCompletedDelegate Delegate) override;
	bool UpdateLobby(FDriftLobbyProperties LobbyProperties, FUpdateLobbyCompletedDelegate Delegate) override;
	bool UpdatePlayer(FDriftLobbyMemberProperties PlayerProperties, FUpdatePlayerCompletedDelegate Delegate) override;
	bool KickLobbyMember(int32 MemberPlayerId, FKickMemberCompletedDelegate Delegate) override;
	bool StartLobbyMatch(FStartLobbyMatchCompletedDelegate Delegate) override;

	FOnLobbyUpdatedDelegate& OnLobbyUpdated() override { return OnLobbyUpdatedDelegate; }
	FOnLobbyDeletedDelegate& OnLobbyDeleted() override { return OnLobbyDeletedDelegate; }
	FOnLobbyMemberJoinedDelegate& OnLobbyMemberJoined() override { return OnLobbyMemberJoinedDelegate; }
	FOnLobbyMemberUpdatedDelegate& OnLobbyMemberUpdated() override { return OnLobbyMemberUpdatedDelegate; }
	FOnLobbyMemberLeftDelegate& OnLobbyMemberLeft() override { return OnLobbyMemberLeftDelegate; }
	FOnLobbyMemberKickedDelegate& OnLobbyMemberKicked() override { return OnLobbyMemberKickedDelegate; }

private:
	void HandleLobbyEvent(const FMessageQueueEntry& Message);

	static EDriftLobbyEvent ParseEvent(const FString& EventName);
	static EDriftLobbyStatus ParseStatus(const FString& Status);

	bool HasSession() const;

	void ExtractLobby(const FDriftLobbyResponse& LobbyResponse);
	bool ExtractMembers(const JsonValue& EventData);
	void ResetCurrentLobby();
	bool IsCurrentLobbyHost() const;

	bool UpdateCurrentPlayerProperties();

	TSharedPtr<JsonRequestManager> RequestManager;
	TSharedPtr<IDriftMessageQueue> MessageQueue;

	FString LobbiesURL;
	FString CurrentLobbyURL;
	FString CurrentLobbyMembersURL;
	FString CurrentLobbyMemberURL;
	int32 PlayerId;

	TSharedPtr<FDriftLobby> CurrentLobby;
	FString CurrentLobbyId;
	FDriftLobbyMemberProperties CurrentPlayerProperties;

	FOnLobbyUpdatedDelegate OnLobbyUpdatedDelegate;
	FOnLobbyDeletedDelegate OnLobbyDeletedDelegate;
	FOnLobbyMemberJoinedDelegate OnLobbyMemberJoinedDelegate;
	FOnLobbyMemberUpdatedDelegate OnLobbyMemberUpdatedDelegate;
	FOnLobbyMemberLeftDelegate OnLobbyMemberLeftDelegate;
	FOnLobbyMemberKickedDelegate OnLobbyMemberKickedDelegate;
};
