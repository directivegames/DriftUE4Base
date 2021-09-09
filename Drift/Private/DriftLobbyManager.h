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

	LobbyMatchStarting,
	LobbyMatchStarted,
	LobbyMatchCancelled,
	LobbyMatchTimedOut,
	LobbyMatchFailed,
};

struct FDriftLobbyMember : IDriftLobbyMember
{
	FDriftLobbyMember(
		int32 InPlayerId,
		FString InPlayerName,
		TOptional<FString> InTeamName,
		bool bInReady,
		bool bInHost,
		bool bInLocalPlayer,
		FString InLobbyMemberURL)
		:
		PlayerId{ InPlayerId },
		PlayerName{ InPlayerName },
		TeamName{ InTeamName },
		bReady{ bInReady },
		bHost{ bInHost },
		bLocalPlayer{ bInLocalPlayer },
		LobbyMemberURL { InLobbyMemberURL }
	{ }

	int32 GetPlayerId() const override { return PlayerId; }
	FString GetPlayerName() const override { return PlayerName; }
	TOptional<FString> GetTeamName() const override { return TeamName; }
	bool IsReady() const override { return bReady; }
	bool IsHost() const override { return bHost; }
	bool IsLocalPlayer() const override { return bLocalPlayer; }

	int32 PlayerId = 0;
	FString PlayerName = "";
	TOptional<FString> TeamName;
	bool bReady = false;
	bool bHost = false;
	bool bLocalPlayer = false;

	FString LobbyMemberURL = "";
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
		TSharedPtr<FDriftLobbyMember> InLocalPlayerMember,
		bool bInAllTeamMembersReady,
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
		LocalPlayerMember{ InLocalPlayerMember },
		bAllTeamMembersReady { bInAllTeamMembersReady },
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
	TSharedPtr<IDriftLobbyMember> GetLocalPlayerMember() const override { return LocalPlayerMember; }
	bool AreAllTeamMembersReady() const override { return bAllTeamMembersReady; }
	FString GetConnectionString() const override { return ConnectionString; }
	FString GetConnectionOptions() const override { return ConnectionOptions; }

	FString LobbyId = "";
	FString LobbyName = "";
	FString MapName = "";
	TArray<FString> TeamNames;
	int32 TeamCapacity = 0;
	EDriftLobbyStatus LobbyStatus = EDriftLobbyStatus::Unknown;
	TArray<TSharedPtr<FDriftLobbyMember>> Members;
	TSharedPtr<FDriftLobbyMember> LocalPlayerMember;
	bool bAllTeamMembersReady = false;

	FString LobbyURL = "";
	FString LobbyMembersURL = "";
	FString LobbyMemberURL = "";

	FString ConnectionString = "";
	FString ConnectionOptions = "";
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

	int32 PlayerId = 0;
	FString PlayerName = "";
	FString TeamName = "";
	bool bReady = false;
	bool bHost = false;
	FString LobbyMemberURL = "";
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
	JSON_SERIALIZE("status", LobbyStatus);
	JSON_SERIALIZE_ARRAY_SERIALIZABLE("members", Members, FDriftLobbyResponseMember);
	JSON_SERIALIZE("create_date", CreateDate);
	JSON_SERIALIZE("start_date", StartDate);
	JSON_SERIALIZE("connection_string", ConnectionString);
	JSON_SERIALIZE("connection_options", ConnectionOptions);
	JSON_SERIALIZE("lobby_url", LobbyURL);
	JSON_SERIALIZE("lobby_members_url", LobbyMembersURL);
	JSON_SERIALIZE("lobby_member_url", LobbyMemberURL);
	END_JSON_SERIALIZER;

	FString LobbyId = "";
	FString LobbyName = "";
	FString MapName = "";
	TArray<FString> TeamNames;
	int32 TeamCapacity = 0;
	FString LobbyStatus = "";
	TArray<FDriftLobbyResponseMember> Members;

	FString ConnectionString = "";
	FString ConnectionOptions = "";

	FDateTime CreateDate;
	FDateTime StartDate;

	FString LobbyURL = "";
	FString LobbyMembersURL = "";
	FString LobbyMemberURL = "";
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
	FOnLobbyStatusChangedDelegate& OnLobbyStatusChanged() override { return OnLobbyStatusChangedDelegate; }
	FOnLobbyMatchStartedDelegate& OnLobbyMatchStarted() override { return OnLobbyMatchStartedDelegate; }

private:
	void HandleLobbyEvent(const FMessageQueueEntry& Message);

	static EDriftLobbyEvent ParseEvent(const FString& EventName);
	static EDriftLobbyStatus ParseStatus(const FString& Status);

	bool HasSession() const;

	void ExtractLobby(const FDriftLobbyResponse& LobbyResponse, bool bUpdateURLs = true);
	bool ExtractMembers(const JsonValue& EventData);
	void ResetCurrentLobby();
	bool IsCurrentLobbyHost() const;

	bool UpdateCurrentPlayerProperties();
	bool ApplyCurrentPlayerProperties();

	TSharedPtr<JsonRequestManager> RequestManager;
	TSharedPtr<IDriftMessageQueue> MessageQueue;

	FString LobbiesURL;
	FString CurrentLobbyURL;
	FString CurrentLobbyMembersURL;
	FString CurrentLobbyMemberURL;
	int32 PlayerId = INDEX_NONE;

	TSharedPtr<FDriftLobby> CurrentLobby;
	FString CurrentLobbyId;
	FDriftLobbyMemberProperties CurrentPlayerProperties;

	FOnLobbyUpdatedDelegate OnLobbyUpdatedDelegate;
	FOnLobbyDeletedDelegate OnLobbyDeletedDelegate;
	FOnLobbyMemberJoinedDelegate OnLobbyMemberJoinedDelegate;
	FOnLobbyMemberUpdatedDelegate OnLobbyMemberUpdatedDelegate;
	FOnLobbyMemberLeftDelegate OnLobbyMemberLeftDelegate;
	FOnLobbyMemberKickedDelegate OnLobbyMemberKickedDelegate;
	FOnLobbyStatusChangedDelegate OnLobbyStatusChangedDelegate;
	FOnLobbyMatchStartedDelegate OnLobbyMatchStartedDelegate;
};
