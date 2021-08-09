
#include "PLuaFileSystem.h"
#include "PDRLuaObject.h"
#include "PDRLuaCppBinding.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"
#include "FileHelper.h"
#include "Paths.h"
#include "cppbuffer.h"
#include "luaadapter.h"


namespace pandora
{
bool PPlatformFileWrapper::FileExists(const FString & path)
{
    IPlatformFile& platform = FPlatformFileManager::Get().GetPlatformFile();
    return platform.FileExists(*path);
}

int64 PPlatformFileWrapper::FileSize(const FString & path)
{
    IPlatformFile& platform = FPlatformFileManager::Get().GetPlatformFile();
    return platform.FileSize(*path);
}

bool PPlatformFileWrapper::DeleteFile(const FString & path)
{
    IPlatformFile& platform = FPlatformFileManager::Get().GetPlatformFile();
    return platform.DeleteFile(*path);
}

bool PPlatformFileWrapper::DirectoryExists(const FString & path)
{
    IPlatformFile& platform = FPlatformFileManager::Get().GetPlatformFile();
    return platform.DirectoryExists(*path);
}

bool PPlatformFileWrapper::CreateDirectory(const FString & path)
{
    IPlatformFile& platform = FPlatformFileManager::Get().GetPlatformFile();
    return platform.CreateDirectory(*path);
}

bool PPlatformFileWrapper::DeleteDirectory(const FString & path)
{
    IPlatformFile& platform = FPlatformFileManager::Get().GetPlatformFile();
    return platform.DeleteDirectory(*path);
}

bool PPlatformFileWrapper::SaveStringToFile(const FString & path, const FString & content)
{
    return FFileHelper::SaveStringToFile(content, *path, FFileHelper::EEncodingOptions::ForceUTF8);
}

bool PPlatformFileWrapper::LoadFileToString(const FString & path, FString & content)
{
    return FFileHelper::LoadFileToString(content, *path);
}

bool PPlatformFileWrapper::SaveArrayToFile(const FString & path, const TArray<uint8> & content)
{
    return FFileHelper::SaveArrayToFile(content, *path);
}

bool PPlatformFileWrapper::LoadFileToArray(const FString & path, TArray<uint8>& content)
{
    return FFileHelper::LoadFileToArray(content, *path);
}

uint32 PPlatformFileWrapper::CopyFile(const FString & dest, const FString & src, bool replace)
{
    return IFileManager::Get().Copy(*dest, *src, replace);
}

FString PPlatformFileWrapper::ToAbsolutePathForRead(const FString & path)
{
#if PLATFORM_ANDROID
    return IAndroidPlatformFile::GetPlatformPhysical().FileRootPath(*path);
#elif PLATFORM_IOS
    return FPlatformFileManager::Get().GetPlatformFile().GetPlatformPhysical().ConvertToAbsolutePathForExternalAppForRead(*path);
#else
    return FPaths::ConvertRelativePathToFull(*path);
#endif
}

FString PPlatformFileWrapper::ToAbsolutePathForWrite(const FString & path)
{
#if PLATFORM_ANDROID
    return IAndroidPlatformFile::GetPlatformPhysical().FileRootPath(*path);
#elif PLATFORM_IOS
    return FPlatformFileManager::Get().GetPlatformFile().GetPlatformPhysical().ConvertToAbsolutePathForExternalAppForWrite(*path);
#else
    return FPaths::ConvertRelativePathToFull(*path);
#endif
}

//FString PPlatformFileWrapper::ToRelativePath(const FString & path)
//{
//    IFileManager& manager = IFileManager::Get();
//    return manager.ConvertToRelativePath(*path);
//}

FString PPaths::GetPandoraFolderName()
{
    return TEXT("Pandora");
}

FString PPaths::GetPandoraActContentDir()
{
    return FPaths::Combine(FPaths::ProjectContentDir(), GetPandoraFolderName());
}

FString PPaths::GetPandoraDir()
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), GetPandoraFolderName());
    //return FPaths::ProjectSavedDir() + GetPandoraFolderName() + TEXT("/");
}

