/*=========================================================================*\
* TCP object
* LuaSocket toolkit
\*=========================================================================*/
#include "tcp.h"
#include <string.h>

#include "lua.h"
#include "lauxlib.h"

#include "auxiliar.h"
#include "socket.h"
#include "inet.h"
#include "options.h"

namespace NS_PDR_SLUA {    

/*=========================================================================*\
* Internal function prototypes
\*=========================================================================*/
static int tcp_global_create(___pdr_lua_State *L);
static int tcp_global_create6(___pdr_lua_State *L);
static int tcp_global_connect(___pdr_lua_State *L);
static int tcp_meth_connect(___pdr_lua_State *L);
static int tcp_meth_listen(___pdr_lua_State *L);
static int tcp_meth_getfamily(___pdr_lua_State *L);
static int tcp_meth_bind(___pdr_lua_State *L);
static int tcp_meth_send(___pdr_lua_State *L);
static int tcp_meth_getstats(___pdr_lua_State *L);
static int tcp_meth_setstats(___pdr_lua_State *L);
static int tcp_meth_getsockname(___pdr_lua_State *L);
static int tcp_meth_getpeername(___pdr_lua_State *L);
static int tcp_meth_shutdown(___pdr_lua_State *L);
static int tcp_meth_receive(___pdr_lua_State *L);
static int tcp_meth_accept(___pdr_lua_State *L);
static int tcp_meth_close(___pdr_lua_State *L);
static int tcp_meth_getoption(___pdr_lua_State *L);
static int tcp_meth_setoption(___pdr_lua_State *L);
static int tcp_meth_settimeout(___pdr_lua_State *L);
static int tcp_meth_getfd(___pdr_lua_State *L);
static int tcp_meth_setfd(___pdr_lua_State *L);
static int tcp_meth_dirty(___pdr_lua_State *L);

/* tcp object methods */
static ___pdr_luaL_Reg tcp_methods[] = {
    {"__gc",        tcp_meth_close},
    {"__tostring",  ___pdr_auxiliar_tostring},
    {"accept",      tcp_meth_accept},
    {"bind",        tcp_meth_bind},
    {"close",       tcp_meth_close},
    {"connect",     tcp_meth_connect},
    {"dirty",       tcp_meth_dirty},
    {"getfamily",   tcp_meth_getfamily},
    {"getfd",       tcp_meth_getfd},
    {"getoption",   tcp_meth_getoption},
    {"getpeername", tcp_meth_getpeername},
    {"getsockname", tcp_meth_getsockname},
    {"getstats",    tcp_meth_getstats},
    {"setstats",    tcp_meth_setstats},
    {"listen",      tcp_meth_listen},
    {"receive",     tcp_meth_receive},
    {"send",        tcp_meth_send},
    {"setfd",       tcp_meth_setfd},
    {"setoption",   tcp_meth_setoption},
    {"setpeername", tcp_meth_connect},
    {"setsockname", tcp_meth_bind},
    {"settimeout",  tcp_meth_settimeout},
    {"shutdown",    tcp_meth_shutdown},
    {NULL,          NULL}
};

/* socket option handlers */
static ___pdr_t_opt tcp_optget[] = {
    {"keepalive",   ___pdr_opt_get_keepalive},
    {"reuseaddr",   ___pdr_opt_get_reuseaddr},
    {"tcp-nodelay", ___pdr_opt_get_tcp_nodelay},
    {"linger",      ___pdr_opt_get_linger},
    {"error",       ___opt_get_error},
    {NULL,          NULL}
};

static ___pdr_t_opt tcp_optset[] = {
    {"keepalive",   ___pdr_opt_set_keepalive},
    {"reuseaddr",   ___pdr_opt_set_reuseaddr},
    {"tcp-nodelay", ___pdr_opt_set_tcp_nodelay},
    {"ipv6-v6only", ___pdr_opt_set_ip6_v6only},
    {"linger",      ___pdr_opt_set_linger},
    {NULL,          NULL}
};

/* functions in library namespace */
static ___pdr_luaL_Reg tcp_func[] = {
    {"tcp", tcp_global_create},
    {"tcp6", tcp_global_create6},
    {"connect", tcp_global_connect},
    {NULL, NULL}
};

/*-------------------------------------------------------------------------*\
* Initializes module
\*-------------------------------------------------------------------------*/
int tcp_open(___pdr_lua_State *L)
{
    /* create classes */
    ___pdr_auxiliar_newclass(L, "tcp{master}", tcp_methods);
    ___pdr_auxiliar_newclass(L, "tcp{client}", tcp_methods);
    ___pdr_auxiliar_newclass(L, "tcp{server}", tcp_methods);
    /* create class groups */
    ___pdr_auxiliar_add2group(L, "tcp{master}", "tcp{any}");
    ___pdr_auxiliar_add2group(L, "tcp{client}", "tcp{any}");
    ___pdr_auxiliar_add2group(L, "tcp{server}", "tcp{any}");
    /* define library functions */
#if ___PDR_LUA_VERSION_NUM > 501 && !defined(___PDR_LUA_COMPAT_MODULE)
    ___pdr_luaL_setfuncs(L, tcp_func, 0);
#else
    ___pdr_luaL_openlib(L, NULL, tcp_func, 0);
#endif
    return 0;
}

/*=========================================================================*\
* Lua methods
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Just call buffered IO methods
\*-------------------------------------------------------------------------*/
static int tcp_meth_send(___pdr_lua_State *L) {
    ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_auxiliar_checkclass(L, "tcp{client}", 1);
    return ___pdr_buffer_meth_send(L, &tcp->buf);
}

static int tcp_meth_receive(___pdr_lua_State *L) {
    ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_auxiliar_checkclass(L, "tcp{client}", 1);
    return ___pdr_buffer_meth_receive(L, &tcp->buf);
}

static int tcp_meth_getstats(___pdr_lua_State *L) {
    ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_auxiliar_checkclass(L, "tcp{client}", 1);
    return ___pdr_buffer_meth_getstats(L, &tcp->buf);
}

static int tcp_meth_setstats(___pdr_lua_State *L) {
    ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_auxiliar_checkclass(L, "tcp{client}", 1);
    return ___pdr_buffer_meth_setstats(L, &tcp->buf);
}

/*-------------------------------------------------------------------------*\
* Just call option handler
\*-------------------------------------------------------------------------*/
static int tcp_meth_getoption(___pdr_lua_State *L)
{
    ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_auxiliar_checkgroup(L, "tcp{any}", 1);
    return ___opt_meth_getoption(L, tcp_optget, &tcp->sock);
}

static int tcp_meth_setoption(___pdr_lua_State *L)
{
    ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_auxiliar_checkgroup(L, "tcp{any}", 1);
    return ___opt_meth_setoption(L, tcp_optset, &tcp->sock);
}

/*-------------------------------------------------------------------------*\
* Select support methods
\*-------------------------------------------------------------------------*/
static int tcp_meth_getfd(___pdr_lua_State *L)
{
    ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_auxiliar_checkgroup(L, "tcp{any}", 1);
    ___pdr_lua_pushnumber(L, (int) tcp->sock);
    return 1;
}

/* this is very dangerous, but can be handy for those that are brave enough */
static int tcp_meth_setfd(___pdr_lua_State *L)
{
    ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_auxiliar_checkgroup(L, "tcp{any}", 1);
    tcp->sock = (___pdr_t_socket) ___pdr_luaL_checknumber(L, 2);
    return 0;
}

static int tcp_meth_dirty(___pdr_lua_State *L)
{
    ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_auxiliar_checkgroup(L, "tcp{any}", 1);
    ___pdr_lua_pushboolean(L, !___pdr_buffer_isempty(&tcp->buf));
    return 1;
}

/*-------------------------------------------------------------------------*\
* Waits for and returns a client object attempting connection to the
* server object
\*-------------------------------------------------------------------------*/
static int tcp_meth_accept(___pdr_lua_State *L)
{
    ___pdr_p_tcp server = (___pdr_p_tcp) ___pdr_auxiliar_checkclass(L, "tcp{server}", 1);
    ___pdr_p_timeout tm = ___pdr_timeout_markstart(&server->tm);
    ___pdr_t_socket sock;
    const char *err = ___pdr_inet_tryaccept(&server->sock, server->family, &sock, tm);
    /* if successful, push client socket */
    if (err == NULL) {
        ___pdr_p_tcp clnt = (___pdr_p_tcp) ___pdr_lua_newuserdata(L, sizeof(___pdr_t_tcp));
        ___pdr_auxiliar_setclass(L, "tcp{client}", -1);
        /* initialize structure fields */
        memset(clnt, 0, sizeof(___pdr_t_tcp));
        ___pdr_socket_setnonblocking(&sock);
        clnt->sock = sock;
        ___pdr_io_init(&clnt->io, (___pdr_p_send) ___pdr_socket_send, (___pdr_p_recv) ___pdr_socket_recv,
                (___pdr_p_error) ___pdr_socket_ioerror, &clnt->sock);
        ___pdr_timeout_init(&clnt->tm, -1, -1);
        ___pdr_buffer_init(&clnt->buf, &clnt->io, &clnt->tm);
        clnt->family = server->family;
        return 1;
    } else {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, err);
        return 2;
    }
}

