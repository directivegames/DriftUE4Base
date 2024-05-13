/**
* This file is part of the Drift Unreal Engine Integration.
*
* Copyright (C) 2016-2019 Directive Games Limited. All Rights Reserved.
*
* Licensed under the MIT License (the "License");
*
* You may not use this file except in compliance with the License.
* You may obtain a copy of the license in the LICENSE file found at the top
* level directory of this module, and at https://mit-license.org/
*/

#include "DriftBase.h"

#include "JsonRequestManager.h"
#include "JsonArchive.h"
#include "JsonUtils.h"
#include "DriftAPI.h"
#include "JTIRequestManager.h"
#include "JWTRequestManager.h"
#include "ErrorResponse.h"
#include "DriftPartyManager.h"
#include "Details/PlatformName.h"
#include "Details/UrlHelper.h"
#include "IDriftAuthProviderFactory.h"
#include "IDriftAuthProvider.h"
#include "IErrorReporter.h"
#include "Auth/DriftUuidAuthProviderFactory.h"
#include "Auth/DriftUserPassAuthProviderFactory.h"
#include "RetryConfig.h"

#include "SocketSubsystem.h"
#include "GeneralProjectSettings.h"
#include "IDriftMatchmaker.h"
#include "IPAddress.h"
#include "Runtime/Analytics/Analytics/Public/AnalyticsEventAttribute.h"
#include "Features/IModularFeatures.h"
#include "OnlineSubsystemTypes.h"
#include "Internationalization/Internationalization.h"
#include "Misc/EngineVersionComparison.h"

#if PLATFORM_APPLE
#include "Apple/AppleUtility.h"
#endif


#define LOCTEXT_NAMESPACE "Drift"


DEFINE_LOG_CATEGORY(LogDriftBase);
DEFINE_LOG_CATEGORY(LogDriftCounterEngine);
DEFINE_LOG_CATEGORY(LogDriftCounterUser);


static const float UPDATE_FRIENDS_INTERVAL = 3.0f;


const TCHAR* defaultSettingsSection = TEXT("/Script/DriftEditor.DriftProjectSettings");


static const FString MatchQueue(TEXT("matchqueue"));
static const FString FriendEvent(TEXT("friendevent"));
static const FString FriendMessage(TEXT("friendmessage"));


static FString GEditorServerPassword;
static FAutoConsoleVariableRef CVarEditorServerPassword(
	TEXT("drift.EditorServerPassword"),
    GEditorServerPassword,
	TEXT("Password used for drift authentication when running the dedicated server in the editor")
);


FDriftBase::FDriftBase(const TSharedPtr<IHttpCache>& cache, const FName& instanceName, int32 instanceIndex, const FString& config)
    : instanceName_(instanceName)
    , instanceDisplayName_(instanceName_ == FName(TEXT("DefaultInstance")) ? TEXT("") : FString::Printf(TEXT("[%s] "), *instanceName_.ToString()))
    , instanceIndex_(instanceIndex)
    , state_(DriftSessionState::Undefined)
    , rootRequestManager_(MakeShareable(new JsonRequestManager()))
    , httpCache_(cache)
{
    ConfigureSettingsSection(config);

    GetRootRequestManager()->DefaultErrorHandler.BindRaw(this, &FDriftBase::DefaultErrorHandler);
    GetRootRequestManager()->DefaultDriftDeprecationMessageHandler.BindRaw(this, &FDriftBase::DriftDeprecationMessageHandler);

    GConfig->GetBool(*settingsSection_, TEXT("IgnoreCommandLineArguments"), ignoreCommandLineArguments_, GGameIni);
    GConfig->GetString(*settingsSection_, TEXT("ProjectName"), projectName_, GGameIni);
    GConfig->GetString(*settingsSection_, TEXT("StaticDataReference"), staticDataReference, GGameIni);

    if (!ignoreCommandLineArguments_)
    {
        FParse::Value(FCommandLine::Get(), TEXT("-drift_url="), cli.drift_url);
        FParse::Value(FCommandLine::Get(), TEXT("-drift_apikey="), versionedApiKey);
    }

    GConfig->GetString(*settingsSection_, TEXT("GameVersion"), gameVersion, GGameIni);
    GConfig->GetString(*settingsSection_, TEXT("GameBuild"), gameBuild, GGameIni);

    {
        FString appGuid;
        GConfig->GetString(*settingsSection_, TEXT("AppGuid"), appGuid, GGameIni);
        if (!appGuid.IsEmpty())
        {
            if (!FGuid::Parse(appGuid, appGuid_))
            {
                IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("AppGuid \"%s\" could not be parsed as a valid GUID"));
            }
        }
    }

    if (cli.drift_url.IsEmpty())
    {
        GConfig->GetString(*settingsSection_, TEXT("DriftUrl"), cli.drift_url, GGameIni);
    }

    if (ignoreCommandLineArguments_ || !FParse::Value(FCommandLine::Get(), TEXT("-drift_env="), environment))
    {
        GConfig->GetString(*settingsSection_, TEXT("Environment"), environment, GGameIni);
    }

    if (apiKey.IsEmpty())
    {
        GConfig->GetString(*settingsSection_, TEXT("ApiKey"), apiKey, GGameIni);
    }
    if (apiKey.IsEmpty() && versionedApiKey.IsEmpty())
    {
        IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("No API key found. Please fill out Project Settings->Drift"));
    }

    ConfigurePlacement();
    ConfigureBuildReference();

    FParse::Value(FCommandLine::Get(), TEXT("-server_url="), cli.server_url);

    rootRequestManager_->SetApiKey(GetApiKeyHeader());
    rootRequestManager_->SetCache(httpCache_);

    CreatePlayerCounterManager();
    CreateEventManager();
    CreateLogForwarder();
    CreateMessageQueue();
    CreatePartyManager();
    CreateMatchmaker();
    CreateLobbyManager();
    CreateMatchPlacementManager();
    CreateSandboxManager();

    DRIFT_LOG(Base, Verbose, TEXT("Drift instance %s (%d) created"), *instanceName_.ToString(), instanceIndex_);
}


void FDriftBase::CreatePlayerCounterManager()
{
    playerCounterManager = MakeUnique<FDriftCounterManager>();
    playerCounterManager->OnPlayerStatsLoaded().AddLambda([this](bool success)
    {
        onPlayerStatsLoaded.Broadcast(success);
    });
}


void FDriftBase::CreateEventManager()
{
    eventManager = MakeShared<FDriftEventManager>();
}


void FDriftBase::CreateLogForwarder()
{
    logForwarder = MakeUnique<FLogForwarder>();
}


void FDriftBase::CreateMessageQueue()
{
    messageQueue = MakeShared<FDriftMessageQueue>();

    messageQueue->OnMessageQueueMessage(MatchQueue).AddRaw(this, &FDriftBase::HandleMatchQueueMessage);
    messageQueue->OnMessageQueueMessage(FriendEvent).AddRaw(this, &FDriftBase::HandleFriendEventMessage);
	messageQueue->OnMessageQueueMessage(FriendMessage).AddRaw(this, &FDriftBase::HandleFriendMessage);
}


void FDriftBase::CreatePartyManager()
{
	partyManager = MakeShared<FDriftPartyManager>(messageQueue);
}

void FDriftBase::CreateMatchmaker()
{
    matchmaker = MakeShared<FDriftFlexmatch>(messageQueue);
}

void FDriftBase::CreateLobbyManager()
{
	lobbyManager = MakeShared<FDriftLobbyManager>(messageQueue);
}

void FDriftBase::CreateMatchPlacementManager()
{
    matchPlacementManager = MakeShared<FDriftMatchPlacementManager>(messageQueue);
}

void FDriftBase::CreateSandboxManager()
{
    sandboxManager = MakeShared<FDriftSandboxManager>(messageQueue);
}

void FDriftBase::ConfigurePlacement()
{
    if (ignoreCommandLineArguments_ || !FParse::Value(FCommandLine::Get(), TEXT("-placement="), defaultPlacement))
    {
        if (!GConfig->GetString(*settingsSection_, TEXT("Placement"), defaultPlacement, GGameIni))
        {
            bool canBindAll;
            const auto address = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, canBindAll);
            uint32 ip;
            address->GetIp(ip);
            defaultPlacement = FString::Printf(TEXT("LAN %d.%d"), (ip & 0xff000000) >> 24, (ip & 0x00ff0000) >> 16);
        }
    }
}


void FDriftBase::ConfigureBuildReference()
{
    if (ignoreCommandLineArguments_ || !FParse::Value(FCommandLine::Get(), TEXT("-ref="), buildReference))
    {
        if (!GConfig->GetString(*settingsSection_, TEXT("BuildReference"), buildReference, GGameIni))
        {
            buildReference = FString::Printf(TEXT("user/%s"), FPlatformProcess::UserName());
        }
    }
}


FDriftBase::~FDriftBase()
{
    DRIFT_LOG(Base, Verbose, TEXT("Drift instance %s (%d) destroyed"), *instanceName_.ToString(), instanceIndex_);
}


void FDriftBase::Tick(float deltaTime)
{
    TickHeartbeat(deltaTime);
    TickMatchInvites();
    TickFriendUpdates(deltaTime);
}


TSharedPtr<JsonRequestManager> FDriftBase::GetRootRequestManager() const
{
    return rootRequestManager_;
}


TSharedPtr<JsonRequestManager> FDriftBase::GetGameRequestManager() const
{
    if (!authenticatedRequestManager.IsValid())
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to use authenticated endpoints without being authenticated."));
    }
    return authenticatedRequestManager;
}


void FDriftBase::TickHeartbeat(float deltaTime)
{
    if (state_ != DriftSessionState::Connected)
    {
        return;
    }

	const auto bHeartbeatInitialized = heartbeatTimeout_ != FDateTime::MinValue();
	const auto bHeartbeatTimedOut = FDateTime::UtcNow() >= heartbeatTimeout_ - FTimespan::FromSeconds(5.0);
    if (bHeartbeatInitialized && bHeartbeatTimedOut)
    {
        DRIFT_LOG(Base, Error, TEXT("Heartbeat timed out"));

        state_ = DriftSessionState::Timedout;
        BroadcastConnectionStateChange(state_);
        Reset();
        return;
    }

    heartbeatDueInSeconds_ -= deltaTime;
    if (heartbeatDueInSeconds_ > 0.0)
    {
        return;
    }
    heartbeatDueInSeconds_ = FLT_MAX; // Prevent re-entrance

    DRIFT_LOG(Base, Verbose, TEXT("[%s] Drift heartbeat..."), *FDateTime::UtcNow().ToIso8601());

    struct FDriftHeartBeatResponse
    {
        FDateTime last_heartbeat;
        FDateTime this_heartbeat;
        FDateTime next_heartbeat;
        int32 next_heartbeat_seconds;
        FDateTime heartbeat_timeout;
        int32 heartbeat_timeout_seconds;

        bool Serialize(SerializationContext& context)
        {
            return SERIALIZE_PROPERTY(context, last_heartbeat)
                && SERIALIZE_PROPERTY(context, this_heartbeat)
                && SERIALIZE_PROPERTY(context, next_heartbeat)
                && SERIALIZE_PROPERTY(context, next_heartbeat_seconds)
                && SERIALIZE_PROPERTY(context, heartbeat_timeout)
                && SERIALIZE_PROPERTY(context, heartbeat_timeout_seconds);
        }
    };

    auto request = GetGameRequestManager()->Put(heartbeatUrl, FString());
    request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
    {
        FDriftHeartBeatResponse response;
        if (JsonUtils::ParseResponseNoLog(context.response, response))
        {
            const auto heartbeatRoundTrip = FTimespan::FromSeconds(context.request.Get()->GetElapsedTime());
            heartbeatDueInSeconds_ = response.next_heartbeat_seconds;
            /**
             * We don't know if making the call, or returning it, takes a long time,
             * and the client clock might be off compared with the server's, so we
             * can't use the actual timeout time returned. Local time minus roundtrip
             * plus timeout, should be on the safe side.
             */
            heartbeatTimeout_ = FDateTime::UtcNow() + FTimespan::FromSeconds(response.heartbeat_timeout_seconds) - heartbeatRoundTrip;
        }
    	else
    	{
    		// Older versions of the server heartbeat endpoint don't return all the details
            heartbeatDueInSeconds_ = doc[TEXT("next_heartbeat_seconds")].GetInt32();
        }

    	if (heartbeatRetryAttempt_ > 0)
    	{
    		DRIFT_LOG(Base, Log, TEXT("[%s] Drift heartbeat recovered after %d retries.")
					, *FDateTime::UtcNow().ToIso8601(), heartbeatRetryAttempt_);
		}
    	heartbeatRetryAttempt_ = 0;

        DRIFT_LOG(Base, Verbose, TEXT("[%s] Drift heartbeat done. Next one in %.1f secs. Timeout at: %s")
                  , *FDateTime::UtcNow().ToIso8601(), heartbeatDueInSeconds_, *heartbeatTimeout_.ToIso8601());
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        if (context.successful && context.response.IsValid())
        {
            GenericRequestErrorResponse response;
            if (JsonUtils::ParseResponse(context.response, response))
            {
                if (context.responseCode == static_cast<int32>(HttpStatusCodes::NotFound) && response.GetErrorCode() == TEXT("user_error"))
                {
                    // Heartbeat timed out
                    DRIFT_LOG(Base, Error, TEXT("Failed to heartbeat\n%s"), *GetDebugText(context.response));

                    state_ = DriftSessionState::Timedout;
                    BroadcastConnectionStateChange(state_);
                    context.errorHandled = true;
                    Reset();
                    return;
                }
                // Some other reason
                FString Error;
                context.errorHandled = GetResponseError(context, Error);
                DRIFT_LOG(Base, Error, TEXT("Failed to heartbeat\n%s"), *Error);
                Disconnect();
            }
        }
        else
        {
            context.errorHandled = true;
        	const auto now = FDateTime::UtcNow();

        	// It'd be pointless to retry outside of the timeout
        	if (now > heartbeatTimeout_)
        	{
        		DRIFT_LOG(Base, Error, TEXT("Failed to heartbeat\n%s"), *GetDebugText(context.response));

                state_ = DriftSessionState::Timedout;
                BroadcastConnectionStateChange(state_);
	            Reset();
        		return;
        	}

        	heartbeatRetryAttempt_ += 1;
        	// Delay the retry for an exponentially expanding random amount of time, up to the cap, and within the timeout
        	const auto retryDelayCap = FMath::Min(heartbeatRetryDelayCap_, static_cast<float>((heartbeatTimeout_ - now).GetTotalSeconds()));
        	const auto maxRetryDelay = FMath::Min(retryDelayCap, FMath::Pow(heartbeatRetryDelay_ * 2, heartbeatRetryAttempt_));
        	heartbeatDueInSeconds_ = FMath::RandRange(
        		heartbeatRetryDelay_ / 2.0f,
        		maxRetryDelay
        		);

            DRIFT_LOG(Base, Warning, TEXT("[%s] Drift heartbeat failed. Retrying in %.1f secs. Timeout at: %s")
                      , *FDateTime::UtcNow().ToIso8601(), heartbeatDueInSeconds_, *heartbeatTimeout_.ToIso8601());
        }
    });
    request->Dispatch();
}


void FDriftBase::FlushCounters()
{
    check(playerCounterManager);

    playerCounterManager->FlushCounters();

    for (auto& counterManager : serverCounterManagers)
    {
        counterManager.Value->FlushCounters();
    }
}


void FDriftBase::FlushEvents()
{
    check(eventManager);

    eventManager->FlushEvents(true);
}


void FDriftBase::Shutdown()
{
    if (state_ == DriftSessionState::Connected)
    {
        Disconnect();
    }
}


const TMap<FString, FDateTime>& FDriftBase::GetDeprecations()
{
    return deprecations_;
}


FString FDriftBase::GetJWT() const
{
	if (serverBearerToken_.Len())
	{
		return serverBearerToken_;
	}
	else if (driftClient.jwt.Len())
	{
		return driftClient.jwt;
	}
    else
	{
		DRIFT_LOG(Base, Warning, TEXT("Both the client and server JWTs are empty!"));
		return {};
	}
}


FString FDriftBase::GetJTI() const
{
	if (serverJTI_.Len())
	{
		return serverJTI_;
	}
	else if (driftClient.jti.Len())
	{
		return driftClient.jti;
	}
    else
	{
		DRIFT_LOG(Base, Warning, TEXT("Both the client and server JTIs are empty!"));
		return {};
	}
}


FString FDriftBase::GetRootURL() const
{
    return driftEndpoints.root;
}


FString FDriftBase::GetEnvironment() const
{
    // If we've been redirected, we always consider ourselves to be running in the "dev" environment
    if (driftEndpoints.root.IsEmpty() || driftEndpoints.root.Compare(cli.drift_url) == 0)
    {
        return environment;
    }

    return TEXT("dev");
}


FString FDriftBase::GetGameVersion() const
{
    return gameVersion;
}


FString FDriftBase::GetGameBuild() const
{
    return gameBuild;
}


FString FDriftBase::GetVersionedAPIKey() const
{
    return GetApiKeyHeader();
}


void FDriftBase::Disconnect()
{
    if (state_ != DriftSessionState::Connected && state_ != DriftSessionState::Usurped && state_ != DriftSessionState::Timedout)
    {
        DRIFT_LOG(Base, Warning, TEXT("Ignoring attempt to disconnect while not connected."));

        return;
    }

    if (state_ == DriftSessionState::Connected && !drift_server.url.IsEmpty())
    {
        UpdateServer(TEXT("quit"), TEXT(""), FDriftServerStatusUpdatedDelegate{});
    }

    auto finalizeDisconnect = [this]()
    {
        Reset();
        onPlayerDisconnected.Broadcast();
    };

    /**
     * If we were connected, then this is most likely the user himself disconnecting,
     * and we should attempt to clear the client session from the backend.
     */
    if (state_ == DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Log, TEXT("Disconnecting"));

        state_ = DriftSessionState::Disconnecting;
        BroadcastConnectionStateChange(state_);

        // Any messages in flight may block shutdown, so it needs to be terminated
        messageQueue.Reset();

        FlushCounters();
        FlushEvents();

        if (!driftClient.url.IsEmpty())
        {
            /**
             * If this is run during shutdown, the HttpManager will disconnect all callbacks in Flush(),
             * and finalizeDisconnect will never be called. Since the object is about to be deleted anyway,
             * this doesn't cause any problems.
             */
            auto request = GetGameRequestManager()->Delete(driftClient.url);
            request->OnResponse.BindLambda([finalizeDisconnect](ResponseContext& context, JsonDocument& doc)
            {
                finalizeDisconnect();
            });
            request->OnError.BindLambda([this, finalizeDisconnect](ResponseContext& context)
            {
                FString Error;
                context.errorHandled = GetResponseError(context, Error);
                DRIFT_LOG(Base, Error, TEXT("Error while disconnecting: %s"), *Error);

                finalizeDisconnect();
            });
            request->Dispatch();
        }
    }
    else
    {
        finalizeDisconnect();
    }
}


void FDriftBase::Reset()
{
    DRIFT_LOG(Base, Warning, TEXT("Resetting all internal state. Connection state: %d"), static_cast<uint8>(state_));

    if (state_ != DriftSessionState::Usurped && state_ != DriftSessionState::Timedout)
    {
        state_ = DriftSessionState::Disconnected;
        BroadcastConnectionStateChange(state_);
    }

    authenticatedRequestManager.Reset();
    secondaryIdentityRequestManager_.Reset();

    driftEndpoints = FDriftEndpointsResponse{};
    driftClient = FClientRegistrationResponse{};
    myPlayer = FDriftPlayerResponse{};
    drift_server = FServerRegistrationResponse{};

    matchQueue = FMatchQueueResponse{};
    matchQueueState = EMatchQueueState::Idle;

	userPassAuthProviderFactory_.Reset();

    CreatePlayerCounterManager();
    CreateEventManager();
    CreateLogForwarder();
    CreateMessageQueue();
    CreatePartyManager();
    CreateMatchmaker();
	CreateLobbyManager();

    heartbeatUrl.Empty();

    userIdentities = FDriftCreatePlayerGroupResponse{};

    heartbeatDueInSeconds_ = FLT_MAX;
    heartbeatTimeout_ = FDateTime::MinValue();
	heartbeatRetryAttempt_ = 0;

    countersLoaded = false;
    playerGameStateInfosLoaded = false;
    userIdentitiesLoaded = false;
    shouldUpdateFriends = false;

    deprecations_.Empty();
    previousDeprecationHeader_.Empty();

	serverJTI_ = {};
	serverBearerToken_ = {};
}


