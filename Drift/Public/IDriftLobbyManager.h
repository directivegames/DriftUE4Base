// Copyright 2020 Directive Games Limited - All Rights Reserved

#pragma once

enum class EDriftLobbyStatus : uint8
{
	Unknown,
	Idle,
	Starting,
	Started,
	Cancelled,
	TimedOut,
	Failed,
};

class IDriftLobbyMember
{
public:
	virtual int32 GetPlayerId() const = 0;
	virtual FString GetPlayerName() const = 0;
	virtual TOptional<FString> GetTeamName() const = 0;
	virtual bool IsReady() const = 0;
	virtual bool IsHost() const = 0;
	virtual bool IsLocalPlayer() const = 0;

	virtual ~IDriftLobbyMember() = default;
};

class IDriftLobby
{
public:
	virtual FString GetLobbyId() const = 0;
	virtual FString GetLobbyName() const = 0;
	virtual FString GetMapName() const = 0;
	virtual TArray<FString> GetTeamNames() const = 0;
	virtual int32 GetTeamCapacity() const = 0;
	virtual EDriftLobbyStatus GetLobbyStatus() const = 0;
	virtual TArray<TSharedPtr<IDriftLobbyMember>> GetMembers() const = 0;
	virtual TSharedPtr<IDriftLobbyMember> GetLocalPlayerMember() const = 0;
	virtual bool AreAllTeamMembersReady() const = 0;
	virtual FString GetConnectionString() const = 0;
	virtual FString GetConnectionOptions() const = 0;
	virtual FString GetCustomData() const = 0;

	virtual ~IDriftLobby() = default;
};

struct FDriftLobbyProperties
{
	TOptional<FString> LobbyName;
	TOptional<FString> MapName;
	TOptional<TArray<FString>> TeamNames;
	TOptional<int32> TeamCapacity;
	TOptional<FString> CustomData;

	FString ToString() const
	{
		FString Ret = "";

		if (LobbyName.IsSet())
		{
			Ret += FString::Printf(TEXT(" | Lobby name: '%s'"), *LobbyName.GetValue());
		}

		if (MapName.IsSet())
		{
			Ret += FString::Printf(TEXT(" | Map name: '%s'"), *MapName.GetValue());
		}

		if (TeamNames.IsSet())
		{
			FString TeamNamesString = "";
			for (const auto& TeamName : TeamNames.GetValue())
			{
				TeamNamesString += FString::Printf(TEXT("%s, "), *TeamName);
			}

			TeamNamesString.RemoveFromEnd(", ");

			Ret += FString::Printf(TEXT(" | Team names: '%s'"), *TeamNamesString.TrimEnd());
		}

		if (TeamCapacity.IsSet())
		{
			Ret += FString::Printf(TEXT(" | Team capacity: '%d'"), TeamCapacity.GetValue());
		}

		if (CustomData.IsSet())
		{
			Ret += FString::Printf(TEXT(" | Custom data: '%s'"), *CustomData.GetValue());
		}

		Ret.RemoveFromStart(" | ");

		return Ret;
	}
};

struct FDriftLobbyMemberProperties
{
	TOptional<FString> TeamName;
	TOptional<bool> bReady;

	FString ToString() const
	{
		FString Ret = "";

		if (TeamName.IsSet())
		{
			Ret += FString::Printf(TEXT(" | Team name: '%s'"), *TeamName.GetValue());
		}

		if (bReady.IsSet())
		{
			Ret += FString::Printf(TEXT(" | Ready: '%s'"), bReady.GetValue() ? TEXT("Yes") : TEXT("No"));
		}

		Ret.RemoveFromStart(" | ");

		return Ret;
	}
};

