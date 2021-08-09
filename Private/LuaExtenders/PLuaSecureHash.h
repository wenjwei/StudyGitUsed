#pragma once

#include "Containers/UnrealString.h"

namespace pandora
{
class PLuaSecureHash
{
public:
    static FString GetFileHashKey();
    static FString Md5HashAnsiString(const FString & );
    static FString Md5HashFile(const FString& path);
    static FString SHA1HashStringToHex(const FString & );
};
}
