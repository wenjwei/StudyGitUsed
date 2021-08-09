/*=========================================================================*\
* Select implementation
* LuaSocket toolkit
\*=========================================================================*/
#include "select.h"
#include <string.h>

#include "lua.h"
#include "lauxlib.h"

#include "socket.h"
#include "timeout.h"

namespace NS_PDR_SLUA {    

/*=========================================================================*\
* Internal function prototypes.
\*=========================================================================*/
static ___pdr_t_socket getfd(___pdr_lua_State *L);
static int dirty(___pdr_lua_State *L);
static void collect_fd(___pdr_lua_State *L, int tab, int itab, 
        fd_set *set, ___pdr_t_socket *max_fd);
static int check_dirty(___pdr_lua_State *L, int tab, int dtab, fd_set *set);
static void return_fd(___pdr_lua_State *L, fd_set *set, ___pdr_t_socket max_fd, 
        int itab, int tab, int start);
static void make_assoc(___pdr_lua_State *L, int tab);
static int global_select(___pdr_lua_State *L);

/* functions in library namespace */
static ___pdr_luaL_Reg select_func[] = {
    {"select", global_select},
    {NULL,     NULL}
};

/*=========================================================================*\
* Exported functions
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Initializes module
\*-------------------------------------------------------------------------*/
int ___pdr_select_open(___pdr_lua_State *L) {
    ___pdr_lua_pushstring(L, "_SETSIZE");
    ___pdr_lua_pushnumber(L, FD_SETSIZE);
    ___pdr_lua_rawset(L, -3);
#if ___PDR_LUA_VERSION_NUM > 501 && !defined(___PDR_LUA_COMPAT_MODULE)
    ___pdr_luaL_setfuncs(L, select_func, 0);
#else
    ___pdr_luaL_openlib(L, NULL, select_func, 0);
#endif
    return 0;
}

/*=========================================================================*\
* Global Lua functions
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Waits for a set of sockets until a condition is met or timeout.
\*-------------------------------------------------------------------------*/
static int global_select(___pdr_lua_State *L) {
    int rtab, wtab, itab, ret, ndirty;
    ___pdr_t_socket max_fd = SOCKET_INVALID;
    fd_set rset, wset;
    ___pdr_t_timeout tm;
    double t = ___pdr_luaL_optnumber(L, 3, -1);
    FD_ZERO(&rset); FD_ZERO(&wset);
    ___pdr_lua_settop(L, 3);
    ___pdr_lua_newtable(L); itab = ___pdr_lua_gettop(L);
    ___pdr_lua_newtable(L); rtab = ___pdr_lua_gettop(L);
    ___pdr_lua_newtable(L); wtab = ___pdr_lua_gettop(L);
    collect_fd(L, 1, itab, &rset, &max_fd);
    collect_fd(L, 2, itab, &wset, &max_fd);
    ndirty = check_dirty(L, 1, rtab, &rset);
    t = ndirty > 0? 0.0: t;
    ___pdr_timeout_init(&tm, t, -1);
    ___pdr_timeout_markstart(&tm);
    ret = ___pdr_socket_select(max_fd+1, &rset, &wset, NULL, &tm);
    if (ret > 0 || ndirty > 0) {
        return_fd(L, &rset, max_fd+1, itab, rtab, ndirty);
        return_fd(L, &wset, max_fd+1, itab, wtab, 0);
        make_assoc(L, rtab);
        make_assoc(L, wtab);
        return 2;
    } else if (ret == 0) {
        ___pdr_lua_pushstring(L, "timeout");
        return 3;
    } else {
        ___pdr_luaL_error(L, "select failed");
        return 3;
    }
}

/*=========================================================================*\
* Internal functions
\*=========================================================================*/
static ___pdr_t_socket getfd(___pdr_lua_State *L) {
    ___pdr_t_socket fd = SOCKET_INVALID;
    ___pdr_lua_pushstring(L, "getfd");
    ___pdr_lua_gettable(L, -2);
    if (!___pdr_lua_isnil(L, -1)) {
        ___pdr_lua_pushvalue(L, -2);
        lua_call(L, 1, 1);
        if (___pdr_lua_isnumber(L, -1)) {
            double numfd = ___pdr_lua_tonumber(L, -1); 
            fd = (numfd >= 0.0)? (___pdr_t_socket) numfd: SOCKET_INVALID;
        }
    } 
    ___pdr_lua_pop(L, 1);
    return fd;
}

static int dirty(___pdr_lua_State *L) {
    int is = 0;
    ___pdr_lua_pushstring(L, "dirty");
    ___pdr_lua_gettable(L, -2);
    if (!___pdr_lua_isnil(L, -1)) {
        ___pdr_lua_pushvalue(L, -2);
        lua_call(L, 1, 1);
        is = ___pdr_lua_toboolean(L, -1);
    } 
    ___pdr_lua_pop(L, 1);
    return is;
}

static void collect_fd(___pdr_lua_State *L, int tab, int itab, 
        fd_set *set, ___pdr_t_socket *max_fd) {
    int i = 1, n = 0;
    /* nil is the same as an empty table */
    if (___pdr_lua_isnil(L, tab)) return;
    /* otherwise we need it to be a table */
    ___pdr_luaL_checktype(L, tab, ___PDR_LUA_TTABLE);
    for ( ;; ) {
        ___pdr_t_socket fd;
        ___pdr_lua_pushnumber(L, i);
        ___pdr_lua_gettable(L, tab);
        if (___pdr_lua_isnil(L, -1)) {
            ___pdr_lua_pop(L, 1);
            break;
        }
        /* getfd figures out if this is a socket */
        fd = getfd(L);
        if (fd != SOCKET_INVALID) {
            /* make sure we don't overflow the fd_set */
#ifdef _WIN32
            if (n >= FD_SETSIZE) 
                ___pdr_luaL_argerror(L, tab, "too many sockets");
#else
            if (fd >= FD_SETSIZE) 
                ___pdr_luaL_argerror(L, tab, "descriptor too large for set size");
#endif
            FD_SET(fd, set);
            n++;
            /* keep track of the largest descriptor so far */
            if (*max_fd == SOCKET_INVALID || *max_fd < fd) 
                *max_fd = fd;
            /* make sure we can map back from descriptor to the object */
            ___pdr_lua_pushnumber(L, (___pdr_lua_Number) fd);
            ___pdr_lua_pushvalue(L, -2);
            ___pdr_lua_settable(L, itab);
        }
        ___pdr_lua_pop(L, 1);
        i = i + 1;
    }
}

static int check_dirty(___pdr_lua_State *L, int tab, int dtab, fd_set *set) {
    int ndirty = 0, i = 1;
    if (___pdr_lua_isnil(L, tab)) 
        return 0;
    for ( ;; ) { 
        ___pdr_t_socket fd;
        ___pdr_lua_pushnumber(L, i);
        ___pdr_lua_gettable(L, tab);
        if (___pdr_lua_isnil(L, -1)) {
            ___pdr_lua_pop(L, 1);
            break;
        }
        fd = getfd(L);
        if (fd != SOCKET_INVALID && dirty(L)) {
            ___pdr_lua_pushnumber(L, ++ndirty);
            ___pdr_lua_pushvalue(L, -2);
            ___pdr_lua_settable(L, dtab);
            FD_CLR(fd, set);
        }
        ___pdr_lua_pop(L, 1);
        i = i + 1;
    }
    return ndirty;
}

static void return_fd(___pdr_lua_State *L, fd_set *set, ___pdr_t_socket max_fd, 
        int itab, int tab, int start) {
    ___pdr_t_socket fd;
    for (fd = 0; fd < max_fd; fd++) {
        if (FD_ISSET(fd, set)) {
            ___pdr_lua_pushnumber(L, ++start);
            ___pdr_lua_pushnumber(L, (___pdr_lua_Number) fd);
            ___pdr_lua_gettable(L, itab);
            ___pdr_lua_settable(L, tab);
        }
    }
}

static void make_assoc(___pdr_lua_State *L, int tab) {
    int i = 1, atab;
    ___pdr_lua_newtable(L); atab = ___pdr_lua_gettop(L);
    for ( ;; ) {
        ___pdr_lua_pushnumber(L, i);
        ___pdr_lua_gettable(L, tab);
        if (!___pdr_lua_isnil(L, -1)) {
            ___pdr_lua_pushnumber(L, i);
            ___pdr_lua_pushvalue(L, -2);
            ___pdr_lua_settable(L, atab);
            ___pdr_lua_pushnumber(L, i);
            ___pdr_lua_settable(L, atab);
        } else {
            ___pdr_lua_pop(L, 1);
            break;
        }
        i = i+1;
    }
}


} // end NS_PDR_SLUA
