#include "PSDKLuaCoreMgr.h"
#include "CoreMinimal.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "PLog.h"
#include "Data/PDefaultLuaCoreData.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Runtime/Core/Public/HAL/UnrealMemory.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#include <WinSock2.h>
#include "HideWindowsPlatformTypes.h"
#else
#include <arpa/inet.h>
#endif

#define PDR_PATCH_FILE_VERSION_PREFIX "version"
#define PDR_PATCH_FILE_VERSION_PREFIX_LEN 7
#define PDR_PATCH_FILE_VERSION_SECTION_LEN 11


namespace pandora
{

PSDKLuaCoreMgr& PSDKLuaCoreMgr::Get()
{
    static PSDKLuaCoreMgr ___sdkCoreMgrInst;
    return ___sdkCoreMgrInst;
}

void PSDKLuaCoreMgr::SetOverrideDataFilePath(const FString& path)
{
    _overrideDataFilePath = path;
}

int PSDKLuaCoreMgr::GetMinPatchVersion()
{
    return PDR_PATCH_FILE_MIN_VERSION;
}

int PSDKLuaCoreMgr::Reload()
{
    TArray<uint8> data;
    bool readOk = false;
    if (!_overrideDataFilePath.IsEmpty() && FPaths::FileExists(_overrideDataFilePath))
    {
        readOk = FFileHelper::LoadFileToArray(data, *_overrideDataFilePath);
        if (!readOk) 
        {
            PLog::LogWarning(TEXT("Failed to read SDK core data, falling back to default. Data path: ") + _overrideDataFilePath);
        }
    }

    // parse file data
    if (readOk)
    {
        if (ParseChunkFileData(data.GetData(), (uint32)data.Num()) != 0)
        {
            PLog::LogWarning(TEXT("Failed to parse SDK core data, falling back to default."));
            if (ParseChunkFileData(DefaultChunkData, DefaultChunkDataLen) != 0)
            {
                PLog::LogError(TEXT("Failed to parse default SDK core data"));
                return -1;
            }
        }
    }
    else
    {
        PLog::LogDebug(TEXT("Reading default SDK core data"));
        if (ParseChunkFileData(DefaultChunkData, DefaultChunkDataLen) != 0)
        {
            PLog::LogError(TEXT("Failed to parse default SDK core data"));
            return -1;
        }
    }

    return 0;
}

TSharedPtr<LuaChunk> PSDKLuaCoreMgr::GetChunkByPath(const FString& path)
{
    if (!_chunks.Contains(path))
        return TSharedPtr<LuaChunk>(nullptr);
    return _chunks[path];
}

void PSDKLuaCoreMgr::Clear()
{
    _chunks.Empty();
}

bool PSDKLuaCoreMgr::CheckVersion(const uint8* databuffer, uint32 len, uint32& offset)
{
    if (PDR_PATCH_FILE_MIN_VERSION <= 0)
    {
        offset = 0;
        PLog::LogWarning(TEXT("no minversion set, version check pass"));
        return true;
    }
    const char* versionstr = PDR_PATCH_FILE_VERSION_PREFIX;
    if (len < PDR_PATCH_FILE_VERSION_SECTION_LEN)
    {
        PLog::LogWarning(TEXT("data len is less than 11, version check failed"));
        return false;
    }
    for (int i = 0; i < PDR_PATCH_FILE_VERSION_PREFIX_LEN; ++i) 
    {
        if ((char)databuffer[i] != versionstr[i])
        {
            PLog::LogWarning(TEXT("version header not valid, version check failed"));
            return false;
        }
    }
    int version = ntohl(*((uint32*)(databuffer+7)));
    if (version < PDR_PATCH_FILE_MIN_VERSION)
    {
        PLog::LogWarning(TEXT("patch version too low, version check failed"));
        return false;
    }
    offset = PDR_PATCH_FILE_VERSION_SECTION_LEN;
    PLog::LogInfo(TEXT("Version check passed"));
    return true;
}

int PSDKLuaCoreMgr::ParseChunkFileData(const uint8* databuffer, uint32 datalen)
{
    Clear();
    uint32 chunkCount=0, chunkNameLen=0, chunkPathLen=0, chunkDataLen=0;
    uint32 offset = 0;
    if (!CheckVersion(databuffer, datalen, offset))
    {
        PLog::LogWarning(TEXT("SDK core file version is outdated"));
        return -1;
    }
    chunkCount = ntohl(*((uint32*)(databuffer+offset)));
    offset += 4;
#if ((ENGINE_MINOR_VERSION<19) && (ENGINE_MAJOR_VERSION>=4))
    char readBuffer[2048];
#endif
    for (uint32 i = 0; i < chunkCount; ++i)
    {
        TSharedPtr<LuaChunk> chunk = MakeShareable(new LuaChunk);
        if (offset + 4 > datalen) 
        {
            // file invalid
            PLog::LogError(TEXT("SDK core file invalid"));
            return -1;
        }
        chunkNameLen = ntohl(*((uint32*)(databuffer+offset)));
        offset += 4;
        if (offset + chunkNameLen > datalen)
        {
            // file invalid
            PLog::LogError(TEXT("SDK core file invalid"));
            return -1;
        }

#if ((ENGINE_MINOR_VERSION<19) && (ENGINE_MAJOR_VERSION>=4))
        readBuffer[chunkNameLen] = '\0';
        FMemory::Memcpy(readBuffer, databuffer + offset, chunkNameLen);
        chunk->ChunkName = FString(UTF8_TO_TCHAR(readBuffer));
#else
        chunk->ChunkName = FString(chunkNameLen, (char*)(databuffer+offset));
#endif
        offset += chunkNameLen;
        if (offset + 4 > datalen)
        {
            // file invalid
            PLog::LogError(TEXT("SDK core file invalid"));
            return -1;
        }
        chunkPathLen = ntohl(*((uint32*)(databuffer + offset)));
        offset += 4;
        if (offset + chunkPathLen > datalen)
        {
            // file invalid
            PLog::LogError(TEXT("SDK core file invalid"));
            return -1;
        }
        
#if ((ENGINE_MINOR_VERSION<19) && (ENGINE_MAJOR_VERSION>=4))
        readBuffer[chunkPathLen] = '\0';
        FMemory::Memcpy(readBuffer, databuffer + offset, chunkPathLen);
        chunk->RuntimePath = FString(UTF8_TO_TCHAR(readBuffer));
#else
        chunk->RuntimePath = FString(chunkPathLen, (char*)(databuffer + offset));
#endif
        offset += chunkPathLen;
        if (offset + 4 > datalen)
        {
            // file invalid
            PLog::LogError(TEXT("SDK core file invalid"));
            return -1;
        }
        chunkDataLen = ntohl(*((uint32*)(databuffer + offset)));
        offset += 4;
        if (offset + chunkDataLen > datalen)
        {
            // file invalid
            PLog::LogError(TEXT("SDK core file invalid"));
            return -1;
        }
        chunk->ChunkData.Append((databuffer + offset), chunkDataLen);
        offset += chunkDataLen;
        _chunks.Add(chunk->RuntimePath, chunk);
    }
    return 0;
}

}