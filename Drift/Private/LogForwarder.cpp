// Copyright 2016-2019 Directive Games Limited - All Rights Reserved


#include "LogForwarder.h"


DEFINE_LOG_CATEGORY(LogDriftLogs);


static const float FLUSH_LOGS_INTERVAL = 5.0f;


FLogForwarder::FLogForwarder()
{
    GLog->AddOutputDevice(this);
}


FLogForwarder::~FLogForwarder()
{
    if (GLog)
    {
        GLog->RemoveOutputDevice(this);
    }
}


void FLogForwarder::Serialize(const TCHAR* text, ELogVerbosity::Type level, const FName& category)
{
	Log(text, level, category);
}


void FLogForwarder::Tick(float DeltaTime)
{
    if (logsUrl.IsEmpty() || !requestManager.IsValid())
    {
        return;
    }

    flushLogsInSeconds -= DeltaTime;
    if (flushLogsInSeconds > 0.0f)
    {
        return;
    }
    
    FlushLogs();
}


TStatId FLogForwarder::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(FDriftLogForwarder, STATGROUP_Tickables);
}


bool FLogForwarder::Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
    if (FParse::Command(&Cmd, TEXT("RemoteLog")))
    {
        ELogVerbosity::Type level = ELogVerbosity::Log;

        if (FParse::Command(&Cmd, TEXT("Fatal")))
        {
            level = ELogVerbosity::Fatal;
        }
        else if (FParse::Command(&Cmd, TEXT("Error")))
        {
            level = ELogVerbosity::Error;
        }
        else if (FParse::Command(&Cmd, TEXT("Warning")))
        {
            level = ELogVerbosity::Warning;
        }
        else if (FParse::Command(&Cmd, TEXT("Info")))
        {
            level = ELogVerbosity::Log;
        }
        else if (FParse::Command(&Cmd, TEXT("Log")))
        {
            level = ELogVerbosity::Log;
        }

        Serialize(Cmd, level, FName(TEXT("Debug")));

        return true;
    }
#endif
    return false;
}


void FLogForwarder::FlushLogs()
{
	if (!logsUrl.IsEmpty() && requestManager.IsValid() && pendingLogs.Num() > 0)
	{
        UE_LOG(LogDriftLogs, Verbose, TEXT("Flushing %d log entries"), pendingLogs.Num());

        auto rm = requestManager.Pin();
		auto request = rm->Post(logsUrl, pendingLogs);
		request->Dispatch();
		pendingLogs.Empty();
	}

	flushLogsInSeconds += FLUSH_LOGS_INTERVAL;
}


void FLogForwarder::SetRequestManager(TSharedPtr<JsonRequestManager> newRequestManager)
{
    requestManager = newRequestManager;
    flushLogsInSeconds = FLUSH_LOGS_INTERVAL;
}


void FLogForwarder::SetLogsUrl(const FString& newLogsUrl)
{
    logsUrl = newLogsUrl;
}


void FLogForwarder::Log(const TCHAR* text, ELogVerbosity::Type level, const FName& category)
{
    if (GIsEditor && !IsRunningGame())
    {
        return;
    }

    if ((int32)level > (int32)minLogLevel)
    {
        return;
    }

    pendingLogs.Emplace(text, *GetLogLevelName(level), category, FDateTime::UtcNow());
}


const FString& FLogForwarder::GetLogLevelName(ELogVerbosity::Type level) const
{
    static const TMap<ELogVerbosity::Type, FString> LogLevelNames =
    {
        { ELogVerbosity::Fatal, TEXT("Fatal") },
        { ELogVerbosity::Error, TEXT("Error") },
        { ELogVerbosity::Warning, TEXT("Warning") },
        { ELogVerbosity::Display, TEXT("Display") },
        { ELogVerbosity::Log, TEXT("Log") },
        { ELogVerbosity::Verbose, TEXT("Verbose") },
        { ELogVerbosity::VeryVerbose, TEXT("VeryVerbose") }
    };

    if (auto LevelName = LogLevelNames.Find(level))
    {
        return *LevelName;
    }

    static const FString LEVEL_OTHER(TEXT("Other"));
    return LEVEL_OTHER;
}

void FLogForwarder::SetForwardedLogLevel(ELogVerbosity::Type Level)
{
    minLogLevel = Level;
}