TStatId FDriftBase::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(FDriftBase, STATGROUP_Tickables);
}


void FDriftBase::LoadStaticData(const FString& name, const FString& ref)
{
    if (driftEndpoints.static_data.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to load static data before static routes have been initialized"));

        onStaticDataLoaded.Broadcast(false, TEXT(""));
        return;
    }

    DRIFT_LOG(Base, Verbose, TEXT("Getting static data endpoints"));

    FString pin = ref;
    if (pin.IsEmpty())
    {
        pin = staticDataReference;
    }

    auto url = driftEndpoints.static_data;
    internal::UrlHelper::AddUrlOption(url, TEXT("static_data_ref"), *pin);
    auto request = GetRootRequestManager()->Get(url);
    request->OnResponse.BindLambda([this, name, pin](ResponseContext& context, JsonDocument& doc)
    {
        FStaticDataResponse static_data;
        if (!JsonArchive::LoadObject(doc, static_data))
        {
            context.error = TEXT("Failed to parse static data response");
            return;
        }

        if (static_data.static_data_urls.Num() == 0)
        {
            context.error = TEXT("No static data entries found");
            return;
        }

        DRIFT_LOG(Base, Log, TEXT("Downloading static data file: '%s'"), *name);

        struct StaticDataSync
        {
            bool succeeded = false;
            int32 remaining = 0;
            int32 bytesRead = 0;
        };

        auto commit = static_data.static_data_urls[0].commit_id;
        auto index_sent = context.sent;
        auto index_received = context.received;
        TSharedPtr<StaticDataSync> sync = MakeShareable(new StaticDataSync);

        const auto loader = [this, commit, pin, index_sent, index_received, sync](const FString& data_url, const FString& data_name, const FString& cdn_name)
        {
            auto data_request = GetRootRequestManager()->Get(data_url + data_name);
            data_request->OnRequestProgress().BindLambda([this, data_name, sync](FHttpRequestPtr req, int32 bytesWritten, int32 bytesRead)
            {
                if (!sync->succeeded)
                {
                    const auto lastBytesRead = sync->bytesRead;
                    if (bytesRead > lastBytesRead)
                    {
                        sync->bytesRead = bytesRead;

                        DRIFT_LOG(Base, Verbose, TEXT("Downloading static data file from: '%s' %d bytes"), *data_name, bytesRead);

                        onStaticDataProgress.Broadcast(data_name, bytesRead);
                    }
                }
            });
            data_request->OnResponse.BindLambda([this, data_name, commit, pin, cdn_name, index_sent, index_received, sync](ResponseContext& data_context, JsonDocument& data_doc)
            {
                DRIFT_LOG(Base, Log, TEXT("Download of static data file: '%s' done"), *data_name);

                sync->remaining -= 1;
                if (!sync->succeeded)
                {
                    sync->succeeded = true;
                    auto data = data_context.response->GetContentAsString();
                    onStaticDataLoaded.Broadcast(true, data);
                }

                auto event = MakeEvent(TEXT("drift.static_data_downloaded"));
                event->Add(TEXT("filename"), *data_name);
                event->Add(TEXT("pin"), *pin);
                event->Add(TEXT("commit"), *commit);
                event->Add(TEXT("bytes"), data_context.response->GetContentLength());
                event->Add(TEXT("cdn"), *cdn_name);
                event->Add(TEXT("index_request_time"), (index_received - index_sent).GetTotalSeconds());
                event->Add(TEXT("data_request_time"), (data_context.received - data_context.sent).GetTotalSeconds());
                event->Add(TEXT("total_time"), (data_context.received - index_sent).GetTotalSeconds());
                AddAnalyticsEvent(MoveTemp(event));
            });
            data_request->OnError.BindLambda([this, data_name, commit, pin, cdn_name, sync](ResponseContext& data_context)
            {
                sync->remaining -= 1;
                if (!sync->succeeded && sync->remaining <= 0)
                {
                    FString Error;
                    data_context.errorHandled = GetResponseError(data_context, Error);
                    DRIFT_LOG(Base, Error, TEXT("Failed to download static data file: '%s'. Error: %s"), *data_name, *Error);
                    onStaticDataLoaded.Broadcast(false, TEXT(""));
                }

                auto event = MakeEvent(TEXT("drift.static_data_download_failed"));
                event->Add(TEXT("filename"), *data_name);
                event->Add(TEXT("pin"), *pin);
                event->Add(TEXT("commit"), *commit);
                event->Add(TEXT("cdn"), *cdn_name);
                event->Add(TEXT("error"), data_context.error);
                AddAnalyticsEvent(MoveTemp(event));
            });
            data_request->Dispatch();
        };

        sync->remaining = FMath::Max(static_data.static_data_urls[0].cdn_list.Num(), 1);
        if (static_data.static_data_urls[0].cdn_list.Num() == 0)
        {
            loader(static_data.static_data_urls[0].data_root_url, name, TEXT("default"));
        }
        else
        {
            for (const auto& cdn : static_data.static_data_urls[0].cdn_list)
            {
                loader(cdn.data_root_url, name, cdn.cdn);
            }
        }
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to get static data endpoints. Error: %s"), *Error);
        onStaticDataLoaded.Broadcast(false, TEXT(""));
    });
    request->Dispatch();
}


void FDriftBase::LoadPlayerStats()
{
    check(playerCounterManager.IsValid());

    playerCounterManager->LoadCounters();
}


void FDriftBase::LoadPlayerGameState(const FString& name, const FDriftGameStateLoadedDelegate& delegate)
{
    if (driftEndpoints.my_gamestates.IsEmpty())
    {
        DRIFT_LOG(Base, Log, TEXT("Player has no game state yet"));

        delegate.ExecuteIfBound(ELoadPlayerGameStateResult::Error_InvalidState, name, FString());
        onPlayerGameStateLoaded.Broadcast(ELoadPlayerGameStateResult::Error_InvalidState, name, FString());
        return;
    }

    LoadPlayerGameStateInfos([this, name, delegate](bool success)
    {
        if (success)
        {
            LoadPlayerGameStateImpl(name, delegate);
        }
        else
        {
            delegate.ExecuteIfBound(ELoadPlayerGameStateResult::Error_Failed, name, FString());
            onPlayerGameStateLoaded.Broadcast(ELoadPlayerGameStateResult::Error_Failed, name, FString());
        }
    });
}

void FDriftBase::LoadPlayerGameState(int32 playerId, const FString& name, const FDriftGameStateLoadedDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to load player game state without being connected"));
        delegate.ExecuteIfBound(ELoadPlayerGameStateResult::Error_InvalidState, name, FString());
        onPlayerGameStateLoaded.Broadcast(ELoadPlayerGameStateResult::Error_InvalidState, name, FString());
        return;
    }

    if (driftEndpoints.template_player_gamestate.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to load player game state with no endpoint"));

        delegate.ExecuteIfBound(ELoadPlayerGameStateResult::Error_InvalidState, name, FString());
        onPlayerGameStateLoaded.Broadcast(ELoadPlayerGameStateResult::Error_InvalidState, name, FString());
        return;
    }

    DRIFT_LOG(Base, Log, TEXT("Getting player game state '%s' for player '%d'"), *name, playerId);

    const auto url = driftEndpoints.template_player_gamestate.Replace(TEXT("{player_id}"), *FString::FromInt(playerId)).Replace(TEXT("{namespace}"), *name);

    InternalLoadPlayerGameState(name, url, delegate);
}


void FDriftBase::LoadPlayerGameStateImpl(const FString& name, const FDriftGameStateLoadedDelegate& delegate)
{
    DRIFT_LOG(Base, Log, TEXT("Getting player game state \"%s\""), *name);

    const auto GameStateInfo = playerGameStateInfos.FindByPredicate([name](const FDriftPlayerGameStateInfo& info)
    {
        return info.name == name;
    });

    if (GameStateInfo == nullptr)
    {
        DRIFT_LOG(Base, Warning, TEXT("Failed to find player game state: \"%s\""), *name);

        delegate.ExecuteIfBound(ELoadPlayerGameStateResult::Error_NotFound, name, FString());
        onPlayerGameStateLoaded.Broadcast(ELoadPlayerGameStateResult::Error_NotFound, name, FString());
        return;
    }

    InternalLoadPlayerGameState(name, GameStateInfo->gamestate_url, delegate);
}

void FDriftBase::InternalLoadPlayerGameState(const FString& name, const FString& url, const FDriftGameStateLoadedDelegate& delegate)
{
    const auto Request = GetGameRequestManager()->Get(url);
    Request->OnResponse.BindLambda([this, name, delegate](ResponseContext& Context, JsonDocument& Doc)
    {
        FPlayerGameStateResponse Response;
        if (!JsonArchive::LoadObject(Doc, Response) || Response.data.IsNull() || !Response.data.HasField(TEXT("data")))
        {
            Context.error = TEXT("Failed to parse game state response");
            return;
        }
        const FString Data{ Response.data[TEXT("data")].GetString() };
        delegate.ExecuteIfBound(ELoadPlayerGameStateResult::Success, name, Data);
        onPlayerGameStateLoaded.Broadcast(ELoadPlayerGameStateResult::Success, name, Data);

        auto Event = MakeEvent(TEXT("drift.gamestate_loaded"));
        Event->Add(TEXT("namespace"), *name);
        Event->Add(TEXT("bytes"), Context.response->GetContentLength());
        Event->Add(TEXT("request_time"), (Context.received - Context.sent).GetTotalSeconds());
        AddAnalyticsEvent(MoveTemp(Event));
    });
    Request->OnError.BindLambda([this, name, delegate](ResponseContext& Context)
    {
        FString Error;
        Context.errorHandled = GetResponseError(Context, Error);

        const bool bErrorNotFound = Context.responseCode == static_cast<int32>(HttpStatusCodes::NotFound);

        if (bErrorNotFound)
        {
            DRIFT_LOG(Base, Verbose, TEXT("game state: %s not found. Error: '%s'"), *name, *Error);
        }
        else
        {
            DRIFT_LOG(Base, Error, TEXT("Failed to load game state: %s. Error: '%s'"), *name, *Error);
        }

        const auto Result = bErrorNotFound ? ELoadPlayerGameStateResult::Error_NotFound : ELoadPlayerGameStateResult::Error_Failed;

        delegate.ExecuteIfBound(Result, name, {});
        onPlayerGameStateLoaded.Broadcast(Result, name, {});
    });
    Request->Dispatch();
}


void FDriftBase::SavePlayerGameState(const FString& name, const FString& gameState, const FDriftGameStateSavedDelegate& delegate)
{
    if (driftEndpoints.my_gamestates.IsEmpty())
    {
        DRIFT_LOG(Base, Log, TEXT("Player has no game state yet"));

        delegate.ExecuteIfBound(false, name);
        onPlayerGameStateSaved.Broadcast(false, name);
        return;
    }

    LoadPlayerGameStateInfos([this, name, gameState, delegate](bool success)
    {
        if (success)
        {
            SavePlayerGameStateImpl(name, gameState, delegate);
        }
        else
        {
            delegate.ExecuteIfBound(false, name);
            onPlayerGameStateSaved.Broadcast(false, name);
        }
    });
}

void FDriftBase::SavePlayerGameState(int32 playerId, const FString& name, const FString& gameState, const FDriftGameStateSavedDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("SavePlayerGameState: attempting to save player game state without being connected"));
        delegate.ExecuteIfBound(false, name);
        onPlayerGameStateSaved.Broadcast(false, name);
        return;
    }

    if (driftEndpoints.template_player_gamestate.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("SavePlayerGameState: attempting to save player game state with no endpoint"));

        delegate.ExecuteIfBound(false, name);
        onPlayerGameStateSaved.Broadcast(false, name);
        return;
    }

    DRIFT_LOG(Base, Log, TEXT("SavePlayerGameState: player_id (%d), state_name (%s), state (%s)"), playerId, *name, *gameState);

    const auto url = driftEndpoints.template_player_gamestate.Replace(TEXT("{player_id}"), *FString::FromInt(playerId)).Replace(TEXT("{namespace}"), *name);

    InternalSavePlayerGameState(name, gameState, url, delegate);
}


void FDriftBase::SavePlayerGameStateImpl(const FString& name, const FString& gameState, const FDriftGameStateSavedDelegate& delegate)
{
    DRIFT_LOG(Base, Log, TEXT("Saving player game state \"%s\""), *name);

    const auto GameStateInfo = playerGameStateInfos.FindByPredicate([name](const FDriftPlayerGameStateInfo& info)
    {
        return info.name == name;
    });

    FString Url;
    if (GameStateInfo != nullptr)
    {
        Url = GameStateInfo->gamestate_url;
    }
    else
    {
        Url = driftEndpoints.my_gamestate.Replace(TEXT("{namespace}"), *name);
        playerGameStateInfosLoaded = false;
    }

    InternalSavePlayerGameState(name, gameState, Url, delegate);
}

void FDriftBase::InternalSavePlayerGameState(const FString& name, const FString& state, const FString& url, const FDriftGameStateSavedDelegate& delegate)
{
    FPlayerGameStatePayload Payload{};
    JsonArchive::AddMember(Payload.gamestate, TEXT("data"), *state);

    const auto Request = GetGameRequestManager()->Put(url, Payload);
    Request->OnResponse.BindLambda([this, name, delegate](ResponseContext& Context, JsonDocument& Doc)
    {
        delegate.ExecuteIfBound(true, name);
        onPlayerGameStateSaved.Broadcast(true, name);

        auto Event = MakeEvent(TEXT("drift.gamestate_saved"));
        Event->Add(TEXT("namespace"), *name);
        Event->Add(TEXT("bytes"), Context.request->GetContentLength());
        Event->Add(TEXT("request_time"), (Context.received - Context.sent).GetTotalSeconds());
        AddAnalyticsEvent(MoveTemp(Event));
    });
    Request->OnError.BindLambda([this, name, delegate](ResponseContext& Context)
    {
        FString Error;
        Context.errorHandled = GetResponseError(Context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to save player game state '%s': %s"), *name, *Error);

        delegate.ExecuteIfBound(false, name);
        onPlayerGameStateSaved.Broadcast(false, name);
    });
    Request->Dispatch();
}


void FDriftBase::LoadPlayerGameStateInfos(TFunction<void(bool)> next)
{
    if (playerGameStateInfosLoaded)
    {
        next(true);
        return;
    }

    auto request = GetGameRequestManager()->Get(driftEndpoints.my_gamestates);
    request->OnResponse.BindLambda([this, next](ResponseContext& context, JsonDocument& doc)
    {
        if (!JsonArchive::LoadObject(doc, playerGameStateInfos))
        {
            context.error = TEXT("Failed to parse gamestates response");
            return;
        }
        playerGameStateInfosLoaded = true;
        next(true);
    });
    request->OnError.BindLambda([this, next](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to load player game state infos: %s"), *Error);
        next(false);
    });
    request->Dispatch();
}


bool FDriftBase::GetCount(const FString& counterName, float& value)
{
    check(playerCounterManager.IsValid());

    return playerCounterManager->GetCount(counterName, value);
}


void FDriftBase::AuthenticatePlayer()
{
    AuthenticatePlayer(FAuthenticationSettings{});
}


void FDriftBase::AuthenticatePlayer(FAuthenticationSettings AuthenticationSettings)
{
    if (state_ >= DriftSessionState::Connecting)
    {
        DRIFT_LOG(Base, Warning, TEXT("Ignoring attempt to authenticate while another attempt is in progress."));

        return;
    }

    if (cli.drift_url.IsEmpty())
    {
        DRIFT_LOG(Base, Log, TEXT("Running in client mode, but no Drift url specified."));
        // TODO: Error handling
        onPlayerAuthenticated.Broadcast(false, FPlayerAuthenticatedInfo{ EAuthenticationResult::Error_Config, TEXT("No Drift URL configured") });
        return;
    }

	if ((AuthenticationSettings.CredentialsType.IsEmpty() || AuthenticationSettings.CredentialsType == TEXT("user+pass"))
		&& AuthenticationSettings.Username.IsEmpty() != AuthenticationSettings.Password.IsEmpty())
	{
		DRIFT_LOG(Base, Error, TEXT("Username and password must be empty or non-empty at the same time!"));
		onPlayerAuthenticated.Broadcast(false, FPlayerAuthenticatedInfo{ EAuthenticationResult::Error_InvalidCredentials, TEXT("Invalid username or password") });
		return;
	}

    if (!ignoreCommandLineArguments_)
    {
        FParse::Value(FCommandLine::Get(), TEXT("-jti="), cli.jti);
    }

    if (IsPreAuthenticated())
    {
        GetRootEndpoints([this]()
        {
            TSharedRef<JsonRequestManager> manager = MakeShareable(new JTIRequestManager(cli.jti));
            manager->DefaultErrorHandler.BindRaw(this, &FDriftBase::DefaultErrorHandler);
            manager->DefaultDriftDeprecationMessageHandler.BindRaw(this, &FDriftBase::DriftDeprecationMessageHandler);
            manager->SetApiKey(GetApiKeyHeader());
            manager->SetCache(httpCache_);
            SetGameRequestManager(manager);
            GetUserInfo();
        });
        return;
    }

	if (AuthenticationSettings.CredentialsType.IsEmpty())
	{
		FString credentialType;
		if (!ignoreCommandLineArguments_)
		{
			FParse::Value(FCommandLine::Get(), TEXT("-DriftCredentialsType="), credentialType);
		}

		if (credentialType.IsEmpty())
		{
			GConfig->GetString(*settingsSection_, TEXT("CredentialsType"), credentialType, GGameIni);
		}

		if (credentialType.IsEmpty())
		{
			DRIFT_LOG(Base, Warning, TEXT("No credential type specified, falling back to uuid credentials."));
			credentialType = TEXT("uuid");
		}
		AuthenticationSettings.CredentialsType = credentialType;
	}

    GetRootEndpoints([this, AuthenticationSettings]()
	{
		InitAuthentication(AuthenticationSettings);
    });
}


EDriftConnectionState FDriftBase::GetConnectionState() const
{
    return InternalToPublicState(state_);
}


EDriftConnectionState FDriftBase::InternalToPublicState(const DriftSessionState internalState)
{
    switch (internalState)
    {
        case DriftSessionState::Undefined:
        case DriftSessionState::Disconnected:
            return EDriftConnectionState::Disconnected;
        case DriftSessionState::Connecting:
            return EDriftConnectionState::Authenticating;
        case DriftSessionState::Connected:
            return EDriftConnectionState::Connected;
        case DriftSessionState::Disconnecting:
            return EDriftConnectionState::Disconnecting;
        case DriftSessionState::Usurped:
            return EDriftConnectionState::Usurped;
        case DriftSessionState::Timedout:
            return EDriftConnectionState::Timedout;
    }
    return EDriftConnectionState::Disconnected;
}


void FDriftBase::BroadcastConnectionStateChange(const DriftSessionState internalState) const
{
    onConnectionStateChanged.Broadcast(InternalToPublicState(internalState));
}


TUniquePtr<IDriftAuthProvider> FDriftBase::MakeAuthProvider(const FString& credentialType) const
{
    auto factories = IModularFeatures::Get().GetModularFeatureImplementations<IDriftAuthProviderFactory>(TEXT("DriftAuthProviderFactory"));

#if WITH_EDITOR
	bool bEnableExternalAuthInPIE = false;
	GConfig->GetBool(*settingsSection_, TEXT("bEnableExternalAuthInPIE"), bEnableExternalAuthInPIE, GGameIni);
	if (GIsEditor && !bEnableExternalAuthInPIE)
	{
		DRIFT_LOG(Base, Warning, TEXT("Bypassing external authentication when running in editor."));

		return nullptr;
	}
#endif // WITH_EDITOR

	for (const auto factory : factories)
    {
        if (credentialType.Compare(factory->GetAuthProviderName().ToString(), ESearchCase::IgnoreCase) == 0)
        {
#if WITH_EDITOR
        	if (GIsEditor && !factory->IsSupportedInPIE())
        	{
	            DRIFT_LOG(Base, Warning, TEXT("Bypassing external authentication when running in editor."));

        		return nullptr;
        	}
#endif // WITH_EDITOR
        	return factory->GetAuthProvider();
        }
    }

    return nullptr;
}

bool FDriftBase::IsRunningAsServer() const
{
    FString dummy;
    return IsPreRegistered() || FParse::Value(FCommandLine::Get(), TEXT("-driftPass="), dummy);
}


const FString& FDriftBase::GetProjectName()
{
    if (projectName_.IsEmpty())
    {
        projectName_ = GetDefault<UGeneralProjectSettings>()->ProjectName;
    }

    if (projectName_.IsEmpty())
    {
        IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("Drift ProjectName is empty or missing. Please fill out Project Settings->Drift"));
    }

    return projectName_;
}


