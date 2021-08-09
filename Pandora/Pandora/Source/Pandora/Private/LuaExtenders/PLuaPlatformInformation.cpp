#include "PLuaPlatformInformation.h"
#include "PLuaStateMgr.h"

namespace pandora 
{
//using namespace NS_PDR_SLUA;

void PLuaPlatformInformation::RegisterToLua(NS_PDR_SLUA::___pdr_lua_State* L) {
    ___pdr_lua_newtable(L);

#if UE_BUILD_SHIPPING
    ___pdr_lua_pushboolean(L, true);
#else
    ___pdr_lua_pushboolean(L, false);
#endif
    ___pdr_lua_setfield(L, -2, "IsShipping");

#if WITH_EDITOR
    ___pdr_lua_pushboolean(L, true);
#else
    ___pdr_lua_pushboolean(L, false);
#endif
    ___pdr_lua_setfield(L, -2, "IsEditor");

#if WITH_EDITOR
    ___pdr_lua_pushstring(L, "Editor");
#elif PLATFORM_WINDOWS
    ___pdr_lua_pushstring(L, "Windows");
#elif PLATFORM_MAC
    ___pdr_lua_pushstring(L, "Mac");
#elif PLATFORM_IOS
    ___pdr_lua_pushstring(L, "IOS");
#elif PLATFORM_ANDROID
    ___pdr_lua_pushstring(L, "Android");
#else
    //#error Unknown Compiler
    ___pdr_lua_pushstring(L, "Unknown Platform");
#endif
    ___pdr_lua_setfield(L, -2, "Platform");

    ___pdr_lua_setglobal(L, "PLuaPlatformInformation");
}

}
