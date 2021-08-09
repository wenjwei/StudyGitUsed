// Add readio and writeio method for luasocket-tcp module
// readio reads data from socket into luabuffer
// writeio writes data from a luabuffer to socket

#pragma once

#define ___PDR_LUA_LIB
#if PLATFORM_WINDOWS
#include "Runtime/Core/Public/Windows/AllowWindowsPlatformTypes.h"
#endif
#include "lua.h"
#include "luasocket/buffer.h"
#if PLATFORM_WINDOWS
#include "Runtime/Core/Public/Windows/HideWindowsPlatformTypes.h"
#endif
#include "luaadapter.h"
#include "cppbuffer.h"

namespace NS_PDR_SLUA
{
    namespace LuaSocket
    {
        namespace Extension
        {
            // read socket buffer data into a luabuffer
            int ReadIO(___pdr_p_buffer socketbuf, NS_CPPBUFFER::p_cppbuffer buffer, size_t wanted, size_t& readcount);
            // send LuaBuffer content via LuaSocket
            int WriteIO(___pdr_p_buffer socketbuf, NS_CPPBUFFER::p_cppbuffer buffer);

            // must call after LuaSocket core opened
            int TcpExtender(___pdr_lua_State* L);
        }
    }
}
