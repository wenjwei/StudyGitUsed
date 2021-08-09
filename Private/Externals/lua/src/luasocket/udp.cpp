/*=========================================================================*\
* UDP object
* LuaSocket toolkit
\*=========================================================================*/
#include "udp.h"
#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"

#include "auxiliar.h"
#include "socket.h"
#include "inet.h"
#include "options.h"

/* min and max macros */
#ifndef ___PDR_MIN
#define ___PDR_MIN(x, y) ((x) < (y) ? x : y)
#endif
#ifndef ___PDR_MAX
#define ___PDR_MAX(x, y) ((x) > (y) ? x : y)
#endif

#ifdef _WIN32
#define gai_strerror gai_strerrorA
#endif

namespace NS_PDR_SLUA {    

/*=========================================================================*\
* Internal function prototypes
\*=========================================================================*/
static int udp_global_create(___pdr_lua_State *L);
static int udp_global_create6(___pdr_lua_State *L);
static int udp_meth_send(___pdr_lua_State *L);
static int udp_meth_sendto(___pdr_lua_State *L);
static int udp_meth_receive(___pdr_lua_State *L);
static int udp_meth_receivefrom(___pdr_lua_State *L);
static int udp_meth_getfamily(___pdr_lua_State *L);
static int udp_meth_getsockname(___pdr_lua_State *L);
static int udp_meth_getpeername(___pdr_lua_State *L);
static int udp_meth_setsockname(___pdr_lua_State *L);
static int udp_meth_setpeername(___pdr_lua_State *L);
static int udp_meth_close(___pdr_lua_State *L);
static int udp_meth_setoption(___pdr_lua_State *L);
static int udp_meth_getoption(___pdr_lua_State *L);
static int udp_meth_settimeout(___pdr_lua_State *L);
static int udp_meth_getfd(___pdr_lua_State *L);
static int udp_meth_setfd(___pdr_lua_State *L);
static int udp_meth_dirty(___pdr_lua_State *L);

/* udp object methods */
static ___pdr_luaL_Reg udp_methods[] = {
    {"__gc",        udp_meth_close},
    {"__tostring",  ___pdr_auxiliar_tostring},
    {"close",       udp_meth_close},
    {"dirty",       udp_meth_dirty},
    {"getfamily",   udp_meth_getfamily},
    {"getfd",       udp_meth_getfd},
    {"getpeername", udp_meth_getpeername},
    {"getsockname", udp_meth_getsockname},
    {"receive",     udp_meth_receive},
    {"receivefrom", udp_meth_receivefrom},
    {"send",        udp_meth_send},
    {"sendto",      udp_meth_sendto},
    {"setfd",       udp_meth_setfd},
    {"setoption",   udp_meth_setoption},
    {"getoption",   udp_meth_getoption},
    {"setpeername", udp_meth_setpeername},
    {"setsockname", udp_meth_setsockname},
    {"settimeout",  udp_meth_settimeout},
    {NULL,          NULL}
};

/* socket options for setoption */
static ___pdr_t_opt udp_optset[] = {
    {"dontroute",            ___pdr_opt_set_dontroute},
    {"broadcast",            ___pdr_opt_set_broadcast},
    {"reuseaddr",            ___pdr_opt_set_reuseaddr},
    {"reuseport",            ___pdr_opt_set_reuseport},
    {"ip-multicast-if",      ___pdr_opt_set_ip_multicast_if},
    {"ip-multicast-ttl",     ___pdr_opt_set_ip_multicast_ttl},
    {"ip-multicast-loop",    ___pdr_opt_set_ip_multicast_loop},
    {"ip-add-membership",    ___pdr_opt_set_ip_add_membership},
    {"ip-drop-membership",   ___pdr_opt_set_ip_drop_membersip},
    {"ipv6-unicast-hops",    ___pdr_opt_set_ip6_unicast_hops},
    {"ipv6-multicast-hops",  ___pdr_opt_set_ip6_unicast_hops},
    {"ipv6-multicast-loop",  ___pdr_opt_set_ip6_multicast_loop},
    {"ipv6-add-membership",  ___pdr_opt_set_ip6_add_membership},
    {"ipv6-drop-membership", ___pdr_opt_set_ip6_drop_membersip},
    {"ipv6-v6only",          ___pdr_opt_set_ip6_v6only},
    {NULL,                   NULL}
};

/* socket options for getoption */
static ___pdr_t_opt udp_optget[] = {
    {"ip-multicast-if",      ___opt_get_ip_multicast_if},
    {"ip-multicast-loop",    ___pdr_opt_get_ip_multicast_loop},
    {"error",                ___opt_get_error},
    {"ipv6-unicast-hops",    ___opt_get_ip6_unicast_hops},
    {"ipv6-multicast-hops",  ___opt_get_ip6_unicast_hops},
    {"ipv6-multicast-loop",  ___opt_get_ip6_multicast_loop},
    {"ipv6-v6only",          ___opt_get_ip6_v6only},
    {NULL,                   NULL}
};

/* functions in library namespace */
static ___pdr_luaL_Reg udp_func[] = {
    {"udp", udp_global_create},
    {"udp6", udp_global_create6},
    {NULL, NULL}
};

/*-------------------------------------------------------------------------*\
* Initializes module
\*-------------------------------------------------------------------------*/
int ___pdr_udp_open(___pdr_lua_State *L)
{
    /* create classes */
    ___pdr_auxiliar_newclass(L, "udp{connected}", udp_methods);
    ___pdr_auxiliar_newclass(L, "udp{unconnected}", udp_methods);
    /* create class groups */
    ___pdr_auxiliar_add2group(L, "udp{connected}",   "udp{any}");
    ___pdr_auxiliar_add2group(L, "udp{unconnected}", "udp{any}");
    ___pdr_auxiliar_add2group(L, "udp{connected}",   "select{able}");
    ___pdr_auxiliar_add2group(L, "udp{unconnected}", "select{able}");
    /* define library functions */
#if ___PDR_LUA_VERSION_NUM > 501 && !defined(___PDR_LUA_COMPAT_MODULE)
    ___pdr_luaL_setfuncs(L, udp_func, 0);
#else
    ___pdr_luaL_openlib(L, NULL, udp_func, 0);
#endif
    return 0;
}

/*=========================================================================*\
* Lua methods
\*=========================================================================*/
const char *udp_strerror(int err) {
    /* a 'closed' error on an unconnected means the target address was not
     * accepted by the transport layer */
    if (err == PDR_IO_CLOSED) return "refused";
    else return ___pdr_socket_strerror(err);
}

/*-------------------------------------------------------------------------*\
* Send data through connected udp socket
\*-------------------------------------------------------------------------*/
static int udp_meth_send(___pdr_lua_State *L) {
    ___pdr_p_udp udp = (___pdr_p_udp) ___pdr_auxiliar_checkclass(L, "udp{connected}", 1);
    ___pdr_p_timeout tm = &udp->tm;
    size_t count, sent = 0;
    int err;
    const char *data = ___pdr_luaL_checklstring(L, 2, &count);
    ___pdr_timeout_markstart(tm);
    err = ___pdr_socket_send(&udp->sock, data, count, &sent, tm);
    if (err != PDR_IO_DONE) {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, udp_strerror(err));
        return 2;
    }
    ___pdr_lua_pushnumber(L, (___pdr_lua_Number) sent);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Send data through unconnected udp socket
\*-------------------------------------------------------------------------*/
static int udp_meth_sendto(___pdr_lua_State *L) {
    ___pdr_p_udp udp = (___pdr_p_udp) ___pdr_auxiliar_checkclass(L, "udp{unconnected}", 1);
    size_t count, sent = 0;
    const char *data = ___pdr_luaL_checklstring(L, 2, &count);
    const char *ip = ___pdr_luaL_checkstring(L, 3);
    const char *port = ___pdr_luaL_checkstring(L, 4);
    ___pdr_p_timeout tm = &udp->tm;
    int err;
    struct addrinfo aihint;
    struct addrinfo *ai;
    memset(&aihint, 0, sizeof(aihint));
    aihint.ai_family = udp->family;
    aihint.ai_socktype = SOCK_DGRAM;
    aihint.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
    err = getaddrinfo(ip, port, &aihint, &ai);
	if (err) {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, gai_strerror(err));
        return 2;
    }
    ___pdr_timeout_markstart(tm);
    err = ___pdr_socket_sendto(&udp->sock, data, count, &sent, ai->ai_addr, 
        (___pdr_socklen_t) ai->ai_addrlen, tm);
    freeaddrinfo(ai);
    if (err != PDR_IO_DONE) {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, udp_strerror(err));
        return 2;
    }
    ___pdr_lua_pushnumber(L, (___pdr_lua_Number) sent);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Receives data from a UDP socket