DECLARE_DELEGATE_ThreeParams(FQueryLobbyCompletedDelegate, bool /* bSuccess */, const FString& /* LobbyId */, const FString& /* ErrorMessage */);
DECLARE_DELEGATE_ThreeParams(FLeaveLobbyCompletedDelegate, bool /* bSuccess */, const FString& /* LobbyId */, const FString& /* ErrorMessage */);
DECLARE_DELEGATE_ThreeParams(FJoinLobbyCompletedDelegate, bool /* bSuccess */, const FString& /* LobbyId */, const FString& /* ErrorMessage */);
DECLARE_DELEGATE_ThreeParams(FCreateLobbyCompletedDelegate, bool /* bSuccess */, const FString& /* LobbyId */, const FString& /* ErrorMessage */);
DECLARE_DELEGATE_ThreeParams(FUpdateLobbyCompletedDelegate, bool /* bSuccess */, const FString& /* LobbyId */, const FString& /* ErrorMessage */);
DECLARE_DELEGATE_ThreeParams(FUpdatePlayerCompletedDelegate, bool /* bSuccess */, const FString& /* LobbyId */, const FString& /* ErrorMessage */);
DECLARE_DELEGATE_FourParams(FKickMemberCompletedDelegate, bool /* bSuccess */, const FString& /* LobbyId */, int32 /* KickedPlayerId */, const FString& /* ErrorMessage */);
DECLARE_DELEGATE_ThreeParams(FStartLobbyMatchCompletedDelegate, bool /* bSuccess */, const FString& /* LobbyId */, const FString& /* ErrorMessage */);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyUpdatedDelegate, const FString& /* LobbyId */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyDeletedDelegate, const FString& /* LobbyId */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyMemberJoinedDelegate, const FString& /* LobbyId */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyMemberUpdatedDelegate, const FString& /* LobbyId */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyMemberLeftDelegate, const FString& /* LobbyId */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyMemberKickedDelegate, const FString& /* LobbyId */);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLobbyStatusChangedDelegate, const FString& /* LobbyId */, EDriftLobbyStatus /* Status */);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnLobbyMatchStartedDelegate, const FString& /* LobbyId */, const FString& /* ConnectionString */, const FString& /* ConnectionOptions */);

class IDriftLobbyManager
{
public:
	/* Get cached information about the current lobby, if any */
	virtual TSharedPtr<IDriftLobby> GetCachedLobby() const = 0;

	/* Get information about the current lobby from the server */
	virtual bool QueryLobby(FQueryLobbyCompletedDelegate Delegate) = 0;

	/* Join a lobby */
	virtual bool JoinLobby(FString LobbyId, FJoinLobbyCompletedDelegate Delegate) = 0;

	/* Leave the current lobby. Deletes the lobby if you're the host */
	virtual bool LeaveLobby(FLeaveLobbyCompletedDelegate Delegate) = 0;

	/* Create a new lobby with you as the host */
	virtual bool CreateLobby(FDriftLobbyProperties LobbyProperties, FCreateLobbyCompletedDelegate Delegate) = 0;

	/* Update the lobby properties. You must be the host */
	virtual bool UpdateLobby(FDriftLobbyProperties LobbyProperties, FUpdateLobbyCompletedDelegate Delegate) = 0;

	/* Update your player properties */
	virtual bool UpdatePlayer(FDriftLobbyMemberProperties PlayerProperties, FUpdatePlayerCompletedDelegate Delegate) = 0;

	/* Kick a lobby member. You must be the host */
	virtual bool KickLobbyMember(int32 MemberPlayerId, FKickMemberCompletedDelegate Delegate) = 0;

	/* Start the lobby match. You must be the host */
	virtual bool StartLobbyMatch(FStartLobbyMatchCompletedDelegate Delegate) = 0;

	/* Raised when the lobby host updates the lobby properties */
	virtual FOnLobbyUpdatedDelegate& OnLobbyUpdated() = 0;

	/* Raised when the lobby host has deleted the lobby */
	virtual FOnLobbyDeletedDelegate& OnLobbyDeleted() = 0;

	/* Raised when a player joins the lobby */
	virtual FOnLobbyMemberJoinedDelegate& OnLobbyMemberJoined() = 0;

	/* Raised when a player's properties have been updated */
	virtual FOnLobbyMemberUpdatedDelegate& OnLobbyMemberUpdated() = 0;

	/* Raised when a player leaves the lobby */
	virtual FOnLobbyMemberLeftDelegate& OnLobbyMemberLeft() = 0;

	/* Raised when the host kicks a player from the lobby */
	virtual FOnLobbyMemberKickedDelegate& OnLobbyMemberKicked() = 0;

	/* Raised when the lobby status changes */
	virtual FOnLobbyStatusChangedDelegate& OnLobbyStatusChanged() = 0;

	/* Raised when the lobby match has started and connection info is available */
	virtual FOnLobbyMatchStartedDelegate& OnLobbyMatchStarted() = 0;

	virtual ~IDriftLobbyManager() = default;
};
