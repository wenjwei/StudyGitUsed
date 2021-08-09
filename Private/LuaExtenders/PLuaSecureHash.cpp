#include "PLuaSecureHash.h"
#include "Misc/SecureHash.h"
#include "PDRLuaObject.h"
#include "PDRLuaCppBinding.h"
#include "cppbuffer.h"
#include "luaadapter.h"
#include "UnrealMemory.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif
#include "utils/msdk_tea.h"
#include "utils/__miniz.h"
#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif

namespace pandora
{
FString PLuaSecureHash::GetFileHashKey()
{
    return TEXT("pandora20151019");
}

FString PLuaSecureHash::Md5HashAnsiString(const FString & input)
{
    return FMD5::HashAnsiString(*input);
}

FString PLuaSecureHash::Md5HashFile(const FString& path)
{
    FMD5Hash hash = FMD5Hash::HashFile(*path);
    return LexToString(hash);
}

FString PLuaSecureHash::SHA1HashStringToHex(const FString & input)
{
    uint8 * outHash = (uint8*)FMemory::Malloc(20);
    FTCHARToUTF8 converter(*input);
    FSHA1::HashBuffer(converter.Get(), converter.Length(), outHash);
    FString result = FString::FromHexBlob(outHash, 20);
    FMemory::Free((void*)outHash);
    return result;
}
}

namespace NS_PDR_SLUA
{
using pandora::PLuaSecureHash;
//using namespace pandorav2;

DefLuaClass(PLuaSecureHash)
    DefLuaMethod(GetFileHashKey, &pandora::PLuaSecureHash::GetFileHashKey)
    DefLuaMethod(Md5HashAnsiString, &pandora::PLuaSecureHash::Md5HashAnsiString)
    DefLuaMethod(Md5HashFile, &pandora::PLuaSecureHash::Md5HashFile)
    DefLuaMethod(SHA1HashStringToHex, &pandora::PLuaSecureHash::SHA1HashStringToHex)
    DefLuaMethod_With_Imp(ZipNetworkPacket, true, {
        NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(lstack, 1);
        if (!buf)
        {
            ___pdr_lua_pushnil(lstack);
            return 1;
        }
        uLong srcLen = static_cast<uLong>(buf->count);
        uLong cmpLen = __compressBound(srcLen);
        NS_CPPBUFFER::p_cppbuffer outbuf = NS_CB_LUAADAPTER::newlbuffer(lstack, cmpLen);
        int ret = __compress(reinterpret_cast<uint8*>(outbuf->buf), &cmpLen, reinterpret_cast<uint8*>(buf->buf), srcLen);
        if (ret != __Z_OK)
        {
            ___pdr_lua_pushnil(lstack);
            return 1;
        }
        NS_CPPBUFFER::cb_peekwrite(outbuf, cmpLen);
        return 1;
    })
    DefLuaMethod_With_Imp(UnzipNetworkPacket, true, {
        const size_t maxRecvSize = 1024 * 1024;
        static uint8 tempRecvBuf[maxRecvSize];

        NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(lstack, 1);
        if (!buf)
        {
            ___pdr_lua_pushnil(lstack);
            return 1;
        }

        uLong outSize = maxRecvSize;
        int ret = __uncompress(tempRecvBuf, &outSize, reinterpret_cast<uint8*>(buf->buf), buf->count);
        if (ret != __Z_OK)
        {
            ___pdr_lua_pushnil(lstack);
            return 1;
        }

        NS_CPPBUFFER::p_cppbuffer outBuf = NS_CB_LUAADAPTER::newlbuffer(lstack, outSize);
        int retVal = NS_CPPBUFFER::cb_write_raw(outBuf, reinterpret_cast<char*>(tempRecvBuf), outSize);
        if (retVal)
        {
            ___pdr_lua_pop(lstack, 1);
            ___pdr_lua_pushnil(lstack);
            return 1;
        }

        return 1;
    })
    DefLuaMethod_With_Imp(MsdkDecode, true, {
        size_t bodyLen;
        const char * bodyStr = ___pdr_luaL_checklstring(lstack, 1, &bodyLen);
        if (!bodyStr)
        {
            ___pdr_lua_pushnil(lstack);
            return 1;
        }

        TArray<pandora::msdk_byte> bodyArr(reinterpret_cast<pandora::msdk_byte*>(const_cast<char*>(bodyStr)), static_cast<int32>(bodyLen));
        TArray<pandora::msdk_byte> base64BodyArr = pandora::base64_decode(bodyArr);
        int decodeBufLen = base64BodyArr.Num() + 1;
        TArray<pandora::msdk_byte> decodeBuf;
        decodeBuf.SetNumZeroed(decodeBufLen);
        int ret = pandora::msdk_decode(base64BodyArr.GetData(), base64BodyArr.Num(), decodeBuf.GetData(), &decodeBufLen);
        if (ret)
        {
            ___pdr_lua_pushnil(lstack);
            return 1;
        }

        ___pdr_lua_pushlstring(lstack, reinterpret_cast<char*>(decodeBuf.GetData()), decodeBufLen);
        return 1;
    })
EndDef(PLuaSecureHash, nullptr)
}

