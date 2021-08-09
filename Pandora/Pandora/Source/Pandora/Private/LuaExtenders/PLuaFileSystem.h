#pragma once

#include "Containers/UnrealString.h"
#include "SharedPointer.h"
#include "Containers/Array.h"


namespace pandora
{
class PPlatformFileWrapper
{
public:
    static bool FileExists(const FString & path);
    static int64 FileSize(const FString & path);
    static bool DeleteFile(const FString & path);
    static bool DirectoryExists(const FString & path);
    static bool CreateDirectory(const FString & path);
    static bool DeleteDirectory(const FString & path);
    static bool SaveStringToFile(const FString & path, const FString & content);
    static bool LoadFileToString(const FString & path, FString& content);
    static bool SaveArrayToFile(const FString & path, const TArray<uint8> & content);
    static bool LoadFileToArray(const FString & path, TArray<uint8>& content);
    static uint32 CopyFile(const FString & dest, const FString & src, bool replace);
    static FString ToAbsolutePathForRead(const FString & path);
    static FString ToAbsolutePathForWrite(const FString & path);
    //static FString ToRelativePath(const FString & path);
};

class PPaths
{
public:
    static FString GetPandoraFolderName();
    static FString GetPandoraActContentDir();
    static FString GetPandoraDir();
    static FString GetLogDir();
    static FString GetPaksDir();
    static FString GetCacheDir();
    static FString GetCookiesDir();
	static FString GetCleanFilename(const FString & url);
	static FString GetBaseFilename(const FString & url);

    static void SetupPaths();
};
}