const FGuid& FDriftBase::GetAppGuid()
{
    if (!appGuid_.IsValid())
    {
        appGuid_ = GetDefault<UGeneralProjectSettings>()->ProjectID;
    }

    if (!appGuid_.IsValid())
    {
        IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("No Drift App GUID found. Please fill out Project Settings->Drift"));
    }

    return appGuid_;
}


IDriftAuthProviderFactory* FDriftBase::GetDeviceAuthProviderFactory()
{
    if (!deviceAuthProviderFactory_.IsValid())
    {
        deviceAuthProviderFactory_ = MakeUnique<FDriftUuidAuthProviderFactory>(instanceIndex_, GetProjectName());
    }
    return deviceAuthProviderFactory_.Get();
}


IDriftAuthProviderFactory* FDriftBase::GetUserPassAuthProviderFactory(const FString& Username, const FString& Password, bool bAllowAutomaticAccountCreation)
{
	if (!userPassAuthProviderFactory_.IsValid())
	{
		userPassAuthProviderFactory_ = MakeUnique<FDriftUserPassAuthProviderFactory>(instanceIndex_, GetProjectName(), Username, Password, bAllowAutomaticAccountCreation);
	}
	return userPassAuthProviderFactory_.Get();
}


void FDriftBase::GetActiveMatches(const TSharedRef<FMatchesSearch>& search)
{
    FString matches_url = driftEndpoints.active_matches;
    FString ref_filter = search->ref_filter;
    if (ref_filter.IsEmpty())
    {
        ref_filter = buildReference;
    }
    internal::UrlHelper::AddUrlOption(matches_url, TEXT("ref"), ref_filter);
    internal::UrlHelper::AddUrlOption(matches_url, TEXT("placement"), defaultPlacement);

    DRIFT_LOG(Base, Verbose, TEXT("Fetching active matches ref='%s', placement='%s'"), *ref_filter, *defaultPlacement);

    auto request = GetGameRequestManager()->Get(matches_url);
    request->OnResponse.BindLambda([this, search](ResponseContext& context, JsonDocument& doc)
    {
        if (!JsonArchive::LoadObject(doc, cached_matches.matches))
        {
            context.error = TEXT("Failed to parse matches");
            return;
        }

        search->matches.Empty();
        for (const auto& match : cached_matches.matches)
        {
            FActiveMatch result;
            result.create_date = match.create_date;
            result.game_mode = match.game_mode;
            result.map_name = match.map_name;
            result.match_id = match.match_id;
            result.match_status = match.match_status;
            result.num_players = match.num_players;
			result.max_players = match.max_players;
            result.server_status = match.server_status;
			result.ue4_connection_url = match.ue4_connection_url;
            result.version = match.version;
            search->matches.Add(result);
        }
        onGotActiveMatches.Broadcast(true);

        auto event = MakeEvent(TEXT("drift.active_matches_loaded"));
        event->Add(TEXT("ref"), *search->ref_filter);
        event->Add(TEXT("placement"), *defaultPlacement);
        event->Add(TEXT("num_results"), cached_matches.matches.Num());
        event->Add(TEXT("request_time"), (context.received - context.sent).GetTotalSeconds());
        AddAnalyticsEvent(MoveTemp(event));
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        onGotActiveMatches.Broadcast(false);
    });
    request->Dispatch();
}


void FDriftBase::JoinMatchQueue(const FDriftJoinedMatchQueueDelegate& delegate)
{
    JoinMatchQueueImpl(buildReference, defaultPlacement, TEXT(""), delegate);
}


void FDriftBase::HandleMatchQueueMessage(const FMessageQueueEntry& message)
{
    const auto tokenIt = message.payload.FindField(TEXT("token"));
	if (!tokenIt.IsString())
    {
        UE_LOG(LogDriftMessages, Error, TEXT("Match queue message contains no valid token"));

        return;
    }
    const FString token = tokenIt.GetString();

    const auto action = message.payload.FindField(TEXT("action"));
    if (action)
    {
        if (!action.IsString())
        {
            UE_LOG(LogDriftMessages, Error, TEXT("Can't parse match queue action"));

            return;
        }

		if (action.GetString() == TEXT("challenge"))
        {
            UE_LOG(LogDriftMessages, Verbose, TEXT("Got match challenge from player: %d, token: %s"), message.sender_id, *token);

            if (onReceivedMatchInvite.IsBound())
            {
                onReceivedMatchInvite.Broadcast(FMatchInvite{ message.sender_id, token, message.timestamp, message.expires });
            }
            else
            {
                matchInvites.Add(FMatchInvite{ message.sender_id, token, message.timestamp, message.expires });
            }
        }
    }
}


void FDriftBase::HandleFriendEventMessage(const FMessageQueueEntry& message)
{
    const auto event = message.payload.FindField(TEXT("event"));
    if (!event.IsString())
    {
        UE_LOG(LogDriftMessages, Error, TEXT("Friend event message contains no event"));

        return;
    }
    const FString eventName = event.GetString();
    if (eventName == TEXT("friend_added"))
    {
        UE_LOG(LogDriftMessages, Verbose, TEXT("Got friend added confirmation from player %d"), message.sender_id);

        onFriendAdded.Broadcast(message.sender_id);
    }
    else if (eventName == TEXT("friend_removed"))
    {
        UE_LOG(LogDriftMessages, Verbose, TEXT("Friend player %d removed friendship"), message.sender_id);

        onFriendRemoved.Broadcast(message.sender_id);
    }
    else if (eventName == TEXT("friend_request"))
    {
        UE_LOG(LogDriftMessages, Verbose, TEXT("Player %d wants to be friends with us. Awwww..."), message.sender_id);
        auto token = message.payload.FindField(TEXT("token"));
        if (!token.IsString())
        {
            UE_LOG(LogDriftMessages, Error, TEXT("Missing or invalid friend invite token"));
        }
        onFriendRequestReceived.Broadcast(message.sender_id, token.ToString());
    }
    else
    {
        #if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
        UE_LOG(LogDriftMessages, Warning, TEXT("Unknown event '%s' not handled"), *event.ToString());
        #endif
    }
}


void FDriftBase::JoinMatchQueueImpl(const FString& ref, const FString& placement, const FString& token, const FDriftJoinedMatchQueueDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("Attempting to join the match queue without being connected"));

        delegate.ExecuteIfBound(false, FMatchQueueStatus{});
        return;
    }

    if (matchQueueState != EMatchQueueState::Idle)
    {
        IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("Attempting to join the match queue while not idle"));

        delegate.ExecuteIfBound(false, FMatchQueueStatus{});
        return;
    }

    matchQueue = FMatchQueueResponse{};
    matchQueueState = EMatchQueueState::Joining;

    DRIFT_LOG(Base, Verbose, TEXT("Joining match queue ref='%s', placement='%s', token='%s'..."), *buildReference, *placement, *token);

    FJoinMatchQueuePayload payload{};
    payload.player_id = myPlayer.player_id;
    payload.ref = buildReference;
    payload.placement = placement;
    payload.token = token;
    auto request = GetGameRequestManager()->Post(driftEndpoints.matchqueue, payload);
    request->OnResponse.BindLambda([this, delegate](ResponseContext& context, JsonDocument& doc)
    {
        if (!JsonArchive::LoadObject(doc, matchQueue))
        {
            context.error = TEXT("Failed to parse join queue response");
            return;
        }
        matchQueueState = matchQueue.status == MatchQueueStatusMatchedName ? EMatchQueueState::Matched : EMatchQueueState::Queued;
        delegate.ExecuteIfBound(true, FMatchQueueStatus{
            matchQueue.status, FMatchQueueMatch{
                matchQueue.match_id,
                matchQueue.create_date,
                matchQueue.ue4_connection_url
            }
        });
    });
    request->OnError.BindLambda([this, delegate](ResponseContext& context)
    {
        matchQueueState = EMatchQueueState::Idle;
        context.errorHandled = true;
        delegate.ExecuteIfBound(false, FMatchQueueStatus{});
    });
    request->Dispatch();
}


void FDriftBase::LeaveMatchQueue(const FDriftLeftMatchQueueDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("Attempting to leave the match queue without being connected"));

        delegate.ExecuteIfBound(false);
        return;
    }

    if (matchQueue.matchqueueplayer_url.IsEmpty())
    {
        IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("Attempting to leave the match queue without being in one"));

        delegate.ExecuteIfBound(false);
        return;
    }

    if (matchQueueState == EMatchQueueState::Matched)
    {
        IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("Attempting to leave the match queue after getting matched"));

        delegate.ExecuteIfBound(false);
        return;
    }

    DRIFT_LOG(Base, Verbose, TEXT("Leaving match queue..."));

    matchQueueState = EMatchQueueState::Leaving;
    auto request = GetGameRequestManager()->Delete(matchQueue.matchqueueplayer_url);
    request->OnResponse.BindLambda([this, delegate](ResponseContext& context, JsonDocument& doc)
    {
        matchQueue = FMatchQueueResponse{};
        matchQueueState = EMatchQueueState::Idle;
        delegate.ExecuteIfBound(true);
    });
    request->OnError.BindLambda([this, delegate](ResponseContext& context)
    {
        if (context.responseCode == static_cast<int32>(HttpStatusCodes::BadRequest))
        {
            GenericRequestErrorResponse response;
            if (JsonUtils::ParseResponse(context.response, response))
            {
                if (response.GetErrorCode() == TEXT("player_already_matched"))
                {
                    // Let the next poll update the state
                    if (matchQueueState == EMatchQueueState::Leaving)
                    {
                        matchQueueState = EMatchQueueState::Queued;
                    }
                    context.errorHandled = true;
                    delegate.ExecuteIfBound(false);
                    return;
                }
            }
        }
        else if (context.responseCode == static_cast<int32>(HttpStatusCodes::NotFound))
        {
            GenericRequestErrorResponse response;
            if (JsonUtils::ParseResponse(context.response, response))
            {
                if (response.GetErrorCode() == TEXT("player_not_in_queue"))
                {
                    // If the player is not in a queue, treat it as success
                    matchQueue = FMatchQueueResponse{};
                    matchQueueState = EMatchQueueState::Idle;
                    context.errorHandled = true;
                    delegate.ExecuteIfBound(true);
                    return;
                }
            }
        }

        IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("Failed to leave the match queue for an unknown reason"));

        context.errorHandled = true;
        delegate.ExecuteIfBound(false);
    });
    request->Dispatch();
}


void FDriftBase::PollMatchQueue(const FDriftPolledMatchQueueDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("Attempting to poll the match queue without being connected"));

        delegate.ExecuteIfBound(false, FMatchQueueStatus{});
        return;
    }

    if (matchQueue.matchqueueplayer_url.IsEmpty())
    {
        IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("Attempting to poll the match queue without being in one"));

        delegate.ExecuteIfBound(false, FMatchQueueStatus{});
        return;
    }

    if (matchQueueState != EMatchQueueState::Queued && matchQueueState != EMatchQueueState::Matched)
    {
        auto extra = MakeShared<FJsonObject>();
        extra->SetNumberField(TEXT("state"), static_cast<int32>(matchQueueState));
        IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("Attempting to poll the match queue while in an incompatible state"), extra);

        delegate.ExecuteIfBound(false, FMatchQueueStatus{});
        return;
    }

    matchQueueState = EMatchQueueState::Updating;
    auto request = GetGameRequestManager()->Get(matchQueue.matchqueueplayer_url);
    request->OnResponse.BindLambda([this, delegate](ResponseContext& context, JsonDocument& doc)
    {
        FMatchQueueResponse response;
        if (!JsonArchive::LoadObject(doc, response))
        {
            context.error = TEXT("Failed to parse poll queue response");
            return;
        }

        DRIFT_LOG(Base, VeryVerbose, TEXT("%s"), *JsonArchive::ToString(doc));

        if (response.status == MatchQueueStatusMatchedName && !response.match_url.IsEmpty())
        {
            matchQueueState = EMatchQueueState::Matched;
        }
        else
        {
            matchQueueState = EMatchQueueState::Queued;
        }

        delegate.ExecuteIfBound(true, FMatchQueueStatus{
            response.status, FMatchQueueMatch{
                response.match_id,
                response.create_date,
                response.ue4_connection_url
            }
        });
    });
    request->OnError.BindLambda([this, delegate](ResponseContext& context)
    {
        // TODO: This is not strictly true, but probably depends on the error
        matchQueueState = EMatchQueueState::Idle;
        context.errorHandled = true;
        delegate.ExecuteIfBound(false, FMatchQueueStatus{});
    });
    request->Dispatch();
}


void FDriftBase::ResetMatchQueue()
{
    if (state_ != DriftSessionState::Connected)
    {
        IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("Attempting to reset the match queue without being connected"));

        return;
    }

    if (matchQueue.matchqueueplayer_url.IsEmpty())
    {
        IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("Attempting to reset the match queue without being in one"));

        return;
    }

    if (matchQueueState != EMatchQueueState::Matched)
    {
        IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("Attempting to reset the match queue without being matched"));

        return;
    }

    matchQueue = FMatchQueueResponse{};
    matchQueueState = EMatchQueueState::Idle;

    DRIFT_LOG(Base, Log, TEXT("Resetting match queue"));
}


EMatchQueueState FDriftBase::GetMatchQueueState() const
{
    return matchQueueState;
}


void FDriftBase::InvitePlayerToMatch(int32 playerID, const FDriftJoinedMatchQueueDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("Attempting to send match challenge without being connected"));

        delegate.ExecuteIfBound(false, FMatchQueueStatus{});
        return;
    }

    if (matchQueueState != EMatchQueueState::Idle)
    {
        IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("Attempting to send match challenge while not idle"));

        delegate.ExecuteIfBound(false, FMatchQueueStatus{});
        return;
    }

    if (playerID == myPlayer.player_id)
    {
        IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("Attempting to challenge yourself to a match is not allowed"));

        delegate.ExecuteIfBound(false, FMatchQueueStatus{});
        return;
    }

    auto playerInfo = GetFriendInfo(playerID);
    if (!playerInfo)
    {
        auto extra = MakeShared<FJsonObject>();
        extra->SetNumberField(TEXT("player_id"), playerID);
        IErrorReporter::Get()->AddError(TEXT("LogDriftBase"), TEXT("Attempting to challenge player to match, but there's no information about the player"), extra);

        delegate.ExecuteIfBound(false, FMatchQueueStatus{});
        return;
    }

    // TODO: Preferably one should create the match/queue entry, and then send the invite separately

    const auto token = FGuid::NewGuid();
    JoinMatchQueueImpl(buildReference, defaultPlacement, token.ToString(), FDriftJoinedMatchQueueDelegate::CreateLambda([this, playerInfo, token, delegate](bool success, const FMatchQueueStatus& status)
    {
        if (success)
        {
            const int inviteTimeoutSeconds = 180;

            JsonValue message{ rapidjson::kObjectType };
            JsonArchive::AddMember(message, TEXT("action"), TEXT("challenge"));
            JsonArchive::AddMember(message, TEXT("token"), *token.ToString());
            const auto messageUrlTemplate = playerInfo->messagequeue_url;
            messageQueue->SendMessage(messageUrlTemplate, MatchQueue, MoveTemp(message), inviteTimeoutSeconds);
        }
        delegate.ExecuteIfBound(success, status);
    }));
}


void FDriftBase::JoinMatch(const FMatchInvite& invite, const FDriftJoinedMatchQueueDelegate& delegate)
{
    JoinMatchQueueImpl(TEXT(""), TEXT(""), invite.token, delegate);
}


void FDriftBase::AcceptMatchInvite(const FMatchInvite& invite, const FDriftJoinedMatchQueueDelegate& delegate)
{
    JoinMatchQueueImpl(TEXT(""), TEXT(""), invite.token, delegate);
}


void FDriftBase::TickMatchInvites()
{
    if (matchInvites.Num() > 0 && onReceivedMatchInvite.IsBound())
    {
        for (const auto& invite : matchInvites)
        {
            onReceivedMatchInvite.Broadcast(invite);
        }
        matchInvites.Empty();
    }
}


void FDriftBase::TickFriendUpdates(float deltaTime)
{
    if (shouldUpdateFriends)
    {
        updateFriendsInSeconds -= deltaTime;
        if (updateFriendsInSeconds < 0.0f)
        {
            updateFriendsInSeconds = UPDATE_FRIENDS_INTERVAL;

            shouldUpdateFriends = false;
            UpdateFriendOnlineInfos();
        }
    }
}


void FDriftBase::AddCount(const FString& counterName, float value, bool absolute)
{
    check(playerCounterManager.IsValid());

    playerCounterManager->AddCount(counterName, value, absolute);
}


void FDriftBase::AddAnalyticsEvent(const FString& eventName, const TArray<FAnalyticsEventAttribute>& attributes)
{
    auto event = MakeEvent(eventName);
    for (const auto& attribute : attributes)
    {
#ifdef WITH_ANALYTICS_EVENT_ATTRIBUTE_TYPES
#ifdef WITH_ENGINE_VERSION_MACROS
#if UE_VERSION_OLDER_THAN(4, 26, 0)
        switch (attribute.AttrType)
        {
            case FAnalyticsEventAttribute::AttrTypeEnum::Boolean:
                 event->Add(attribute.AttrName, attribute.AttrValueBool);
                break;
            case FAnalyticsEventAttribute::AttrTypeEnum::JsonFragment:
                event->Add(attribute.AttrName, attribute.AttrValueString);
                break;
            case FAnalyticsEventAttribute::AttrTypeEnum::Null:
                event->Add(attribute.AttrName, nullptr);
                break;
            case FAnalyticsEventAttribute::AttrTypeEnum::Number:
                event->Add(attribute.AttrName, attribute.AttrValueNumber);
                break;
            case FAnalyticsEventAttribute::AttrTypeEnum::String:
                event->Add(attribute.AttrName, attribute.AttrValueString);
                break;
            }
#else
        event->Add(attribute.GetName(), attribute.GetValue());
#endif
#endif
#else
        event->Add(attribute.AttrName, attribute.AttrValue);
#endif
    }
    AddAnalyticsEvent(MoveTemp(event));
}


void FDriftBase::AddAnalyticsEvent(TUniquePtr<IDriftEvent> event)
{
    if (eventManager.IsValid())
    {
        if (match_info.match_id != 0)
        {
            event->Add(TEXT("match_id"), match_info.match_id);
        }
        eventManager->AddEvent(MoveTemp(event));
    }
}


const FDriftCounterInfo* FDriftBase::GetCounterInfo(const FString& counterName) const
{
    auto canonical_name = FDriftCounterManager::MakeCounterName(counterName);
    const auto counter = counterInfos.FindByPredicate([canonical_name](const FDriftCounterInfo& info) -> bool
    {
        return info.name == canonical_name;
    });
    return counter;
}