FString PPaths::GetLogDir()
{
    //return GetPandoraDir() + TEXT("Logs/");
    return FPaths::Combine(GetPandoraDir(), TEXT("Logs"));
}

FString PPaths::GetPaksDir()
{
    //return GetPandoraDir() + TEXT("Paks/");
    return FPaths::Combine(GetPandoraDir(), TEXT("Paks"));
}

FString PPaths::GetCacheDir()
{
    //return GetPandoraDir() + TEXT("Caches/");
    return FPaths::Combine(GetPandoraDir(), TEXT("Caches"));
}

FString PPaths::GetCookiesDir()
{
    //return GetPandoraDir() + TEXT("Cookies/");
    return FPaths::Combine(GetPandoraDir(), TEXT("Cookies"));
}

FString PPaths::GetCleanFilename(const FString & url)
{
    return FPaths::GetCleanFilename(url);
}

FString PPaths::GetBaseFilename(const FString & url)
{
    return FPaths::GetBaseFilename(url);
}

void PPaths::SetupPaths()
{
    TArray<FString> paths;
    paths.Add(GetPandoraDir());
    paths.Add(GetLogDir());
    paths.Add(GetPaksDir());
    paths.Add(GetCacheDir());
    paths.Add(GetCookiesDir());

    for (int i = 0; i < paths.Num(); ++i)
    {
        if (!PPlatformFileWrapper::DirectoryExists(paths[i]))
        {
            PPlatformFileWrapper::CreateDirectory(paths[i]);
        }
    }
}
} // namespace pandora

