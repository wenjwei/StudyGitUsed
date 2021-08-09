#pragma once

#include "CoreMinimal.h"
#include "Array.h"

#define PDR_PATCH_FILE_MIN_VERSION 2

namespace pandora
{

typedef struct LuaChunk_t
{
    FString ChunkName;
    FString RuntimePath;
    TArray<uint8> ChunkData;
} LuaChunk;


class PSDKLuaCoreMgr
{
public:
    virtual ~PSDKLuaCoreMgr() {};
    static PSDKLuaCoreMgr& Get();
    void SetOverrideDataFilePath(const FString& path);
    int GetMinPatchVersion();
    int Reload();
    TSharedPtr<LuaChunk> GetChunkByPath(const FString& path);
    void Clear();
private:
    PSDKLuaCoreMgr() {};

    bool CheckVersion(const uint8* databuffer, uint32 len, uint32& offset);
    int ParseChunkFileData(const uint8* databuffer, uint32 len);
    FString _overrideDataFilePath;
    TMap<FString, TSharedPtr<LuaChunk>> _chunks;
};

}