void FDriftBase::GetLeaderboard(const FString& counterName, const TSharedRef<FDriftLeaderboard>& leaderboard, const FDriftLeaderboardLoadedDelegate& delegate)
{
    leaderboard->state = ELeaderboardState::Failed;

    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to load player counters without being connected"));

        delegate.ExecuteIfBound(false, FDriftCounterManager::MakeCounterName(counterName));
        return;
    }

    if (myPlayer.counter_url.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to load player counters before the player session has been initialized"));

        delegate.ExecuteIfBound(false, FDriftCounterManager::MakeCounterName(counterName));
        return;
    }

    leaderboard->state = ELeaderboardState::Loading;

    BeginGetLeaderboard(counterName, leaderboard, TEXT(""), delegate);
}


void FDriftBase::GetFriendsLeaderboard(const FString& counterName, const TSharedRef<FDriftLeaderboard>& leaderboard, const FDriftLeaderboardLoadedDelegate& delegate)
{
    leaderboard->state = ELeaderboardState::Failed;

    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to load player counters without being connected"));

        delegate.ExecuteIfBound(false, FDriftCounterManager::MakeCounterName(counterName));
        return;
    }

    if (driftEndpoints.my_player_groups.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to load friend counters before the player session has been initialized"));

        delegate.ExecuteIfBound(false, FDriftCounterManager::MakeCounterName(counterName));
        return;
    }

    leaderboard->state = ELeaderboardState::Loading;

    /**
     * TODO: Handle the case where a player adds/removes friends during a session.
     * External IDs and userIdentities would have to be refreshed.
     */
    if (userIdentitiesLoaded)
    {
        BeginGetFriendLeaderboard(counterName, leaderboard, delegate);
    }
    else
    {
        LoadFriendsList(FDriftFriendsListLoadedDelegate::CreateLambda([this, counterName, leaderboard, delegate](bool success)
        {
            if (success)
            {
                BeginGetFriendLeaderboard(counterName, leaderboard, delegate);
            }
            else
            {
                leaderboard->state = ELeaderboardState::Failed;
                delegate.ExecuteIfBound(false, FDriftCounterManager::MakeCounterName(counterName));
            }
        }));
    }
}


void FDriftBase::BeginGetFriendLeaderboard(const FString& counterName, const TWeakPtr<FDriftLeaderboard>& leaderboard, const FDriftLeaderboardLoadedDelegate& delegate)
{
    BeginGetLeaderboard(counterName, leaderboard, TEXT("friends"), delegate);
}


void FDriftBase::BeginGetLeaderboard(const FString& counterName, const TWeakPtr<FDriftLeaderboard>& leaderboard, const FString& playerGroup, const FDriftLeaderboardLoadedDelegate& delegate)
{
    /**
     * TODO: If the game adds a new counter, it won't be in the counter_infos list until the next connect.
     * This may or may not be something we want to solve, for instance by marking counter_infos as dirty
     * when the new counter is added.
     */
    if (countersLoaded)
    {
        GetLeaderboardImpl(counterName, leaderboard, playerGroup, delegate);
    }
    else
    {
        auto request = GetGameRequestManager()->Get(driftEndpoints.counters);
        request->OnResponse.BindLambda([counterName, leaderboard, playerGroup, delegate, this](ResponseContext& context, JsonDocument& doc)
        {
            if (!JsonArchive::LoadObject(doc, counterInfos))
            {
                context.error = TEXT("Failed to parse leaderboards response");
                return;
            }
            countersLoaded = true;
            GetLeaderboardImpl(counterName, leaderboard, playerGroup, delegate);
        });
        request->OnError.BindLambda([counterName, leaderboard, delegate](ResponseContext& context)
        {
            auto leaderboardPtr = leaderboard.Pin();
            if (leaderboardPtr.IsValid())
            {
                leaderboardPtr->state = ELeaderboardState::Failed;
            }
            context.errorHandled = true;
            delegate.ExecuteIfBound(false, FDriftCounterManager::MakeCounterName(counterName));
        });
        request->Dispatch();
    }
}


void FDriftBase::GetLeaderboardImpl(const FString& counterName, const TWeakPtr<FDriftLeaderboard>& leaderboard, const FString& playerGroup, const FDriftLeaderboardLoadedDelegate& delegate)
{
    auto canonicalName = FDriftCounterManager::MakeCounterName(counterName);

    DRIFT_LOG(Base, Log, TEXT("Getting leaderboard for %s"), *canonicalName);

    auto leaderboardPtr = leaderboard.Pin();
    if (leaderboardPtr.IsValid())
    {
        leaderboardPtr->rows.Empty();
    }

    const auto counter = GetCounterInfo(counterName);

    if (counter == nullptr || counter->url.IsEmpty())
    {
        DRIFT_LOG(Base, Log, TEXT("Found no leaderboard for %s"), *canonicalName);

        delegate.ExecuteIfBound(false, canonicalName);

        return;
    }

    auto url = counter->url;
    if (!playerGroup.IsEmpty())
    {
        internal::UrlHelper::AddUrlOption(url, TEXT("player_group"), *playerGroup);
    }

    auto request = GetGameRequestManager()->Get(url);
    request->OnResponse.BindLambda([this, leaderboard, canonicalName, delegate, playerGroup](ResponseContext& context, JsonDocument& doc)
    {
        TArray<FDriftLeaderboardResponseItem> entries;
        if (!JsonArchive::LoadObject(doc, entries))
        {
            context.error = TEXT("Failed to parse leaderboard entries response");
            return;
        }

        DRIFT_LOG(Base, Verbose, TEXT("Got %d entries for leaderboard %s"), entries.Num(), *canonicalName);

        auto leaderboardPin = leaderboard.Pin();
        if (leaderboardPin.IsValid())
        {
            for (const auto& entry : entries)
            {
                leaderboardPin->rows.Add(FDriftLeaderboardEntry{ entry.player_name, entry.player_id, entry.total, entry.position });
            }

            leaderboardPin->state = ELeaderboardState::Ready;
        }
        delegate.ExecuteIfBound(true, canonicalName);

        auto event = MakeEvent(TEXT("drift.leaderboard_loaded"));
        event->Add(TEXT("counter_name"), *canonicalName);
        event->Add(TEXT("num_entries"), entries.Num());
        event->Add(TEXT("player_group"), *playerGroup);
        event->Add(TEXT("request_time"), (context.received - context.sent).GetTotalSeconds());
        AddAnalyticsEvent(MoveTemp(event));
    });
    request->OnError.BindLambda([canonicalName, delegate](ResponseContext& context)
    {
        context.errorHandled = true;
        delegate.ExecuteIfBound(false, canonicalName);
    });
    request->Dispatch();
}


void FDriftBase::LoadFriendsList(const FDriftFriendsListLoadedDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to load friends list without being connected"));

        delegate.ExecuteIfBound(false);
        return;
    }

    LoadDriftFriends(delegate);
}


void FDriftBase::UpdateFriendsList()
{
    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to update friends list without being connected"));

        return;
    }

    if (driftEndpoints.my_player_groups.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to update friends list before the player session has been initialized"));

        return;
    }

    if (friendInfos.Num() > 0)
    {
        shouldUpdateFriends = true;
    }
}


bool FDriftBase::GetFriendsList(TArray<FDriftFriend>& friends)
{
    for (const auto& entry : userIdentities.players)
    {
        if (entry.player_id == myPlayer.player_id)
        {
            continue;
        }
        const auto playerInfo = GetFriendInfo(entry.player_id);
        const auto presence = (playerInfo && playerInfo->is_online) ? EDriftPresence::Online : EDriftPresence::Offline;
        const auto type = (driftFriends.Find(entry.player_id) != nullptr)
            ? EDriftFriendType::Drift : EDriftFriendType::External;
        friends.Add(FDriftFriend{ entry.player_id, entry.player_name, presence, type });
    }
    return true;
}


FString FDriftBase::GetFriendName(int32 friendID)
{
    if (const auto info = GetFriendInfo(friendID))
    {
        return info->player_name;
    }
    return TEXT("");
}


bool FDriftBase::IssueFriendToken(int32 PlayerID, FDriftFriendTokenProperties TokenProperties, const FDriftIssueFriendTokenDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        const FString Error = TEXT("Attempting to get a friend request token without being connected");
        DRIFT_LOG(Base, Warning, TEXT("%s"), *Error);
        delegate.ExecuteIfBound(false, {}, Error);
        return false;
    }

    if (driftEndpoints.my_friends.IsEmpty())
    {
        const FString Error = TEXT("Attempting to get a friends request token before the player session has been initialized");
        DRIFT_LOG(Base, Warning, TEXT("%s"), *Error);
        delegate.ExecuteIfBound(false, {}, Error);
        return false;
    }

    JsonValue Payload{ rapidjson::kObjectType };

    if (PlayerID > 0)
    {
        JsonArchive::AddMember(Payload, TEXT("player_id"), PlayerID);
    }

    if (TokenProperties.TokenFormat.IsSet())
    {
        JsonArchive::AddMember(Payload, TEXT("token_format"), *TokenProperties.TokenFormat.GetValue());
    }

    if (TokenProperties.WordlistNumberOfWords.IsSet())
    {
        JsonArchive::AddMember(Payload, TEXT("worldlist_number_of_words"), TokenProperties.WordlistNumberOfWords.GetValue());
    }

    if (TokenProperties.ExpirationTimeInSeconds.IsSet())
    {
        JsonArchive::AddMember(Payload, TEXT("expiration_time_seconds"), TokenProperties.ExpirationTimeInSeconds.GetValue());
    }

    DRIFT_LOG(Base, Verbose, TEXT("Issuing a friend request token to %s"), PlayerID > 0 ? *FString::Printf(TEXT("player with ID %d"), PlayerID) : TEXT("any player"));

    const auto Request = GetGameRequestManager()->Post(driftEndpoints.friend_invites, Payload);
    Request->OnResponse.BindLambda([this, delegate](ResponseContext& Context, JsonDocument& Doc)
    {
        FString Token;
        const auto member = Doc.FindField(TEXT("token"));
        if (member.IsString())
        {
            Token = member.GetString();
        }
        if (Token.IsEmpty())
        {
            Context.error = TEXT("Response 'token' missing.");
            delegate.ExecuteIfBound(false, {}, Context.error);
            return;
        }

        DRIFT_LOG(Base, Verbose, TEXT("Got friend request token: %s"), *Token);

        delegate.ExecuteIfBound(true, Token, {});
    });
    Request->OnError.BindLambda([this, delegate](ResponseContext& Context)
    {
        FString Error;
        Context.errorHandled = GetResponseError(Context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to issue friend request token. Error: %s"), *Error);
        delegate.ExecuteIfBound(false, {}, Error);
    });
    Request->Dispatch();
    return true;
}


bool FDriftBase::AcceptFriendRequestToken(const FString& token, const FDriftAcceptFriendRequestDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to accept a friend request without being connected"));

        return false;
    }

    if (driftEndpoints.my_friends.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to accept a friends request before the player session has been initialized"));

        return false;
    }

    DRIFT_LOG(Base, Verbose, TEXT("Accepting a friend request with token %s"), *token);

    JsonValue Payload{ rapidjson::kObjectType };
    JsonArchive::AddMember(Payload, TEXT("token"), *token);

    // Returns 201 friendship established and 200 if friendship exists. 200 response has an empty body
    const auto Request = GetGameRequestManager()->Post(driftEndpoints.my_friends, Payload);
    Request->OnResponse.BindLambda([this, delegate](ResponseContext& Context, JsonDocument& Doc)
    {
        int32 FriendID{ 0 };
        const auto Member = Doc.FindField(TEXT("friend_id"));
        if (Member.IsInt32())
        {
            FriendID = Member.GetInt32();
        }

        if (FriendID == 0)
        {
            Context.error = TEXT("Friend ID is not valid");
            return;
        }

        LoadFriendsList(FDriftFriendsListLoadedDelegate::CreateLambda([this, FriendID](bool bSuccess)
        {
            if (state_ != DriftSessionState::Connected)
            {
                return;
            }
            if (const auto FriendInfo = friendInfos.Find(FriendID))
            {
                JsonValue Message{ rapidjson::kObjectType };
                JsonArchive::AddMember(Message, TEXT("event"), TEXT("friend_added"));
                const auto MessageUrlTemplate = FriendInfo->messagequeue_url;
                messageQueue->SendMessage(MessageUrlTemplate, FriendEvent, MoveTemp(Message));
            }
        }));

        delegate.ExecuteIfBound(true, FriendID, "");
    });
    Request->OnError.BindLambda([delegate](ResponseContext& Context)
    {
        if (Context.responseCode == static_cast<int32>(HttpStatusCodes::Ok))
        {
            Context.errorHandled = true;
            delegate.ExecuteIfBound(false, 0, TEXT("Already friends"));
            return;
        }

        FString Error;
        Context.errorHandled = GetResponseError(Context, Error);
        delegate.ExecuteIfBound(false, 0, Error);
    });
    Request->Dispatch();
    return true;
}

bool FDriftBase::DeclineFriendRequest(int32 RequestId, FDriftDeclineFriendRequestDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to decline a friend request without being connected"));
        return false;
    }

    if (driftEndpoints.my_friends.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to decline a friends request before the player session has been initialized"));
        return false;
    }

    DRIFT_LOG(Base, Verbose, TEXT("Declining friend request %d"), RequestId);
    const FString url = FString::Printf(TEXT("%s/%i"), *driftEndpoints.friend_invites, RequestId);
    auto request = GetGameRequestManager()->Delete(url, HttpStatusCodes::NoContent);

    request->OnResponse.BindLambda([delegate](ResponseContext& context, JsonDocument& doc)
    {
        delegate.ExecuteIfBound(true);
    });
    request->OnError.BindLambda([this, delegate](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to decline friend request. Error: %s"), *Error);
        delegate.ExecuteIfBound(false);
    });
    request->Dispatch();
    return true;
}


bool FDriftBase::GetFriendRequests(const FDriftGetFriendRequestsDelegate& Delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to fetch friend requests without being connected"));
        return false;
    }
    if (driftEndpoints.friend_requests.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to fetch friend requests without a player session"));
        return false;
    }
    DRIFT_LOG(Base, Verbose, TEXT("Getting friend requests...."));
    auto request = GetGameRequestManager()->Get(driftEndpoints.friend_requests);
    request->OnResponse.BindLambda([this, Delegate](ResponseContext& context, JsonDocument& doc)
    {
        DRIFT_LOG(Base, Verbose, TEXT("Loaded friend requests: %s"), *doc.ToString());

        TArray<FDriftFriendRequestsResponse> response;
        if (!JsonArchive::LoadObject(doc, response))
        {
            context.error = TEXT("Failed to parse invites response");
            return;
        }

        TArray<FDriftFriendRequest> friend_requests;
        for (const auto& It: response)
        {
            friend_requests.Add({
                It.id,
                It.create_date,
                It.expiry_date,
                It.issued_by_player_id,
                It.issued_by_player_url,
                It.issued_by_player_name,
                It.issued_to_player_id,
                It.issued_to_player_url,
                It.issued_to_player_name,
                It.accept_url,
                It.token
            });
        }

        Delegate.ExecuteIfBound(true, friend_requests);
    });
    request->OnError.BindLambda([this, Delegate](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to get friend requests. Error: %s"), *Error);
        Delegate.ExecuteIfBound(false, {});
    });

    return request->Dispatch();
}

bool FDriftBase::GetSentFriendInvites(const FDriftGetFriendRequestsDelegate& Delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to fetch friend invites without being connected"));
        return false;
    }
    if (driftEndpoints.friend_invites.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to fetch friend invites without a player session"));
        return false;
    }
    DRIFT_LOG(Base, Verbose, TEXT("Getting friend invites...."));
    const auto Request = GetGameRequestManager()->Get(driftEndpoints.friend_invites);
    Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
    {
        DRIFT_LOG(Base, Verbose, TEXT("Loaded friend invites: %s"), *Doc.ToString());

        TArray<FDriftFriendRequestsResponse> Response;
        if (!JsonArchive::LoadObject(Doc, Response))
        {
            Context.error = TEXT("Failed to parse invites response");
            return;
        }

        TArray<FDriftFriendRequest> FriendInvites;
        for (const auto& It: Response)
        {
            FriendInvites.Add({
                It.id,
                It.create_date,
                It.expiry_date,
                It.issued_by_player_id,
                It.issued_by_player_url,
                It.issued_by_player_name,
                It.issued_to_player_id,
                It.issued_to_player_url,
                It.issued_to_player_name,
                It.accept_url,
                It.token
            });
        }

        (void)Delegate.ExecuteIfBound(true, FriendInvites);
    });
    Request->OnError.BindLambda([this, Delegate](ResponseContext& Context)
    {
        FString Error;
        Context.errorHandled = GetResponseError(Context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to get sent friend invites. Error: %s"), *Error);
        Delegate.ExecuteIfBound(false, {});
    });

    return Request->Dispatch();
}

bool FDriftBase::RemoveFriend(int32 friendID, const FDriftRemoveFriendDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to remove a friend without being connected"));
        return false;
    }

    if (driftEndpoints.my_friends.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to remove a friend before the player session has been initialized"));
        return false;
    }

    const auto friendInfo = driftFriends.Find(friendID);
    if (friendInfo == nullptr)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to remove a friend which is not (yet) known to the system"));
        return false;
    }

    DRIFT_LOG(Base, Verbose, TEXT("Removing friend %d"), friendID);

    auto request = GetGameRequestManager()->Delete(friendInfo->friendship_url, HttpStatusCodes::NoContent);
    request->OnResponse.BindLambda([this, friendID, delegate](ResponseContext& context, JsonDocument& doc)
    {
        if (auto f = friendInfos.Find(friendID))
        {
            JsonValue message{ rapidjson::kObjectType };
            JsonArchive::AddMember(message, TEXT("event"), TEXT("friend_removed"));
            const auto messageUrlTemplate = f->messagequeue_url;
            messageQueue->SendMessage(messageUrlTemplate, FriendEvent, MoveTemp(message));
        }

        LoadFriendsList({});

        delegate.ExecuteIfBound(true, friendID);
    });
    request->OnError.BindLambda([this, friendID, delegate](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to remove friend '%d'. Error: %s"), friendID, *Error);
        delegate.ExecuteIfBound(false, friendID);
    });
    request->Dispatch();
    return true;
}

bool FDriftBase::FindPlayersByName(const FString& SearchString, const FDriftFindPlayerByNameDelegate& delegate)
{
    DRIFT_LOG(Base, Verbose, TEXT("Searching for %s"), *SearchString);
    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to search without being connected"));
        return false;
    }
    FString url = driftEndpoints.players;
    internal::UrlHelper::AddUrlOption(url, TEXT("player_name"), FString::Printf(TEXT("%s"), *SearchString));
    auto request = GetGameRequestManager()->Get(url);
    request->OnResponse.BindLambda([this, SearchString, delegate](ResponseContext& context, JsonDocument& doc)
    {
        DRIFT_LOG(Base, Verbose, TEXT("Search for %s yielded %s"), *SearchString, *doc.ToString());
        TArray<FDriftPlayerResponse> response;
        if (!JsonArchive::LoadObject(doc, response))
        {
            context.error = TEXT("Failed to parse search response");
            delegate.ExecuteIfBound(false, {});
            return;
        }
        TArray<FDriftFriend> results;
        for (const auto& It: response)
        {
            results.Add({It.player_id, It.player_name, EDriftPresence::Unknown, EDriftFriendType::NotFriend});
        }
        delegate.ExecuteIfBound(true, results);
    });
    request->OnError.BindLambda([this, SearchString, delegate](ResponseContext& context)
    {
        DRIFT_LOG(Base, Warning, TEXT("Failed to search for %s: %s"), *SearchString, *context.error);
        context.errorHandled = true;
        delegate.ExecuteIfBound(false, {});
    });
    request->Dispatch();
    return true;
}

