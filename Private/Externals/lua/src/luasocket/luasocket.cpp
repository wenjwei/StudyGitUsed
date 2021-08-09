/*=========================================================================*\
* LuaSocket toolkit
* Networking support for the Lua language
* Diego Nehab
* 26/11/1999
*
* This library is part of an  effort to progressively increase the network
* connectivity  of  the Lua  language.  The  Lua interface  to  networking
* functions follows the Sockets API  closely, trying to simplify all tasks
* involved in setting up both  client and server connections. The provided
* IO routines, however, follow the Lua  style, being very similar  to the
* standard Lua read and write functions.
\*=========================================================================*/
#include "luasocket.h"

/*=========================================================================*\
* Standard include files
\*=========================================================================*/
#include "lua.h"
#include "lauxlib.h"


/*=========================================================================*\
* LuaSocket includes
\*=========================================================================*/
#include "auxiliar.h"
#include "except.h"
#include "timeout.h"
#include "buffer.h"
#include "inet.h"
#include "tcp.h"
#include "udp.h"
#include "select.h"

namespace NS_PDR_SLUA {    

/*-------------------------------------------------------------------------*\
* Internal function prototypes
\*-------------------------------------------------------------------------*/
static int global_skip(___pdr_lua_State *L);
static int global_unload(___pdr_lua_State *L);
static int base_open(___pdr_lua_State *L);

/*-------------------------------------------------------------------------*\
* Modules and functions
\*-------------------------------------------------------------------------*/
static const ___pdr_luaL_Reg luasocket_mod[] = {
    {"auxiliar", ___pdr_auxiliar_open},
    {"except", ___pdr_except_open},
    {"timeout", ___pdr_timeout_open},
    {"buffer", ___pdr_buffer_open},
    {"inet", ___pdr_inet_open},
    {"tcp", tcp_open},
    {"udp", ___pdr_udp_open},
    {"select", ___pdr_select_open},
    {NULL, NULL}
};

static ___pdr_luaL_Reg luasocket_func[] = {
    {"skip",      global_skip},
    {"__unload",  global_unload},
    {NULL,        NULL}
};

/*-------------------------------------------------------------------------*\
* Skip a few arguments
\*-------------------------------------------------------------------------*/
static int global_skip(___pdr_lua_State *L) {
    int amount = ___pdr_luaL_checkinteger(L, 1);
    int ret = ___pdr_lua_gettop(L) - amount - 1;
    return ret >= 0 ? ret : 0;
}

/*-------------------------------------------------------------------------*\
* Unloads the library
\*-------------------------------------------------------------------------*/
static int global_unload(___pdr_lua_State *L) {
    (void) L;
    ___pdr_socket_close();
    return 0;
}

#if ___PDR_LUA_VERSION_NUM > 501
int luaL_typerror (___pdr_lua_State *L, int narg, const char *tname) {
  const char *msg = ___pdr_lua_pushfstring(L, "%s expected, got %s",
                                    tname, ___pdr_luaL_typename(L, narg));
  return ___pdr_luaL_argerror(L, narg, msg);
}
#endif

/*-------------------------------------------------------------------------*\
* Setup basic stuff.
\*-------------------------------------------------------------------------*/
static int base_open(___pdr_lua_State *L) {
    if (___pdr_socket_open()) {
        /* export functions (and leave namespace table on top of stack) */
#if ___PDR_LUA_VERSION_NUM > 501 && !defined(___PDR_LUA_COMPAT_MODULE)
        ___pdr_lua_newtable(L);
        ___pdr_luaL_setfuncs(L, luasocket_func, 0);
#else
        ___pdr_luaL_openlib(L, "socket", luasocket_func, 0);
#endif
#ifdef ___PDR_LUASOCKET_DEBUG
        ___pdr_lua_pushstring(L, "_DEBUG");
        ___pdr_lua_pushboolean(L, 1);
        ___pdr_lua_rawset(L, -3);
#endif
        /* make version string available to scripts */
        ___pdr_lua_pushstring(L, "_VERSION");
        ___pdr_lua_pushstring(L, ___PDR_LUASOCKET_VERSION);
        ___pdr_lua_rawset(L, -3);
        return 1;
    } else {
        ___pdr_lua_pushstring(L, "unable to initialize library");
        ___pdr_lua_error(L);
        return 0;
    }
}

/*-------------------------------------------------------------------------*\
* Initializes all library modules.
\*-------------------------------------------------------------------------*/
___PDR_LUASOCKET_API int ___pdr_luaopen_socket_core(___pdr_lua_State *L) {
    int i;
    base_open(L);
    for (i = 0; luasocket_mod[i].name; i++) luasocket_mod[i].func(L);
    return 1;
}

} // end NS_PDR_SLUA
