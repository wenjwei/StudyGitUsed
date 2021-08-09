#pragma once

#include "PDRLuaState.h"
#include "CoreMinimal.h"
#include "TimerManager.h"
#include "PDRLuaObject.h"
#include "PDRLuaVar.h"

class UGameInstance;
class IPlatformFile;
class FString;

namespace pandora
{

class PANDORA_API PLuaStateMgr
{
public:
    virtual ~PLuaStateMgr();

    static PLuaStateMgr& GetMgr();
    static UGameInstance* GetGameInstance() {
        return GetMgr()._gameInstance;
    }

    int Init(UGameInstance *);
    int Close();
    bool IsInitialized();

    NS_PDR_SLUA::LuaState & GetLuaState();

private:
    NS_PDR_SLUA::LuaState _mainState;
    UGameInstance* _gameInstance;
    bool _initialized;

    PLuaStateMgr();
#if PDR_COMPATIBLE_WITH_OBSOLETED_LOAD_ASSET_METHOD
    static bool  GetFullFilePath_OBSOLETED(const char* fileName, const char* suffix, FString& filePath);
#endif
    static bool GetFileFullPath(const char* fileName, const char* suffix, FString& filePath);
    static uint8* ReadFile(const FString& filePath, uint32& len);
    static uint8* LoadSDKCoreChunk(const FString& runtimePath, uint32& len, FString& fileName);
#if WITH_EDITOR
    static uint8* LoadEditorFileDelegate(const char* fileName, uint32& len, FString& filePath);
#endif
    static uint8* LoadPakFileDelegate(const char* fileName, uint32& len, FString& filePath);
    static uint8* LoadFileDelegate(const char* fileName, uint32& len, FString& filePath);
    static void OnLuaScriptError(const char* err);
    void OnLuaStateInit();
};
}