void FDriftBase::ConfigureSettingsSection(const FString& config)
{
    if (config.IsEmpty())
    {
        settingsSection_ = defaultSettingsSection;
    }
    else
    {
        settingsSection_ = FString::Printf(TEXT("%s.%s"), defaultSettingsSection, *config);
    }
}


void FDriftBase::GetRootEndpoints(TFunction<void()> onSuccess)
{
    FString url = cli.drift_url;

    check(!url.IsEmpty());

    DRIFT_LOG(Base, Verbose, TEXT("Getting root endpoints from %s"), *url);

    auto request = GetRootRequestManager()->Get(url);
    request->OnResponse.BindLambda([this, onSuccess = std::move(onSuccess)](ResponseContext& context, JsonDocument& doc)
    {
        if (!JsonArchive::LoadObject(doc[TEXT("endpoints")], driftEndpoints))
        {
            context.error = TEXT("Failed to parse endpoints");
            return;
        }
        onSuccess();
        eventManager->SetEventsUrl(driftEndpoints.eventlogs);
        logForwarder->SetLogsUrl(driftEndpoints.clientlogs);
        onStaticRoutesInitialized.Broadcast();
    });
	request->OnError.BindLambda([this](ResponseContext& context)
	{
	    FString Error;
        context.errorHandled = GetResponseError(context, Error);
	    DRIFT_LOG(Base, Error, TEXT("Failed to get root endpoints. Error: %s"), *Error);

		Reset();
		onPlayerAuthenticated.Broadcast(false, FPlayerAuthenticatedInfo{ EAuthenticationResult::Error_Failed, context.error });
	});
    request->Dispatch();
}


void FDriftBase::InitAuthentication(const FAuthenticationSettings& AuthenticationSettings)
{
    authProvider.Reset();

    /**
     * Note that the "uuid" and "user+pass" auth provider factories are not registered anywhere, they're always
     * created as a fallback, so this is expected to fail for "uuid" and "user+pass" and will be dealt with below.
     * TODO: Make this not so confusing?
     */
    authProvider = MakeShareable(MakeAuthProvider(AuthenticationSettings.CredentialsType).Release());

    if (!authProvider.IsValid())
    {
    	if (AuthenticationSettings.CredentialsType == TEXT("user+pass") && !AuthenticationSettings.Username.IsEmpty() && !AuthenticationSettings.Password.IsEmpty())
    	{
	        authProvider = MakeShareable(GetUserPassAuthProviderFactory(AuthenticationSettings.Username, AuthenticationSettings.Password, AuthenticationSettings.bAutoCreateAccount)->GetAuthProvider().Release());
    	}
    	else
    	{
    		if (AuthenticationSettings.CredentialsType.Compare(TEXT("uuid"), ESearchCase::IgnoreCase) != 0)
    		{
    			DRIFT_LOG(Base, Warning, TEXT("Failed to find or auth provider for '%s', falling back to uuid credentials"), *AuthenticationSettings.CredentialsType);
    		}

    		authProvider = MakeShareable(GetDeviceAuthProviderFactory()->GetAuthProvider().Release());
    	}
    }

    authProvider->InitCredentials([this](bool credentialSuccess)
    {
        if (credentialSuccess)
        {
            AuthenticatePlayer(authProvider.Get());
            authProvider->GetFriends([this](bool success, const TArray<TSharedRef<FOnlineFriend>>& friends)
            {
                if (success)
                {
                    externalFriendIDs.Empty();
                    for (const auto& f : friends)
                    {
                        externalFriendIDs.Add(FString::Printf(TEXT("%s:%s"), *authProvider->GetProviderName(), *f->GetUserId()->ToString()));
                    }
                }
                else
                {
                    DRIFT_LOG(Base, Warning, TEXT("Failed to get friends from OnlineSubsystem"));
                }
            });
        }
        else
        {
            DRIFT_LOG(Base, Error, TEXT("Failed to aquire credentials"));

            Reset();
            onPlayerAuthenticated.Broadcast(false, FPlayerAuthenticatedInfo{ EAuthenticationResult::Error_NoOnlineSubsystemCredentials, TEXT("Failed to aquire credentials") });
        }
    });
}


void FDriftBase::AuthenticatePlayer(IDriftAuthProvider* provider)
{
    FUserPassAuthenticationPayload payload{};
    payload.provider = provider->GetProviderName();
    payload.automatic_account_creation = provider->AllowAutomaticAccountCreation();
    provider->FillProviderDetails([&payload](const FString& key, const FString& value) {
        JsonArchive::AddMember(payload.provider_details, *key, *value);
    });

    /**
     * TODO: Remove legacy username/password logic. Pull this out manually here to
     * avoid having to change the provider interface later.
     */
    if (provider->GetProviderName() == "uuid")
    {
        payload.username = payload.provider_details[TEXT("key")].GetString();
        payload.password = payload.provider_details[TEXT("secret")].GetString();
    }
	else if (provider->GetProviderName() == "user+pass")
	{
		payload.username = payload.provider_details[TEXT("username")].GetString();
		payload.password = payload.provider_details[TEXT("password")].GetString();
	}

    DRIFT_LOG(Base, Verbose, TEXT("Authenticating player with: %s"), *provider->ToString());

    state_ = DriftSessionState::Connecting;
    BroadcastConnectionStateChange(state_);

    auto request = GetRootRequestManager()->Post(driftEndpoints.auth, payload, HttpStatusCodes::Ok);
    request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
    {
        FString bearerToken;
        const auto member = doc.FindField(TEXT("token"));
        if (member.IsString())
        {
            bearerToken = member.GetString();
        }
        if (bearerToken.IsEmpty())
        {
            context.error = TEXT("Session 'token' missing.");
            return;
        }

        DRIFT_LOG(Base, Verbose, TEXT("Got bearer token %s"), *bearerToken);

        TSharedRef<JsonRequestManager> manager = MakeShareable(new JWTRequestManager(bearerToken));
        manager->DefaultErrorHandler.BindRaw(this, &FDriftBase::DefaultErrorHandler);
        manager->DefaultDriftDeprecationMessageHandler.BindRaw(this, &FDriftBase::DriftDeprecationMessageHandler);
        manager->SetApiKey(GetApiKeyHeader());
        manager->SetCache(httpCache_);
        SetGameRequestManager(manager);
        GetUserInfo();
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Authentication failed: %s"), *Error);

        Reset();
        onPlayerAuthenticated.Broadcast(false, FPlayerAuthenticatedInfo{ EAuthenticationResult::Error_Failed, context.error });
    });
    request->Dispatch();
}


void FDriftBase::GetUserInfo()
{
    auto request = GetGameRequestManager()->Get(driftEndpoints.root, HttpStatusCodes::Ok);
    request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
    {
        auto currentUser = doc.FindField(TEXT("current_user"));
        if (!currentUser.IsObject())
        {
            context.error = TEXT("Failed to read user info");
            return;
        }

        const auto userId = currentUser.FindField(TEXT("user_id"));
        if (!userId.IsUint64())
        {
            context.error = TEXT("Failed to read user id");
            return;
        }

        if (userId.GetUint64() == 0)
        {
            context.error = TEXT("User creation failed");
            return;
        }

        const JsonValue PlayerUuid = currentUser.FindField(TEXT("player_uuid"));
        if (PlayerUuid.IsString())
        {
            myPlayer.player_uuid = PlayerUuid.ToString();
        }

        RegisterClient();
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to get user info. Error: %s"), *Error);

        Reset();
        onPlayerAuthenticated.Broadcast(false, FPlayerAuthenticatedInfo{ EAuthenticationResult::Error_Failed, context.error });
    });
    request->Dispatch();
}


void FDriftBase::RegisterClient()
{
    FClientRegistrationPayload payload;
    payload.client_type = TEXT("UE4");
    payload.platform_type = details::GetPlatformName();
    payload.app_guid = GetAppGuid().ToString(EGuidFormats::DigitsWithHyphens);

    JsonArchive::AddMember(payload.platform_info, TEXT("cpu_physical_cores"), FPlatformMisc::NumberOfCores());
    JsonArchive::AddMember(payload.platform_info, TEXT("cpu_logical_cores"), FPlatformMisc::NumberOfCoresIncludingHyperthreads());
    JsonArchive::AddMember(payload.platform_info, TEXT("cpu_vendor"), *FPlatformMisc::GetCPUVendor());
    JsonArchive::AddMember(payload.platform_info, TEXT("cpu_brand"), *FPlatformMisc::GetCPUBrand());
    JsonArchive::AddMember(payload.platform_info, TEXT("gpu_adapter"), *GRHIAdapterName);
    JsonArchive::AddMember(payload.platform_info, TEXT("gpu_vendor_id"), GRHIVendorId);
    JsonArchive::AddMember(payload.platform_info, TEXT("gpu_device_id"), GRHIDeviceId);

    const auto& stats = FPlatformMemory::GetConstants();
    JsonArchive::AddMember(payload.platform_info, TEXT("total_physical_ram"), static_cast<int64>(stats.TotalPhysical));

    JsonArchive::AddMember(payload.platform_info, TEXT("os_version"), *FPlatformMisc::GetOSVersion());

    const auto& I18N = FInternationalization::Get();
    JsonArchive::AddMember(payload.platform_info, TEXT("language"), *I18N.GetCurrentLanguage()->GetName());
    JsonArchive::AddMember(payload.platform_info, TEXT("locale"), *I18N.GetCurrentLocale()->GetName());

#if PLATFORM_IOS
    payload.platform_version = IOSUtility::GetIOSVersion();
#else
    FString osVersion, osSubVersion;
    FPlatformMisc::GetOSVersions(osVersion, osSubVersion);
    payload.platform_version = FString::Printf(TEXT("%s.%s"), *osVersion, *osSubVersion);
#endif

#if PLATFORM_APPLE
    payload.version = AppleUtility::GetBundleShortVersion();
    payload.build = AppleUtility::GetBundleVersion();
#else
    payload.version = gameVersion;
    payload.build = gameBuild;
#endif

    DRIFT_LOG(Base, Verbose, TEXT("Registering client"));

    auto request = GetGameRequestManager()->Post(driftEndpoints.clients, payload);
    request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
    {
        if (!JsonArchive::LoadObject(doc, driftClient))
        {
            context.error = TEXT("Failed to parse client registration response");
            return;
        }
        heartbeatUrl = driftClient.url;
        heartbeatDueInSeconds_ = driftClient.next_heartbeat_seconds;
        TSharedRef<JsonRequestManager> manager = MakeShareable(new JWTRequestManager(driftClient.jwt));
        manager->DefaultErrorHandler.BindRaw(this, &FDriftBase::DefaultErrorHandler);
        manager->DefaultDriftDeprecationMessageHandler.BindRaw(this, &FDriftBase::DriftDeprecationMessageHandler);
        manager->SetApiKey(GetApiKeyHeader());
        manager->SetCache(httpCache_);
        SetGameRequestManager(manager);
        playerCounterManager->SetRequestManager(manager);
        eventManager->SetRequestManager(manager);
        logForwarder->SetRequestManager(manager);
        messageQueue->SetRequestManager(manager);
        partyManager->SetRequestManager(manager);
        matchmaker->SetRequestManager(manager);
        lobbyManager->SetRequestManager(manager);
        matchPlacementManager->SetRequestManager(manager);
        sandboxManager->SetRequestManager(manager);
        partyManager->ConfigureSession(driftClient.player_id, driftEndpoints.party_invites, driftEndpoints.parties);
        GetPlayerEndpoints();
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to register client: %s"), *Error);

        Reset();
        onPlayerAuthenticated.Broadcast(false, FPlayerAuthenticatedInfo{ EAuthenticationResult::Error_Failed, context.error });
    });
    request->Dispatch();
}


void FDriftBase::GetPlayerEndpoints()
{
    DRIFT_LOG(Base, Verbose, TEXT("Fetching player endpoints"));

    auto request = GetGameRequestManager()->Get(driftEndpoints.root);
    request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
    {
        if (!JsonArchive::LoadObject(doc[TEXT("endpoints")], driftEndpoints))
        {
            context.error = TEXT("Failed to parse drift endpoints");
            return;
        }

        if (driftEndpoints.my_player.IsEmpty())
        {
            context.error = TEXT("My player endpoint is empty");
            return;
        }
        matchmaker->ConfigureSession(driftEndpoints, driftClient.player_id);
        lobbyManager->ConfigureSession(driftEndpoints, driftClient.player_id);
        matchPlacementManager->ConfigureSession(driftEndpoints, driftClient.player_id);
        sandboxManager->ConfigureSession(driftEndpoints, driftClient.player_id);
        GetPlayerInfo();
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to fetch drift endpoints: %s"), *Error);

        Disconnect();
        onPlayerAuthenticated.Broadcast(false, FPlayerAuthenticatedInfo{ EAuthenticationResult::Error_Failed, context.error });
    });
    request->Dispatch();
}


void FDriftBase::GetPlayerInfo()
{
    DRIFT_LOG(Base, Verbose, TEXT("Loading player info"));

    auto request = GetGameRequestManager()->Get(driftEndpoints.my_player);
    request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
    {
        if (!JsonArchive::LoadObject(doc, myPlayer))
        {
            context.error = TEXT("Failed to parse my player");
            return;
        }
        playerCounterManager->SetCounterUrl(myPlayer.counter_url);
        messageQueue->SetMessageQueueUrl(myPlayer.messages_url);
        state_ = DriftSessionState::Connected;
        BroadcastConnectionStateChange(state_);

        // TODO: Let user determine if the name should be set or not
        if (authProvider.IsValid())
        {
            const auto currentName = myPlayer.player_name;
            auto ossNickName = authProvider->GetNickname();
            if (!ossNickName.IsEmpty() && currentName != ossNickName)
            {
                SetPlayerName(ossNickName);
            }
        }
        onPlayerAuthenticated.Broadcast(true, FPlayerAuthenticatedInfo{ myPlayer.player_id, myPlayer.player_name });
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to load player info: %s"), *Error);

        Disconnect();
        onPlayerAuthenticated.Broadcast(false, FPlayerAuthenticatedInfo{ EAuthenticationResult::Error_Failed, context.error });
    });
    request->Dispatch();
}


FString FDriftBase::GetPlayerName()
{
    return myPlayer.player_name;
}


int32 FDriftBase::GetPlayerID()
{
    return myPlayer.player_id;
}


FString FDriftBase::GetPlayerUUID()
{
    return myPlayer.player_uuid;
}


void FDriftBase::SetPlayerName(const FString& name)
{
    if (state_ != DriftSessionState::Connected)
    {
        return;
    }

    DRIFT_LOG(Base, Log, TEXT("Setting player name: %s"), *name);
    auto OldName = myPlayer.player_name;
    myPlayer.player_name = name;

    const FChangePlayerNamePayload payload{ name };
    auto request = GetGameRequestManager()->Put(driftEndpoints.my_player, payload);
    request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
    {
        onPlayerNameSet.Broadcast(true, myPlayer.player_name);
    });
    request->OnError.BindLambda([this, OldName](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to set player name: %s"), *Error);

        auto FailedName = myPlayer.player_name;
        myPlayer.player_name = OldName;
        onPlayerNameSet.Broadcast(false, FailedName);
    });
    request->Dispatch();
}


FString FDriftBase::GetAuthProviderName() const
{
    return authProvider.IsValid() ? authProvider->GetProviderName() : TEXT("");
}


void FDriftBase::AddPlayerIdentity(const FString& credentialType, const FDriftAddPlayerIdentityProgressDelegate& progressDelegate)
{
    if (credentialType.Compare(TEXT("uuid"), ESearchCase::IgnoreCase) == 0)
    {
        UE_LOG(LogDriftBase, Error, TEXT("UUID may not be used as a secondary player identity"));

        return;
    }

    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Error, TEXT("You cannot add a new player identity without connecting first"));

        return;
    }

    if (credentialType.Compare(authProvider->GetProviderName(), ESearchCase::IgnoreCase) == 0)
    {
        UE_LOG(LogDriftBase, Error, TEXT("Secondary player identity cannot be the same type as the current one"));

        return;
    }

    if (!progressDelegate.IsBound())
    {
        UE_LOG(LogDriftBase, Error, TEXT("Caller must listen for progress, delegate is not bound"));

        return;
    }

    TSharedPtr<IDriftAuthProvider> provider = MakeShareable(MakeAuthProvider(credentialType).Release());
    if (!provider.IsValid())
    {
        DRIFT_LOG(Base, Error, TEXT("Failed to find an auth provider for credential type %s"), *credentialType);

        return;
    }

    provider->InitCredentials([this, provider, progressDelegate](bool credentialSuccess)
    {
        if (credentialSuccess)
        {
            AddPlayerIdentity(provider, progressDelegate);
        }
        else
        {
            DRIFT_LOG(Base, Warning, TEXT("Failed to aquire credentials from %s"), *provider->GetProviderName());

            progressDelegate.ExecuteIfBound(FDriftAddPlayerIdentityProgress{ EAddPlayerIdentityStatus::Error_FailedToAquireCredentials });
        }
    });
}

