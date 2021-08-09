/*=========================================================================*\
* Unix domain socket 
* LuaSocket toolkit
\*=========================================================================*/

#include "unix.h"

#include <string.h> 

#include "lua.h"
#include "lauxlib.h"

#include "auxiliar.h"
#include "socket.h"
#include "options.h"

#ifndef _WIN32
#include <sys/un.h> 
#endif // _WIN32

namespace NS_PDR_SLUA {    

/*=========================================================================*\
* Internal function prototypes
\*=========================================================================*/
static int unix_global_create(___pdr_lua_State *L);
static int unix_meth_connect(___pdr_lua_State *L);
static int unix_meth_listen(___pdr_lua_State *L);
static int unix_meth_bind(___pdr_lua_State *L);
static int unix_meth_send(___pdr_lua_State *L);
static int unix_meth_shutdown(___pdr_lua_State *L);
static int unix_meth_receive(___pdr_lua_State *L);
static int unix_meth_accept(___pdr_lua_State *L);
static int unix_meth_close(___pdr_lua_State *L);
static int unix_meth_setoption(___pdr_lua_State *L);
static int unix_meth_settimeout(___pdr_lua_State *L);
static int unix_meth_getfd(___pdr_lua_State *L);
static int unix_meth_setfd(___pdr_lua_State *L);
static int unix_meth_dirty(___pdr_lua_State *L);
static int unix_meth_getstats(___pdr_lua_State *L);
static int unix_meth_setstats(___pdr_lua_State *L);

static const char *unix_tryconnect(___pdr_p_unix un, const char *path);
static const char *unix_trybind(___pdr_p_unix un, const char *path);

/* unix object methods */
static ___pdr_luaL_Reg unix_methods[] = {
    {"__gc",        unix_meth_close},
    {"__tostring",  ___pdr_auxiliar_tostring},
    {"accept",      unix_meth_accept},
    {"bind",        unix_meth_bind},
    {"close",       unix_meth_close},
    {"connect",     unix_meth_connect},
    {"dirty",       unix_meth_dirty},
    {"getfd",       unix_meth_getfd},
    {"getstats",    unix_meth_getstats},
    {"setstats",    unix_meth_setstats},
    {"listen",      unix_meth_listen},
    {"receive",     unix_meth_receive},
    {"send",        unix_meth_send},
    {"setfd",       unix_meth_setfd},
    {"setoption",   unix_meth_setoption},
    {"setpeername", unix_meth_connect},
    {"setsockname", unix_meth_bind},
    {"settimeout",  unix_meth_settimeout},
    {"shutdown",    unix_meth_shutdown},
    {NULL,          NULL}
};

/* socket option handlers */
static ___pdr_t_opt unix_optset[] = {
    {"keepalive",   ___pdr_opt_set_keepalive},
    {"reuseaddr",   ___pdr_opt_set_reuseaddr},
    {"linger",      ___pdr_opt_set_linger},
    {NULL,          NULL}
};

/* our socket creation function */
/* this is an ad-hoc module that returns a single function 
 * as such, do not include other functions in this array. */
static ___pdr_luaL_Reg unix_func[] = {
    {"unix", unix_global_create},
    {NULL,          NULL}
};


/*-------------------------------------------------------------------------*\
* Initializes module
\*-------------------------------------------------------------------------*/
int ___pdr_luaopen_socket_unix(___pdr_lua_State *L) {
    /* create classes */
    ___pdr_auxiliar_newclass(L, "unix{master}", unix_methods);
    ___pdr_auxiliar_newclass(L, "unix{client}", unix_methods);
    ___pdr_auxiliar_newclass(L, "unix{server}", unix_methods);
    /* create class groups */
    ___pdr_auxiliar_add2group(L, "unix{master}", "unix{any}");
    ___pdr_auxiliar_add2group(L, "unix{client}", "unix{any}");
    ___pdr_auxiliar_add2group(L, "unix{server}", "unix{any}");
#if ___PDR_LUA_VERSION_NUM > 501 && !defined(___PDR_LUA_COMPAT_MODULE)
    ___pdr_lua_pushcfunction(L, unix_global_create);
    (void)unix_func;
#else
    /* set function into socket namespace */
    ___pdr_luaL_openlib(L, "socket", unix_func, 0);
    ___pdr_lua_pushcfunction(L, unix_global_create);
#endif
    /* return the function instead of the 'socket' table */
    return 1;
}

/*=========================================================================*\
* Lua methods
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Just call buffered IO methods
\*-------------------------------------------------------------------------*/
static int unix_meth_send(___pdr_lua_State *L) {
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkclass(L, "unix{client}", 1);
    return ___pdr_buffer_meth_send(L, &un->buf);
}

static int unix_meth_receive(___pdr_lua_State *L) {
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkclass(L, "unix{client}", 1);
    return ___pdr_buffer_meth_receive(L, &un->buf);
}

static int unix_meth_getstats(___pdr_lua_State *L) {
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkclass(L, "unix{client}", 1);
    return ___pdr_buffer_meth_getstats(L, &un->buf);
}

static int unix_meth_setstats(___pdr_lua_State *L) {
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkclass(L, "unix{client}", 1);
    return ___pdr_buffer_meth_setstats(L, &un->buf);
}

/*-------------------------------------------------------------------------*\
* Just call option handler
\*-------------------------------------------------------------------------*/
static int unix_meth_setoption(___pdr_lua_State *L) {
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkgroup(L, "unix{any}", 1);
    return ___opt_meth_setoption(L, unix_optset, &un->sock);
}

/*-------------------------------------------------------------------------*\
* Select support methods
\*-------------------------------------------------------------------------*/
static int unix_meth_getfd(___pdr_lua_State *L) {
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkgroup(L, "unix{any}", 1);
    ___pdr_lua_pushnumber(L, (int) un->sock);
    return 1;
}

/* this is very dangerous, but can be handy for those that are brave enough */
static int unix_meth_setfd(___pdr_lua_State *L) {
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkgroup(L, "unix{any}", 1);
    un->sock = (___pdr_t_socket) ___pdr_luaL_checknumber(L, 2); 
    return 0;
}

static int unix_meth_dirty(___pdr_lua_State *L) {
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkgroup(L, "unix{any}", 1);
    ___pdr_lua_pushboolean(L, !___pdr_buffer_isempty(&un->buf));
    return 1;
}

/*-------------------------------------------------------------------------*\
* Waits for and returns a client object attempting connection to the 
* server object 
\*-------------------------------------------------------------------------*/
static int unix_meth_accept(___pdr_lua_State *L) {
    ___pdr_p_unix server = (___pdr_p_unix) ___pdr_auxiliar_checkclass(L, "unix{server}", 1);
    ___pdr_p_timeout tm = ___pdr_timeout_markstart(&server->tm);
    ___pdr_t_socket sock;
    int err = ___pdr_socket_accept(&server->sock, &sock, NULL, NULL, tm);
    /* if successful, push client socket */
    if (err == PDR_IO_DONE) {
        ___pdr_p_unix clnt = (___pdr_p_unix) ___pdr_lua_newuserdata(L, sizeof(___pdr_t_unix));
        ___pdr_auxiliar_setclass(L, "unix{client}", -1);
        /* initialize structure fields */
        ___pdr_socket_setnonblocking(&sock);
        clnt->sock = sock;
        ___pdr_io_init(&clnt->io, (___pdr_p_send)___pdr_socket_send, (___pdr_p_recv)___pdr_socket_recv, 
                (___pdr_p_error) ___pdr_socket_ioerror, &clnt->sock);
        ___pdr_timeout_init(&clnt->tm, -1, -1);
        ___pdr_buffer_init(&clnt->buf, &clnt->io, &clnt->tm);
        return 1;
    } else {
        ___pdr_lua_pushnil(L); 
        ___pdr_lua_pushstring(L, ___pdr_socket_strerror(err));
        return 2;
    }
}

/*-------------------------------------------------------------------------*\
* Binds an object to an address 
\*-------------------------------------------------------------------------*/
static const char *unix_trybind(___pdr_p_unix un, const char *path) {
#ifndef _WIN32
    struct sockaddr_un local;
    size_t len = strlen(path);
    int err;
    if (len >= sizeof(local.sun_path)) return "path too long";
    memset(&local, 0, sizeof(local));
    strcpy(local.sun_path, path);
    local.sun_family = AF_UNIX;
#ifdef UNIX_HAS_SUN_LEN
    local.sun_len = sizeof(local.sun_family) + sizeof(local.sun_len) 
        + len + 1;
    err = ___pdr_socket_bind(&un->sock, (___pdr_SA *) &local, local.sun_len);

#else 
    err = ___pdr_socket_bind(&un->sock, (___pdr_SA *) &local, 
            sizeof(local.sun_family) + len);
#endif
    if (err != PDR_IO_DONE) ___pdr_socket_destroy(&un->sock);
    return ___pdr_socket_strerror(err); 
#else
	return NULL;
#endif
}

static int unix_meth_bind(___pdr_lua_State *L) {
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkclass(L, "unix{master}", 1);
    const char *path =  ___pdr_luaL_checkstring(L, 2);
    const char *err = unix_trybind(un, path);
    if (err) {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, err);
        return 2;
    }
    ___pdr_lua_pushnumber(L, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Turns a master unix object into a client object.
\*-------------------------------------------------------------------------*/
static const char *unix_tryconnect(___pdr_p_unix un, const char *path)
{
#ifndef _WIN32
    struct sockaddr_un remote;
    int err;
    size_t len = strlen(path);
    if (len >= sizeof(remote.sun_path)) return "path too long";
    memset(&remote, 0, sizeof(remote));
    strcpy(remote.sun_path, path);
    remote.sun_family = AF_UNIX;
    ___pdr_timeout_markstart(&un->tm);
#ifdef UNIX_HAS_SUN_LEN
    remote.sun_len = sizeof(remote.sun_family) + sizeof(remote.sun_len) 
        + len + 1;
    err = ___pdr_socket_connect(&un->sock, (___pdr_SA *) &remote, remote.sun_len, &un->tm);
#else
    err = ___pdr_socket_connect(&un->sock, (___pdr_SA *) &remote, 
            sizeof(remote.sun_family) + len, &un->tm);
#endif
    if (err != PDR_IO_DONE) ___pdr_socket_destroy(&un->sock);
    return ___pdr_socket_strerror(err);
#else
	return NULL;
#endif
}

static int unix_meth_connect(___pdr_lua_State *L)
{
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkclass(L, "unix{master}", 1);
    const char *path =  ___pdr_luaL_checkstring(L, 2);
    const char *err = unix_tryconnect(un, path);
    if (err) {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, err);
        return 2;
    }
    /* turn master object into a client object */
    ___pdr_auxiliar_setclass(L, "unix{client}", 1);
    ___pdr_lua_pushnumber(L, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Closes socket used by object 
\*-------------------------------------------------------------------------*/
static int unix_meth_close(___pdr_lua_State *L)
{
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkgroup(L, "unix{any}", 1);
    ___pdr_socket_destroy(&un->sock);
    ___pdr_lua_pushnumber(L, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Puts the sockt in listen mode
\*-------------------------------------------------------------------------*/
static int unix_meth_listen(___pdr_lua_State *L)
{
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkclass(L, "unix{master}", 1);
    int backlog = (int) ___pdr_luaL_optnumber(L, 2, 32);
    int err = ___pdr_socket_listen(&un->sock, backlog);
    if (err != PDR_IO_DONE) {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, ___pdr_socket_strerror(err));
        return 2;
    }
    /* turn master object into a server object */
    ___pdr_auxiliar_setclass(L, "unix{server}", 1);
    ___pdr_lua_pushnumber(L, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Shuts the connection down partially
\*-------------------------------------------------------------------------*/
static int unix_meth_shutdown(___pdr_lua_State *L)
{
    /* SHUT_RD,  SHUT_WR,  SHUT_RDWR  have  the value 0, 1, 2, so we can use method index directly */
    static const char* methods[] = { "receive", "send", "both", NULL };
    ___pdr_p_unix tcp = (___pdr_p_unix) ___pdr_auxiliar_checkclass(L, "unix{client}", 1);
    int how = ___pdr_luaL_checkoption(L, 2, "both", methods);
    ___pdr_socket_shutdown(&tcp->sock, how);
    ___pdr_lua_pushnumber(L, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Just call tm methods
\*-------------------------------------------------------------------------*/
static int unix_meth_settimeout(___pdr_lua_State *L) {
    ___pdr_p_unix un = (___pdr_p_unix) ___pdr_auxiliar_checkgroup(L, "unix{any}", 1);
    return ___pdr_timeout_meth_settimeout(L, &un->tm);
}

/*=========================================================================*\
* Library functions
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Creates a master unix object 
\*-------------------------------------------------------------------------*/
static int unix_global_create(___pdr_lua_State *L) {
    ___pdr_t_socket sock;
    int err = ___pdr_socket_create(&sock, AF_UNIX, SOCK_STREAM, 0);
    /* try to allocate a system socket */
    if (err == PDR_IO_DONE) { 
        /* allocate unix object */
        ___pdr_p_unix un = (___pdr_p_unix) ___pdr_lua_newuserdata(L, sizeof(___pdr_t_unix));
        /* set its type as master object */
        ___pdr_auxiliar_setclass(L, "unix{master}", -1);
        /* initialize remaining structure fields */
        ___pdr_socket_setnonblocking(&sock);
        un->sock = sock;
        ___pdr_io_init(&un->io, (___pdr_p_send) ___pdr_socket_send, (___pdr_p_recv) ___pdr_socket_recv, 
                (___pdr_p_error) ___pdr_socket_ioerror, &un->sock);
        ___pdr_timeout_init(&un->tm, -1, -1);
        ___pdr_buffer_init(&un->buf, &un->io, &un->tm);
        return 1;
    } else {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, ___pdr_socket_strerror(err));
        return 2;
    }
}

} // end NS_PDR_SLUA