\*-------------------------------------------------------------------------*/
static int udp_meth_receive(___pdr_lua_State *L) {
    ___pdr_p_udp udp = (___pdr_p_udp) ___pdr_auxiliar_checkgroup(L, "udp{any}", 1);
    char buffer[___PDR_UDP_DATAGRAMSIZE];
    size_t got, count = (size_t) ___pdr_luaL_optnumber(L, 2, sizeof(buffer));
    int err;
    ___pdr_p_timeout tm = &udp->tm;
    count = ___PDR_MIN(count, sizeof(buffer));
    ___pdr_timeout_markstart(tm);
    err = ___pdr_socket_recv(&udp->sock, buffer, count, &got, tm);
    /* Unlike TCP, recv() of zero is not closed, but a zero-length packet. */
    if (err == PDR_IO_CLOSED)
        err = PDR_IO_DONE;
    if (err != PDR_IO_DONE) {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, udp_strerror(err));
        return 2;
    }
    ___pdr_lua_pushlstring(L, buffer, got);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Receives data and sender from a UDP socket
\*-------------------------------------------------------------------------*/
static int udp_meth_receivefrom(___pdr_lua_State *L)
{
    ___pdr_p_udp udp = (___pdr_p_udp) ___pdr_auxiliar_checkclass(L, "udp{unconnected}", 1);
    char buffer[___PDR_UDP_DATAGRAMSIZE];
    size_t got, count = (size_t) ___pdr_luaL_optnumber(L, 2, sizeof(buffer));
    int err;
    ___pdr_p_timeout tm = &udp->tm;
    struct sockaddr_storage addr;
    ___pdr_socklen_t addr_len = sizeof(addr);
    char addrstr[INET6_ADDRSTRLEN];
    char portstr[6];
    ___pdr_timeout_markstart(tm);
    count = ___PDR_MIN(count, sizeof(buffer));
    err = ___pdr_socket_recvfrom(&udp->sock, buffer, count, &got, (___pdr_SA *) &addr, 
            &addr_len, tm);
    /* Unlike TCP, recv() of zero is not closed, but a zero-length packet. */
    if (err == PDR_IO_CLOSED)
        err = PDR_IO_DONE;
    if (err != PDR_IO_DONE) {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, udp_strerror(err));
        return 2;
    }
    err = getnameinfo((struct sockaddr *)&addr, addr_len, addrstr, 
        INET6_ADDRSTRLEN, portstr, 6, NI_NUMERICHOST | NI_NUMERICSERV);
	if (err) {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, gai_strerror(err));
        return 2;
    }
    ___pdr_lua_pushlstring(L, buffer, got);
    ___pdr_lua_pushstring(L, addrstr);
    ___pdr_lua_pushinteger(L, (int) strtol(portstr, (char **) NULL, 10));
    return 3;
}