void FDriftBase::GetMatches(const FGetDriftMatchesParameters& Parameters, const FDriftGetMatchesDelegate& Delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Error, TEXT("Attempting to get matches without being connected"));
        return;
    }

    FString QueryParams = FString::Printf(TEXT("?use_pagination=true&page=%d&per_page=%d"), Parameters.PageNumber, Parameters.MatchesPerPage);
    if (Parameters.bIncludePlayers)
    {
        QueryParams.Append(TEXT("&include_match_players=true"));
    }
    if (Parameters.PlayerId.IsSet())
    {
        QueryParams.Append(FString::Printf(TEXT("&player_id=%d"), Parameters.PlayerId.GetValue()));
    }
    if (Parameters.GameMode.IsSet())
    {
        QueryParams.Append(FString::Printf(TEXT("&game_mode=%s"), *Parameters.GameMode.GetValue()));
    }
    if (Parameters.MapName.IsSet())
    {
        QueryParams.Append(FString::Printf(TEXT("&map_name=%s"), *Parameters.MapName.GetValue()));
    }
    if (Parameters.DetailsFilter.IsSet())
    {
        FString DetailsFilter = TEXT("{");

        int32 i = 0;
        for (const auto& Elem : Parameters.DetailsFilter.GetValue())
        {
            if (i > 0)
            {
                DetailsFilter.Append(TEXT(","));
            }

            DetailsFilter.Append(FString::Printf(TEXT("\"%s\":\"%s\""), *Elem.Key, *Elem.Value));

            i++;
        }

        DetailsFilter.Append(TEXT("}"));

        QueryParams.Append(FString::Printf(TEXT("&details_filter=%s"), *DetailsFilter));
    }
    if (Parameters.StatisticsFilter.IsSet())
    {
        FString StatisticsFilter = TEXT("{");

        int32 i = 0;
        for (const auto& Elem : Parameters.StatisticsFilter.GetValue())
        {
            if (i > 0)
            {
                StatisticsFilter.Append(TEXT(","));
            }

            StatisticsFilter.Append(FString::Printf(TEXT("\"%s\":\"%s\""), *Elem.Key, *Elem.Value));

            i++;
        }

        StatisticsFilter.Append(TEXT("}"));

        QueryParams.Append(FString::Printf(TEXT("&statistics_filter=%s"), *StatisticsFilter));
    }

    const auto Request = GetGameRequestManager()->Get(driftEndpoints.matches + QueryParams);
    Request->OnResponse.BindLambda([this, bIncludePlayers = Parameters.bIncludePlayers, Delegate](ResponseContext& Context, JsonDocument& Doc)
    {
        FDriftGetMatchesResponse MatchesResponse;
        if (!JsonArchive::LoadObject(Doc, MatchesResponse))
        {
            DRIFT_LOG(Base, Error, TEXT("Failed to parse matches response"));
            Delegate.ExecuteIfBound(false, {});
            return;
        }

        FDriftMatchesResult MatchesResult;
        MatchesResult.TotalMatches = MatchesResponse.total;
        MatchesResult.Pages = MatchesResponse.pages;
        MatchesResult.CurrentPage = MatchesResponse.page;
        MatchesResult.MatchesPerPage = MatchesResponse.per_page;

        for (const auto& ResponseMatch : MatchesResponse.items)
        {
            FDriftMatch Match;
            Match.MatchId = ResponseMatch.match_id;
            Match.ServerId = ResponseMatch.server_id;
            Match.CreateDate = ResponseMatch.create_date;
            Match.StartDate = ResponseMatch.start_date;
            Match.EndDate = ResponseMatch.end_date;
            Match.GameMode = ResponseMatch.game_mode;
            Match.MapName = ResponseMatch.map_name;
            Match.Status = ResponseMatch.status;
            Match.NumPlayers = ResponseMatch.num_players;
            Match.MaxPlayers = ResponseMatch.max_players;
            Match.Details = ResponseMatch.details;
            Match.Statistics = ResponseMatch.match_statistics;
            Match.Url = ResponseMatch.url;
            Match.MatchPlayersUrl = ResponseMatch.matchplayers_url;
            Match.TeamsUrl = ResponseMatch.teams_url;

            if (bIncludePlayers)
            {
                TArray<FDriftMatchPlayer> Players;
                for (const auto& ResponsePlayer : ResponseMatch.players)
                {
                    FDriftMatchPlayer Player;
                    Player.Id = ResponsePlayer.id;
                    Player.MatchId = ResponsePlayer.match_id;
                    Player.PlayerId = ResponsePlayer.player_id;
                    Player.TeamId = ResponsePlayer.team_id;
                    Player.CreateDate = ResponsePlayer.create_date;
                    Player.JoinDate = ResponsePlayer.join_date;
                    Player.LeaveDate = ResponsePlayer.leave_date;
                    Player.PlayerName = ResponsePlayer.player_name;
                    Player.Status = ResponsePlayer.status;
                    Player.NumJoins = ResponsePlayer.num_joins;
                    Player.Seconds = ResponsePlayer.seconds;
                    Player.Details = ResponsePlayer.details;
                    Player.Statistics = ResponsePlayer.statistics;
                    Player.MatchPlayerUrl = ResponsePlayer.matchplayer_url;
                    Player.PlayerUrl = ResponsePlayer.player_url;

                    Players.Emplace(MoveTemp(Player));
                }

                TArray<FDriftMatchTeam> Teams;
                for (const auto& ResponseTeam : ResponseMatch.teams)
                {
                    FDriftMatchTeam Team;
                    Team.MatchId = ResponseTeam.match_id;
                    Team.TeamId = ResponseTeam.team_id;
                    Team.CreateDate = ResponseTeam.create_date;
                    Team.TeamName = ResponseTeam.name;
                    Team.Details = ResponseTeam.details;
                    Team.Statistics = ResponseTeam.statistics;
                    Team.Url = ResponseTeam.url;

                    Teams.Emplace(MoveTemp(Team));
                }

                Match.Players = Players;
                Match.Teams = Teams;
            }

            MatchesResult.Matches.Emplace(MoveTemp(Match));
        }

        Delegate.ExecuteIfBound(true, MatchesResult);
    });
    Request->OnError.BindLambda([this, Delegate](ResponseContext& Context)
    {
        FString Error;
        Context.errorHandled = GetResponseError(Context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to get matches: %s"), *Error);

        Delegate.ExecuteIfBound(false, {});
    });
    Request->Dispatch();
}


void FDriftBase::AddPlayerIdentity(const TSharedPtr<IDriftAuthProvider>& provider, const FDriftAddPlayerIdentityProgressDelegate& progressDelegate)
{
    FUserPassAuthenticationPayload payload{};
    payload.provider = provider->GetProviderName();
    payload.automatic_account_creation = false;
    provider->FillProviderDetails([&payload](const FString& key, const FString& value) {
        JsonArchive::AddMember(payload.provider_details, *key, *value);
    });

    DRIFT_LOG(Base, Verbose, TEXT("Adding player identity: %s"), *provider->ToString());

    auto request = GetRootRequestManager()->Post(driftEndpoints.auth, payload, HttpStatusCodes::Ok);
    request->OnResponse.BindLambda([this, progressDelegate, provider](ResponseContext& context, JsonDocument& doc)
    {
        FString bearerToken;
        auto member = doc.FindField(TEXT("token"));
        if (member.IsString())
        {
            bearerToken = member.GetString();
        }
        if (bearerToken.IsEmpty())
        {
            context.error = TEXT("No authorization token found.");
            return;
        }

        DRIFT_LOG(Base, Verbose, TEXT("Got bearer token %s"), *bearerToken);
        TSharedRef<JsonRequestManager> manager = MakeShareable(new JWTRequestManager(bearerToken));
        manager->DefaultErrorHandler.BindRaw(this, &FDriftBase::DefaultErrorHandler);
        manager->DefaultDriftDeprecationMessageHandler.BindRaw(this, &FDriftBase::DriftDeprecationMessageHandler);
        manager->SetApiKey(GetApiKeyHeader());
        secondaryIdentityRequestManager_ = manager;

        BindUserIdentity(provider->GetNickname(), progressDelegate);
    });
    request->OnError.BindLambda([this, progressDelegate](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to add player identity: %s"), *Error);

        progressDelegate.ExecuteIfBound(FDriftAddPlayerIdentityProgress{ EAddPlayerIdentityStatus::Error_FailedToAuthenticate });
        secondaryIdentityRequestManager_.Reset();
    });
    request->Dispatch();
}


void FDriftBase::BindUserIdentity(const FString& newIdentityName, const FDriftAddPlayerIdentityProgressDelegate& progressDelegate)
{
    /**
     * Get the user details, if any, associated with the new identity.
     */
    auto request = secondaryIdentityRequestManager_->Get(driftEndpoints.root);
    request->OnResponse.BindLambda([this, progressDelegate, newIdentityName](ResponseContext& context, JsonDocument& doc)
    {
        if (state_ != DriftSessionState::Connected || !secondaryIdentityRequestManager_.IsValid())
        {
            // Connection was reset somewhere along the way, no point in continuing
            return;
        }

        FDriftUserInfoResponse userInfo;
        if (doc.HasField(TEXT("current_user")) && JsonArchive::LoadObject(doc[TEXT("current_user")], userInfo))
        {
            if (userInfo.user_id == 0)
            {
                /**
                 * Since we don't auto-create new accounts for additional identities, if the user_id == 0 here,
                 * it means this identity has never been assigned to a user, and we can assign it immediately.
                 */
                DRIFT_LOG(Base, Verbose, TEXT("Identity has no previous user, automatically bind it the the current one"));
                ConnectNewIdentityToCurrentUser(newIdentityName, progressDelegate);
            }
            else if (userInfo.user_id != driftClient.user_id)
            {
                /**
                 * The user_id for the new identity is not the same as the current user's, which means
                 * we need to give the player the choice of:
                 * 1) Do not bind with the new identity, effectively staying with the current player,
                 * dropping the new identity assignment.
                 * 2) Switch the current identity to point to the new identity's user. Switching will
                 * most likely cause the current user to be lost forever as it will no longer have any
                 * identities pointing to it.
                 */
                DRIFT_LOG(Base, Verbose, TEXT("Identity is bound to a different user, player needs to decide what to do"));
                FDriftAddPlayerIdentityProgress progress{ EAddPlayerIdentityStatus::Progress_IdentityAssociatedWithOtherUser };
                progress.localUserPlayerName = myPlayer.player_name;
                progress.newIdentityUserPlayerName = userInfo.player_name;
                progress.newIdentityName = newIdentityName;
                progress.overrideDelegate = FDriftPlayerIdentityOverrideContinuationDelegate::CreateLambda([this, progressDelegate, userInfo](EPlayerIdentityOverrideOption option)
                {
                    if (state_ != DriftSessionState::Connected || !secondaryIdentityRequestManager_.IsValid())
                    {
                        // Connection was reset somewhere along the way, no point in continuing
                        return;
                    }

                    switch (option)
                    {
                    case EPlayerIdentityOverrideOption::AssignIdentityToNewUser:
                        MoveCurrentIdentityToUserOfNewIdentity(userInfo, progressDelegate);
                        break;
                    case EPlayerIdentityOverrideOption::DoNotOverrideExistingUserAssociation:
                        DRIFT_LOG(Base, Verbose, TEXT("User skipped identity association"));
                        progressDelegate.ExecuteIfBound(FDriftAddPlayerIdentityProgress{ EAddPlayerIdentityStatus::Success_NoChange });
                        secondaryIdentityRequestManager_.Reset();
                        break;
                    default:
                        check(false);
                    }
                });
                progressDelegate.ExecuteIfBound(progress);
            }
            else
            {
                /**
                 * The new user identity has the same user_id as the current user, so the identity
                 * assignment was already done in the past. No need to do anything more.
                 */
                DRIFT_LOG(Base, Verbose, TEXT("Identity is already bound to this user, no action taken"));
                progressDelegate.ExecuteIfBound(FDriftAddPlayerIdentityProgress{ EAddPlayerIdentityStatus::Success_NoChange });
                secondaryIdentityRequestManager_.Reset();
            }
        }
        else
        {
            DRIFT_LOG(Base, Error, TEXT("Failed to get current_user details from root using secondary identity."));

            progressDelegate.ExecuteIfBound(FDriftAddPlayerIdentityProgress{ EAddPlayerIdentityStatus::Error_Failed });
            secondaryIdentityRequestManager_.Reset();
        }
    });
    request->OnError.BindLambda([this, progressDelegate](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to get current_user details from root using secondary identity. Error: %s"), *Error);

        progressDelegate.ExecuteIfBound(FDriftAddPlayerIdentityProgress{ EAddPlayerIdentityStatus::Error_FailedToAuthenticate });
        secondaryIdentityRequestManager_.Reset();
    });
    request->Dispatch();
}


void FDriftBase::ConnectNewIdentityToCurrentUser(const FString& newIdentityName, const FDriftAddPlayerIdentityProgressDelegate& progressDelegate)
{
    if (state_ != DriftSessionState::Connected || !secondaryIdentityRequestManager_.IsValid())
    {
        // Connection was reset somewhere along the way, no point in continuing
        return;
    }

    /**
     * Give the user a chance to confirm before making the association.
     */
    FDriftAddPlayerIdentityProgress progress{ EAddPlayerIdentityStatus::Progress_IdentityCanBeAssociatedWithUser };
    progress.localUserPlayerName = myPlayer.player_name;
    progress.newIdentityName = newIdentityName;
    progress.assignDelegate = FDriftPlayerIdentityAssignContinuationDelegate::CreateLambda([this, progressDelegate](EPlayerIdentityAssignOption option)
    {
        if (state_ != DriftSessionState::Connected || !secondaryIdentityRequestManager_.IsValid())
        {
            // Connection was reset, possibly while we waited for player input, no point in continuing
            return;
        }

        switch (option)
        {
        case EPlayerIdentityAssignOption::DoNotAssignIdentityToUser:
            progressDelegate.ExecuteIfBound(FDriftAddPlayerIdentityProgress{ EAddPlayerIdentityStatus::Success_NoChange } );
            secondaryIdentityRequestManager_.Reset();
            break;
        case EPlayerIdentityAssignOption::AssignIdentityToExistingUser:
            {
                DRIFT_LOG(Base, Log, TEXT("Assigning unbound identity with current user"));

                FDriftUserIdentityPayload payload{};
                payload.link_with_user_jti = driftClient.jti;
                payload.link_with_user_id = driftClient.user_id;
                auto request = secondaryIdentityRequestManager_->Post(driftEndpoints.user_identities, payload);
                request->OnResponse.BindLambda([this, progressDelegate](ResponseContext& context, JsonDocument& doc)
                {
                    progressDelegate.ExecuteIfBound(FDriftAddPlayerIdentityProgress{ EAddPlayerIdentityStatus::Success_NewIdentityAddedToExistingUser } );
                    secondaryIdentityRequestManager_.Reset();
                });
                request->OnError.BindLambda([this, progressDelegate](ResponseContext& context)
                {
                    // Could happen if the user already has an association with a different id from the same provider

                    FString Error;
                    context.errorHandled = GetResponseError(context, Error);
                    DRIFT_LOG(Base, Error, TEXT("Failed to link identity with current user. Error: %s"), *Error);

                    // TODO: Check if this is broken or if there's a previous association
                    FDriftAddPlayerIdentityProgress error{ EAddPlayerIdentityStatus::Error_UserAlreadyBoundToSameIdentityType };
                    error.localUserPlayerName = myPlayer.player_name;
                    progressDelegate.ExecuteIfBound(error);
                    secondaryIdentityRequestManager_.Reset();
                });
                request->Dispatch();
            }
            break;
        default:
            check(false);
        }
    });
    progressDelegate.ExecuteIfBound(progress);
}


void FDriftBase::MoveCurrentIdentityToUserOfNewIdentity(const FDriftUserInfoResponse& targetUser, const FDriftAddPlayerIdentityProgressDelegate& progressDelegate)
{
    DRIFT_LOG(Base, Log, TEXT("Re-assigning identity to a different user"));

    FDriftUserIdentityPayload payload;
    payload.link_with_user_jti = targetUser.jti;
    payload.link_with_user_id = targetUser.user_id;
    auto request = GetGameRequestManager()->Post(driftEndpoints.user_identities, payload);
    request->OnResponse.BindLambda([this, progressDelegate](ResponseContext& context, JsonDocument& doc)
    {
        progressDelegate.ExecuteIfBound(FDriftAddPlayerIdentityProgress{ EAddPlayerIdentityStatus::Success_OldIdentityMovedToNewUser });
        secondaryIdentityRequestManager_.Reset();
    });
    request->OnError.BindLambda([this, progressDelegate](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to move identity to new user. Error: %s"), *Error);

        progressDelegate.ExecuteIfBound(FDriftAddPlayerIdentityProgress{ EAddPlayerIdentityStatus::Error_FailedToReAssignOldIdentity });
        secondaryIdentityRequestManager_.Reset();
    });
    request->Dispatch();
}


