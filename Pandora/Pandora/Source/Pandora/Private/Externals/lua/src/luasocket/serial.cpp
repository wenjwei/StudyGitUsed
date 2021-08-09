/*=========================================================================*\
* Serial stream
* LuaSocket toolkit
\*=========================================================================*/
#include <string.h> 

#include "lua.h"
#include "lauxlib.h"
#include "luasocket.h"

#include "auxiliar.h"
#include "socket.h"
#include "options.h"
#include "unix.h"

#ifndef _WIN32
#include <sys/un.h> 
#endif

/*
Reuses userdata definition from unix.h, since it is useful for all
stream-like objects.

If we stored the serial path for use in error messages or userdata
printing, we might need our own userdata definition.

Group usage is semi-inherited from unix.c, but unnecessary since we
have only one object type.
*/

namespace NS_PDR_SLUA {    

/*=========================================================================*\
* Internal function prototypes
\*=========================================================================*/
static int serial_global_create(___pdr_lua_State *L);
static int serial_meth_send(___pdr_lua_State *L);
static int serial_meth_receive(___pdr_lua_State *L);
static int serial_meth_close(___pdr_lua_State *L);
static int serial_meth_settimeout(___pdr_lua_State *L);
static int serial_meth_getfd(___pdr_lua_State *L);
static int serial_meth_setfd(___pdr_lua_State *L);
static int serial_meth_dirty(___pdr_lua_State *L);
static int serial_meth_getstats(___pdr_lua_State *L);
static int serial_meth_setstats(___pdr_lua_State *L);

/* serial object methods */
static ___pdr_luaL_Reg serial_methods[] = {
    {"__gc",        serial_meth_close},
    {"__tostring",  ___pdr_auxiliar_tostring},
    {"close",       serial_meth_close},
    {"dirty",       serial_meth_dirty},
    {"getfd",       serial_meth_getfd},
    {"getstats",    serial_meth_getstats},
    {"setstats",    serial_meth_setstats},
    {"receive",     serial_meth_receive},
    {"send",        serial_meth_send},
    {"setfd",       serial_meth_setfd},
    {"settimeout",  serial_meth_settimeout},
    {NULL,          NULL}
};

/* our socket creation function */
/* this is an ad-hoc module that returns a single function 
 * as such, do not include other functions in this array. */
static ___pdr_luaL_Reg serial_func[] = {
    {"serial", serial_global_create},
    {NULL,          NULL}
};


/*-------------------------------------------------------------------------*\
* Initializes module
\*-------------------------------------------------------------------------*/
___PDR_LUASOCKET_API int luaopen_socket_serial(___pdr_lua_State *L) {
    /* create classes */
    ___pdr_auxiliar_newclass(L, "serial{client}", serial_methods);
    /* create class groups */
    ___pdr_auxiliar_add2group(L, "serial{client}", "serial{any}");
#if ___PDR_LUA_VERSION_NUM > 501 && !defined(___PDR_LUA_COMPAT_MODULE)
    ___pdr_lua_pushcfunction(L, serial_global_create);
    (void)serial_func;
#else
    /* set function into socket namespace */
    ___pdr_luaL_openlib(L, "socket", serial_func, 0);
    ___pdr_lua_pushcfunction(L, serial_global_create);
#endif
    return 1;
}

/*=========================================================================*\
* Lua methods
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Just call buffered IO methods
\*-------------------------------------------------------------------------*/
static int serial_meth_send(___pdr_lua_State *L) {
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkclass(L, "serial{client}", 1);
    return ___pdr_buffer_meth_send(L, &un->buf);
}

static int serial_meth_receive(___pdr_lua_State *L) {
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkclass(L, "serial{client}", 1);
    return ___pdr_buffer_meth_receive(L, &un->buf);
}

static int serial_meth_getstats(___pdr_lua_State *L) {
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkclass(L, "serial{client}", 1);
    return ___pdr_buffer_meth_getstats(L, &un->buf);
}

static int serial_meth_setstats(___pdr_lua_State *L) {
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkclass(L, "serial{client}", 1);
    return ___pdr_buffer_meth_setstats(L, &un->buf);
}

/*-------------------------------------------------------------------------*\
* Select support methods
\*-------------------------------------------------------------------------*/
static int serial_meth_getfd(___pdr_lua_State *L) {
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkgroup(L, "serial{any}", 1);
    ___pdr_lua_pushnumber(L, (int) un->sock);
    return 1;
}

/* this is very dangerous, but can be handy for those that are brave enough */
static int serial_meth_setfd(___pdr_lua_State *L) {
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkgroup(L, "serial{any}", 1);
    un->sock = (___pdr_t_socket) ___pdr_luaL_checknumber(L, 2); 
    return 0;
}

static int serial_meth_dirty(___pdr_lua_State *L) {
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkgroup(L, "serial{any}", 1);
    ___pdr_lua_pushboolean(L, !___pdr_buffer_isempty(&un->buf));
    return 1;
}

/*-------------------------------------------------------------------------*\
* Closes socket used by object 
\*-------------------------------------------------------------------------*/
static int serial_meth_close(___pdr_lua_State *L)
{
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkgroup(L, "serial{any}", 1);
    ___pdr_socket_destroy(&un->sock);
    ___pdr_lua_pushnumber(L, 1);
    return 1;
}


/*-------------------------------------------------------------------------*\
* Just call tm methods
\*-------------------------------------------------------------------------*/
static int serial_meth_settimeout(___pdr_lua_State *L) {
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkgroup(L, "serial{any}", 1);
    return ___pdr_timeout_meth_settimeout(L, &un->tm);
}

/*=========================================================================*\
* Library functions
\*=========================================================================*/


/*-------------------------------------------------------------------------*\
* Creates a serial object 
\*-------------------------------------------------------------------------*/
static int serial_global_create(___pdr_lua_State *L) {
#ifndef _WIN32
    const char* path = ___pdr_luaL_checkstring(L, 1);

    /* allocate unix object */
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_lua_newuserdata(L, sizeof(___pdr_t_unix));

    /* open serial device */
    ___pdr_t_socket sock = open(path, O_NOCTTY|O_RDWR);

    /*printf("open %s on %d\n", path, sock);*/

    if (sock < 0)  {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, ___pdr_socket_strerror(errno));
        ___pdr_lua_pushnumber(L, errno);
        return 3;
    }
    /* set its type as client object */
    ___pdr_auxiliar_setclass(L, "serial{client}", -1);
    /* initialize remaining structure fields */
    ___pdr_socket_setnonblocking(&sock);
    un->sock = sock;
    ___pdr_io_init(&un->io, (___pdr_p_send) ___pdr_socket_write, (___pdr_p_recv) ___pdr_socket_read, 
            (___pdr_p_error) ___pdr_socket_ioerror, &un->sock);
    ___pdr_timeout_init(&un->tm, -1, -1);
    ___pdr_buffer_init(&un->buf, &un->io, &un->tm);
    return 1;
#else
	return -1;
#endif
}

} // end NS_PDR_SLUA