/*-------------------------------------------------------------------------*\
* Returns family as string
\*-------------------------------------------------------------------------*/
static int udp_meth_getfamily(___pdr_lua_State *L)
{
    ___pdr_p_udp udp = (___pdr_p_udp) ___pdr_auxiliar_checkgroup(L, "udp{any}", 1);
    if (udp->family == PF_INET6) {
        ___pdr_lua_pushliteral(L, "inet6");
        return 1;
    } else {
        ___pdr_lua_pushliteral(L, "inet4");
        return 1;
    }
}

/*-------------------------------------------------------------------------*\
* Select support methods
\*-------------------------------------------------------------------------*/
static int udp_meth_getfd(___pdr_lua_State *L) {
    ___pdr_p_udp udp = (___pdr_p_udp) ___pdr_auxiliar_checkgroup(L, "udp{any}", 1);
    ___pdr_lua_pushnumber(L, (int) udp->sock);
    return 1;
}

/* this is very dangerous, but can be handy for those that are brave enough */
static int udp_meth_setfd(___pdr_lua_State *L) {
    ___pdr_p_udp udp = (___pdr_p_udp) ___pdr_auxiliar_checkgroup(L, "udp{any}", 1);
    udp->sock = (___pdr_t_socket) ___pdr_luaL_checknumber(L, 2);
    return 0;
}