void FDriftBase::InitServerRootInfo()
{
    auto DriftURL = cli.drift_url;
    if (DriftURL.IsEmpty())
    {
        if (!GConfig->GetString(*settingsSection_, TEXT("DriftUrl"), DriftURL, GGameIni))
        {
            DRIFT_LOG(Base, Error, TEXT("Running in server mode, but no Drift url specified."));

            FDriftResponseInfo2 error;
            error.error_text = TEXT("Running in server mode, but no Drift url specified.");
            state_ = DriftSessionState::Disconnected;
            return;
        }
    }

    const auto Request = GetRootRequestManager()->Get(DriftURL);
    Request->OnResponse.BindLambda([this](ResponseContext& Context, JsonDocument& Doc)
    {
        if (!JsonArchive::LoadObject(Doc[TEXT("endpoints")], driftEndpoints))
        {
            Context.error = TEXT("Failed to parse drift endpoints");
            state_ = DriftSessionState::Disconnected;
            return;
        }

        InitServerAuthentication();
        eventManager->SetEventsUrl(driftEndpoints.eventlogs);
        onStaticRoutesInitialized.Broadcast();
    });
    Request->OnError.BindLambda([this](ResponseContext& Context)
    {
        FString Error;
        Context.errorHandled = GetResponseError(Context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to fetch Drift endpoints. Error: %s"), *Error);

    	Reset();
    });

	Request->SetRetryConfig(FRetryOnServerError{});

    Request->Dispatch();
}


bool FDriftBase::IsPreAuthenticated() const
{
    return !cli.jti.IsEmpty();
}


// TEMP HACK:
static FString SERVER_CREDENTIALS_USERNAME(TEXT("user+pass:$SERVICE$"));
static FString SERVER_CREDENTIALS_PROVIDER(TEXT("user+pass"));


void FDriftBase::InitServerAuthentication()
{
    if (IsPreAuthenticated())
    {
        const TSharedRef<JsonRequestManager> Manager = MakeShareable(new JTIRequestManager(cli.jti));
        Manager->DefaultErrorHandler.BindRaw(this, &FDriftBase::DefaultErrorHandler);
        Manager->DefaultDriftDeprecationMessageHandler.BindRaw(this, &FDriftBase::DriftDeprecationMessageHandler);
        Manager->SetApiKey(GetApiKeyHeader());
        Manager->SetCache(httpCache_);
        SetGameRequestManager(Manager);
        eventManager->SetRequestManager(Manager);
        InitServerRegistration();
        return;
    }

    FString Password;
    FParse::Value(FCommandLine::Get(), TEXT("-driftPass="), Password);

#if WITH_EDITOR
    if (GIsEditor && Password.IsEmpty())
    {
        Password = GEditorServerPassword;
    }
#endif

    if (Password.IsEmpty())
    {
        DRIFT_LOG(Base, Error, TEXT("When not pre-authenticated, credentials must be passed on the command line -driftPass=yyy"));

        Reset();
        return;
    }

    // Post to 'auth' and get token. Use hacky credentials
    FUserPassAuthenticationPayload Payload{};
    Payload.provider = SERVER_CREDENTIALS_PROVIDER;
    Payload.automatic_account_creation = false;
    JsonArchive::AddMember(Payload.provider_details, TEXT("username"), *SERVER_CREDENTIALS_USERNAME);
    JsonArchive::AddMember(Payload.provider_details, TEXT("password"), *Password);

    const auto Request = GetRootRequestManager()->Post(driftEndpoints.auth, Payload, HttpStatusCodes::Ok);

    DRIFT_LOG(Base, Verbose, TEXT("Authenticating server: %s"), *Request->GetAsDebugString(true));

    Request->OnResponse.BindLambda([this](ResponseContext& Context, JsonDocument& Doc)
    {
        serverJTI_ = Doc[TEXT("jti")].GetString();
        serverBearerToken_ = Doc[TEXT("token")].GetString();

        if (serverBearerToken_.IsEmpty())
        {
            Context.error = TEXT("Session 'token' missing.");
            return;
        }
        DRIFT_LOG(Base, Verbose, TEXT("Got bearer token %s"), *serverBearerToken_);

        const TSharedRef<JsonRequestManager> Manager = MakeShareable(new JWTRequestManager(serverBearerToken_));
        Manager->DefaultErrorHandler.BindRaw(this, &FDriftBase::DefaultErrorHandler);
        Manager->DefaultDriftDeprecationMessageHandler.BindRaw(this, &FDriftBase::DriftDeprecationMessageHandler);
        Manager->SetApiKey(GetApiKeyHeader());
        Manager->SetCache(httpCache_);
        SetGameRequestManager(Manager);
        eventManager->SetRequestManager(Manager);

        InitServerRegistration();
    });
    Request->OnError.BindLambda([this](ResponseContext& Context)
    {
        FString Error;
        Context.errorHandled = GetResponseError(Context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to authenticate server. Error: %s"), *Error);

    	Reset();
    });

	Request->SetRetryConfig(FRetryOnServerError{});

    Request->Dispatch();
}


bool FDriftBase::IsPreRegistered() const
{
    return !cli.server_url.IsEmpty();
}


FString FDriftBase::GetInstanceName() const
{
    return FString::Printf(TEXT("%s@%s"), FPlatformProcess::UserName(), FPlatformProcess::ComputerName());
}


FString FDriftBase::GetPublicIP() const
{
    if (cli.public_ip.IsEmpty())
    {
        bool canBindAll;
        return ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, canBindAll)->ToString(false);
    }
    else
    {
        return cli.public_ip;
    }
}


void FDriftBase::InitServerRegistration()
{
    if (IsPreRegistered())
    {
        InitServerInfo();
        return;
    }

    static constexpr int32 DefaultPort = 7777;

    FServerRegistrationPayload Payload;
    Payload.placement = defaultPlacement;
    Payload.instance_name = GetInstanceName();
    Payload.ref = buildReference;
    Payload.public_ip = GetPublicIP();
    Payload.port = !cli.port.IsEmpty() && cli.port.IsNumeric() ? FCString::Atoi(*cli.port) : DefaultPort;
    Payload.command_line = FCommandLine::Get();
    Payload.pid = FPlatformProcess::GetCurrentProcessId();
    Payload.status = TEXT("starting");

    DRIFT_LOG(Base, Log, TEXT("Registering server ip='%s', ref='%s', placement='%s'"), *Payload.public_ip, *Payload.ref, *Payload.placement);

    const auto Request = GetGameRequestManager()->Post(driftEndpoints.servers, Payload);
    Request->OnResponse.BindLambda([this](ResponseContext& Context, JsonDocument& Doc)
    {
    	cli.server_url = Doc[TEXT("url")].GetString();
        InitServerInfo();
    });
    Request->OnError.BindLambda([this](ResponseContext& Context)
    {
        FString Error;
        Context.errorHandled = GetResponseError(Context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to register server. Error: %s"), *Error);

    	Reset();
    });

	Request->SetRetryConfig(FRetryOnServerError{});

    Request->Dispatch();
}


void FDriftBase::InitServerInfo()
{
    DRIFT_LOG(Base, Log, TEXT("Fetching server info"));

    JsonValue Payload{ rapidjson::kObjectType };
    JsonArchive::AddMember(Payload, TEXT("status"), TEXT("initializing"));

    const auto Request = GetGameRequestManager()->Put(cli.server_url, Payload);
    Request->OnResponse.BindLambda([this](ResponseContext& Context, JsonDocument& Doc)
    {
    	FinalizeRegisteringServer();

    });
    Request->OnError.BindLambda([this](ResponseContext& Context)
    {
        FString Error;
        Context.errorHandled = GetResponseError(Context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to initialize server info. Error: %s"), *Error);

    	Reset();
    });

	Request->SetRetryConfig(FRetryOnServerError{});

    Request->Dispatch();
}

void FDriftBase::FinalizeRegisteringServer()
{
	const auto Request = GetGameRequestManager()->Get(cli.server_url);
	Request->OnResponse.BindLambda([this](ResponseContext& Context, JsonDocument& Doc)
	{
		if (!JsonArchive::LoadObject(Doc, drift_server))
		{
			Context.error = TEXT("Failed to parse drift server endpoint response.");
			return;
		}
		heartbeatUrl = drift_server.heartbeat_url;
		heartbeatDueInSeconds_ = -1.0;
		state_ = DriftSessionState::Connected;
		onServerRegistered.Broadcast(true);
		UpdateServer(TEXT("ready"), TEXT(""), FDriftServerStatusUpdatedDelegate{});
	});
	Request->OnError.BindLambda([this](ResponseContext& Context)
	{
	    FString Error;
        Context.errorHandled = GetResponseError(Context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to finalize registering server. Error: %s"), *Error);

        Reset();
	});

	Request->SetRetryConfig(FRetryOnServerError{});

	Request->Dispatch();
}


bool FDriftBase::RegisterServer()
{
    if (state_ == DriftSessionState::Connected)
    {
        onServerRegistered.Broadcast(true);
        return true;
    }

	if (state_ == DriftSessionState::Connecting)
	{
        DRIFT_LOG(Base, Log, TEXT("Ignoring attempt to register server while another attempt is in progress."));

		return true;
	}

	state_ = DriftSessionState::Connecting;

    FParse::Value(FCommandLine::Get(), TEXT("-public_ip="), cli.public_ip);
    FParse::Value(FCommandLine::Get(), TEXT("-drift_url="), cli.drift_url);
    FParse::Value(FCommandLine::Get(), TEXT("-port="), cli.port);
    FParse::Value(FCommandLine::Get(), TEXT("-jti="), cli.jti);

    if (cli.drift_url.IsEmpty())
    {
        GConfig->GetString(*settingsSection_, TEXT("DriftUrl"), cli.drift_url, GGameIni);
    }

    if (cli.drift_url.IsEmpty())
    {
        DRIFT_LOG(Base, Error, TEXT("Running in server mode, but no Drift url specified."));
        // TODO: Error handling
        state_ = DriftSessionState::Disconnected;
        return false;
    }

    InitServerRootInfo();
    return true;
}

void FDriftBase::AddPlayerToMatch(int32 playerID, int32 teamID, const FDriftPlayerAddedDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        /**
         * TODO: Is this the best approach? This should only ever happen in the editor,
         * as in the real game no client can connect before the match has been initialized.
         */
        return;
    }

    FString Payload;

	if (const auto TID = PlayerIdToTeamId.Find(playerID))
	{
		teamID = *TID;
	}

    if (teamID != 0)
    {
        Payload = FString::Printf(TEXT("{\"player_id\": %i, \"team_id\": %i}"), playerID, teamID);
    }
    else
    {
        Payload = FString::Printf(TEXT("{\"player_id\": %i}"), playerID);
    }

    DRIFT_LOG(Base, Log, TEXT("Adding player '%i' to match '%i' in team '%i'"), playerID, match_info.match_id, teamID);

    const auto Request = GetGameRequestManager()->Post(match_info.matchplayers_url, Payload);
    Request->OnResponse.BindLambda([this, playerID, delegate](ResponseContext& Context, JsonDocument& Doc)
    {
        FAddPlayerToMatchResponse AddPlayerToMatchResponse;
        if (!JsonArchive::LoadObject(Doc, AddPlayerToMatchResponse))
        {
            Context.error = TEXT("Failed to parse add match player response");
            return;
        }

        match_players_urls.Emplace(playerID, AddPlayerToMatchResponse.url);

        delegate.ExecuteIfBound(true);
        onPlayerAddedToMatch.Broadcast(true, playerID);
    });
    Request->OnError.BindLambda([this, playerID, teamID, delegate](ResponseContext& Context)
    {
        FString Error;
        Context.errorHandled = GetResponseError(Context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to add player '%d' to match '%d' in team '%d'. Error: %s"), playerID, match_info.match_id, teamID, *Error);

        delegate.ExecuteIfBound(false);
        onPlayerAddedToMatch.Broadcast(false, playerID);
    });
    Request->Dispatch();

    CachePlayerInfo(playerID);
}


void FDriftBase::RemovePlayerFromMatch(int32 playerID, const FDriftPlayerRemovedDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        /**
         * TODO: Is this the best approach? This should only ever happen in the editor,
         * as in the real game no client can connect before the match has been initialized.
         */
        return;
    }

    if (!match_players_urls.Contains(playerID))
    {
        DRIFT_LOG(Base, Error, TEXT("RemovePlayerFromMatch: attempting to remove player_id %d from the match without the player being added to the match, aborted"), playerID);
        delegate.ExecuteIfBound(false);
        return;
    }

    DRIFT_LOG(Base, Log, TEXT("RemovePlayerFromMatch: removing player_id (%d) from match_id (%d)"), playerID, match_info.match_id);

    const auto Url = match_players_urls.FindChecked(playerID);
    const auto Request = GetGameRequestManager()->Delete(Url);
    Request->OnResponse.BindLambda([this, playerID, delegate](ResponseContext& Context, JsonDocument& Doc)
    {
        DRIFT_LOG(Base, Log, TEXT("RemovePlayerFromMatch: player_id (%d) removed from match_id (%d)"), playerID, match_info.match_id);
        match_players_urls.Remove(playerID);

        delegate.ExecuteIfBound(true);
        onPlayerRemovedFromMatch.Broadcast(true, playerID);
    });
    Request->OnError.BindLambda([this, playerID, delegate](ResponseContext& Context)
    {
        FString Error;
        Context.errorHandled = GetResponseError(Context, Error);
        DRIFT_LOG(Base, Error, TEXT("RemovePlayerFromMatch: failed to remove player_id (%d) from match_id (%d) with error (%s)"), playerID, match_info.match_id, *Error);

        delegate.ExecuteIfBound(false);
        onPlayerRemovedFromMatch.Broadcast(false, playerID);
    });
    Request->Dispatch();
}

void FDriftBase::UpdatePlayerInMatch(int32 playerID, const FDriftUpdateMatchPlayerProperties& properties, const FDriftPlayerUpdatedDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("UpdatePlayerInMatch: attempting to update player in match without being connected"));
        delegate.ExecuteIfBound(false);
        return;
    }

    if (!match_players_urls.Contains(playerID))
    {
        DRIFT_LOG(Base, Warning, TEXT("UpdatePlayerInMatch: attempting to update player in match without the player being added to the match"));
        delegate.ExecuteIfBound(false);
        return;
    }

    JsonValue Payload{ rapidjson::kObjectType };
    if (properties.status.IsSet())
    {
        JsonArchive::AddMember(Payload, TEXT("status"), properties.status.GetValue());
    }
    if (properties.team_id.IsSet())
    {
        JsonArchive::AddMember(Payload, TEXT("team_id"), properties.team_id.GetValue());
    }
    if (properties.statistics.IsSet())
    {
        JsonArchive::AddMember(Payload, TEXT("statistics"), properties.statistics.GetValue());
    }
    if (properties.details.IsSet())
    {
        JsonArchive::AddMember(Payload, TEXT("details"), properties.details.GetValue());
    }

    const auto matchID = match_info.match_id;
    DRIFT_LOG(Base, Log, TEXT("UpdatePlayerInMatch: updating player_id (%d) in match_id (%d) with payload (%s)"), playerID, matchID, *Payload.ToString());

    const auto Url = match_players_urls.FindChecked(playerID);
    const auto Request = GetGameRequestManager()->Patch(Url, Payload);
    Request->OnResponse.BindLambda([this, playerID, matchID, delegate](ResponseContext& Context, JsonDocument& Doc)
    {
        DRIFT_LOG(Base, Log, TEXT("UpdatePlayerInMatch: player_id (%d) updated in match_id"), playerID, matchID);
        delegate.ExecuteIfBound(true);
        onPlayerUpdatedInMatch.Broadcast(true, playerID);
    });
    Request->OnError.BindLambda([this, playerID, matchID, delegate](ResponseContext& Context)
    {
        FString Error;
        Context.errorHandled = GetResponseError(Context, Error);
        DRIFT_LOG(Base, Error, TEXT("UpdatePlayerInMatch: failed to update player_id (%d) in match_id (%d) with error (%s)"), playerID, matchID, *Error);

        delegate.ExecuteIfBound(false);
        onPlayerUpdatedInMatch.Broadcast(false, playerID);
    });
    Request->Dispatch();
}


void FDriftBase::AddMatch(const FString& mapName, const FString& gameMode, int32 numTeams, int32 maxPlayers)
{
    InternalAddMatch(mapName, gameMode, maxPlayers, {}, numTeams);
}

void FDriftBase::AddMatch(const FString& mapName, const FString& gameMode, TArray<FString> teamNames, int32 maxPlayers)
{
    InternalAddMatch(mapName, gameMode, maxPlayers, teamNames, {});
}


void FDriftBase::UpdateServer(const FString& status, const FString& reason, const FDriftServerStatusUpdatedDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected || drift_server.url.IsEmpty())
    {
        /**
        * TODO: Is this the best approach? This should only ever happen in the editor,
        * as in the real game no client can connect before the match has been initialized.
        */
        delegate.ExecuteIfBound(false);
        return;
    }

    DRIFT_LOG(Base, Log, TEXT("Updating server status to '%s'"), *status);

    JsonValue payload{ rapidjson::kObjectType };
    JsonArchive::AddMember(payload, TEXT("status"), *status);
    if (!reason.IsEmpty())
    {
        JsonValue details{ rapidjson::kObjectType };
        JsonArchive::AddMember(details, TEXT("status-reason"), *reason);
        JsonArchive::AddMember(payload, TEXT("details"), details);
    }

    auto request = GetGameRequestManager()->Put(drift_server.url, payload);
    request->OnResponse.BindLambda([delegate](ResponseContext& context, JsonDocument& doc)
    {
        delegate.ExecuteIfBound(true);
    });
    request->OnError.BindLambda([this, delegate](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to update server status. Error: %s"), *Error);

        delegate.ExecuteIfBound(false);
    });
    request->Dispatch();
}


void FDriftBase::UpdateMatch(const FString& status, const FString& reason, const FDriftMatchStatusUpdatedDelegate& delegate)
{
	UpdateMatch(status, delegate);
}


void FDriftBase::UpdateMatch(const FString& status, const FDriftMatchStatusUpdatedDelegate& delegate)
{
    FDriftUpdateMatchProperties Properties{};
    Properties.status = status;
    UpdateMatch(Properties, delegate);
}


void FDriftBase::UpdateMatch(const FDriftUpdateMatchProperties& properties, const FDriftMatchStatusUpdatedDelegate& delegate)
{
	if (state_ != DriftSessionState::Connected || match_info.url.IsEmpty())
	{
		/**
		 * TODO: Is this the best approach? This should only ever happen in the editor,
		 * as in the real game no client can connect before the match has been initialized.
		 */
		(void)delegate.ExecuteIfBound(false);
		return;
	}

	JsonValue payload{ rapidjson::kObjectType };
	if (properties.status.IsSet())
	{
		const auto Status = properties.status.GetValue();
		JsonArchive::AddMember(payload, TEXT("status"), Status);
		match_info.status = Status;
	}
	if (properties.mapName.IsSet())
	{
		JsonArchive::AddMember(payload, TEXT("map_name"), properties.mapName.GetValue());
		match_info.map_name = properties.mapName.GetValue();
	}
	if (properties.gameMode.IsSet())
	{
		JsonArchive::AddMember(payload, TEXT("game_mode"), properties.gameMode.GetValue());
		match_info.game_mode = properties.gameMode.GetValue();
	}
    if (properties.uniqueKey.IsSet())
    {
        JsonArchive::AddMember(payload, TEXT("unique_key"), properties.uniqueKey.GetValue());
    	match_info.unique_key = properties.uniqueKey.GetValue();
    }
    if (properties.maxPlayers.IsSet())
	{
		JsonArchive::AddMember(payload, TEXT("max_players"), properties.maxPlayers.GetValue());
    	match_info.max_players = properties.maxPlayers.GetValue();
	}
	if (properties.details.IsSet())
	{
		JsonArchive::AddMember(payload, TEXT("details"), properties.details.GetValue());
		match_info.details = properties.details.GetValue();
	}
	if (properties.match_statistics.IsSet())
	{
		JsonArchive::AddMember(payload, TEXT("match_statistics"), properties.match_statistics.GetValue());
		match_info.match_statistics = properties.match_statistics.GetValue();
	}

    const auto matchID = match_info.match_id;
    DRIFT_LOG(Base, Log, TEXT("UpdateMatch: updating match_id (%d) with payload (%s)"), matchID, *payload.ToString());
	auto request = GetGameRequestManager()->Put(match_info.url, payload);
	request->OnResponse.BindLambda([this, delegate, matchID](ResponseContext& context, JsonDocument& doc)
	{
        DRIFT_LOG(Base, Log, TEXT("UpdateMatch: match_id (%d) updated"), matchID);
		(void)delegate.ExecuteIfBound(true);
		onMatchUpdated.Broadcast(true);
	});
	request->OnError.BindLambda([this, delegate, matchID](ResponseContext& context)
	{
	    FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("UpdateMatch: failed to update match_id (%d) with error (%s)"), matchID, *Error);

		(void)delegate.ExecuteIfBound(false);
		onMatchUpdated.Broadcast(false);
	});    
	request->Dispatch();
}


int32 FDriftBase::GetMatchID() const
{
    return match_info.url.IsEmpty() ? 0 : match_info.match_id;
}

void FDriftBase::CachePlayerInfo(int32 playerID)
{
    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to cache player info without being connected"));

        return;
    }

    const auto entry = serverCounterManagers.Find(playerID);
    if (entry != nullptr && (*entry).IsValid())
    {
        return;
    }

    serverCounterManagers.Add(playerID, MakeUnique<FDriftCounterManager>());

    FString url = driftEndpoints.players;
    internal::UrlHelper::AddUrlOption(url, TEXT("player_id"), FString::Printf(TEXT("%d"), playerID));
    auto request = GetGameRequestManager()->Get(url);
    request->OnResponse.BindLambda([this, playerID](ResponseContext& context, JsonDocument& doc)
    {
        TArray<FDriftPlayerResponse> info;
        if (!JsonArchive::LoadObject(doc, info))
        {
            context.error = TEXT("Failed to parse player info response");
            return;
        }
        if (info.Num() != 1)
        {
            context.error = FString::Printf(TEXT("Expected a single player info, but got %d"), info.Num());
            return;
        }
        auto playerInfo = info[0];
        auto& manager = serverCounterManagers.FindChecked(playerID);
        manager->SetRequestManager(GetGameRequestManager());
        manager->SetCounterUrl(playerInfo.counter_url);
        manager->LoadCounters();

        DRIFT_LOG(Base, Verbose, TEXT("Server cached info for player: %s (%d)"), *playerInfo.player_name, playerInfo.player_id);
    });
    request->OnError.BindLambda([this, playerID](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to cache player info for player id: %d. Error: %s"), playerID, *Error);
    });
    request->Dispatch();
}


void FDriftBase::LoadDriftFriends(const FDriftFriendsListLoadedDelegate& delegate)
{
    if (driftEndpoints.my_friends.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to load friends list before the player session has been initialized"));

        delegate.ExecuteIfBound(false);
        return;
    }

    DRIFT_LOG(Base, Verbose, TEXT("Fetching Drift friends"));

    driftFriends.Reset();

    auto request = GetGameRequestManager()->Get(driftEndpoints.my_friends);
    request->OnResponse.BindLambda([this, delegate = delegate](ResponseContext& context, JsonDocument& doc)
    {
        TArray<FDriftFriendResponse> friends;
        if (!JsonArchive::LoadObject(doc, friends))
        {
            context.error = TEXT("Failed to parse friends response");
            return;
        }

        DRIFT_LOG(Base, Verbose, TEXT("Loaded %d Drift managed friends"), friends.Num());

        if (UE_LOG_ACTIVE(LogDriftBase, VeryVerbose))
        {
            for (const auto& entry : friends)
            {
                DRIFT_LOG(Base, VeryVerbose, TEXT("Friend: %d"), entry.friend_id);
            }
        }

        for (const auto& f : friends)
        {
            driftFriends.Add(f.friend_id, f);
        }

        auto event = MakeEvent(TEXT("drift.friends_loaded"));
        event->Add(TEXT("friends"), driftFriends.Num());
        event->Add(TEXT("request_time"), (context.received - context.sent).GetTotalSeconds());
        AddAnalyticsEvent(MoveTemp(event));
        MakeFriendsGroup(delegate);
    });
    request->OnError.BindLambda([this, delegate = delegate](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to load Drift friends. Error: %s"), *Error);

        delegate.ExecuteIfBound(false);
    });
    request->Dispatch();
}


void FDriftBase::MakeFriendsGroup(const FDriftFriendsListLoadedDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to map third party friends without being connected"));

        return;
    }

    if (driftEndpoints.my_player_groups.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to load third party friends list before the player session has been initialized"));

        delegate.ExecuteIfBound(false);
        return;
    }

    FDriftCreatePlayerGroupPayload payload;
    payload.player_ids.Add(myPlayer.player_id);
    for (const auto& entry : driftFriends)
    {
        payload.player_ids.Add(entry.Value.friend_id);
    }
    for (const auto& id : externalFriendIDs)
    {
        payload.identity_names.Add(id);
    }

#if !UE_BUILD_SHIPPING
    {
        FString fakeFriendsArgument;
        FParse::Value(FCommandLine::Get(), TEXT("-friends="), fakeFriendsArgument, false);
        TArray<FString> fakeFriends;
        fakeFriendsArgument.ParseIntoArray(fakeFriends, TEXT(","));

        auto addAndLog = [this, &payload](int32 id)
        {
            if (id)
            {
                payload.player_ids.Add(id);

                DRIFT_LOG(Base, Warning, TEXT("Adding fake friend ID: %d"), id);
            }
        };

        for (auto fakeFriend : fakeFriends)
        {
            if (fakeFriend.Find(TEXT("-")) != INDEX_NONE)
            {
                FString low, high;
                if (fakeFriend.Split(TEXT("-"), &low, &high))
                {
                    const auto lowID = FCString::Atoi(*low);
                    const auto highID = FCString::Atoi(*high);

                    for (int32 fakeFriendID = lowID; fakeFriendID <= highID; ++fakeFriendID)
                    {
                        addAndLog(fakeFriendID);
                    }
                }
            }
            else
            {
                addAndLog(FCString::Atoi(*fakeFriend));
            }
        }
    }
