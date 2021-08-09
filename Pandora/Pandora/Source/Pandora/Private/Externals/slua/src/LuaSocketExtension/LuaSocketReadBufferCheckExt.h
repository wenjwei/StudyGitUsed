#pragma once

#if PLATFORM_WINDOWS
#include "Runtime/Core/Public/Windows/AllowWindowsPlatformTypes.h"
#endif
#include "lua.h"
#if PLATFORM_WINDOWS
#include "Runtime/Core/Public/Windows/HideWindowsPlatformTypes.h"
#endif

namespace NS_PDR_SLUA
{
    namespace LuaSocket
    {
        namespace Extension
        {
            //int IsSocketReadyForRead(___pdr_lua_State* L);
            int GetErrno(___pdr_lua_State* L);
            int IsSocketReadyForRead_SELECT(___pdr_lua_State* L);
            int IsSocketReadyForRead_IOCTL(___pdr_lua_State* L);
            int ByteOrderHostToNetwork32(___pdr_lua_State* L);
            int ByteOrderNetworkToHost32(___pdr_lua_State* L);
        }
    }
}