#include "PLuaStateMgr.h"
#include "PLuaTimer.h"
#include "PLuaPlatformInformation.h"
#include "PLuaPakHelper.h"
#include "PLog.h"
#include "PDNSResolver.h"
#include "PLuaMethodExtension.h"
#include "PSDKLuaCoreMgr.h"

#include "Engine/GameInstance.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "PLuaJson.h"
#include "PLuaHttp.h"
#include "PLuaFileSystem.h"
#include "SecureHash.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "Interfaces/IPluginManager.h"
#endif

#define LUACORE_USE_BINARYDATA 1

namespace pandora {

static TArray<FString> luaExts = { UTF8_TO_TCHAR(".lua"), UTF8_TO_TCHAR(".luac"), UTF8_TO_TCHAR(".bytes") };

#if PDR_COMPATIBLE_WITH_OBSOLETED_LOAD_ASSET_METHOD
bool PLuaStateMgr::GetFullFilePath_OBSOLETED(const char* fileName, const char* suffix, FString& filePath)
{
    PLog::LogWarning(FString(TEXT("requiring lua by name is obsoleted, require it by proper format, lua file name: ")) + UTF8_TO_TCHAR(fileName));
    TArray<FString>& mountedFlies = PLuaPakHelper::GetMountedFiles();
    FString fFileName = FString(UTF8_TO_TCHAR(fileName)).ToLower();
    FString fSuffix(UTF8_TO_TCHAR(suffix));
    TArray<FString> tmpArr;
    for (int i = 0; i < mountedFlies.Num(); ++i)
    {
        FString cleanName = FPaths::GetCleanFilename(mountedFlies[i].ToLower());
        tmpArr.Empty();
        cleanName.ParseIntoArray(tmpArr, UTF8_TO_TCHAR("."));
        FString cmpFileName = tmpArr[0];
        if (tmpArr[0] == fFileName.ToLower())
        {
            if (mountedFlies[i].EndsWith(fSuffix))
            {
                filePath = mountedFlies[i];
                return true;
            }
        }
    }
    return false;
}
#endif

bool PLuaStateMgr::GetFileFullPath(const char * fileName, const char * suffix, FString & filePath)
{
    TArray<FString> splitter;
    FString fFileName = FString(UTF8_TO_TCHAR(fileName));

#if WITH_EDITOR
    FString luaFolderName = TEXT("Lua");
#else
    FString luaFolderName = TEXT("Luac");
#endif

    fFileName.ParseIntoArray(splitter, TEXT("/"), true);
    //filePath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Pandora"));
    filePath = PPaths::GetPandoraActContentDir();
    splitter.Insert(luaFolderName, 1);
    for (int i = 0; i < splitter.Num(); ++i)
    {
        if (splitter.Num() - 1 != i)
            filePath = FPaths::Combine(filePath, splitter[i]);
        else
            filePath = FPaths::Combine(filePath, splitter[i] + UTF8_TO_TCHAR(suffix));
    }

    if (FPaths::FileExists(filePath))
    {
        return true;
    }
    else
    {
#if PDR_COMPATIBLE_WITH_OBSOLETED_LOAD_ASSET_METHOD
        return  GetFullFilePath_OBSOLETED(fileName, suffix, filePath);
#endif
    }

    return false;
}

uint8* PLuaStateMgr::ReadFile(const FString& filePath, uint32& len)
{
    auto& platformFile = FPlatformFileManager::Get().GetPlatformFile();
    IFileHandle* fileHandle = platformFile.OpenRead(*filePath);
    if (fileHandle)
    {
        len = (uint32)fileHandle->Size();
        uint8* buf = new uint8[len];
        fileHandle->Read(buf, len);
        delete fileHandle;
        return buf;
    }
    return nullptr;
}

uint8* PLuaStateMgr::LoadSDKCoreChunk(const FString& runtimePath, uint32& len, FString& fileName)
{
#if !LUACORE_USE_BINARYDATA && WITH_EDITOR
    TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin(TEXT("Pandora"));
    if (!plugin.IsValid())
        return nullptr;

    auto luafile = plugin->GetContentDir() + TEXT("/Lua/") + runtimePath;
    for (auto &it : luaExts)
    {
        auto fullPath = luafile + *it;
        auto buf = ReadFile(fullPath, len);
        if (buf)
        {
            fullPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*fullPath);
            // use file path as chunk name
            fileName = fullPath;
            return buf;
        }
    }

