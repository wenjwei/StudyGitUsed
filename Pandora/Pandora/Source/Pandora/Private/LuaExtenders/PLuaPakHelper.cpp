#include "PLuaPakHelper.h"
#include "PLuaStateMgr.h"
#include "PLog.h"
#include "PLuaFileSystem.h"

#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/FileManager.h"

namespace pandora {
using namespace NS_PDR_SLUA;

TArray<FString> PLuaPakHelper::_mountedPakPaths;
TMap<FString, FString> PLuaPakHelper::_pakMountPointMap;
TMap<FString, FString> PLuaPakHelper::_pakEntranceFileMap;
#if PDR_COMPATIBLE_WITH_OBSOLETED_LOAD_ASSET_METHOD
TMap<FString, FString> PLuaPakHelper::_mountedFilesMap;
TArray<FString> PLuaPakHelper::_mountedFiles;
#endif

FPakPlatformFile* PLuaPakHelper::GetPakPlatformFile() 
{
    FPakPlatformFile* platformPakFile = static_cast<FPakPlatformFile*>(FPlatformFileManager::Get().GetPlatformFile(UTF8_TO_TCHAR("PakFile")));
    return platformPakFile;
}

#if PDR_COMPATIBLE_WITH_OBSOLETED_LOAD_ASSET_METHOD
TMap<FString, FString>& PLuaPakHelper::GetMountedFilesMap()
{
    return _mountedFilesMap;
}

TArray<FString>& PLuaPakHelper::GetMountedFiles()
{
    return _mountedFiles;
}
#endif

#if WITH_EDITOR
int PLuaPakHelper::SimulateMountPakFile(const FString& pakFilePath)
{
    // simulate mount paks here
    // we do not really mount on editor, just fill some info into some array
    TArray<FString> splitter;
    FString pakName = FPaths::GetCleanFilename(pakFilePath);
    pakName.ParseIntoArray(splitter, TEXT("_"));
    if (splitter.Num() < 2)
    {
        //PLog::LogWarning(".pak file name invalid");
        return 1;
    }
    FString moduleName = splitter[0].ToLower();
    FString pandoraRoot = PPaths::GetPandoraActContentDir();
    FString findPath = FPaths::Combine(pandoraRoot, TEXT("*"));
    IFileManager& mgr = IFileManager::Get();
    TArray<FString> files;
    mgr.FindFiles(files, *findPath, false, true);
    for (int i = 0; i < files.Num(); ++i)
    {
        if (files[i].ToLower() == moduleName)
        {
            // module folder found
            _pakMountPointMap.Add(pakName, FPaths::Combine(pandoraRoot, files[i]));
            _pakEntranceFileMap.Add(pakName, FPaths::Combine(pandoraRoot, files[i], TEXT("Lua"), files[i] + TEXT(".lua")));
        }
    }

    return 0;
}
#if PDR_COMPATIBLE_WITH_OBSOLETED_LOAD_ASSET_METHOD
int PLuaPakHelper::SimulateMountPakFile_OBSOLETED(const FString& pakFilePath)
{
    TArray<FString> splitter;
    FString pakName = FPaths::GetCleanFilename(pakFilePath);
    pakName.ParseIntoArray(splitter, TEXT("_"));
    if (splitter.Num() < 2)
    {
        return 1;
    }
    FString moduleName = splitter[0].ToLower();
    FString pandoraRoot = PPaths::GetPandoraActContentDir();
    FString findPath = FPaths::Combine(pandoraRoot, TEXT("*"));
    IFileManager& mgr = IFileManager::Get();
    TArray<FString> files;
    mgr.FindFiles(files, *findPath, false, true);
    FString modulePath;
    for (int i = 0; i < files.Num(); ++i)
    {
        if (files[i].ToLower() == moduleName)
        {
            modulePath = FPaths::Combine(pandoraRoot, files[i]);
            break;
        }
    }

    files.Empty();
    mgr.FindFilesRecursive(files, *modulePath, TEXT("*.*"), true, false);
    for (int i = 0; i < files.Num(); ++i)
    {
        _mountedFilesMap.Add(FPaths::GetCleanFilename(files[i]), files[i]);
        _mountedFiles.Add(files[i]);
    }
    return 0;
}
#endif
#endif
    
int PLuaPakHelper::MountPakFile(const FString& pakFilePath) 
{
#if WITH_EDITOR
#if PDR_COMPATIBLE_WITH_OBSOLETED_LOAD_ASSET_METHOD
    SimulateMountPakFile_OBSOLETED(pakFilePath);
#endif
    return SimulateMountPakFile(pakFilePath);
#else
    if (_mountedPakPaths.Contains(pakFilePath))
    {
        return -1;
    }

    if (!FPaths::FileExists(pakFilePath))
    {
        return -2;
    }

    FPakPlatformFile* platformPakFile = GetPakPlatformFile();
    if (!platformPakFile)
    {
        return -3;
    }

    if (!platformPakFile->Mount(*pakFilePath, 0, nullptr))
    {
        return -4;
    }

    _mountedPakPaths.AddUnique(pakFilePath);
    bool bSigned = false;
    FPakFile pakFile(platformPakFile, *pakFilePath, bSigned);
    FString mountPoint = pakFile.GetMountPoint();
    PLog::LogInfo(FString(TEXT("MountPoint: ")) + mountPoint);
    FString pakName = FPaths::GetCleanFilename(pakFilePath);
    if (!_pakMountPointMap.Contains(pakName))
        _pakMountPointMap.Add(pakName, mountPoint);

    TArray<FString> splitter;
    mountPoint.ParseIntoArray(splitter, *(FString(TEXT("Content/")) + PPaths::GetPandoraFolderName()));
    if (splitter.Num() >= 2)
    {
        FString projPart = splitter[0];
        FString modulePart = splitter[1];
        modulePart.ParseIntoArray(splitter, TEXT("/"));
        if (splitter.Num() >= 1)
            if (!_pakEntranceFileMap.Contains(pakName))
            {
                _pakEntranceFileMap.Add(
                    pakName,
                    FPaths::Combine(projPart, TEXT("Content"), PPaths::GetPandoraFolderName(), splitter[0], TEXT("Luac"), splitter[0] + TEXT(".luac"))
                );
            }

    }
#if PDR_COMPATIBLE_WITH_OBSOLETED_LOAD_ASSET_METHOD
    TArray<FString> fileList;
    pakFile.FindFilesAtPath(fileList, *pakFile.GetMountPoint(), true, false, true);
    _mountedFiles += fileList;
    for (int i = 0; i < fileList.Num(); ++i)
    {
        FString cleanFileName = FPaths::GetCleanFilename(fileList[i]);
        if (_mountedFilesMap.Contains(cleanFileName))
        {
            FString msg = FString(TEXT("File name duplicated, duplicated file: ")) + fileList[i];
            PLog::LogWarning(msg);
            continue;
        }
        _mountedFilesMap.Add(cleanFileName, fileList[i]);
    }
#endif
    return 0;
#endif
}

//int PLuaPakHelper::UnmountPakFile(const FString& pakFilePath)
//{
//    if (!_mountedPakPaths.Contains(*pakFilePath))
//    {
//        return 0;
//    }
//    FString pakName = FPaths::GetCleanFilename(pakFilePath);
//    FString mountPoint = TEXT("");
//    if (_pakMountPointMap.Contains(pakName))
//    {
//        mountPoint = _pakMountPointMap[pakName];
//    }
//    _pakEntranceFileMap.Remove(pakName);
//    _pakMountPointMap.Remove(pakName);
//#if PDR_COMPATIBLE_WITH_OBSOLETED_LOAD_ASSET_METHOD
//    _mountedFilesMap.Remove(pakName);
//    // clear file map
//    if (!mountPoint.IsEmpty())
//    {
//        TArray<FString> files;
//        files.Empty();
//        mgr.FindFilesRecursive(files, *mountPoint, TEXT("*.*"), true, false);
//        for (int i = 0; i < files.Num(); ++i)
//        {
//            _mountedFiles.Remove(files[i]);
//            _mountedFilesMap.Remove(FPaths::GetCleanFilename(files[i]));
//        }
//    }
//#endif
//    FPakPlatformFile* platformPakFile = GetPakPlatformFile();
//    if (!platformPakFile)
//    {
//        return 0;
//    }
//    platformPakFile->Unmount(*pakFilePath);
//    _mountedPakPaths.Remove(pakFilePath);
//
//    return 0;
//}

int PLuaPakHelper::Mount(___pdr_lua_State* L)
{
    const char* pakFileStr = ___pdr_luaL_checkstring(L, 1);
    if (!pakFileStr) 
    {
        ___pdr_lua_pushinteger(L, -9);
        return 1;
    }

    FString pakFileFStr(UTF8_TO_TCHAR(pakFileStr));
    int ret = PLuaPakHelper::MountPakFile(pakFileFStr);
    ___pdr_lua_pushinteger(L, ret);
    return 1;
}

//int PLuaPakHelper::Unmount(lua_State* L)
//{
//    const char* pakFileStr = luaL_checkstring(L, 1);
//    if (!pakFileStr)
//    {
//        lua_pushinteger(L, -9);
//        return 1;
//    }
//
//    FString pakFileFStr(UTF8_TO_TCHAR(pakFileStr));
//    int ret = PLuaPakHelper::UnmountPakFile(pakFileFStr);
//    lua_pushinteger(L, ret);
//    return 1;
//}

int PLuaPakHelper::UnmountAll(___pdr_lua_State* L)
{
    PLuaPakHelper::Clear();
    return 0;
}

int PLuaPakHelper::GetMountPoint(___pdr_lua_State* L)
{
    const char* name = ___pdr_luaL_checkstring(L, 1);
    FString fname(UTF8_TO_TCHAR(name));
    if (!_pakMountPointMap.Contains(fname))
    {
        ___pdr_lua_pushnil(L);
        return 1;
    }

    FString path = _pakMountPointMap[fname];
    FTCHARToUTF8 parser(*path);
    ___pdr_lua_pushlstring(L, parser.Get(), parser.Length());
    return 1;
}

int PLuaPakHelper::GetEntranceFilePath(___pdr_lua_State* L)
{
    const char* name = ___pdr_luaL_checkstring(L, 1);
    FString fname(UTF8_TO_TCHAR(name));
    if (!_pakEntranceFileMap.Contains(fname))
    {
        ___pdr_lua_pushnil(L);
        return 1;
    }

    FString path = _pakEntranceFileMap[fname];
    FTCHARToUTF8 parser(*path);
    ___pdr_lua_pushlstring(L, parser.Get(), parser.Length());
    return 1;
}

int PLuaPakHelper::GetModuleNameByPakName(___pdr_lua_State* L)
{
    const char* name = ___pdr_luaL_checkstring(L, 1);
    FString fname(UTF8_TO_TCHAR(name));
    if (!_pakEntranceFileMap.Contains(fname))
    {
        ___pdr_lua_pushnil(L);
        return 1;
    }

    FString entrancePath = _pakEntranceFileMap[fname];
    FString moduleName = FPaths::GetBaseFilename(entrancePath);
    FTCHARToUTF8 parser(*moduleName);
    ___pdr_lua_pushlstring(L, parser.Get(), parser.Length());
    return 1;
}

#if PDR_COMPATIBLE_WITH_OBSOLETED_LOAD_ASSET_METHOD
int PLuaPakHelper::GetFileMountPath(___pdr_lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    FString fname(UTF8_TO_TCHAR(name));
    if (!_mountedFilesMap.Contains(fname))
    {
        ___pdr_lua_pushnil(L);
        return 1;
    }

    FString mountPath = _mountedFilesMap[fname];
    FTCHARToUTF8 parser(*mountPath);
    ___pdr_lua_pushlstring(L, parser.Get(), parser.Length());
    return 1;
}
#endif

void PLuaPakHelper::RegisterToLua(___pdr_lua_State* L) 
{
    ___pdr_lua_newtable(L);
    RegMetaMethodByName(L, "Mount", Mount);
    //RegMetaMethodByName(L, "Unmount", Unmount)
    RegMetaMethodByName(L, "GetMountPoint", GetMountPoint);
    RegMetaMethodByName(L, "GetEntranceFilePath", GetEntranceFilePath);
    RegMetaMethodByName(L, "GetModuleNameByPakName", GetModuleNameByPakName);
    RegMetaMethodByName(L, "UnmountAll", UnmountAll);

#if PDR_COMPATIBLE_WITH_OBSOLETED_LOAD_ASSET_METHOD
    RegMetaMethodByName(L, "GetFileMountPath", GetFileMountPath);
#endif

    ___pdr_lua_setglobal(L, "PLuaPakHelper");
}

void PLuaPakHelper::Clear() 
{
    FPakPlatformFile* platformPakFile = GetPakPlatformFile();
    if (!platformPakFile) 
    {
        return;
    }

    for (int i = 0; i < _mountedPakPaths.Num(); ++i)
    {
        platformPakFile->Unmount(*(_mountedPakPaths[i]));
    }
    _mountedPakPaths.Empty();

#if PDR_COMPATIBLE_WITH_OBSOLETED_LOAD_ASSET_METHOD
    _mountedFilesMap.Empty();
    _mountedFiles.Empty();
#endif

    _pakMountPointMap.Empty();
    _pakEntranceFileMap.Empty();
}
}
