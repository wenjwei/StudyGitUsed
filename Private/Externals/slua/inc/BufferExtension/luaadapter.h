#pragma once

#define LUA_LIB
#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif
#include "lua.h"
#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif
#include "cppbuffer.h"

#define CB_LUA_ADAPTER_API PANDORA_API

#define NS_CB_LUAADAPTER NS_CPPBUFFER::luaadapter

namespace NS_CPPBUFFER
{
    namespace luaadapter
    {
        CB_LUA_ADAPTER_API p_cppbuffer checkbuffer(NS_PDR_SLUA::___pdr_lua_State* L, int index);
        CB_LUA_ADAPTER_API p_cppbuffer newlbuffer(NS_PDR_SLUA::___pdr_lua_State* L, size_t bufsize, CB_EENDIAN endian = CB_BIG_ENDIAN);
        CB_LUA_ADAPTER_API bool ismtcreated(NS_PDR_SLUA::___pdr_lua_State* L);

        // create lbuffer to the top of vm stack
        int open(NS_PDR_SLUA::___pdr_lua_State* L);

        // register lbuffer to _G
        int init(NS_PDR_SLUA::___pdr_lua_State* L);
    }
}
