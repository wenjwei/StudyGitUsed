
#include "LuaSocketExt.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif
#include "lauxlib.h"
#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif

#include "LuaSocketExtDefines.h"
#include "LuaSocketTcpExt.h"
#include "LuaSocketReadBufferCheckExt.h"


inline int PushMethodsToTable(NS_PDR_SLUA::___pdr_lua_State * L, NS_PDR_SLUA::___pdr_luaL_Reg * funcs)
{
    for (; funcs->name; ++funcs)
    {
        ___pdr_lua_pushstring(L, funcs->name);
        ___pdr_lua_pushcfunction(L, funcs->func);
        ___pdr_lua_rawset(L, -3);
    }
    return 0;
}

static NS_PDR_SLUA::___pdr_luaL_Reg luasocket_module_extenders[] = {
    {"TcpExtender",                 NS_LUASOCKET_EXT::TcpExtender},
    {NULL, NULL}
};

inline int PushModuleExtendersToTable(NS_PDR_SLUA::___pdr_lua_State * L)
{
    ___pdr_lua_pushstring(L, "LuaSocketModuleExtenders");
    ___pdr_lua_newtable(L);
    PushMethodsToTable(L, luasocket_module_extenders);
    ___pdr_lua_rawset(L, -3);
    return 0;
}

static NS_PDR_SLUA::___pdr_luaL_Reg luasocket_global_extend_methods[] = {
    {"GetErrno", NS_LUASOCKET_EXT::GetErrno},
    {"IsSocketReadyForRead_SELECT", NS_LUASOCKET_EXT::IsSocketReadyForRead_SELECT},
    {"IsSocketReadyForRead_IOCTL",  NS_LUASOCKET_EXT::IsSocketReadyForRead_IOCTL},
    {"ByteOrderHostToNetwork32", NS_LUASOCKET_EXT::ByteOrderHostToNetwork32},
    {"ByteOrderNetworkToHost32", NS_LUASOCKET_EXT::ByteOrderNetworkToHost32},
    {NULL, NULL}
};

inline int PushGlobalExtendMethodsToTable(NS_PDR_SLUA::___pdr_lua_State * L)
{
    ___pdr_lua_pushstring(L, "LuaSocketExtension");
    ___pdr_lua_newtable(L);
    PushMethodsToTable(L, luasocket_global_extend_methods);
    ___pdr_lua_rawset(L, -3);
    return 0;
}

int NS_LUASOCKET_EXT::Extend(___pdr_lua_State * L)
{
    ___pdr_lua_pushglobaltable(L);
    PushModuleExtendersToTable(L);
    PushGlobalExtendMethodsToTable(L);
    ___pdr_lua_pop(L, 1);
    return 0;
}
