/**
* This file is part of the Drift Unreal Engine Integration.
*
* Copyright (C) 2016-2017 Directive Games Limited. All Rights Reserved.
*
* Licensed under the MIT License (the "License");
*
* You may not use this file except in compliance with the License.
* You may obtain a copy of the license in the LICENSE file found at the top
* level directory of this module, and at https://mit-license.org/
*/

#include "DriftPrivatePCH.h"
#include "DriftBase.h"
#include "JsonRequestManager.h"
#include "JsonArchive.h"
#include "JsonUtils.h"
#include "DriftAPI.h"
#include "JTIRequestManager.h"
#include "ErrorResponse.h"
#include "DriftCredentialsFactory.h"
#include "Details/PlatformName.h"
#include "Details/UrlHelper.h"
#include "IDriftAuthProviderFactory.h"
#include "IDriftAuthProvider.h"
#include "IErrorReporter.h"
#include "Auth/DriftUuidAuthProviderFactory.h"

#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Runtime/Analytics/Analytics/Public/AnalyticsEventAttribute.h"
#include "Features/IModularFeatures.h"
#include "OnlineSubsystemTypes.h"

#if PLATFORM_APPLE
#include "Apple/AppleUtility.h"
#endif


#define LOCTEXT_NAMESPACE "Drift"


DEFINE_LOG_CATEGORY(LogDriftBase);
DEFINE_LOG_CATEGORY(LogDriftCounterEngine);
DEFINE_LOG_CATEGORY(LogDriftCounterUser);


static const float UPDATE_FRIENDS_INTERVAL = 3.0f;


const TCHAR* settingsSection = TEXT("/Script/DriftEditor.DriftProjectSettings");


FDriftBase::FDriftBase(TSharedPtr<IHttpCache> cache, const FName& instanceName, int32 instanceIndex)
: instanceName_(instanceName)
, instanceDisplayName_(instanceName_ == FName(TEXT("DefaultInstance")) ? TEXT("") : FString::Printf(TEXT("[%s] "), *instanceName_.ToString()))
, instanceIndex_(instanceIndex)
, state_(DriftSessionState::Undefined)
, rootRequestManager_(MakeShareable(new JsonRequestManager()))
, httpCache_(cache)
{
    GetRootRequestManager()->DefaultErrorHandler.BindRaw(this, &FDriftBase::DefaultErrorHandler);

    GConfig->GetString(settingsSection, TEXT("ApiKey"), apiKey, GGameIni);
    GConfig->GetString(settingsSection, TEXT("ProjectName"), projectName, GGameIni);
    GConfig->GetString(settingsSection, TEXT("GameVersion"), gameVersion, GGameIni);
    GConfig->GetString(settingsSection, TEXT("GameBuild"), gameBuild, GGameIni);
    GConfig->GetString(settingsSection, TEXT("Environment"), environment, GGameIni);
    GConfig->GetString(settingsSection, TEXT("StaticDataReference"), staticDataReference, GGameIni);

    if (apiKey.IsEmpty())
    {
        IErrorReporter::Get()->AddError(L"LogDriftBase", TEXT("No API key found. Please fill out Project Settings->Drift"));
    }

    deviceAuthProviderFactory = MakeUnique<FDriftUuidAuthProviderFactory>(instanceIndex_, projectName);

    ConfigurePlacement();
    ConfigureBuildReference();

    rootRequestManager_->SetApiKey(GetApiKeyHeader());
    rootRequestManager_->SetCache(httpCache_);

    CreatePlayerCounterManager();
    CreateEventManager();
    CreateLogForwarder();
    CreateMessageQueue();
    
    DRIFT_LOG(Base, Verbose, TEXT("Drift instance %s created"), *instanceName_.ToString());
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
    eventManager = MakeUnique<FDriftEventManager>();
}


void FDriftBase::CreateLogForwarder()
{
    logForwarder = MakeUnique<FLogForwarder>();
}


void FDriftBase::CreateMessageQueue()
{
    messageQueue = MakeUnique<FDriftMessageQueue>();

    messageQueue->OnMessageQueueMessage(TEXT("matchqueue")).AddRaw(this, &FDriftBase::HandleMatchQueueMessage);
}


void FDriftBase::ConfigurePlacement()
{
    if (!FParse::Value(FCommandLine::Get(), TEXT("-placement="), defaultPlacement))
    {
        if (!GConfig->GetString(settingsSection, TEXT("Placement"), defaultPlacement, GGameIni))
        {
            bool canBindAll;
            auto address = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, canBindAll);
            uint32 ip;
            address->GetIp(ip);
            defaultPlacement = FString::Printf(TEXT("LAN %d.%d"), (ip & 0xff000000) >> 24, (ip & 0x00ff0000) >> 16);
        }
    }
}


void FDriftBase::ConfigureBuildReference()
{
    if (!FParse::Value(FCommandLine::Get(), TEXT("-ref="), buildReference))
    {
        if (!GConfig->GetString(settingsSection, TEXT("BuildReference"), buildReference, GGameIni))
        {
            buildReference = FString::Printf(TEXT("user/%s"), FPlatformProcess::UserName());
        }
    }
}


FDriftBase::~FDriftBase()
{
    DRIFT_LOG(Base, Verbose, TEXT("Drift instance %s destroyed"), *instanceName_.ToString());
}


void FDriftBase::Tick(float deltaTime)
{
    TickHeartbeat(deltaTime);
    TickMatchInvites();
    TickFriendUpdates(deltaTime);
}


TSharedPtr<JsonRequestManager> FDriftBase::GetRootRequestManager()
{
    return rootRequestManager_;
}


