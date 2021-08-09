#pragma once

#include "PDRLuaObject.h"

#include "Runtime/PakFile/Public/IPlatformFilePak.h"
#include "Runtime/Core/Public/HAL/PlatformFilemanager.h"
#include "Runtime/Core/Public/Misc/Paths.h"

#include "UnrealString.h"
#include "Containers/Map.h"
#include "Containers/Array.h"

namespace pandora {
//using namespace NS_PDR_SLUA;

class PLuaPakHelper {
public:
    static void RegisterToLua(NS_PDR_SLUA::___pdr_lua_State* L);
    static void Clear();

    static FPakPlatformFile* GetPakPlatformFile();
    static int MountPakFile(const FString& pakFilePath);
#if PDR_COMPATIBLE_WITH_OBSOLETED_LOAD_ASSET_METHOD
    static TMap<FString, FString>& GetMountedFilesMap();
    static TArray<FString>& GetMountedFiles();
#endif

#if WITH_EDITOR
    static int SimulateMountPakFile(const FString& pakFilePath);
#if PDR_COMPATIBLE_WITH_OBSOLETED_LOAD_ASSET_METHOD
    static int SimulateMountPakFile_OBSOLETED(const FString& pakFilePath);
#endif
#endif

private:
    static TArray<FString> _mountedPakPaths;
    static TMap<FString, FString> _pakMountPointMap;
    static TMap<FString, FString> _pakEntranceFileMap;
#if PDR_COMPATIBLE_WITH_OBSOLETED_LOAD_ASSET_METHOD
    static TMap<FString, FString> _mountedFilesMap;
    static TArray<FString> _mountedFiles;
#endif

    static int Mount(NS_PDR_SLUA::___pdr_lua_State *L);
    static int UnmountAll(NS_PDR_SLUA::___pdr_lua_State* L);
    static int GetMountPoint(NS_PDR_SLUA::___pdr_lua_State* L);
    static int GetEntranceFilePath(NS_PDR_SLUA::___pdr_lua_State* L);
    static int GetModuleNameByPakName(NS_PDR_SLUA::___pdr_lua_State* L);

#if PDR_COMPATIBLE_WITH_OBSOLETED_LOAD_ASSET_METHOD
    static int GetFileMountPath(NS_PDR_SLUA::___pdr_lua_State* L);
#endif
};

}