    return nullptr;
#else
    //UE_LOG(LogTemp, Log, TEXT("loading chunk: %s"), *runtimePath);
    TSharedPtr<LuaChunk> chunk = PSDKLuaCoreMgr::Get().GetChunkByPath(runtimePath);
    if (!chunk.IsValid()) 
        return nullptr;

    if (chunk->ChunkData.Num() <= 0)
        return nullptr;

    uint8 * buf = new uint8[chunk->ChunkData.Num()];
    memcpy((void*)buf, (void*)chunk->ChunkData.GetData(), chunk->ChunkData.Num());
    len = chunk->ChunkData.Num();
    fileName = runtimePath;
    return buf;

#endif
}

#if WITH_EDITOR
uint8* PLuaStateMgr::LoadEditorFileDelegate(const char* fileName, uint32& len, FString& filePath)
{
    uint8* buf = LoadSDKCoreChunk(fileName, len, filePath);
    if (buf != nullptr)
    {
        return buf;
    }

    if (!GetFileFullPath(fileName, ".lua", filePath))
    {
        // recognize .bytes files on editor
        if (!GetFileFullPath(fileName, ".lua.bytes", filePath))
        {
            return nullptr;
        }
    }

    buf = ReadFile(filePath, len);
    if (buf)
    {
        filePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*filePath);
        return buf;
    }
    return nullptr;
}
#endif

uint8* PLuaStateMgr::LoadPakFileDelegate(const char* fileName, uint32& len, FString& filePath)
{
    uint8* buf = LoadSDKCoreChunk(fileName, len, filePath);
    if (buf != nullptr)
    {
        return buf;
    }

    FPakPlatformFile* platformPakFile = PLuaPakHelper::GetPakPlatformFile();
    if (!platformPakFile)
    {
        return nullptr;
    }

    if (!GetFileFullPath(fileName, ".luac", filePath))
    {
        return nullptr;
    }

    buf = ReadFile(filePath, len);
    if (buf)
    {
        filePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*filePath);
        return buf;
    }
    return nullptr;
}

uint8* PLuaStateMgr::LoadFileDelegate(const char* fileName, uint32& len, FString& filePath)
{
#if WITH_EDITOR
    return LoadEditorFileDelegate(fileName, len, filePath);
#else
    return LoadPakFileDelegate(fileName, len, filePath);
#endif
}

void PLuaStateMgr::OnLuaScriptError (const char* err)
{
    GetMgr().GetLuaState().call("pandora.LuaInterface.ReportLuaScriptErrorToTNM2", err);
    PLog::LogError(FString(UTF8_TO_TCHAR(err)));
}

void PLuaStateMgr::OnLuaStateInit()
{
    extension::LuaMethodExtension();
}

PLuaStateMgr::PLuaStateMgr()
    : _mainState("pandoraMainState"), _initialized(false)
{
    _mainState.onInitEvent.AddRaw(this, &PLuaStateMgr::OnLuaStateInit);
}

PLuaStateMgr::~PLuaStateMgr()
{
    //Close();
}

PLuaStateMgr & PLuaStateMgr::GetMgr()
{
    static PLuaStateMgr _pandoraLuaStateMgr;
    return _pandoraLuaStateMgr;
}

int PLuaStateMgr::Init(UGameInstance * instance)
{
    if (_initialized)
    {
        return 0;
    }
    _gameInstance = instance;
    if (_gameInstance == nullptr)
        return 1;

//#if WITH_EDITOR
//    PLuaPakHelper::SimulateMountedPaks();
//#endif

    auto ret = _mainState.init();
    if (!ret) {
        return 2;
    }

    auto L = _mainState.getLuaState();
    PLuaTimer::RegisterToLua(L);
    PLuaPlatformInformation::RegisterToLua(L);
    PLuaPakHelper::RegisterToLua(L);

    json::lua_init(L);

    _mainState.setLoadFileDelegate(&PLuaStateMgr::LoadFileDelegate);
    _mainState.setErrorDelegate(&PLuaStateMgr::OnLuaScriptError);
    _mainState.doString("require (\"Pandora/Pandora\")");
    _initialized = true;
    return 0;
}

int PLuaStateMgr::Close()
{
    if (!_initialized)
        return 0;

    PLuaTimer::Clear();
    PLuaPakHelper::Clear();
    PDNSResolver::Get().Clear();
    _mainState.close();
    _initialized = false;
    return 0;
}

bool PLuaStateMgr::IsInitialized()
{
    return _initialized;
}

NS_PDR_SLUA::LuaState& PLuaStateMgr::GetLuaState()
{
    return _mainState;
}
}
