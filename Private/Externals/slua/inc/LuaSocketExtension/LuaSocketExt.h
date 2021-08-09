// push all luasocket related extensions into global table "LuaSocketExtension"

#pragma once

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif
#include "lua.h"
#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif
#include "LuaSocketExtDefines.h"

namespace NS_PDR_SLUA
{
    namespace LuaSocket
    {
        namespace Extension
        {
            int Extend(___pdr_lua_State * L);
        }
    }
}
