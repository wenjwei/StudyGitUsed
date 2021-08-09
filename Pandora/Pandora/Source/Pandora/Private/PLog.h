#pragma once

#include "UnrealString.h"
#include "PLogDispatcher.h"

#include "Logging/LogVerbosity.h"
#include "Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPDR, Log, All)
DEFINE_LOG_CATEGORY(LogPDR)


namespace pandora
{
class PLog
{
private:
    static FString buffer;
    static FString logPath;
    static bool enabled;

    static FString GenerateWrapper();

public:
    static void SetupLogPath();
    static void SetEnabled(bool);
    static void LogDebug(const FString & msg);
    static void LogInfo(const FString & msg);
    static void LogWarning(const FString & msg);
    static void LogError(const FString & msg);
    static void Flush();
    static void Clear();
    static FOnLogDispatchedDelegate& GetOnLogDispatch();

private:
    static void InternalSetEnable(bool);
    static void InternalLogDebug(const FString& msg);
    static void InternalLogInfo(const FString& msg);
    static void InternalLogWarning(const FString& msg);
    static void InternalLogError(const FString& msg);
    static void InternalFlush();
    static void InternalClear();
};
}

