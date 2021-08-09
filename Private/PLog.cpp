#include "PLog.h"
#include "PLuaFileSystem.h"
#include "PDRslua.h"
#include "PDRLuaObject.h"
#include "PDRLuaCppBinding.h"
#include "CoreMinimal.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"
#include "Runtime/Core/Public/CoreGlobals.h"
#include "Runtime/Core/Public/Async/Async.h"

DEFINE_LOG_CATEGORY(LogPDR)

namespace pandora
{
FString PLog::buffer = TEXT("");
FString PLog::logPath = TEXT("");
bool PLog::enabled = true;

FString PLog::GenerateWrapper()
{
    FDateTime now = FDateTime::Now();
    return FString::Printf(TEXT("[#Level#][%s.%03d]:  #Content#"), *(now.ToString()), now.GetMillisecond());
}

void PLog::SetupLogPath()
{
    logPath = FPaths::Combine(PPaths::GetLogDir(), FString::Printf(TEXT("Pandora-%s.log"), *(FDateTime::Now().ToString())));
}

void PLog::SetEnabled(bool val)
{
    if (IsInGameThread())
    {
        InternalSetEnable(val);
    }
    else
    {
        AsyncTask(ENamedThreads::GameThread, [=]() {
            InternalSetEnable(val);
        });
    }
}

void PLog::LogDebug(const FString & msg)
{
    if (IsInGameThread())
    {
        InternalLogDebug(msg);
    }
    else
    {
        AsyncTask(ENamedThreads::GameThread, [=]() {
            InternalLogDebug(msg);
        });
    }
}

void PLog::LogInfo(const FString & msg)
{
    if (IsInGameThread())
    {
        InternalLogInfo(msg);
    }
    else
    {
        AsyncTask(ENamedThreads::GameThread, [=]() {
            InternalLogInfo(msg);
        });
    }
}

void PLog::LogWarning(const FString & msg)
{
    if (IsInGameThread())
    {
        InternalLogWarning(msg);
    }
    else
    {
        AsyncTask(ENamedThreads::GameThread, [=]() {
            InternalLogWarning(msg);
        });
    }
}

void PLog::LogError(const FString & msg)
{
    if (IsInGameThread())
    {
        InternalLogError(msg);
    }
    else
    {
        AsyncTask(ENamedThreads::GameThread, [=]() {
            InternalLogError(msg);
        });
    }
}

void PLog::Flush()
{
    if (IsInGameThread())
    {
        InternalFlush();
    }
    else
    {
        AsyncTask(ENamedThreads::GameThread, [=]() {
            InternalFlush();
        });
    }
}

FOnLogDispatchedDelegate& PLog::GetOnLogDispatch()
{
    return PLogDispatcher::Get().OnLogDispatchedDelegate;
}

void PLog::Clear()
{
    if (IsInGameThread())
    {
        InternalClear();
    }
    else
    {
        AsyncTask(ENamedThreads::GameThread, [=]() {
            InternalClear();
        });
    }
}

void PLog::InternalSetEnable(bool val)
{
    enabled = val;
}

void PLog::InternalLogDebug(const FString& msg)
{
    if (!enabled)
        return;
    FString wrapper = GenerateWrapper();
    wrapper.ReplaceInline(TEXT("#Level#"), TEXT(" DEBUG "));
    wrapper.ReplaceInline(TEXT("#Content#"), *msg);
    UE_LOG(LogPDR, Log, TEXT("%s"), *wrapper);
    buffer.Append(wrapper + TEXT("\n"));
    Flush();
}

void PLog::InternalLogInfo(const FString& msg)
{
    if (!enabled)
        return;
    FString wrapper = GenerateWrapper();
    wrapper.ReplaceInline(TEXT("#Level#"), TEXT("  LOG  "));
    wrapper.ReplaceInline(TEXT("#Content#"), *msg);
    UE_LOG(LogPDR, Log, TEXT("%s"), *wrapper);
    buffer.Append(wrapper+TEXT("\n"));
    Flush();
}

void PLog::InternalLogWarning(const FString& msg)
{
    if (!enabled)
        return;
    FString wrapper = GenerateWrapper();
    wrapper.ReplaceInline(TEXT("#Level#"), TEXT("WARNING"));
    wrapper.ReplaceInline(TEXT("#Content#"), *msg);
    UE_LOG(LogPDR, Warning, TEXT("%s"), *wrapper);
    buffer.Append(wrapper+TEXT("\n"));
    Flush();
}

void PLog::InternalLogError(const FString& msg)
{
    if (!enabled)
        return;
    FString wrapper = GenerateWrapper();
    wrapper.ReplaceInline(TEXT("#Level#"), TEXT(" ERROR "));
    wrapper.ReplaceInline(TEXT("#Content#"), *msg);
    UE_LOG(LogPDR, Error, TEXT("%s"), *wrapper);
    buffer.Append(wrapper+TEXT("\n"));
    Flush();
}

void PLog::InternalFlush()
{
    PLogDispatcher::Get().DispatchLog(buffer);
    IPlatformFile & platformFile = FPlatformFileManager::Get().GetPlatformFile();
    IFileHandle * handle = platformFile.OpenWrite(*logPath, true, false);
    if (handle == nullptr)
        return;

    FTCHARToUTF8 utf8(*buffer);
    if (!handle->Write(reinterpret_cast<const uint8*>(utf8.Get()), utf8.Length()))
    {
        delete handle;
        return;
    }
    delete handle;
    buffer.Empty();
}

void PLog::InternalClear()
{
    GetOnLogDispatch().Unbind();
}

}

namespace NS_PDR_SLUA
{
    using pandora::PLog;
    DefLuaClass(PLog)
        DefLuaMethod(SetEnabled, &pandora::PLog::SetEnabled)
        DefLuaMethod(Debug, &pandora::PLog::LogDebug)
        DefLuaMethod(Info, &pandora::PLog::LogInfo)
        DefLuaMethod(Warning, &pandora::PLog::LogWarning)
        DefLuaMethod(Error, &pandora::PLog::LogError)
        DefLuaMethod(GetOnLogDispatch, &pandora::PLog::GetOnLogDispatch)
    EndDef(PLog, nullptr)
}