namespace NS_PDR_SLUA
{
using pandora::PPlatformFileWrapper;
using pandora::PPaths;

DefLuaClass(PPlatformFileWrapper)
    DefLuaMethod(FileExists, &PPlatformFileWrapper::FileExists)
    DefLuaMethod(FileSize, &PPlatformFileWrapper::FileSize)
    DefLuaMethod(DeleteFile, &PPlatformFileWrapper::DeleteFile)
    DefLuaMethod(DirectoryExists, &PPlatformFileWrapper::DirectoryExists)
    DefLuaMethod(CreateDirectory, &PPlatformFileWrapper::CreateDirectory)
    DefLuaMethod(DeleteDirectory, &PPlatformFileWrapper::DeleteDirectory)
    DefLuaMethod(SaveStringToFile, &PPlatformFileWrapper::SaveStringToFile)
    DefLuaMethod_With_Imp(LoadFileToString, true, {
        FString path = LuaObject::checkValue<FString>(lstack, 1);
        FString content;
        bool isok = PPlatformFileWrapper::LoadFileToString(path, content);
        FTCHARToUTF8 converter(*content);
        ___pdr_lua_pushlstring(lstack, converter.Get(), converter.Length());
        ___pdr_lua_pushboolean(lstack, isok);
        return 2;
    })
    // DefLuaMethod(SaveArrayToFile, &PPlatformFileWrapper::SaveArrayToFile)
    // DefLuaMethod_With_Imp(LoadFileToArray, true, {
    //     FString path = LuaObject::checkValue<FString>(lstack, 1);
    //     TSharedPtr<TArray<uint8>> arr = MakeShareable(new TArray<uint8>);
    //     bool isok = PPlatformFileWrapper::LoadFileToArray(path, *arr);
    //     LuaObject::push(lstack, arr);
    //     lua_pushboolean(lstack, isok);
    //     return 2;
    // })
    DefLuaMethod(CopyFile, &PPlatformFileWrapper::CopyFile)
    DefLuaMethod(ToAbsolutePathForRead, &PPlatformFileWrapper::ToAbsolutePathForRead)
    DefLuaMethod(ToAbsolutePathForWrite, &PPlatformFileWrapper::ToAbsolutePathForWrite)
    //DefLuaMethod(ToRelativePath, &PPlatformFileWrapper::ToRelativePath)
    DefLuaMethod_With_Imp(GetFileListUnderDir, true, {
        FString dirpath = LuaObject::checkValue<FString>(lstack, 1);
        FString filter = LuaObject::checkValue<FString>(lstack, 2);
        bool getFile = LuaObject::checkValue<bool>(lstack, 3);
        bool getDir = LuaObject::checkValue<bool>(lstack, 4);
        bool recursive = LuaObject::checkValue<bool>(lstack, 5);
        IFileManager & manager = IFileManager::Get();
        TArray<FString> files;
        if (recursive)
            manager.FindFilesRecursive(files, *dirpath, *filter, getFile, getDir, false);
        else
        {
            if (dirpath.EndsWith(TEXT("/")) || dirpath.EndsWith(TEXT("\\")))
            {
                manager.FindFiles(files, *(dirpath + filter), getFile, getDir);
                for (int i = 0; i < files.Num(); ++i)
                    files[i] = dirpath + files[i];
            }
            else
            {
                manager.FindFiles(files, *(dirpath + TEXT("/") + filter), getFile, getDir);
                for (int i = 0; i < files.Num(); ++i)
                    files[i] = dirpath + TEXT("/") + files[i];
            }
        }
        ___pdr_lua_newtable(lstack);
        for (int i = 0; i < files.Num(); ++i)
        {
            ___pdr_lua_pushinteger(lstack, i + 1);
            FTCHARToUTF8 converter(*files[i]);
            ___pdr_lua_pushlstring(lstack, converter.Get(), converter.Length());
            ___pdr_lua_rawset(lstack, -3);
        }
        return 1;
    })
    DefLuaMethod_With_Imp(SaveLBufferToFile, true, {
        if (!___pdr_lua_isstring(lstack, 1)) ___pdr_luaL_argcheck(lstack, false, 1, "'string' expected");
        NS_CPPBUFFER::p_cppbuffer buffer = NS_CB_LUAADAPTER::checkbuffer(lstack, 2);
        FString fpath(UTF8_TO_TCHAR(___pdr_luaL_checkstring(lstack, 1)));
        size_t len = NS_CPPBUFFER::cb_get_bytes_count(buffer);
        char * data = new char[len];
        NS_CPPBUFFER::cb_read_raw(buffer, data, len, 0);
        IPlatformFile& platform = FPlatformFileManager::Get().GetPlatformFile();
        IFileHandle * handle = platform.OpenWrite(*fpath);
        int err = 0;
        if (!handle)
        {
            delete[] data;
            ___pdr_lua_pushinteger(lstack, -1);
            return 1;
        }
        err = handle->Write(reinterpret_cast<uint8*>(data), len) ? 0 : -1;
        if (err == 0)
            handle->Flush();
        delete[]data;
        delete handle;
        ___pdr_lua_pushinteger(lstack, err);
        return 1;
    })
    DefLuaMethod_With_Imp(LoadFileToLBuffer, true, {
        if (!___pdr_lua_isstring(lstack, 1)) ___pdr_luaL_argcheck(lstack, false, 1, "'string' expected");
        FString fpath(UTF8_TO_TCHAR(___pdr_luaL_checkstring(lstack, 1)));
        IPlatformFile& platform = FPlatformFileManager::Get().GetPlatformFile();
        size_t filesize = platform.FileSize(*fpath);
        IFileHandle * handle = platform.OpenRead(*fpath);
        int err = 0;
        if (!handle)
        {
            ___pdr_lua_pushnil(lstack);
            ___pdr_lua_pushinteger(lstack, -1);
            return 2;
        }
        NS_CPPBUFFER::p_cppbuffer buffer = NS_CB_LUAADAPTER::newlbuffer(lstack, filesize);
        err = handle->Read(reinterpret_cast<uint8*>(buffer->buf), filesize) ? 0 : 1;
        buffer->count = filesize;
        buffer->pwrite = filesize % buffer->capacity;
        delete handle;
        ___pdr_lua_pushinteger(lstack, err);
        return 2;
    })
    DefLuaMethod_With_Imp(GetFileStats, true, {
        if (!___pdr_lua_isstring(lstack, 1)) ___pdr_luaL_argcheck(lstack, false, 1, "'string' expected");
        FString fpath(UTF8_TO_TCHAR(___pdr_luaL_checkstring(lstack, 1)));
        IPlatformFile& platform = FPlatformFileManager::Get().GetPlatformFile();
        int err = 0;
        if (!platform.FileExists(*fpath) && !platform.DirectoryExists(*fpath))
        {
            ___pdr_lua_pushnil(lstack);
            ___pdr_lua_pushinteger(lstack, -1);
            return 2;
        }       
        FFileStatData stats = platform.GetStatData(*fpath);
        ___pdr_lua_newtable(lstack);
        ___pdr_lua_pushinteger(lstack, stats.AccessTime.ToUnixTimestamp());
        ___pdr_lua_setfield(lstack, -2, "AccessTime");
        ___pdr_lua_pushinteger(lstack, stats.CreationTime.ToUnixTimestamp());
        ___pdr_lua_setfield(lstack, -2, "CreationTime");
        ___pdr_lua_pushinteger(lstack, stats.ModificationTime.ToUnixTimestamp());
        ___pdr_lua_setfield(lstack, -2, "ModificationTime");
        ___pdr_lua_pushinteger(lstack, stats.FileSize);
        ___pdr_lua_setfield(lstack, -2, "FileSize");
        ___pdr_lua_pushboolean(lstack, stats.bIsDirectory);
        ___pdr_lua_setfield(lstack, -2, "bIsDirectory");
        ___pdr_lua_pushboolean(lstack, stats.bIsReadOnly);
        ___pdr_lua_setfield(lstack, -2, "bIsReadOnly");
        ___pdr_lua_pushboolean(lstack, stats.bIsValid);
        ___pdr_lua_setfield(lstack, -2, "bIsValid");
        ___pdr_lua_pushinteger(lstack, 0);
        return 2;
    })
EndDef(PPlatformFileWrapper, nullptr)

DefLuaClass(PPaths)
    DefLuaMethod(LaunchDir, &FPaths::LaunchDir)
    DefLuaMethod(RootDir, &FPaths::RootDir)
    DefLuaMethod(ProjectDir, &FPaths::ProjectDir)
    DefLuaMethod(ProjectUserDir, &FPaths::ProjectUserDir)
    DefLuaMethod(ProjectContentDir, &FPaths::ProjectContentDir)
    DefLuaMethod(ProjectConfigDir, &FPaths::ProjectConfigDir)
    DefLuaMethod(ProjectSavedDir, &FPaths::ProjectSavedDir)
    DefLuaMethod(ProjectIntermediateDir, &FPaths::ProjectIntermediateDir)
    DefLuaMethod(ProjectPluginsDir, &FPaths::ProjectPluginsDir)
    DefLuaMethod(ProjectModsDir, &FPaths::ProjectModsDir)
    DefLuaMethod(GetPandoraFolderName, &PPaths::GetPandoraFolderName)
    DefLuaMethod(GetPandoraActContentDir, &PPaths::GetPandoraActContentDir)
    DefLuaMethod(GetPandoraDir, &PPaths::GetPandoraDir)
    DefLuaMethod(GetLogDir, &PPaths::GetLogDir)
    DefLuaMethod(GetPaksDir, &PPaths::GetPaksDir)
    DefLuaMethod(GetCacheDir, &PPaths::GetCacheDir)
    DefLuaMethod(GetCookiesDir, &PPaths::GetCookiesDir)
    DefLuaMethod(GetCleanFilename, &PPaths::GetCleanFilename)
    DefLuaMethod(GetBaseFilename, &PPaths::GetBaseFilename)
    DefLuaMethod_With_Imp(Combine, true, {
        int nargs = ___pdr_lua_gettop(lstack);
        FString result = TEXT("");
        for (int i = 0; i < nargs; ++i)
        {
            if (!___pdr_lua_isstring(lstack, i + 1)) ___pdr_luaL_argcheck(lstack, false, i + 1, "'string' expected");
            result = FPaths::Combine(result, UTF8_TO_TCHAR(___pdr_lua_tostring(lstack, i + 1)));
        }
        FTCHARToUTF8 parser(*result);
        ___pdr_lua_pushlstring(lstack, parser.Get(), parser.Length());
        return 1;
    })
EndDef(PPaths, nullptr)
}

