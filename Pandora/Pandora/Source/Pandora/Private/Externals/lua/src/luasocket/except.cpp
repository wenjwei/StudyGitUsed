/*=========================================================================*\
* Simple exception support
* LuaSocket toolkit
\*=========================================================================*/
#include "except.h"
#include <stdio.h>

#include "lua.h"
#include "lauxlib.h"


namespace NS_PDR_SLUA {

/*=========================================================================*\
* Internal function prototypes.
\*=========================================================================*/
static int global_protect(___pdr_lua_State *L);
static int global_newtry(___pdr_lua_State *L);
static int protected_(___pdr_lua_State *L);
static int finalize(___pdr_lua_State *L);
static int do_nothing(___pdr_lua_State *L);

/* except functions */
static ___pdr_luaL_Reg except_func[] = {
    {"newtry",    global_newtry},
    {"protect",   global_protect},
    {NULL,        NULL}
};

/*-------------------------------------------------------------------------*\
* Try factory
\*-------------------------------------------------------------------------*/
static void wrap(___pdr_lua_State *L) {
    ___pdr_lua_newtable(L);
    ___pdr_lua_pushnumber(L, 1);
    ___pdr_lua_pushvalue(L, -3);
    ___pdr_lua_settable(L, -3);
    ___pdr_lua_insert(L, -2);
    ___pdr_lua_pop(L, 1);
}

static int finalize(___pdr_lua_State *L) {
    if (!___pdr_lua_toboolean(L, 1)) {
        ___pdr_lua_pushvalue(L, ___pdr_lua_upvalueindex(1));
        ___pdr_lua_pcall(L, 0, 0, 0);
        ___pdr_lua_settop(L, 2);
        wrap(L);
        ___pdr_lua_error(L);
        return 0;
    } else return ___pdr_lua_gettop(L);
}

static int do_nothing(___pdr_lua_State *L) { 
    (void) L;
    return 0; 
}

static int global_newtry(___pdr_lua_State *L) {
    ___pdr_lua_settop(L, 1);
    if (___pdr_lua_isnil(L, 1)) ___pdr_lua_pushcfunction(L, do_nothing);
    ___pdr_lua_pushcclosure(L, finalize, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Protect factory
\*-------------------------------------------------------------------------*/
static int unwrap(___pdr_lua_State *L) {
    if (___pdr_lua_istable(L, -1)) {
        ___pdr_lua_pushnumber(L, 1);
        ___pdr_lua_gettable(L, -2);
        ___pdr_lua_pushnil(L);
        ___pdr_lua_insert(L, -2);
        return 1;
    } else return 0;
}

static int protected_(___pdr_lua_State *L) {
    ___pdr_lua_pushvalue(L, ___pdr_lua_upvalueindex(1));
    ___pdr_lua_insert(L, 1);
    if (___pdr_lua_pcall(L, ___pdr_lua_gettop(L) - 1, ___PDR_LUA_MULTRET, 0) != 0) {
        if (unwrap(L)) return 2;
        else ___pdr_lua_error(L);
        return 0;
    } else return ___pdr_lua_gettop(L);
}

static int global_protect(___pdr_lua_State *L) {
    ___pdr_lua_pushcclosure(L, protected_, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Init module
\*-------------------------------------------------------------------------*/
int ___pdr_except_open(___pdr_lua_State *L) {
#if ___PDR_LUA_VERSION_NUM > 501 && !defined(___PDR_LUA_COMPAT_MODULE)
    ___pdr_luaL_setfuncs(L, except_func, 0);
#else
    ___pdr_luaL_openlib(L, NULL, except_func, 0);
#endif
    return 0;
}

} // end NS_PDR_SLUA