static int udp_meth_dirty(___pdr_lua_State *L) {
    ___pdr_p_udp udp = (___pdr_p_udp) ___pdr_auxiliar_checkgroup(L, "udp{any}", 1);
    (void) udp;
    ___pdr_lua_pushboolean(L, 0);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Just call inet methods
\*-------------------------------------------------------------------------*/
static int udp_meth_getpeername(___pdr_lua_State *L) {
    ___pdr_p_udp udp = (___pdr_p_udp) ___pdr_auxiliar_checkclass(L, "udp{connected}", 1);
    return ___pdr_inet_meth_getpeername(L, &udp->sock, udp->family);
}

static int udp_meth_getsockname(___pdr_lua_State *L) {
    ___pdr_p_udp udp = (___pdr_p_udp) ___pdr_auxiliar_checkgroup(L, "udp{any}", 1);
    return ___pdr_inet_meth_getsockname(L, &udp->sock, udp->family);
}

/*-------------------------------------------------------------------------*\
* Just call option handler
\*-------------------------------------------------------------------------*/
static int udp_meth_setoption(___pdr_lua_State *L) {
    ___pdr_p_udp udp = (___pdr_p_udp) ___pdr_auxiliar_checkgroup(L, "udp{any}", 1);
    return ___opt_meth_setoption(L, udp_optset, &udp->sock);
}

/*-------------------------------------------------------------------------*\
* Just call option handler
\*-------------------------------------------------------------------------*/
static int udp_meth_getoption(___pdr_lua_State *L) {
    ___pdr_p_udp udp = (___pdr_p_udp) ___pdr_auxiliar_checkgroup(L, "udp{any}", 1);
    return ___opt_meth_getoption(L, udp_optget, &udp->sock);
}

/*-------------------------------------------------------------------------*\
* Just call tm methods
\*-------------------------------------------------------------------------*/
static int udp_meth_settimeout(___pdr_lua_State *L) {
    ___pdr_p_udp udp = (___pdr_p_udp) ___pdr_auxiliar_checkgroup(L, "udp{any}", 1);
    return ___pdr_timeout_meth_settimeout(L, &udp->tm);
}

/*-------------------------------------------------------------------------*\
* Turns a master udp object into a client object.
\*-------------------------------------------------------------------------*/
static int udp_meth_setpeername(___pdr_lua_State *L) {
    ___pdr_p_udp udp = (___pdr_p_udp) ___pdr_auxiliar_checkgroup(L, "udp{any}", 1);
    ___pdr_p_timeout tm = &udp->tm;
    const char *address = ___pdr_luaL_checkstring(L, 2);
    int connecting = strcmp(address, "*");
    const char *port = connecting? ___pdr_luaL_checkstring(L, 3): "0";
    struct addrinfo connecthints;
    const char *err;
    memset(&connecthints, 0, sizeof(connecthints));
    connecthints.ai_socktype = SOCK_DGRAM;
    /* make sure we try to connect only to the same family */
    connecthints.ai_family = udp->family;
    if (connecting) {
        err = ___pdr_inet_tryconnect(&udp->sock, &udp->family, address, 
            port, tm, &connecthints);
        if (err) {
            ___pdr_lua_pushnil(L);
            ___pdr_lua_pushstring(L, err);
            return 2;
        }
        ___pdr_auxiliar_setclass(L, "udp{connected}", 1);
    } else {
        /* we ignore possible errors because Mac OS X always
         * returns EAFNOSUPPORT */
        ___pdr_inet_trydisconnect(&udp->sock, udp->family, tm);
        ___pdr_auxiliar_setclass(L, "udp{unconnected}", 1);
    }
    /* change class to connected or unconnected depending on address */
    ___pdr_lua_pushnumber(L, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Closes socket used by object
\*-------------------------------------------------------------------------*/
static int udp_meth_close(___pdr_lua_State *L) {
    ___pdr_p_udp udp = (___pdr_p_udp) ___pdr_auxiliar_checkgroup(L, "udp{any}", 1);
    ___pdr_socket_destroy(&udp->sock);
    ___pdr_lua_pushnumber(L, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Turns a master object into a server object
\*-------------------------------------------------------------------------*/
static int udp_meth_setsockname(___pdr_lua_State *L) {
    ___pdr_p_udp udp = (___pdr_p_udp) ___pdr_auxiliar_checkclass(L, "udp{unconnected}", 1);
    const char *address =  ___pdr_luaL_checkstring(L, 2);
    const char *port = ___pdr_luaL_checkstring(L, 3);
    const char *err;
    struct addrinfo bindhints;
    memset(&bindhints, 0, sizeof(bindhints));
    bindhints.ai_socktype = SOCK_DGRAM;
    bindhints.ai_family = udp->family;
    bindhints.ai_flags = AI_PASSIVE;
    err = ___pdr_inet_trybind(&udp->sock, address, port, &bindhints);
    if (err) {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, err);
        return 2;
    }
    ___pdr_lua_pushnumber(L, 1);
    return 1;
}

/*=========================================================================*\
* Library functions
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Creates a master udp object
\*-------------------------------------------------------------------------*/
static int udp_create(___pdr_lua_State *L, int family) {
    ___pdr_t_socket sock;
    const char *err = ___pdr_inet_trycreate(&sock, family, SOCK_DGRAM);
    /* try to allocate a system socket */
    if (!err) {
        /* allocate udp object */
        ___pdr_p_udp udp = (___pdr_p_udp) ___pdr_lua_newuserdata(L, sizeof(___pdr_t_udp));
        ___pdr_auxiliar_setclass(L, "udp{unconnected}", -1);
        /* initialize remaining structure fields */
        ___pdr_socket_setnonblocking(&sock);
        if (family == PF_INET6) {
            int yes = 1;
            setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY,
                reinterpret_cast<const char*>(&yes), sizeof(yes));
        }
        udp->sock = sock;
        ___pdr_timeout_init(&udp->tm, -1, -1);
        udp->family = family;
        return 1;
    } else {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, err);
        return 2;
    }
}

static int udp_global_create(___pdr_lua_State *L) {
    return udp_create(L, AF_INET);
}

static int udp_global_create6(___pdr_lua_State *L) {
    return udp_create(L, AF_INET6);
}

} // end NS_PDR_SLUA