/*-------------------------------------------------------------------------*\
* Binds an object to an address
\*-------------------------------------------------------------------------*/
static int tcp_meth_bind(___pdr_lua_State *L)
{
    ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_auxiliar_checkclass(L, "tcp{master}", 1);
    const char *address =  ___pdr_luaL_checkstring(L, 2);
    const char *port = ___pdr_luaL_checkstring(L, 3);
    const char *err;
    struct addrinfo bindhints;
    memset(&bindhints, 0, sizeof(bindhints));
    bindhints.ai_socktype = SOCK_STREAM;
    bindhints.ai_family = tcp->family;
    bindhints.ai_flags = AI_PASSIVE;
    err = ___pdr_inet_trybind(&tcp->sock, address, port, &bindhints);
    if (err) {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, err);
        return 2;
    }
    ___pdr_lua_pushnumber(L, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Turns a master tcp object into a client object.
\*-------------------------------------------------------------------------*/
// static int tcp_meth_connect(___pdr_lua_State *L)
// {
//     p_tcp tcp = (p_tcp) auxiliar_checkgroup(L, "tcp{any}", 1);
//     const char *address =  luaL_checkstring(L, 2);
//     const char *port = luaL_checkstring(L, 3);
//     struct addrinfo connecthints;
//     const char *err;
//     memset(&connecthints, 0, sizeof(connecthints));
//     connecthints.ai_socktype = SOCK_STREAM;
//     /* make sure we try to connect only to the same family */
//     connecthints.ai_family = tcp->family;
//     timeout_markstart(&tcp->tm);
//     err = inet_tryconnect(&tcp->sock, &tcp->family, address, port, 
//         &tcp->tm, &connecthints);
//     /* have to set the class even if it failed due to non-blocking connects */
//     auxiliar_setclass(L, "tcp{client}", 1);
//     if (err) {
//         lua_pushnil(L);
//         lua_pushstring(L, err);
//         return 2;
//     }
//     lua_pushnumber(L, 1);
//     return 1;
// }

/*-------------------------------------------------------------------------*\
* replacement of old tcp_meth_connect
* Turns a master tcp object into a client object.
* only accept ip address as host
\*-------------------------------------------------------------------------*/
static int tcp_meth_connect(___pdr_lua_State *L)
{
    ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_auxiliar_checkgroup(L, "tcp{any}", 1);
    const char *ip = ___pdr_luaL_checkstring(L, 2);
    int port = ___pdr_luaL_checkinteger(L, 3);
    struct sockaddr_in ipv4tester;
    struct sockaddr_in6 ipv6tester;
    if (inet_pton(AF_INET, ip, &(ipv4tester.sin_addr)) <= 0
        && inet_pton(AF_INET6, ip, &(ipv6tester.sin6_addr)) <= 0) {
        ___pdr_luaL_argcheck(L, false, 2, "ip address invalid");
    }

    ___pdr_timeout_markstart(&tcp->tm);
    const char * err;
    err = ___pdr_inet_tryconnect_with_ip(&tcp->sock, &tcp->family, ip, port, SOCK_STREAM, &tcp->tm);
    ___pdr_auxiliar_setclass(L, "tcp{client}", 1);
    if (err) {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, err);
        return 2;
    }
    ___pdr_lua_pushnumber(L, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Closes socket used by object
\*-------------------------------------------------------------------------*/
static int tcp_meth_close(___pdr_lua_State *L)
{
    ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_auxiliar_checkgroup(L, "tcp{any}", 1);
    ___pdr_socket_destroy(&tcp->sock);
    ___pdr_lua_pushnumber(L, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Returns family as string
\*-------------------------------------------------------------------------*/
static int tcp_meth_getfamily(___pdr_lua_State *L)
{
    ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_auxiliar_checkgroup(L, "tcp{any}", 1);
    if (tcp->family == PF_INET6) {
        ___pdr_lua_pushliteral(L, "inet6");
        return 1;
    } else {
        ___pdr_lua_pushliteral(L, "inet4");
        return 1;
    }
}

/*-------------------------------------------------------------------------*\
* Puts the sockt in listen mode
\*-------------------------------------------------------------------------*/
static int tcp_meth_listen(___pdr_lua_State *L)
{
    ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_auxiliar_checkclass(L, "tcp{master}", 1);
    int backlog = (int) ___pdr_luaL_optnumber(L, 2, 32);
    int err = ___pdr_socket_listen(&tcp->sock, backlog);
    if (err != PDR_IO_DONE) {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, ___pdr_socket_strerror(err));
        return 2;
    }
    /* turn master object into a server object */
    ___pdr_auxiliar_setclass(L, "tcp{server}", 1);
    ___pdr_lua_pushnumber(L, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Shuts the connection down partially
\*-------------------------------------------------------------------------*/
static int tcp_meth_shutdown(___pdr_lua_State *L)
{
    /* SHUT_RD,  SHUT_WR,  SHUT_RDWR  have  the value 0, 1, 2, so we can use method index directly */
    static const char* methods[] = { "receive", "send", "both", NULL };
    ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_auxiliar_checkclass(L, "tcp{client}", 1);
    int how = ___pdr_luaL_checkoption(L, 2, "both", methods);
    ___pdr_socket_shutdown(&tcp->sock, how);
    ___pdr_lua_pushnumber(L, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Just call inet methods
\*-------------------------------------------------------------------------*/
static int tcp_meth_getpeername(___pdr_lua_State *L)
{
    ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_auxiliar_checkgroup(L, "tcp{any}", 1);
    return ___pdr_inet_meth_getpeername(L, &tcp->sock, tcp->family);
}

static int tcp_meth_getsockname(___pdr_lua_State *L)
{
    ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_auxiliar_checkgroup(L, "tcp{any}", 1);
    return ___pdr_inet_meth_getsockname(L, &tcp->sock, tcp->family);
}

/*-------------------------------------------------------------------------*\
* Just call tm methods
\*-------------------------------------------------------------------------*/
static int tcp_meth_settimeout(___pdr_lua_State *L)
{
    ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_auxiliar_checkgroup(L, "tcp{any}", 1);
    return ___pdr_timeout_meth_settimeout(L, &tcp->tm);
}

/*=========================================================================*\
* Library functions
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Creates a master tcp object
\*-------------------------------------------------------------------------*/
static int tcp_create(___pdr_lua_State *L, int family) {
    ___pdr_t_socket sock;
    const char *err = ___pdr_inet_trycreate(&sock, family, SOCK_STREAM);
    /* try to allocate a system socket */
    if (!err) {
        /* allocate tcp object */
        ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_lua_newuserdata(L, sizeof(___pdr_t_tcp));
        memset(tcp, 0, sizeof(___pdr_t_tcp));
        /* set its type as master object */
        ___pdr_auxiliar_setclass(L, "tcp{master}", -1);
        /* initialize remaining structure fields */
        ___pdr_socket_setnonblocking(&sock);
        if (family == PF_INET6) {
            int yes = 1;
            setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY,
                reinterpret_cast<const char *>(&yes), sizeof(yes));
        }
        tcp->sock = sock;
        ___pdr_io_init(&tcp->io, (___pdr_p_send) ___pdr_socket_send, (___pdr_p_recv) ___pdr_socket_recv,
                (___pdr_p_error) ___pdr_socket_ioerror, &tcp->sock);
        ___pdr_timeout_init(&tcp->tm, -1, -1);
        ___pdr_buffer_init(&tcp->buf, &tcp->io, &tcp->tm);
        tcp->family = family;
        return 1;
    } else {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, err);
        return 2;
    }
}

static int tcp_global_create(___pdr_lua_State *L) {
    return tcp_create(L, AF_INET);
}

static int tcp_global_create6(___pdr_lua_State *L) {
    return tcp_create(L, AF_INET6);
}

#if 0
static const char *tryconnect6(const char *remoteaddr, const char *remoteserv,
    struct addrinfo *connecthints, p_tcp tcp) {
    struct addrinfo *iterator = NULL, *resolved = NULL;
    const char *err = NULL;
    /* try resolving */
    err = ___pdr_socket_gaistrerror(getaddrinfo(remoteaddr, remoteserv,
                connecthints, &resolved));
    if (err != NULL) {
        if (resolved) freeaddrinfo(resolved);
        return err;
    }
    /* iterate over all returned addresses trying to connect */
    for (iterator = resolved; iterator; iterator = iterator->ai_next) {
        p_timeout tm = timeout_markstart(&tcp->tm);
        /* create new socket if necessary. if there was no
         * bind, we need to create one for every new family
         * that shows up while iterating. if there was a
         * bind, all families will be the same and we will
         * not enter this branch. */
        if (tcp->family != iterator->ai_family) {
            ___pdr_socket_destroy(&tcp->sock);
            err = ___pdr_socket_strerror(___pdr_socket_create(&tcp->sock,
                iterator->ai_family, iterator->ai_socktype,
                iterator->ai_protocol));
            if (err != NULL) {
                freeaddrinfo(resolved);
                return err;
            }
            tcp->family = iterator->ai_family;
            /* all sockets initially non-blocking */
            ___pdr_socket_setnonblocking(&tcp->sock);
        }
        /* finally try connecting to remote address */
        err = ___pdr_socket_strerror(___pdr_socket_connect(&tcp->sock,
            (___pdr_SA *) iterator->ai_addr,
            (socklen_t) iterator->ai_addrlen, tm));
        /* if success, break out of loop */
        if (err == NULL) break;
    }

    freeaddrinfo(resolved);
    /* here, if err is set, we failed */
    return err;
}
#endif

static int tcp_global_connect(___pdr_lua_State *L) {
    const char *remoteaddr = ___pdr_luaL_checkstring(L, 1);
    const char *remoteserv = ___pdr_luaL_checkstring(L, 2);
    const char *localaddr  = ___pdr_luaL_optstring(L, 3, NULL);
    const char *localserv  = ___pdr_luaL_optstring(L, 4, "0");
    int family = ___pdr_inet_optfamily(L, 5, "unspec");
    ___pdr_p_tcp tcp = (___pdr_p_tcp) ___pdr_lua_newuserdata(L, sizeof(___pdr_t_tcp));
    struct addrinfo bindhints, connecthints;
    const char *err = NULL;
    /* initialize tcp structure */
    memset(tcp, 0, sizeof(___pdr_t_tcp));
    ___pdr_io_init(&tcp->io, (___pdr_p_send) ___pdr_socket_send, (___pdr_p_recv) ___pdr_socket_recv,
            (___pdr_p_error) ___pdr_socket_ioerror, &tcp->sock);
    ___pdr_timeout_init(&tcp->tm, -1, -1);
    ___pdr_buffer_init(&tcp->buf, &tcp->io, &tcp->tm);
    tcp->sock = SOCKET_INVALID;
    tcp->family = PF_UNSPEC;
    /* allow user to pick local address and port */
    memset(&bindhints, 0, sizeof(bindhints));
    bindhints.ai_socktype = SOCK_STREAM;
    bindhints.ai_family = family;
    bindhints.ai_flags = AI_PASSIVE;
    if (localaddr) {
        err = ___pdr_inet_trybind(&tcp->sock, localaddr, localserv, &bindhints);
        if (err) {
            ___pdr_lua_pushnil(L);
            ___pdr_lua_pushstring(L, err);
            return 2;
        }
        tcp->family = bindhints.ai_family;
    }
    /* try to connect to remote address and port */
    memset(&connecthints, 0, sizeof(connecthints));
    connecthints.ai_socktype = SOCK_STREAM;
    /* make sure we try to connect only to the same family */
    connecthints.ai_family = bindhints.ai_family;
    err = ___pdr_inet_tryconnect(&tcp->sock, &tcp->family, remoteaddr, remoteserv,
         &tcp->tm, &connecthints);
    if (err) {
        ___pdr_socket_destroy(&tcp->sock);
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, err);
        return 2;
    }
    ___pdr_auxiliar_setclass(L, "tcp{client}", -1);
    return 1;
}

} // end NS_PDR_SLUA