#endif // !UE_BUILD_SHIPPING

    DRIFT_LOG(Base, Verbose, TEXT("Mapping %d third party friend IDs to Drift counterparts"), payload.identity_names.Num());

    const auto url = driftEndpoints.my_player_groups.Replace(TEXT("{group_name}"), TEXT("friends"));
    auto request = GetGameRequestManager()->Put(url, payload);
    request->OnResponse.BindLambda([this, delegate = delegate](ResponseContext& context, JsonDocument& doc)
    {
        if (!JsonArchive::LoadObject(doc, userIdentities))
        {
            context.error = TEXT("Failed to parse player identity response");
            return;
        }

        DRIFT_LOG(Base, Verbose, TEXT("Created player group 'friends' with %d of %d mappable IDs"), userIdentities.players.Num(), externalFriendIDs.Num());

        if (UE_LOG_ACTIVE(LogDriftBase, VeryVerbose))
        {
            for (const auto& entry : userIdentities.players)
            {
                DRIFT_LOG(Base, VeryVerbose, TEXT("Friend: %d - %s [%s]"), entry.player_id, *entry.player_name, *entry.identity_name);
            }
        }

        auto event = MakeEvent(TEXT("drift.player_group_created"));
        event->Add(TEXT("external_ids"), userIdentities.players.Num());
        event->Add(TEXT("mapped_ids"), externalFriendIDs.Num());
        event->Add(TEXT("friend_ids"), driftFriends.Num());
        event->Add(TEXT("group_name"), TEXT("friends"));
        event->Add(TEXT("request_time"), (context.received - context.sent).GetTotalSeconds());
        AddAnalyticsEvent(MoveTemp(event));

        CacheFriendInfos([this, delegate = delegate](bool success)
        {
            userIdentitiesLoaded = success;
            delegate.ExecuteIfBound(success);
        });
    });
    request->OnError.BindLambda([this, delegate = delegate](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Warning, TEXT("Failed to create player group 'friends': %s"), *Error);

        delegate.ExecuteIfBound(false);
    });
    request->Dispatch();
}


void FDriftBase::CacheFriendInfos(const TFunction<void(bool)>& delegate)
{
    auto url = driftEndpoints.players;
    internal::UrlHelper::AddUrlOption(url, TEXT("player_group"), TEXT("friends"));
    auto request = GetGameRequestManager()->Get(url);
    request->OnResponse.BindLambda([this, delegate = delegate](ResponseContext& context, JsonDocument& doc)
    {
        TArray<FDriftPlayerResponse> infos;
        if (!JsonArchive::LoadObject(doc, infos))
        {
            context.error = TEXT("Failed to parse friend info response");
            return;
        }
        friendInfos.Empty(infos.Num());
        for (auto& info : infos)
        {
            friendInfos.Add(info.player_id, MoveTemp(info));
        }
        delegate(true);
    });
    request->OnError.BindLambda([this, delegate = delegate](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to cache friend infos: %s"), *Error);

        delegate(false);
    });
    request->Dispatch();
}


void FDriftBase::UpdateFriendOnlineInfos()
{
    if (driftEndpoints.players.IsEmpty())
    {
        return;
    }

    auto url = driftEndpoints.players;
    internal::UrlHelper::AddUrlOption(url, TEXT("player_group"), TEXT("friends"));
    internal::UrlHelper::AddUrlOption(url, TEXT("key"), TEXT("is_online"));
    auto request = GetGameRequestManager()->Get(url);
    request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
    {
        TArray<FDriftPlayerUpdateResponse> infos;
        if (!JsonArchive::LoadObject(doc, infos))
        {
            context.error = TEXT("Failed to parse friend info update response");
            return;
        }
        for (const auto& info : infos)
        {
            DRIFT_LOG(Base, VeryVerbose, TEXT("Got online status for %d: %d"), info.player_id, info.is_online ? 1 : 0);

            if (auto friendInfo = friendInfos.Find(info.player_id))
            {
                auto oldOnlineStatus = friendInfo->is_online;
                if (oldOnlineStatus != info.is_online)
                {
                    friendInfo->is_online = info.is_online;
                    onFriendPresenceChanged.Broadcast(info.player_id, info.is_online ? EDriftPresence::Online : EDriftPresence::Offline);
                }
            }
            else
            {
                DRIFT_LOG(Base, Warning, TEXT("Got an update for a friend that was not cached locally: %d"), info.player_id);
            }
        }
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to update friend online infos: %s"), *Error);
    });
    request->Dispatch();
}


const FDriftPlayerResponse* FDriftBase::GetFriendInfo(int32 playerID) const
{
    return friendInfos.Find(playerID);
}

void FDriftBase::InternalAddMatch(const FString& mapName, const FString& gameMode, int32 maxPlayers, TOptional<TArray<FString>> teamNames, TOptional<int32> numTeams)
{
    if (state_ != DriftSessionState::Connected)
    {
        /**
         * TODO: Is this the best approach? This should only ever happen in the editor,
         * as in the real game no client can connect before the match has been initialized.
         */
        DRIFT_LOG(Base, Warning, TEXT("Attempted to add match while not connected. Internal state is %d"), static_cast<uint8>(state_));
        onMatchAdded.Broadcast(false);
        return;
    }

    FMatchesPayload payload;
    payload.server_id = drift_server.server_id;
    payload.num_players = 0;
    payload.max_players = maxPlayers;
    payload.map_name = mapName;
    payload.game_mode = gameMode;
    payload.status = TEXT("idle");

    if (numTeams.IsSet())
    {
        payload.num_teams = numTeams.GetValue();
    }

    if (teamNames.IsSet())
    {
        payload.team_names = teamNames.GetValue();
    }

    DRIFT_LOG(Base, Log,
        TEXT("Adding match to server: '%d' map: '%s' mode: '%s' players: '%d' %s %s"),
        drift_server.server_id, *mapName, *gameMode, maxPlayers,
        numTeams.IsSet() ? *FString::Printf(TEXT("num_teams: '%d'"), numTeams.GetValue()) : TEXT(""),
        teamNames.IsSet() ? *FString::Printf(TEXT("teams: '%s'"), *FString::Join(teamNames.GetValue(), TEXT(", "))) : TEXT("")
    );

    const auto request = GetGameRequestManager()->Post(driftEndpoints.matches, payload);
    request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
    {
        FAddMatchResponse match;
        if (!JsonArchive::LoadObject(doc, match))
        {
            context.error = TEXT("Failed to parse add match response.");
            onMatchAdded.Broadcast(false);
            return;
        }

        DRIFT_LOG(Base, VeryVerbose, TEXT("%s"), *JsonArchive::ToString(doc));

        const auto match_request = GetGameRequestManager()->Get(match.url);
        match_request->OnResponse.BindLambda([this](ResponseContext& matchContext, JsonDocument& matchDoc)
        {
            if (!JsonArchive::LoadObject(matchDoc, match_info))
            {
                matchContext.error = TEXT("Failed to parse match info response.");
                onMatchAdded.Broadcast(false);
                return;
            }

            DRIFT_LOG(Base, VeryVerbose, TEXT("%s"), *JsonArchive::ToString(matchDoc));

            DRIFT_LOG(Base, Log, TEXT("Match '%i' added to server '%i'"), match_info.match_id, match_info.server_id);
            onMatchAdded.Broadcast(true);
        });
        match_request->Dispatch();
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        FString Error;
        context.errorHandled = GetResponseError(context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to add match: %s"), *Error);

        onMatchAdded.Broadcast(false);
    });
    request->Dispatch();
}


void FDriftBase::ModifyPlayerCounter(int32 playerID, const FString& counterName, float value, bool absolute)
{
    const auto counterManager = serverCounterManagers.Find(playerID);
    if (counterManager != nullptr && (*counterManager).IsValid())
    {
        (*counterManager).Get()->AddCount(counterName, value, absolute);
    }
    else
    {
        DRIFT_LOG(Base, Warning, TEXT("Failed to find counters for player ID %d. Please make sure AddPlayerToMatch() has been called first."), playerID);
    }
}


bool FDriftBase::GetPlayerCounter(int32 playerID, const FString& counterName, float& value)
{
    const auto counterManager = serverCounterManagers.Find(playerID);
    if (counterManager != nullptr && (*counterManager).IsValid())
    {
        return (*counterManager).Get()->GetCount(counterName, value);
    }
    else
    {
        DRIFT_LOG(Base, Warning, TEXT("Failed to find counters for player ID %d. Please make sure AddPlayerToMatch() has been called first."), playerID);
    }

    return false;
}


TArray<FDriftMatchTeam> FDriftBase::GetMatchTeams() const
{
	TArray<FDriftMatchTeam> Teams;

	for (const auto& Team : match_info.teams)
	{
		Teams.Emplace(FDriftMatchTeam
		{
		    Team.team_id,
		    Team.match_id,
		    Team.create_date,
		    Team.name,
		    Team.details,
		    Team.statistics,
		    Team.url,
		});
	}

	return Teams;
}


FString FDriftBase::GetApiKeyHeader() const
{
    if (!versionedApiKey.IsEmpty())
    {
        return versionedApiKey;
    }
    return FString::Printf(TEXT("%s:%s"), *apiKey, IsRunningAsServer() ? TEXT("service") : *gameVersion);
}


void FDriftBase::DefaultErrorHandler(ResponseContext& context)
{
    // TODO: make this whole thing data-driven
    const auto responseCode = context.responseCode;
    if (responseCode >= static_cast<int32>(HttpStatusCodes::FirstClientError) && responseCode <= static_cast<int32>(HttpStatusCodes::LastClientError))
    {
        auto contentType = context.response->GetHeader(TEXT("Content-Type"));
        if (contentType.StartsWith(TEXT("application/json")))
        {
            ClientUpgradeResponse message;
            if (JsonUtils::ParseResponse(context.response, message) && message.action == TEXT("upgrade_client"))
            {
                context.errorHandled = true;
                // Reset instead of disconnect as we know all future calls will fail
                Reset();
                DRIFT_LOG(Base, Error, TEXT("Client needs updating. Message '%s', upgrade_url '%s'"), *message.message, *message.upgrade_url);
                OnGameVersionMismatch().Broadcast(message.message, message.upgrade_url);
                return;
            }

            GenericRequestErrorResponse response;
            if (JsonUtils::ParseResponse(context.response, response))
            {
                if (response.GetErrorCode() == TEXT("client_session_terminated"))
                {
                    const auto reason = response.GetErrorReason();
                    if (reason == TEXT("usurped"))
                    {
                        // User logged in on another device with the same credentials
                        state_ = DriftSessionState::Usurped;
                        BroadcastConnectionStateChange(state_);
                    }
                    else if (reason == TEXT("timeout"))
                    {
                        // User session timed out, requires regular heartbeats
                        state_ = DriftSessionState::Timedout;
                        BroadcastConnectionStateChange(state_);
                        context.errorHandled = true;
                        Reset();
                        return;
                    }
                    // Some other reason
                    context.errorHandled = true;
                    Disconnect();
                }
                else if (response.GetErrorCode() == TEXT("api_key_missing"))
                {
                    context.error = response.GetErrorDescription();
                }
            }
        }
    }
    else if (responseCode >= static_cast<int32>(HttpStatusCodes::FirstServerError) && responseCode <= static_cast<int32>(HttpStatusCodes::LastServerError))
    {
        GenericRequestErrorResponse response;
        if (JsonUtils::ParseResponse(context.response, response))
        {
            // Server error, client will have to try again later, but we can't do much about it right now
        }
    }
    else
    {
        // No repsonse at all, and no error code
    }
}


void FDriftBase::DriftDeprecationMessageHandler(const FString& deprecations)
{
    if (deprecations == previousDeprecationHeader_)
    {
        return;
    }

    previousDeprecationHeader_ = deprecations;
    FString deprecation;
    FString remaining = deprecations;
    while (remaining.Split(TEXT(","), &deprecation, &remaining))
    {
        ParseDeprecation(deprecation);
    }
    ParseDeprecation(remaining);
}


void FDriftBase::ParseDeprecation(const FString& deprecation)
{
    FString feature;
    FString deprecationDateString;
    if (deprecation.Split(TEXT("@"), &feature, &deprecationDateString))
    {
        FDateTime deprecationDate;
        if (FDateTime::ParseIso8601(*deprecationDateString, deprecationDate))
        {
            auto& entry = deprecations_.FindOrAdd(feature);
            if (entry == deprecationDate)
            {
                return;
            }
            entry = deprecationDate;

            DRIFT_LOG(Base, Log, TEXT("Got new feature deprecation: %s by %s"), *feature, *entry.ToString());

            onDeprecation.Broadcast(feature, deprecationDate);
        }
        else
        {
            DRIFT_LOG(Base, Warning, TEXT("Failed to parse deprecation date for feature: %s"), *feature);
        }
    }
    else
    {
        DRIFT_LOG(Base, Warning, TEXT("Failed to locate deprecation date for feature: %s"), *feature);
    }
}


void FDriftBase::LoadPlayerAvatarUrl(const FDriftLoadPlayerAvatarUrlDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to get avatar url without being connected"));
        delegate.ExecuteIfBound(TEXT(""));
        return;
    }

    check(authProvider.IsValid());

    authProvider->GetAvatarUrl([delegate](const FString& avatarUrl)
    {
        delegate.ExecuteIfBound(avatarUrl);
    });
}

void FDriftBase::GetUserIdentitiesByPlayerId(int32 PlayerId, const FDriftGetUserIdentitiesDelegate& delegate)
{
    if (driftEndpoints.user_identities.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to get user identities with no endpoint"));

        delegate.ExecuteIfBound(false, {});
        return;
    }

    DRIFT_LOG(Base, Log, TEXT("Getting get user identities for player id: '%d'"), PlayerId);

    auto url = driftEndpoints.user_identities + TEXT("?player_id=") + FString::FromInt(PlayerId);

    InternalGetUserIdentities(url, delegate);
}

void FDriftBase::GetUserIdentitiesByNames(const TArray<FString>& namesArray,
    const FDriftGetUserIdentitiesDelegate& delegate)
{
    if (namesArray.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to get user identities with empty names array"));
        delegate.ExecuteIfBound(false, {});
        return;
    }

    if (driftEndpoints.user_identities.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to get user identities with no endpoint"));

        delegate.ExecuteIfBound(false, {});
        return;
    }

    DRIFT_LOG(Base, Log, TEXT("Getting get user identities for names: '%s'"), *FString::Join(namesArray, TEXT(", ")));

    auto url = driftEndpoints.user_identities + TEXT("?name=") + FString::Join(namesArray, TEXT("&name="));

    InternalGetUserIdentities(url, delegate);
}

void FDriftBase::GetUserIdentitiesByName(const FString& name, const FDriftGetUserIdentitiesDelegate& delegate)
{
    TArray<FString> namesArray;
    namesArray.Add(name);
    GetUserIdentitiesByNames(namesArray, delegate);
}

void FDriftBase::InternalGetUserIdentities(const FString& url, const FDriftGetUserIdentitiesDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to get user identities without being connected"));
        delegate.ExecuteIfBound(false, {});
        return;
    }

    const auto Request = GetGameRequestManager()->Get(url);
    Request->OnResponse.BindLambda([this, delegate](ResponseContext& Context, JsonDocument& Doc)
    {
        TArray<FDriftUserIdentity> Response;
        if (!JsonArchive::LoadObject(Doc, Response))
        {
            Context.error = TEXT("Failed to parse user identities response");
            return;
        }

        delegate.ExecuteIfBound(true, Response);
    });
    Request->OnError.BindLambda([this, url, delegate](ResponseContext& Context)
    {
        FString Error;
        Context.errorHandled = GetResponseError(Context, Error);
        DRIFT_LOG(Base, Error, TEXT("Failed to get user identites from url: %s. Error: '%s'"), *url, *Error);

        delegate.ExecuteIfBound(false, {});
    });
    Request->Dispatch();
}


bool FDriftBase::DoSendFriendMessage(int32 FriendId, JsonValue&& MessagePayload)
{
	if (state_ != DriftSessionState::Connected)
	{
		DRIFT_LOG(Base, Warning, TEXT("DoSendFriendMessage: attempting to send friend message without being connected"));

		return false;
	}

	if (driftEndpoints.my_friends.IsEmpty())
	{
		DRIFT_LOG(Base, Warning, TEXT("DoSendFriendMessage: attempting to send friend message before the player session has been initialized"));

		return false;
	}

	if (auto friendInfo = friendInfos.Find(FriendId))
	{
		const auto messageUrlTemplate = friendInfo->messagequeue_url;
		messageQueue->SendMessage(messageUrlTemplate, FriendMessage, MoveTemp(MessagePayload));

		DRIFT_LOG(Base, Verbose, TEXT("DoSendFriendMessage: message sent to friend Id %d"), FriendId);

		return true;
	}
	else
	{
		DRIFT_LOG(Base, Warning, TEXT("DoSendFriendMessage: friend Id is unknown or invalid: %d"), FriendId);
	}

	return false;
}

bool FDriftBase::GetResponseError(const ResponseContext& Context, FString& Error)
{
    if (!Context.error.IsEmpty())
    {
        Error = Context.error;
        return true;
    }

    Error = TEXT("Unknown error");

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


bool FDriftBase::SendFriendMessage(int32 FriendId, const FString& Message)
{
	JsonValue message { rapidjson::kObjectType };
	JsonArchive::AddMember(message, TEXT("message"), *Message);
	return DoSendFriendMessage(FriendId, MoveTemp(message));
}


bool FDriftBase::SendFriendMessage(int32 FriendId, class JsonValue&& Message)
{
	JsonValue message { rapidjson::kObjectType };
	JsonArchive::AddMember(message, TEXT("message"), Message);
	return DoSendFriendMessage(FriendId, MoveTemp(message));
}


void FDriftBase::HandleFriendMessage(const FMessageQueueEntry& message)
{
	const auto messageField = message.payload.FindField(TEXT("message"));
	if (messageField.IsString())
	{
		const FString messageString = messageField.GetString();
		UE_LOG(LogDriftMessages, Verbose, TEXT("HandleFriendMessage: received text message from friend Id %d: \"%s\""), message.sender_id, *messageString);

		onReceivedTextMessage.Broadcast({ EMessageType::Text, message.sender_id, message.message_number, message.message_id, message.timestamp, message.expires, messageString });
	}
	else if (messageField.IsObject())
	{
		UE_LOG(LogDriftMessages, Verbose, TEXT("HandleFriendMessage: received json message from friend Id %d"), message.sender_id);
		onReceivedTextMessage.Broadcast({ EMessageType::Json, message.sender_id, message.message_number, message.message_id, message.timestamp, message.expires, messageField.ToString() });
	}
	else
	{
		UE_LOG(LogDriftMessages, Error, TEXT("HandleFriendMessage: friend message contains no message field"));
	}
}

TSharedPtr<IDriftMatchmaker> FDriftBase::GetMatchmaker()
{
    return matchmaker;
}

TSharedPtr<IDriftLobbyManager> FDriftBase::GetLobbyManager()
{
	return lobbyManager;
}

TSharedPtr<IDriftMatchPlacementManager> FDriftBase::GetMatchPlacementManager()
{
    return matchPlacementManager;
}

TSharedPtr<IDriftSandboxManager> FDriftBase::GetSandboxManager()
{
    return sandboxManager;
}

TSharedPtr<IDriftMessageQueue> FDriftBase::GetMessageQueue() const
{
    return messageQueue;
}

TSharedPtr<IDriftPartyManager> FDriftBase::GetPartyManager()
{
	return partyManager;
}


void FDriftBase::SetForwardedLogLevel(ELogVerbosity::Type Level)
{
    if (logForwarder)
    {
        logForwarder->SetForwardedLogLevel(Level);
    }
}

#undef LOCTEXT_NAMESPACE