TSharedPtr<JsonRequestManager> FDriftBase::GetGameRequestManager()
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

    heartbeatDueInSeconds -= deltaTime;
    if (heartbeatDueInSeconds > 0.0)
    {
        return;
    }
    heartbeatDueInSeconds = FLT_MAX; // Prevent re-entrancy

    DRIFT_LOG(Base, Verbose, TEXT("[%s] Drift heartbeat..."), *FDateTime::UtcNow().ToString());

    auto request = GetGameRequestManager()->Put(hearbeatUrl, FString());
    request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
    {
        heartbeatDueInSeconds = doc[TEXT("next_heartbeat_seconds")].GetInt();

        DRIFT_LOG(Base, Verbose, TEXT("[%s] Drift heartbeat done. Next one in %.1f secs"), *FDateTime::UtcNow().ToString(), heartbeatDueInSeconds);
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        if (context.response.IsValid())
        {
            GenericRequestErrorResponse response;
            if (JsonUtils::ParseResponse(context.response, response))
            {
                if (context.responseCode == static_cast<int32>(HttpStatusCodes::NotFound) && response.GetErrorCode() == TEXT("user_error"))
                {
                    // Hearbeat timed out
                    DRIFT_LOG(Base, Warning, TEXT("Failed to heartbeat\n%s"), *GetDebugText(context.response));

                    state_ = DriftSessionState::Timedout;
                    BroadcastConnectionStateChange(state_);
                    context.errorHandled = true;
                    Disconnect();
                    return;
                }
                // Some other reason
                context.errorHandled = true;
                Disconnect();
            }
        }
        else
        {
            // No response, unknown error
            context.errorHandled = true;
            Disconnect();
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
    
    eventManager->FlushEvents();
}


void FDriftBase::Shutdown()
{
    if (state_ == DriftSessionState::Connected)
    {
        Disconnect();
    }
}


void FDriftBase::Disconnect()
{
    auto oldState = state_;
    if (state_ != DriftSessionState::Connected && state_ != DriftSessionState::Usurped && state_ != DriftSessionState::Timedout)
    {
        DRIFT_LOG(Base, Warning, TEXT("Ignoring attempt to disconnect while not connected."));
        
        return;
    }

    if (state_ == DriftSessionState::Connected && !drift_server.url.IsEmpty())
    {
        UpdateServer(TEXT("quit"), TEXT(""), FDriftServerStatusUpdatedDelegate{});
    }

    state_ = DriftSessionState::Disconnecting;
    BroadcastConnectionStateChange(state_);

    auto finalizeDisconnect = [this]()
    {
        state_ = DriftSessionState::Disconnected;
        Reset();
        onPlayerDisconnected.Broadcast();
        BroadcastConnectionStateChange(state_);
    };

    /**
     * If we were connected, then this is most likely the user himself disconnecting,
     * and we should attempt to clear the client session from the backend.
     */
    if (oldState == DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Verbose, TEXT("Disconnecting"));

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
            request->OnError.BindLambda([finalizeDisconnect](ResponseContext& context)
            {
                context.errorHandled = true;

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
    DRIFT_LOG(Base, Verbose, TEXT("Resetting all internal state"));
    
    authenticatedRequestManager.Reset();
    secondaryIdentityRequestManager_.Reset();

    driftEndpoints = FDriftEndpointsResponse{};
    driftClient = FClientRegistrationResponse{};
    myPlayer = FDriftPlayerResponse{};
    drift_server = FServerRegistrationResponse{};

    matchQueue = FMatchQueueResponse{};
    matchQueueState = EMatchQueueState::Idle;
    
    CreatePlayerCounterManager();
    CreateEventManager();
    CreateLogForwarder();
    CreateMessageQueue();
    
    hearbeatUrl.Empty();

    userIdentities = FDriftCreatePlayerGroupResponse{};
    
    countersLoaded = false;
    playerGameStateInfosLoaded = false;
    userIdentitiesLoaded = false;
    shouldUpdateFriends = false;
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
            context.error = L"Failed to parse static data response";
            return;
        }
        
        if (static_data.static_data_urls.Num() == 0)
        {
            context.error = L"No static data entries found";
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

        auto loader = [this, commit, pin, index_sent, index_received, sync](const FString& data_url, const FString& data_name, const FString& cdn_name)
        {
            auto data_request = GetRootRequestManager()->Get(data_url + data_name);
            data_request->OnRequestProgress().BindLambda([this, data_name, sync](FHttpRequestPtr req, int32 bytesWritten, int32 bytesRead)
            {
                if (!sync->succeeded)
                {
                    auto lastBytesRead = sync->bytesRead;
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
                    onStaticDataLoaded.Broadcast(false, TEXT(""));
                    data_context.errorHandled = true;
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
        onStaticDataLoaded.Broadcast(false, TEXT(""));
        context.errorHandled = true;
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


void FDriftBase::LoadPlayerGameStateImpl(const FString& name, const FDriftGameStateLoadedDelegate& delegate)
{
    DRIFT_LOG(Base, Log, TEXT("Getting player game state \"%s\""), *name);

    auto gameStateInfo = playerGameStateInfos.FindByPredicate([name](const FDriftPlayerGameStateInfo& info)
    {
        return info.name == name;
    });
    if (gameStateInfo == nullptr)
    {
        DRIFT_LOG(Base, Warning, TEXT("Failed to find player game state: \"%s\""), *name);

        delegate.ExecuteIfBound(ELoadPlayerGameStateResult::Error_NotFound, name, FString());
        onPlayerGameStateLoaded.Broadcast(ELoadPlayerGameStateResult::Error_NotFound, name, FString());
        return;
    }
    auto request = GetGameRequestManager()->Get(gameStateInfo->gamestate_url);
    request->OnResponse.BindLambda([this, name, delegate](ResponseContext& context, JsonDocument& doc)
    {
        FPlayerGameStateResponse response;
        if (!JsonArchive::LoadObject(doc, response) || response.data.IsNull() || !response.data.HasMember(TEXT("data")))
        {
            context.error = TEXT("Failed to parse game state response");
            return;
        }
        const FString data{ response.data[TEXT("data")].GetString() };
        delegate.ExecuteIfBound(ELoadPlayerGameStateResult::Success, name, data);
        onPlayerGameStateLoaded.Broadcast(ELoadPlayerGameStateResult::Success, name, data);

        auto event = MakeEvent(TEXT("drift.gamestate_loaded"));
        event->Add(TEXT("namespace"), *name);
        event->Add(TEXT("bytes"), context.response->GetContentLength());
        event->Add(TEXT("request_time"), (context.received - context.sent).GetTotalSeconds());
        AddAnalyticsEvent(MoveTemp(event));
    });
    request->OnError.BindLambda([this, name, delegate](ResponseContext& context)
    {
        context.errorHandled = true;
        delegate.ExecuteIfBound(ELoadPlayerGameStateResult::Error_Failed, name, FString());
        onPlayerGameStateLoaded.Broadcast(ELoadPlayerGameStateResult::Error_Failed, name, FString());
    });
    request->Dispatch();
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


void FDriftBase::SavePlayerGameStateImpl(const FString& name, const FString& gameState, const FDriftGameStateSavedDelegate& delegate)
{
    DRIFT_LOG(Base, Log, TEXT("Saving player game state \"%s\""), *name);
    
    auto gameStateInfo = playerGameStateInfos.FindByPredicate([name](const FDriftPlayerGameStateInfo& info)
    {
        return info.name == name;
    });
    FString url;
    if (gameStateInfo != nullptr)
    {
        url = gameStateInfo->gamestate_url;
    }
    else
    {
        url = driftEndpoints.my_gamestate.Replace(TEXT("{namespace}"), *name);
    }

    FPlayerGameStatePayload payload{};
    JsonArchive::AddMember(payload.gamestate, TEXT("data"), *gameState);
    auto request = GetGameRequestManager()->Put(url, payload);
    request->OnResponse.BindLambda([this, name, delegate](ResponseContext& context, JsonDocument& doc)
    {
        delegate.ExecuteIfBound(true, name);
        onPlayerGameStateSaved.Broadcast(true, name);

        auto event = MakeEvent(TEXT("drift.gamestate_saved"));
        event->Add(TEXT("namespace"), *name);
        event->Add(TEXT("bytes"), context.request->GetContentLength());
        event->Add(TEXT("request_time"), (context.received - context.sent).GetTotalSeconds());
        AddAnalyticsEvent(MoveTemp(event));
    });
    request->OnError.BindLambda([this, name, delegate](ResponseContext& context)
    {
        context.errorHandled = true;
        delegate.ExecuteIfBound(false, name);
        onPlayerGameStateSaved.Broadcast(false, name);
    });
    request->Dispatch();
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
    request->OnError.BindLambda([next](ResponseContext& context)
    {
        context.errorHandled = true;
        next(false);
    });
    request->Dispatch();
}


bool FDriftBase::GetCount(const FString& counter_name, float& value)
{
    check(playerCounterManager.IsValid());

    return playerCounterManager->GetCount(counter_name, value);
}


void FDriftBase::AuthenticatePlayer()
{
    if (state_ >= DriftSessionState::Connecting)
    {
        DRIFT_LOG(Base, Warning, TEXT("Ignoring attempt to authenticate while another attempt is in progress."));

        return;
    }

    FParse::Value(FCommandLine::Get(), TEXT("-drift_url="), cli.drift_url);

    if (cli.drift_url.IsEmpty())
    {
        GConfig->GetString(settingsSection, TEXT("DriftUrl"), cli.drift_url, GGameIni);
    }

    if (cli.drift_url.IsEmpty())
    {
        DRIFT_LOG(Base, Log, TEXT("Running in client mode, but no Drift url specified."));
        // TODO: Error handling
        onPlayerAuthenticated.Broadcast(false, FPlayerAuthenticatedInfo{ EAuthenticationResult::Error_Config, TEXT("No Drift URL configured") });
        return;
    }

    FString credentialType;
    FParse::Value(FCommandLine::Get(), TEXT("-auth_type="), credentialType);
    if (credentialType.IsEmpty())
    {
        GConfig->GetString(settingsSection, TEXT("CredentialsType"), credentialType, GGameIni);
    }

    if (credentialType.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("No credential type specified, falling back to device credentials."));
        credentialType = TEXT("device");
    }

    GetRootEndpoints([this, credentialType]() {
        InitAuthentication(credentialType);
    });
}


EDriftConnectionState FDriftBase::GetConnectionState() const
{
    return InternalToPublicState(state_);
}


EDriftConnectionState FDriftBase::InternalToPublicState(DriftSessionState internalState) const
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


void FDriftBase::BroadcastConnectionStateChange(DriftSessionState internalState)
{
    onConnectionStateChanged.Broadcast(InternalToPublicState(internalState));
}


TUniquePtr<IDriftAuthProvider> FDriftBase::MakeAuthProvider(const FString& credentialType)
{
    auto factories = IModularFeatures::Get().GetModularFeatureImplementations<IDriftAuthProviderFactory>(TEXT("DriftAuthProviderFactory"));
    for (const auto factory : factories)
    {
        if (credentialType.Compare(factory->GetAuthProviderName().ToString(), ESearchCase::IgnoreCase) == 0)
        {
            return factory->GetAuthProvider();
        }
    }

    return nullptr;
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
            context.error = L"Failed to parse matches";
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
            result.server_status = match.server_status;
            result.ue4_connection_url = match.ue4_connection_url;
            result.version = match.version;
            result.matchplayers_url = match.matchplayers_url;
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
    auto tokenIt = message.payload.FindMember(TEXT("token"));
    if (tokenIt == message.payload.MemberEnd() || !(*tokenIt).value.IsString())
    {
        UE_LOG(LogDriftMessages, Error, TEXT("Match queue message contains no valid token"));
        
        return;
    }
    FString token = (*tokenIt).value.GetString();

    auto actionIt = message.payload.FindMember(TEXT("action"));
    if (actionIt != message.payload.MemberEnd())
    {
        auto& value = (*actionIt).value;
        if (!value.IsString())
        {
            UE_LOG(LogDriftMessages, Error, TEXT("Can't parse match queue action"));

            return;
        }
        FString action = value.GetString();
        if (action == TEXT("challenge"))
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


void FDriftBase::JoinMatchQueueImpl(const FString& ref, const FString& placement, const FString& token, const FDriftJoinedMatchQueueDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        IErrorReporter::Get()->AddError(L"LogDriftBase", TEXT("Attempting to join the match queue without being connected"));
        
        delegate.ExecuteIfBound(false, FMatchQueueStatus{});
        return;
    }

    if (matchQueueState != EMatchQueueState::Idle)
    {
        IErrorReporter::Get()->AddError(L"LogDriftBase", TEXT("Attempting to join the match queue while not idle"));
        
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
            context.error = L"Failed to parse join queue response";
            return;
        }
        matchQueueState = matchQueue.status == MatchQueueStatusMatchedName ? EMatchQueueState::Matched : EMatchQueueState::Queued;
        delegate.ExecuteIfBound(true, FMatchQueueStatus{
            matchQueue.status, FActiveMatch{
                matchQueue.match_id,
                0,
                matchQueue.create_date,
                TEXT(""), TEXT(""),
                TEXT(""), TEXT(""),
                matchQueue.ue4_connection_url,
                TEXT(""),
                TEXT("")
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
        IErrorReporter::Get()->AddError(L"LogDriftBase", TEXT("Attempting to leave the match queue without being connected"));
        
        delegate.ExecuteIfBound(false);
        return;
    }

    if (matchQueue.matchqueueplayer_url.IsEmpty())
    {
        IErrorReporter::Get()->AddError(L"LogDriftBase", TEXT("Attempting to leave the match queue without being in one"));
        
        delegate.ExecuteIfBound(false);
        return;
    }
    
    if (matchQueueState == EMatchQueueState::Matched)
    {
        IErrorReporter::Get()->AddError(L"LogDriftBase", TEXT("Attempting to leave the match queue after getting matched"));
        
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

        IErrorReporter::Get()->AddError(L"LogDriftBase", TEXT("Failed to leave the match queue for an unknown reason"));

        context.errorHandled = true;
        delegate.ExecuteIfBound(false);
    });
    request->Dispatch();
}


void FDriftBase::PollMatchQueue(const FDriftPolledMatchQueueDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        IErrorReporter::Get()->AddError(L"LogDriftBase", TEXT("Attempting to poll the match queue without being connected"));
        
        delegate.ExecuteIfBound(false, FMatchQueueStatus{});
        return;
    }

    if (matchQueue.matchqueueplayer_url.IsEmpty())
    {
        IErrorReporter::Get()->AddError(L"LogDriftBase", TEXT("Attempting to poll the match queue without being in one"));

        delegate.ExecuteIfBound(false, FMatchQueueStatus{});
        return;
    }

    if (matchQueueState != EMatchQueueState::Queued && matchQueueState != EMatchQueueState::Matched)
    {
        auto extra = MakeShared<FJsonObject>();
        extra->SetNumberField(L"state", (int32)matchQueueState);
        IErrorReporter::Get()->AddError(L"LogDriftBase", TEXT("Attempting to poll the match queue while in an incompatible state"), extra);
        
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
            context.error = L"Failed to parse poll queue response";
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
            response.status, FActiveMatch{
                response.match_id,
                0,
                response.create_date,
                TEXT(""), TEXT(""),
                TEXT(""), TEXT(""),
                response.ue4_connection_url,
                TEXT(""),
                TEXT("")
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
        IErrorReporter::Get()->AddError(L"LogDriftBase", TEXT("Attempting to reset the match queue without being connected"));
        
        return;
    }
    
    if (matchQueue.matchqueueplayer_url.IsEmpty())
    {
        IErrorReporter::Get()->AddError(L"LogDriftBase", TEXT("Attempting to reset the match queue without being in one"));
        
        return;
    }
    
    if (matchQueueState != EMatchQueueState::Matched)
    {
        IErrorReporter::Get()->AddError(L"LogDriftBase", TEXT("Attempting to reset the match queue without being matched"));
        
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
        IErrorReporter::Get()->AddError(L"LogDriftBase", TEXT("Attempting to send match challenge without being connected"));
        
        delegate.ExecuteIfBound(false, FMatchQueueStatus{});
        return;
    }
    
    if (matchQueueState != EMatchQueueState::Idle)
    {
        IErrorReporter::Get()->AddError(L"LogDriftBase", TEXT("Attempting to send match challenge while not idle"));
        
        delegate.ExecuteIfBound(false, FMatchQueueStatus{});
        return;
    }
    
    if (playerID == myPlayer.player_id)
    {
        IErrorReporter::Get()->AddError(L"LogDriftBase", TEXT("Attempting to challenge yourself to a match is not allowed"));
        
        delegate.ExecuteIfBound(false, FMatchQueueStatus{});
        return;
    }
    
    auto playerInfo = GetFriendInfo(playerID);
    if (!playerInfo)
    {
        auto extra = MakeShared<FJsonObject>();
        extra->SetNumberField(L"player_id", playerID);
        IErrorReporter::Get()->AddError(L"LogDriftBase", TEXT("Attempting to challenge player to match, but there's no information about the player"), extra);

        delegate.ExecuteIfBound(false, FMatchQueueStatus{});
        return;
    }

    // TODO: Preferably one should create the match/queue entry, and then send the invite separately

    auto token = FGuid::NewGuid();
    JoinMatchQueueImpl(buildReference, defaultPlacement, token.ToString(), FDriftJoinedMatchQueueDelegate::CreateLambda([this, playerInfo, token, delegate](bool success, const FMatchQueueStatus& status)
    {
        if (success)
        {
            const int inviteTimeoutSeconds = 180;

            JsonValue message{ rapidjson::kObjectType };
            JsonArchive::AddMember(message, TEXT("action"), TEXT("challenge"));
            JsonArchive::AddMember(message, TEXT("token"), *token.ToString());
            auto messageUrlTemplate = playerInfo->messagequeue_url;
            messageQueue->SendMessage(messageUrlTemplate, TEXT("matchqueue"), MoveTemp(message), inviteTimeoutSeconds);
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


void FDriftBase::AddCount(const FString& counter_name, float value, bool absolute)
{
    check(playerCounterManager.IsValid());

    playerCounterManager->AddCount(counter_name, value, absolute);
}


void FDriftBase::AddAnalyticsEvent(const FString& event_name, const TArray<FAnalyticsEventAttribute>& attributes)
{
    auto event = MakeEvent(event_name);
    for (const auto& attribute : attributes)
    {
        event->Add(attribute.AttrName, attribute.AttrValue);
    }
    AddAnalyticsEvent(MoveTemp(event));
}


void FDriftBase::AddAnalyticsEvent(TUniquePtr<IDriftEvent> event)
{
    // TODO: Handle multiple matches
    if (eventManager.IsValid())
    {
        if (match_info.match_id != 0)
        {
            event->Add(TEXT("match_id"), match_info.match_id);
        }
        eventManager->AddEvent(MoveTemp(event));
    }
}


const FDriftCounterInfo* FDriftBase::GetCounterInfo(const FString& counter_name) const
{
    auto canonical_name = FDriftCounterManager::MakeCounterName(counter_name);
    auto counter = counterInfos.FindByPredicate([canonical_name](const FDriftCounterInfo& info) -> bool
    {
        return info.name == canonical_name;
    });
    return counter;
}


void FDriftBase::GetLeaderboard(const FString& counter_name, const TSharedRef<FDriftLeaderboard>& leaderboard, const FDriftLeaderboardLoadedDelegate& delegate)
{
    leaderboard->state = ELeaderboardState::Failed;

    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to load player counters without being connected"));
        
        delegate.ExecuteIfBound(false, FDriftCounterManager::MakeCounterName(counter_name));
        return;
    }
    
    if (myPlayer.counter_url.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to load player counters before the player session has been initialized"));

        delegate.ExecuteIfBound(false, FDriftCounterManager::MakeCounterName(counter_name));
        return;
    }

    leaderboard->state = ELeaderboardState::Loading;

    BeginGetLeaderboard(counter_name, leaderboard, TEXT(""), delegate);
}


void FDriftBase::GetFriendsLeaderboard(const FString& counter_name, const TSharedRef<FDriftLeaderboard>& leaderboard, const FDriftLeaderboardLoadedDelegate& delegate)
{
    leaderboard->state = ELeaderboardState::Failed;

    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to load player counters without being connected"));
        
        delegate.ExecuteIfBound(false, FDriftCounterManager::MakeCounterName(counter_name));
        return;
    }
    
    if (driftEndpoints.my_player_groups.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to load friend counters before the player session has been initialized"));
        
        delegate.ExecuteIfBound(false, FDriftCounterManager::MakeCounterName(counter_name));
        return;
    }

    leaderboard->state = ELeaderboardState::Loading;

    /**
     * TODO: Handle the case where a player adds/removes friends during a session.
     * External IDs and userIdentities would have to be refreshed.
     */
    if (userIdentitiesLoaded)
    {
        BeginGetFriendLeaderboard(counter_name, leaderboard, delegate);
    }
    else
    {
        LoadFriendsList(FDriftFriendsListLoadedDelegate::CreateLambda([this, counter_name, leaderboard, delegate](bool success)
        {
            if (success)
            {
                BeginGetFriendLeaderboard(counter_name, leaderboard, delegate);
            }
            else
            {
                leaderboard->state = ELeaderboardState::Failed;
                delegate.ExecuteIfBound(false, FDriftCounterManager::MakeCounterName(counter_name));
            }
        }));
    }
}


void FDriftBase::BeginGetFriendLeaderboard(const FString& counter_name, const TWeakPtr<FDriftLeaderboard>& leaderboard, const FDriftLeaderboardLoadedDelegate& delegate)
{
    BeginGetLeaderboard(counter_name, leaderboard, TEXT("friends"), delegate);
}


void FDriftBase::BeginGetLeaderboard(const FString& counter_name, const TWeakPtr<FDriftLeaderboard>& leaderboard, const FString& player_group, const FDriftLeaderboardLoadedDelegate& delegate)
{
    /**
     * TODO: If the game adds a new counter, it won't be in the counter_infos list until the next connect.
     * This may or may not be something we want to solve, for instance by marking counter_infos as dirty
     * when the new counter is added.
     */
    if (countersLoaded)
    {
        GetLeaderboardImpl(counter_name, leaderboard, player_group, delegate);
    }
    else
    {
        auto request = GetGameRequestManager()->Get(driftEndpoints.counters);
        request->OnResponse.BindLambda([counter_name, leaderboard, player_group, delegate, this](ResponseContext& context, JsonDocument& doc)
        {
            if (!JsonArchive::LoadObject(doc, counterInfos))
            {
                context.error = TEXT("Failed to parse leaderboards response");
                return;
            }
            countersLoaded = true;
            GetLeaderboardImpl(counter_name, leaderboard, player_group, delegate);
        });
        request->OnError.BindLambda([this, counter_name, leaderboard, delegate](ResponseContext& context)
        {
            auto leaderboardPtr = leaderboard.Pin();
            if (leaderboardPtr.IsValid())
            {
                leaderboardPtr->state = ELeaderboardState::Failed;
            }
            context.errorHandled = true;
            delegate.ExecuteIfBound(false, FDriftCounterManager::MakeCounterName(counter_name));
        });
        request->Dispatch();
    }
}


void FDriftBase::GetLeaderboardImpl(const FString& counter_name, const TWeakPtr<FDriftLeaderboard>& leaderboard, const FString& player_group, const FDriftLeaderboardLoadedDelegate& delegate)
{
    auto canonical_name = FDriftCounterManager::MakeCounterName(counter_name);
    
    DRIFT_LOG(Base, Log, TEXT("Getting leaderboard for %s"), *canonical_name);
    
    auto leaderboardPtr = leaderboard.Pin();
    if (leaderboardPtr.IsValid())
    {
        leaderboardPtr->rows.Empty();
    }

    auto counter = GetCounterInfo(counter_name);

    if (counter == nullptr || counter->url.IsEmpty())
    {
        DRIFT_LOG(Base, Log, TEXT("Found no leaderboard for %s"), *canonical_name);

        delegate.ExecuteIfBound(false, canonical_name);

        return;
    }

    auto url = counter->url;
    if (!player_group.IsEmpty())
    {
        internal::UrlHelper::AddUrlOption(url, TEXT("player_group"), *player_group);
    }

    auto request = GetGameRequestManager()->Get(url);
    request->OnResponse.BindLambda([this, leaderboard, canonical_name, delegate, player_group](ResponseContext& context, JsonDocument& doc)
    {
        TArray<FDriftLeaderboardResponseItem> entries;
        if (!JsonArchive::LoadObject(doc, entries))
        {
            context.error = L"Failed to parse leaderboard entries response";
            return;
        }

        DRIFT_LOG(Base, Verbose, TEXT("Got %d entries for leaderboard %s"), entries.Num(), *canonical_name);
        
        auto leaderboardPin = leaderboard.Pin();
        if (leaderboardPin.IsValid())
        {
            for (const auto& entry : entries)
            {
                leaderboardPin->rows.Add(FDriftLeaderboardEntry{ entry.player_name, entry.total, entry.position });
            }

            leaderboardPin->state = ELeaderboardState::Ready;
        }
        delegate.ExecuteIfBound(true, canonical_name);

        auto event = MakeEvent(TEXT("drift.leaderboard_loaded"));
        event->Add(TEXT("counter_name"), *canonical_name);
        event->Add(TEXT("num_entries"), entries.Num());
        event->Add(TEXT("player_group"), *player_group);
        event->Add(TEXT("request_time"), (context.received - context.sent).GetTotalSeconds());
        AddAnalyticsEvent(MoveTemp(event));
    });
    request->OnError.BindLambda([this, canonical_name, delegate](ResponseContext& context)
    {
        context.errorHandled = true;
        delegate.ExecuteIfBound(false, canonical_name);
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
    
    if (driftEndpoints.my_player_groups.IsEmpty())
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to load friends list before the player session has been initialized"));
        
        delegate.ExecuteIfBound(false);
        return;
    }
    
    FDriftCreatePlayerGroupPayload payload;
    payload.player_ids.Add(myPlayer.player_id);
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
                    auto lowID = FCString::Atoi(*low);
                    auto highID = FCString::Atoi(*high);
                    
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
    
    auto url = driftEndpoints.my_player_groups.Replace(TEXT("{group_name}"), TEXT("friends"));
    auto request = GetGameRequestManager()->Put(url, payload);
    request->OnResponse.BindLambda([this, delegate](ResponseContext& context, JsonDocument& doc)
    {
        if  (!JsonArchive::LoadObject(doc, userIdentities))
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
        event->Add(TEXT("group_name"), TEXT("friends"));
        event->Add(TEXT("request_time"), (context.received - context.sent).GetTotalSeconds());
        AddAnalyticsEvent(MoveTemp(event));

        CacheFriendInfos([this, delegate](bool success)
        {
            userIdentitiesLoaded = success;
            delegate.ExecuteIfBound(success);
        });
    });
    request->OnError.BindLambda([this, delegate](ResponseContext& context)
    {
        context.errorHandled = true;
        delegate.ExecuteIfBound(false);
    });
    request->Dispatch();
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
        auto playerInfo = GetFriendInfo(entry.player_id);
        auto presence = (playerInfo && playerInfo->is_online) ? EDriftPresence::Online : EDriftPresence::Offline;
        friends.Add(FDriftFriend{ entry.player_id, entry.player_name, presence });
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


void FDriftBase::GetRootEndpoints(TFunction<void()> onSuccess)
{
    FString url = cli.drift_url;

    check(!url.IsEmpty());

    DRIFT_LOG(Base, Verbose, TEXT("Getting root endpoints from %s"), *url);

    auto request = GetRootRequestManager()->Get(url);
    request->OnResponse.BindLambda([this, onSuccess](ResponseContext& context, JsonDocument& doc)
    {
        if (!JsonArchive::LoadObject(doc[L"endpoints"], driftEndpoints))
        {
            context.error = L"Failed to parse endpoints";
            return;
        }
        onSuccess();
        eventManager->SetEventsUrl(driftEndpoints.eventlogs);
        logForwarder->SetLogsUrl(driftEndpoints.clientlogs);
        onStaticRoutesInitialized.Broadcast();
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        if (context.response.IsValid() && context.response->GetResponseCode() == EHttpResponseCodes::Forbidden)
        {
            ClientUpgradeResponse message;
            if (JsonUtils::ParseResponse(context.response, message) && message.action == TEXT("upgrade_client"))
            {
                context.errorHandled = true;
                Reset();
                onGameVersionMismatch.Broadcast(message.upgrade_url);
                return;
            }
        }
        context.errorHandled = true;
        Reset();
        onPlayerAuthenticated.Broadcast(false, FPlayerAuthenticatedInfo{ EAuthenticationResult::Error_Failed, context.error });
        return;
    });
    request->Dispatch();
}


void FDriftBase::InitAuthentication(const FString& credentialType)
{
    authProvider.Reset();

    if (GIsEditor && !IsRunningGame())
    {
        if (credentialType.Compare(TEXT("device"), ESearchCase::IgnoreCase) != 0)
        {
            DRIFT_LOG(Base, Warning, TEXT("Bypassing external authentication when running in editor."));
        }

        authProvider = MakeShareable(deviceAuthProviderFactory->GetAuthProvider().Release());
    }
    else
    {
        authProvider = MakeShareable(MakeAuthProvider(credentialType).Release());
    }

    if (!authProvider.IsValid())
    {
        DRIFT_LOG(Base, Warning, TEXT("Failed to find auth provider for '%s', falling back to device credentials"), *credentialType);

        authProvider = MakeShareable(deviceAuthProviderFactory->GetAuthProvider().Release());
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
            DRIFT_LOG(Base, Warning, TEXT("Failed to aquire credentials"));

            Reset();
            onPlayerAuthenticated.Broadcast(false, FPlayerAuthenticatedInfo{ EAuthenticationResult::Error_NoOnlineSubsystemCredentials, TEXT("Failed to aquire credentials") });
        }
        return;
    });
}


void FDriftBase::AuthenticatePlayer(IDriftAuthProvider* provider)
{
    FUserPassAuthenticationPayload payload{};
    payload.provider = provider->GetProviderName();
    payload.automatic_account_creation = true;
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

    DRIFT_LOG(Base, Verbose, TEXT("Authenticating player with: %s"), *provider->ToString());

    state_ = DriftSessionState::Connecting;
    BroadcastConnectionStateChange(state_);

    auto request = GetRootRequestManager()->Post(driftEndpoints.auth, payload, HttpStatusCodes::Ok);
    request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
    {
        FString jti;
        auto member = doc.FindMember(TEXT("jti"));
        if (member != doc.MemberEnd() && member->value.IsString())
        {
            jti = member->value.GetString();
        }
        if (jti.IsEmpty())
        {
            context.error = L"Session 'jti' missing.";
            return;
        }

        DRIFT_LOG(Base, Verbose, TEXT("Got JTI %s"), *jti);

        TSharedRef<JsonRequestManager> manager = MakeShareable(new JTIRequestManager(jti));
        manager->DefaultErrorHandler.BindRaw(this, &FDriftBase::DefaultErrorHandler);
        manager->SetApiKey(GetApiKeyHeader());
        manager->SetCache(httpCache_);
        SetGameRequestManager(manager);
        GetUserInfo();
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        context.errorHandled = true;
        Reset();
        if (context.error.IsEmpty() && context.response.IsValid())
        {
            GenericRequestErrorResponse response;
            if (JsonUtils::ParseResponse(context.response, response))
            {
                context.error = response.GetErrorDescription();
            }
        }
        onPlayerAuthenticated.Broadcast(false, FPlayerAuthenticatedInfo{ EAuthenticationResult::Error_Failed, context.error });
    });
    request->Dispatch();
}


void FDriftBase::GetUserInfo()
{
    auto request = GetGameRequestManager()->Get(cli.drift_url, HttpStatusCodes::Ok);
    request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
    {
        auto currentUser = doc.FindMember(TEXT("current_user"));
        if (currentUser == doc.MemberEnd() || !currentUser->value.IsObject())
        {
            context.error = TEXT("Failed to read user info");
            return;
        }

        auto userObject = currentUser->value.GetObject();
        auto userId = userObject.FindMember(TEXT("user_id"));
        if (userId == userObject.MemberEnd() || !userId->value.IsUint64())
        {
            context.error = TEXT("Failed to read user id");
            return;
        }

        if (userId->value.GetUint64() == 0)
        {
            context.error = TEXT("User creation failed");
            return;
        }

        RegisterClient();
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        context.errorHandled = true;
        Reset();
        onPlayerAuthenticated.Broadcast(false, FPlayerAuthenticatedInfo{ EAuthenticationResult::Error_Failed, context.error });
    });
    request->Dispatch();
}


void FDriftBase::RegisterClient()
{
    FClientRegistrationPayload payload;
    payload.client_type = L"UE4";
    payload.platform_type = details::GetPlatformName();
    payload.app_guid = L"TEMP";
    payload.product_name = projectName;

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
            context.error = L"Failed to parse client registration response";
            return;
        }
        hearbeatUrl = driftClient.url;
        heartbeatDueInSeconds = driftClient.next_heartbeat_seconds;
        TSharedRef<JsonRequestManager> manager = MakeShareable(new JTIRequestManager(driftClient.jti));
        manager->DefaultErrorHandler.BindRaw(this, &FDriftBase::DefaultErrorHandler);
        manager->SetApiKey(GetApiKeyHeader());
        manager->SetCache(httpCache_);
        SetGameRequestManager(manager);
        playerCounterManager->SetRequestManager(manager);
        eventManager->SetRequestManager(manager);
        logForwarder->SetRequestManager(manager);
        messageQueue->SetRequestManager(manager);
        GetPlayerEndpoints();
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        context.errorHandled = true;
        Reset();
        onPlayerAuthenticated.Broadcast(false, FPlayerAuthenticatedInfo{ EAuthenticationResult::Error_Failed, context.error });
    });
    request->Dispatch();
}


void FDriftBase::GetPlayerEndpoints()
{
    DRIFT_LOG(Base, Verbose, TEXT("Fetching player endpoints"));
    
    auto request = GetGameRequestManager()->Get(cli.drift_url);
    request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
    {
        if (!JsonArchive::LoadObject(doc[TEXT("endpoints")], driftEndpoints))
        {
            context.error = "Failed to parse drift endpoints";
            return;
        }

        if (driftEndpoints.my_player.IsEmpty())
        {
            context.error = L"My player endpoint is empty";
            return;
        }

        GetPlayerInfo();
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        context.errorHandled = true;
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
            context.error = L"Failed to parse my player";
            return;
        }
        playerCounterManager->SetCounterUrl(myPlayer.counter_url);
        messageQueue->SetMessageQueueUrl(myPlayer.messages_url);
        state_ = DriftSessionState::Connected;
        BroadcastConnectionStateChange(state_);

        // TODO: Let user determine if the name should be set or not
        if (authProvider.IsValid())
        {
            auto currentName = myPlayer.player_name;
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
        context.errorHandled = true;
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


void FDriftBase::SetPlayerName(const FString& name)
{
    if (state_ != DriftSessionState::Connected)
    {
        return;
    }

    DRIFT_LOG(Base, Log, TEXT("Setting player name: %s"), *name);

    myPlayer.player_name = name;

    FChangePlayerNamePayload payload{ name };
    auto request = GetGameRequestManager()->Put(driftEndpoints.my_player, payload);
    request->OnResponse.BindLambda([this, name](ResponseContext& context, JsonDocument& doc)
    {
        onPlayerNameSet.Broadcast(true);
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        context.errorHandled = true;
        onPlayerNameSet.Broadcast(false);
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
            AddPlayerIdentity(provider.Get(), progressDelegate);
        }
        else
        {
            DRIFT_LOG(Base, Warning, TEXT("Failed to aquire credentials from %s"), *provider->GetProviderName());

            progressDelegate.ExecuteIfBound(EAddPlayerIdentityResult::Error_FailedToAquireCredentials, {});
        }
    });
}


void FDriftBase::AddPlayerIdentity(IDriftAuthProvider* provider, const FDriftAddPlayerIdentityProgressDelegate& progressDelegate)
{
    FUserPassAuthenticationPayload payload{};
    payload.provider = provider->GetProviderName();
    payload.automatic_account_creation = false;
    provider->FillProviderDetails([&payload](const FString& key, const FString& value) {
        JsonArchive::AddMember(payload.provider_details, *key, *value);
    });

    DRIFT_LOG(Base, Verbose, TEXT("Adding player identity: %s"), *provider->ToString());

    auto request = GetRootRequestManager()->Post(driftEndpoints.auth, payload, HttpStatusCodes::Ok);
    request->OnResponse.BindLambda([this, progressDelegate](ResponseContext& context, JsonDocument& doc)
    {
        FString jti;
        auto member = doc.FindMember(TEXT("jti"));
        if (member != doc.MemberEnd() && member->value.IsString())
        {
            jti = member->value.GetString();
        }
        if (jti.IsEmpty())
        {
            context.error = L"Identity 'jti' missing.";
            return;
        }

        DRIFT_LOG(Base, Verbose, TEXT("Got JTI %s"), *jti);

        TSharedRef<JsonRequestManager> manager = MakeShareable(new JTIRequestManager(jti));
        manager->DefaultErrorHandler.BindRaw(this, &FDriftBase::DefaultErrorHandler);
        manager->SetApiKey(GetApiKeyHeader());
        secondaryIdentityRequestManager_ = manager;

        BindUserIdentity(progressDelegate);
    });
    request->OnError.BindLambda([this, progressDelegate](ResponseContext& context)
    {
        context.errorHandled = true;
        if (context.error.IsEmpty() && context.response.IsValid())
        {
            GenericRequestErrorResponse response;
            if (JsonUtils::ParseResponse(context.response, response))
            {
                context.error = response.GetErrorDescription();
            }
        }
        progressDelegate.ExecuteIfBound(EAddPlayerIdentityResult::Error_FailedToAuthenticate, {});
        secondaryIdentityRequestManager_.Reset();
    });
    request->Dispatch();
}


void FDriftBase::BindUserIdentity(const FDriftAddPlayerIdentityProgressDelegate& progressDelegate)
{
    auto request = secondaryIdentityRequestManager_->Get(driftEndpoints.root);
    request->OnResponse.BindLambda([this, progressDelegate](ResponseContext& context, JsonDocument& doc)
    {
        if (doc.HasMember(TEXT("current_user")))
        {
            FDriftUserInfoResponse userInfo;
            if (JsonArchive::LoadObject(doc[TEXT("current_user")], userInfo))
            {
                if (userInfo.user_id == 0)
                {
                    AssociateNewIdentityWithCurrentUser(progressDelegate);
                }
                else if (userInfo.user_id != driftClient.user_id)
                {
                    progressDelegate.ExecuteIfBound(
                        EAddPlayerIdentityResult::Progress_IdentityAssociatedWithOtherUser,
                        FDriftPlayerIdentityContinuationDelegate::CreateLambda([this, progressDelegate, userInfo](EPlayerIdentityOverrideOption option)
                    {
                        switch (option)
                        {
                        case EPlayerIdentityOverrideOption::AssignIdentityToNewUser:
                            AssociateCurrentUserWithSecondaryIdentity(userInfo, progressDelegate);
                            break;
                        case EPlayerIdentityOverrideOption::DoNotOverrideExistingUserAssociation:
                            DRIFT_LOG(Base, Verbose, TEXT("User skipped identity association"));
                            secondaryIdentityRequestManager_.Reset();
                            break;
                        default:
                            check(false);
                        }
                    }));
                }
                else
                {
                    progressDelegate.ExecuteIfBound(EAddPlayerIdentityResult::Success, {});
                }
            }
        }
    });
    request->OnError.BindLambda([this, progressDelegate](ResponseContext& context)
    {
        context.errorHandled = true;
        progressDelegate.ExecuteIfBound(EAddPlayerIdentityResult::Error_FailedToBindNewIdentity, {});
        secondaryIdentityRequestManager_.Reset();
    });
    request->Dispatch();
}


void FDriftBase::AssociateNewIdentityWithCurrentUser(const FDriftAddPlayerIdentityProgressDelegate& progressDelegate)
{
    FDriftUserIdentityPayload payload{};
    payload.link_with_user_jti = driftClient.jti;
    payload.link_with_user_id = driftClient.user_id;
    auto request = secondaryIdentityRequestManager_->Post(driftEndpoints.user_identities, payload);
    request->OnResponse.BindLambda([this, progressDelegate](ResponseContext& context, JsonDocument& doc)
    {
        progressDelegate.ExecuteIfBound(EAddPlayerIdentityResult::Success, {});
        secondaryIdentityRequestManager_.Reset();
    });
    request->OnError.BindLambda([this, progressDelegate](ResponseContext& context)
    {
        // Could happen if the user already has an association with a different id from the same provider
        context.errorHandled = true;
        progressDelegate.ExecuteIfBound(EAddPlayerIdentityResult::Error_UserAlreadyBoundToSameIdentityType, {});
        secondaryIdentityRequestManager_.Reset();
    });
    request->Dispatch();
}


void FDriftBase::AssociateCurrentUserWithSecondaryIdentity(const FDriftUserInfoResponse& targetUser, const FDriftAddPlayerIdentityProgressDelegate& progressDelegate)
{
    FDriftUserIdentityPayload payload{};
    payload.link_with_user_jti = targetUser.jti;
    payload.link_with_user_id = targetUser.user_id;
    auto request = GetGameRequestManager()->Post(driftEndpoints.user_identities, payload);
    request->OnResponse.BindLambda([this, progressDelegate](ResponseContext& context, JsonDocument& doc)
    {
        // TODO: Switch player
        progressDelegate.ExecuteIfBound(EAddPlayerIdentityResult::Success, {});
        secondaryIdentityRequestManager_.Reset();
    });
    request->OnError.BindLambda([this, progressDelegate](ResponseContext& context)
    {
        context.errorHandled = true;
        progressDelegate.ExecuteIfBound(EAddPlayerIdentityResult::Error_FailedToBindNewIdentity, {});
        secondaryIdentityRequestManager_.Reset();
    });
    request->Dispatch();
}


void FDriftBase::InitServerRootInfo()
{
    FString drift_url = cli.drift_url;
    if (drift_url.IsEmpty())
    {
        if (!GConfig->GetString(settingsSection, TEXT("DriftUrl"), drift_url, GGameIni))
        {
            DRIFT_LOG(Base, Error, TEXT("Running in server mode, but no Drift url specified."));

            FDriftResponseInfo2 error;
            error.error_text = TEXT("Running in server mode, but no Drift url specified.");
            state_ = DriftSessionState::Disconnected;
            return;
        }
    }

    auto request = GetRootRequestManager()->Get(drift_url);
    request->OnResponse.BindLambda([this, drift_url](ResponseContext& context, JsonDocument& doc)
    {
        if (!JsonArchive::LoadObject(doc[L"endpoints"], driftEndpoints))
        {
            context.error = L"Failed to parse drift endpoints";
            state_ = DriftSessionState::Disconnected;
            return;
        }
        InitServerAuthentication();
        eventManager->SetEventsUrl(driftEndpoints.eventlogs);
        onStaticRoutesInitialized.Broadcast();
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        context.errorHandled = true;
        Reset();
    });
    request->Dispatch();
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
        TSharedRef<JsonRequestManager> manager = MakeShareable(new JTIRequestManager(cli.jti));
        manager->DefaultErrorHandler.BindRaw(this, &FDriftBase::DefaultErrorHandler);
        manager->SetApiKey(GetApiKeyHeader());
        manager->SetCache(httpCache_);
        SetGameRequestManager(manager);
        eventManager->SetRequestManager(manager);
        InitServerRegistration();
        return;
    }

    FString password;
    FParse::Value(FCommandLine::Get(), TEXT("-driftPass="), password);
    
    if (password.IsEmpty())
    {
        DRIFT_LOG(Base, Error, TEXT("When not pre-authenticated, credentials must be passed on the command line -driftPass=yyy"));

        Reset();
        return;
    }
    
    // Post to 'auth' and get token. Use hacky credentials
    auto payload = FString::Printf(
        TEXT("{\"username\": \"%s\", \"password\": \"%s\", \"provider\": \"%s\"}"),
        *SERVER_CREDENTIALS_USERNAME, *password, *SERVER_CREDENTIALS_PROVIDER
        );

    DRIFT_LOG(Base, Log, TEXT("Authenticating server"));
    
    auto request = GetRootRequestManager()->Post(driftEndpoints.auth, payload, HttpStatusCodes::Ok);
    request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
    {
        FString jti = doc[TEXT("jti")].GetString();
        if (jti.IsEmpty())
        {
            context.error = L"Session 'jti' missing.";
            return;
        }

        DRIFT_LOG(Base, Verbose, TEXT("GOT JTI %s"), *jti);

        TSharedRef<JsonRequestManager> manager = MakeShareable(new JTIRequestManager(jti));
        manager->DefaultErrorHandler.BindRaw(this, &FDriftBase::DefaultErrorHandler);
        manager->SetApiKey(GetApiKeyHeader());
        manager->SetCache(httpCache_);
        SetGameRequestManager(manager);
        eventManager->SetRequestManager(manager);

        InitServerRegistration();
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        // TODO: Error handling
        context.errorHandled = true;
        Reset();
    });
    request->Dispatch();
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
        InitServerInfo(cli.server_url);
        return;
    }

    static const int32 defaultPort = 7777;

    FServerRegistrationPayload payload;
    payload.placement = defaultPlacement;
    payload.instance_name = GetInstanceName();
    payload.ref = buildReference;
    payload.public_ip = GetPublicIP();
    payload.port = !cli.port.IsEmpty() && cli.port.IsNumeric() ? FCString::Atoi(*cli.port) : defaultPort;
    payload.command_line = FCommandLine::Get();
    payload.pid = FPlatformProcess::GetCurrentProcessId();
    payload.status = TEXT("starting");

    DRIFT_LOG(Base, Log, TEXT("Registering server ip='%s', ref='%s', placement='%s'"), *payload.public_ip, *payload.ref, *payload.placement);

    auto request = GetGameRequestManager()->Post(driftEndpoints.servers, payload);
    request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
    {
        InitServerInfo(doc[L"url"].GetString());
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        // TODO: Error handling
        context.errorHandled = true;
        Reset();
    });
    request->Dispatch();
}


void FDriftBase::InitServerInfo(const FString& server_url)
{
    cli.server_url = server_url;

    DRIFT_LOG(Base, Log, TEXT("Fetching server info"));
    
    JsonValue payload{ rapidjson::kObjectType };
    JsonArchive::AddMember(payload, TEXT("status"), TEXT("initializing"));
    auto request = GetGameRequestManager()->Put(cli.server_url, payload);
    request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
    {
        auto server_request = GetGameRequestManager()->Get(cli.server_url);
        server_request->OnResponse.BindLambda([this](ResponseContext& server_context, JsonDocument& server_doc)
        {
            if (!JsonArchive::LoadObject(server_doc, drift_server))
            {
                server_context.error = L"Failed to parse drift server endpoint response.";
                return;
            }
            hearbeatUrl = drift_server.heartbeat_url;
            heartbeatDueInSeconds = -1.0;
            state_ = DriftSessionState::Connected;
            onServerRegistered.Broadcast(true);
            UpdateServer(TEXT("ready"), TEXT(""), FDriftServerStatusUpdatedDelegate{});
        });
        server_request->Dispatch();
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        // TODO: Error handling
        context.errorHandled = true;
        Reset();
    });
    request->Dispatch();
}


bool FDriftBase::RegisterServer()
{
    if (state_ == DriftSessionState::Connected)
    {
        onServerRegistered.Broadcast(true);
        return true;
    }
    
    state_ = DriftSessionState::Connecting;
    
    FParse::Value(FCommandLine::Get(), TEXT("-public_ip="), cli.public_ip);
    FParse::Value(FCommandLine::Get(), TEXT("-drift_url="), cli.drift_url);
    FParse::Value(FCommandLine::Get(), TEXT("-server_url="), cli.server_url);
    FParse::Value(FCommandLine::Get(), TEXT("-port="), cli.port);
    FParse::Value(FCommandLine::Get(), TEXT("-jti="), cli.jti);

    if (cli.drift_url.IsEmpty())
    {
        GConfig->GetString(settingsSection, TEXT("DriftUrl"), cli.drift_url, GGameIni);
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


void FDriftBase::AddPlayerToMatch(int32 player_id, int32 team_id, const FDriftPlayerAddedDelegate& delegate)
{
    // TODO: Deprecate
    AddPlayerToMatch(match_info.match_id, team_id, player_id, delegate);
}


void FDriftBase::AddPlayerToMatch(int32 match_id, int32 team_id, int32 player_id, const FDriftPlayerAddedDelegate & delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        /**
         * TODO: Is this the best approach? This should only ever happen in the editor,
         * as in the real game no client can connect before the match has been initialized.
         */
        delegate.ExecuteIfBound(false, match_id, player_id);
        return;
    }
    
    FString url;
    {
        FScopeLock lock{ &matchInfoMutex };
        auto matchInfo = matchInfos.Find(match_id);
        if (matchInfo)
        {
            url = matchInfo->matchplayers_url;
        }
    }

    if (url.IsEmpty())
    {
        DRIFT_LOG(Base, Error, TEXT("Attempting to add player %i to match %i for which the server has no information"), player_id, match_id);
        delegate.ExecuteIfBound(false, match_id, player_id);
        return;
    }

    FString payload;
    if (team_id != 0)
    {
        payload = FString::Printf(TEXT("{\"player_id\": %i, \"team_id\": %i}"), player_id, team_id);
    }
    else
    {
        payload = FString::Printf(TEXT("{\"player_id\": %i}"), player_id);
    }

    DRIFT_LOG(Base, Verbose, TEXT("Adding player: %i to match %i"), player_id, match_id);

    auto request = GetGameRequestManager()->Post(url, payload);
    request->OnResponse.BindLambda([this, match_id, player_id, delegate](ResponseContext& context, JsonDocument& doc)
    {
        delegate.ExecuteIfBound(true, match_id, player_id);
        onPlayerAddedToMatch.Broadcast(true, match_id, player_id);
    });
    request->OnError.BindLambda([this, match_id, player_id, delegate](ResponseContext& context)
    {
        context.errorHandled = true;

        delegate.ExecuteIfBound(false, match_id, player_id);
        onPlayerAddedToMatch.Broadcast(false, match_id, player_id);
    });
    request->Dispatch();

    CachePlayerInfo(player_id);
}


void FDriftBase::RemovePlayerFromMatch(int32 player_id, const FDriftPlayerRemovedDelegate& delegate)
{
    // TODO: Deprecate
    RemovePlayerFromMatch(match_info.match_id, player_id, delegate);
}


void FDriftBase::RemovePlayerFromMatch(int32 match_id, int32 player_id, const FDriftPlayerRemovedDelegate & delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        /**
        * TODO: Is this the best approach? This should only ever happen in the editor,
        * as in the real game no client can connect before the match has been initialized.
        */
        delegate.ExecuteIfBound(false, match_id, player_id);
        return;
    }

    FString url;
    {
        FScopeLock lock{ &matchInfoMutex };
        auto matchInfo = matchInfos.Find(match_id);
        if (matchInfo)
        {
            url = matchInfo->matchplayers_url;
        }
    }

    if (url.IsEmpty())
    {
        DRIFT_LOG(Base, Error, TEXT("Attempting to remove player %i from match %i for which the server has no information"), player_id, match_id);
        delegate.ExecuteIfBound(false, match_id, player_id);
        return;
    }

    DRIFT_LOG(Base, Verbose, TEXT("Removing player: %i from match %i"), player_id, match_id);

    // TODO: Don't use hard-coded URL
    FString player_url = FString::Printf(TEXT("%s/%i"), *url, player_id);
    auto request = GetGameRequestManager()->Delete(player_url);
    request->OnResponse.BindLambda([this, match_id, player_id, delegate](ResponseContext& context, JsonDocument& doc)
    {
        delegate.ExecuteIfBound(true, match_id, player_id);
        onPlayerRemovedFromMatch.Broadcast(true, match_id, player_id);
    });
    request->OnError.BindLambda([this, match_id, player_id, delegate](ResponseContext& context)
    {
        context.errorHandled = true;

        delegate.ExecuteIfBound(false, match_id, player_id);
        onPlayerRemovedFromMatch.Broadcast(false, match_id, player_id);
    });
    request->Dispatch();
}


void FDriftBase::AddMatch(const FString& map_name, const FString& game_mode, int32 num_teams, int32 max_players)
{
    AddMatch(map_name, game_mode, num_teams, max_players, FDriftOnMatchAddedDelegate{});
}


void FDriftBase::AddMatch(const FString& map_name, const FString& game_mode, int32 num_teams, int32 max_players, const FDriftOnMatchAddedDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        /**
         * TODO: Is this the best approach? This should only ever happen in the editor,
         * as in the real game no client can connect before the match has been initialized.
         */
        delegate.ExecuteIfBound(false, -1);
        return;
    }
    
    FMatchesPayload payload;
    payload.server_id = drift_server.server_id;
    payload.num_players = 0;
    payload.max_players = max_players;
    payload.map_name = map_name;
    payload.game_mode = game_mode;
    payload.status = TEXT("idle");
    payload.num_teams = num_teams;

    DRIFT_LOG(Base, Log, TEXT("Adding match to server: %i map: '%s' mode: '%s' players: %i teams: %i"), drift_server.server_id, *map_name, *game_mode, max_players, num_teams);

    auto request = GetGameRequestManager()->Post(driftEndpoints.matches, payload);
    request->OnResponse.BindLambda([this, delegate](ResponseContext& context, JsonDocument& doc)
    {
        FAddMatchResponse match;
        if (!JsonArchive::LoadObject(doc, match))
        {
            context.error = L"Failed to parse add match response.";
            return;
        }

        DRIFT_LOG(Base, VeryVerbose, TEXT("%s"), *JsonArchive::ToString(doc));
        
        auto match_request = GetGameRequestManager()->Get(match.url);
        match_request->OnResponse.BindLambda([this, delegate](ResponseContext& match_context, JsonDocument& match_doc)
        {
            FGetMatchResponseItem matchInfo;
            if (!JsonArchive::LoadObject(match_doc, matchInfo))
            {
                match_context.error = L"Failed to parse match info response.";
                return;
            }

            DRIFT_LOG(Base, VeryVerbose, TEXT("%s"), *JsonArchive::ToString(match_doc));

            {
                FScopeLock lock{ &matchInfoMutex };
                auto& match = matchInfos.Add(matchInfo.match_id, MoveTemp(matchInfo));

                delegate.ExecuteIfBound(true, match.match_id);
                onMatchAdded.Broadcast(true, match.match_id);
            }
        });
        match_request->Dispatch();
    });
    request->OnError.BindLambda([this, delegate](ResponseContext& context)
    {
        context.errorHandled = true;
        delegate.ExecuteIfBound(false, -1);
        onMatchAdded.Broadcast(false, -1);
    });
    request->Dispatch();
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
    request->OnResponse.BindLambda([this, delegate](ResponseContext& context, JsonDocument& doc)
    {
        delegate.ExecuteIfBound(true);
    });
    request->OnError.BindLambda([this, delegate](ResponseContext& context)
    {
        delegate.ExecuteIfBound(false);
        context.errorHandled = true;
    });
    request->Dispatch();
}


void FDriftBase::UpdateMatch(const FString& status, const FString& reason, const FDriftMatchStatusUpdatedDelegate& delegate)
{
    // TODO: Deprecate
    UpdateMatch(match_info.match_id, status, reason, delegate);
}


void FDriftBase::UpdateMatch(int32 match_id, const FString& status, const FString& reason, const FDriftMatchStatusUpdatedDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        /**
         * TODO: Is this the best approach? This should only ever happen in the editor,
         * as in the real game no client can connect before the match has been initialized.
         */
        delegate.ExecuteIfBound(false, match_id);
        return;
    }

    FString url;
    {
        FScopeLock lock{ &matchInfoMutex };
        auto matchInfo = matchInfos.Find(match_id);
        if (matchInfo)
        {
            url = matchInfo->url;
        }
    }

    if (url.IsEmpty())
    {
        DRIFT_LOG(Base, Error, TEXT("Attempting to update status for match %i for which the server has no information"), match_id);
        delegate.ExecuteIfBound(false, match_id);
        return;
    }
/* TODO: Is this the right place?
    if (status == TEXT("completed"))
    {
        matchInfos.Remove(match_id);
    }
*/
    DRIFT_LOG(Base, Log, TEXT("Updating match status to '%s'"), *status);
    
    JsonValue payload{ rapidjson::kObjectType };
    JsonArchive::AddMember(payload, TEXT("status"), *status);
    if (!reason.IsEmpty())
    {
        JsonValue details{ rapidjson::kObjectType };
        JsonArchive::AddMember(details, TEXT("status-reason"), *reason);
        JsonArchive::AddMember(payload, TEXT("details"), details);
    }

    auto request = GetGameRequestManager()->Put(url, payload);
    request->OnResponse.BindLambda([this, match_id, delegate](ResponseContext& context, JsonDocument& doc)
    {
        delegate.ExecuteIfBound(true, match_id);
        onMatchUpdated.Broadcast(true, match_id);
    });
    request->OnError.BindLambda([this, match_id, delegate](ResponseContext& context)
    {
        delegate.ExecuteIfBound(false, match_id);
        onMatchUpdated.Broadcast(false, match_id);
        context.errorHandled = true;
    });
    request->Dispatch();
}


int32 FDriftBase::GetMatchID() const
{
    // TODO: Handle multiple matches
    return match_info.url.IsEmpty() ? 0 : match_info.match_id;
}


int32 FDriftBase::GetTeamID(int32 match_id, int32 team_index) const
{
    FScopeLock lock{ &matchInfoMutex };
    if (auto matchInfo = matchInfos.Find(match_id))
    {
        if (team_index < matchInfo->teams.Num())
        {
            return matchInfo->teams[team_index].team_id;
        }
    }
    return 0;
}


void FDriftBase::CachePlayerInfo(int32 player_id)
{
    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to cache player info without being connected"));

        return;
    }

    auto entry = serverCounterManagers.Find(player_id);
    if (entry != nullptr && (*entry).IsValid())
    {
        return;
    }

    serverCounterManagers.Add(player_id, MakeUnique<FDriftCounterManager>());

    FString url = driftEndpoints.players;
    internal::UrlHelper::AddUrlOption(url, TEXT("player_id"), FString::Printf(TEXT("%d"), player_id));
    auto request = GetGameRequestManager()->Get(url);
    request->OnResponse.BindLambda([this, player_id](ResponseContext& context, JsonDocument& doc)
    {
        TArray<FDriftPlayerResponse> info;
        if (!JsonArchive::LoadObject(doc, info))
        {
            context.error = L"Failed to parse player info response";
            return;
        }
        if (info.Num() != 1)
        {
            context.error = FString::Printf(TEXT("Expected a single player info, but got %d"), info.Num());
            return;
        }
        auto playerInfo = info[0];
        auto& manager = serverCounterManagers.FindChecked(player_id);
        manager->SetRequestManager(GetGameRequestManager());
        manager->SetCounterUrl(playerInfo.counter_url);
        manager->LoadCounters();

        DRIFT_LOG(Base, Verbose, TEXT("Server cached info for player: %s (%d)"), *playerInfo.player_name, playerInfo.player_id);
    });
    request->OnError.BindLambda([this, player_id](ResponseContext& context)
    {
        DRIFT_LOG(Base, Warning, TEXT("Failed to cache player info for player id: %d"), player_id);

        context.errorHandled = true;
    });
    request->Dispatch();
}


void FDriftBase::CacheFriendInfos(TFunction<void(bool)> delegate)
{
    auto url = driftEndpoints.players;
    internal::UrlHelper::AddUrlOption(url, TEXT("player_group"), TEXT("friends"));
    auto request = GetGameRequestManager()->Get(url);
    request->OnResponse.BindLambda([this, delegate](ResponseContext& context, JsonDocument& doc)
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
    request->OnError.BindLambda([this, delegate](ResponseContext& context)
    {
        DRIFT_LOG(Base, Warning, TEXT("Failed to cache friend infos"));
        
        context.errorHandled = true;
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
        DRIFT_LOG(Base, Warning, TEXT("Failed to update friend infos"));
        
        context.errorHandled = true;
    });
    request->Dispatch();
}


const FDriftPlayerResponse* FDriftBase::GetFriendInfo(int32 player_id) const
{
    return friendInfos.Find(player_id);
}


void FDriftBase::ModifyPlayerCounter(int32 player_id, const FString& counter_name, float value, bool absolute)
{
    auto counterManager = serverCounterManagers.Find(player_id);
    if (counterManager != nullptr && (*counterManager).IsValid())
    {
        (*counterManager).Get()->AddCount(counter_name, value, absolute);
    }
    else
    {
        DRIFT_LOG(Base, Warning, TEXT("Failed to find counters for player ID %d. Please make sure AddPlayerToMatch() has been called first."), player_id);
    }
}


bool FDriftBase::GetPlayerCounter(int32 player_id, const FString& counter_name, float& value)
{
    auto counterManager = serverCounterManagers.Find(player_id);
    if (counterManager != nullptr && (*counterManager).IsValid())
    {
        return (*counterManager).Get()->GetCount(counter_name, value);
    }
    else
    {
        DRIFT_LOG(Base, Warning, TEXT("Failed to find counters for player ID %d. Please make sure AddPlayerToMatch() has been called first."), player_id);
    }
    
    return false;
}


FString FDriftBase::GetApiKeyHeader() const
{
    return FString::Printf(TEXT("%s:%s"), *apiKey, *gameVersion);
}


void FDriftBase::DefaultErrorHandler(ResponseContext& context)
{
    // TODO: make this whole thing data-driven
    auto responseCode = context.responseCode;
    if (responseCode >= static_cast<int32>(HttpStatusCodes::FirstClientError) && responseCode <= static_cast<int32>(HttpStatusCodes::LastClientError))
    {
        GenericRequestErrorResponse response;
        if (JsonUtils::ParseResponse(context.response, response))
        {
            if (response.GetErrorCode() == TEXT("client_session_terminated"))
            {
                auto reason = response.GetErrorReason();
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
                    Disconnect();
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
    else if (responseCode >= (int32)HttpStatusCodes::FirstServerError && responseCode <= (int32)HttpStatusCodes::LastServerError)
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


void FDriftBase::LoadPlayerAvatarUrl(const FDriftLoadPlayerAvatarUrlDelegate& delegate)
{
    if (state_ != DriftSessionState::Connected)
    {
        DRIFT_LOG(Base, Warning, TEXT("Attempting to get avatar url without being connected"));
        delegate.ExecuteIfBound(TEXT(""));
        return;
    }

    check(authProvider.IsValid());

    authProvider->GetAvatarUrl([this, delegate](const FString& avatarUrl)
    {
        delegate.ExecuteIfBound(avatarUrl);
    });
}


#undef LOCTEXT_NAMESPACE
