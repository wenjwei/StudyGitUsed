/*=========================================================================*\
* Common option interface 
* LuaSocket toolkit
\*=========================================================================*/
#include "options.h"
#include <string.h> 

#include "lauxlib.h"

#include "auxiliar.h"
#include "inet.h"

namespace NS_PDR_SLUA {    

/*=========================================================================*\
* Internal functions prototypes
\*=========================================================================*/
static int opt_setmembership(___pdr_lua_State *L, ___pdr_p_socket ps, int level, int name);
static int opt_ip6_setmembership(___pdr_lua_State *L, ___pdr_p_socket ps, int level, int name);
static int opt_setboolean(___pdr_lua_State *L, ___pdr_p_socket ps, int level, int name);
static int opt_getboolean(___pdr_lua_State *L, ___pdr_p_socket ps, int level, int name);
static int opt_setint(___pdr_lua_State *L, ___pdr_p_socket ps, int level, int name);
static int opt_getint(___pdr_lua_State *L, ___pdr_p_socket ps, int level, int name);
static int opt_set(___pdr_lua_State *L, ___pdr_p_socket ps, int level, int name, 
        void *val, int len);
static int opt_get(___pdr_lua_State *L, ___pdr_p_socket ps, int level, int name, 
        void *val, int* len);

/*=========================================================================*\
* Exported functions
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Calls appropriate option handler
\*-------------------------------------------------------------------------*/
int ___opt_meth_setoption(___pdr_lua_State *L, ___pdr_p_opt opt, ___pdr_p_socket ps)
{
    const char *name = ___pdr_luaL_checkstring(L, 2);      /* obj, name, ... */
    while (opt->name && strcmp(name, opt->name))
        opt++;
    if (!opt->func) {
        char msg[45];
        sprintf(msg, "unsupported option `%.35s'", name);
        ___pdr_luaL_argerror(L, 2, msg);
    }
    return opt->func(L, ps);
}

int ___opt_meth_getoption(___pdr_lua_State *L, ___pdr_p_opt opt, ___pdr_p_socket ps)
{
    const char *name = ___pdr_luaL_checkstring(L, 2);      /* obj, name, ... */
    while (opt->name && strcmp(name, opt->name))
        opt++;
    if (!opt->func) {
        char msg[45];
        sprintf(msg, "unsupported option `%.35s'", name);
        ___pdr_luaL_argerror(L, 2, msg);
    }
    return opt->func(L, ps);
}

/* enables reuse of local address */
int ___pdr_opt_set_reuseaddr(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_setboolean(L, ps, SOL_SOCKET, SO_REUSEADDR); 
}

int ___pdr_opt_get_reuseaddr(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_getboolean(L, ps, SOL_SOCKET, SO_REUSEADDR); 
}

/* enables reuse of local port */
int ___pdr_opt_set_reuseport(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_setboolean(L, ps, SOL_SOCKET, SO_REUSEPORT); 
}

int opt_get_reuseport(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_getboolean(L, ps, SOL_SOCKET, SO_REUSEPORT); 
}

/* disables the Naggle algorithm */
int ___pdr_opt_set_tcp_nodelay(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_setboolean(L, ps, IPPROTO_TCP, TCP_NODELAY); 
}

int ___pdr_opt_get_tcp_nodelay(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_getboolean(L, ps, IPPROTO_TCP, TCP_NODELAY);
}

int ___pdr_opt_set_keepalive(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_setboolean(L, ps, SOL_SOCKET, SO_KEEPALIVE); 
}

int ___pdr_opt_get_keepalive(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_getboolean(L, ps, SOL_SOCKET, SO_KEEPALIVE); 
}

int ___pdr_opt_set_dontroute(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_setboolean(L, ps, SOL_SOCKET, SO_DONTROUTE);
}

int ___pdr_opt_set_broadcast(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_setboolean(L, ps, SOL_SOCKET, SO_BROADCAST);
}

int ___pdr_opt_set_ip6_unicast_hops(___pdr_lua_State *L, ___pdr_p_socket ps)
{
  return opt_setint(L, ps, IPPROTO_IPV6, IPV6_UNICAST_HOPS);
}

int ___opt_get_ip6_unicast_hops(___pdr_lua_State *L, ___pdr_p_socket ps)
{
  return opt_getint(L, ps, IPPROTO_IPV6, IPV6_UNICAST_HOPS);
}

int ___pdr_opt_set_ip6_multicast_hops(___pdr_lua_State *L, ___pdr_p_socket ps)
{
  return opt_setint(L, ps, IPPROTO_IPV6, IPV6_MULTICAST_HOPS);
}

int ___opt_get_ip6_multicast_hops(___pdr_lua_State *L, ___pdr_p_socket ps)
{
  return opt_getint(L, ps, IPPROTO_IPV6, IPV6_MULTICAST_HOPS);
}

int ___pdr_opt_set_ip_multicast_loop(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_setboolean(L, ps, IPPROTO_IP, IP_MULTICAST_LOOP);
}

int ___pdr_opt_get_ip_multicast_loop(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_getboolean(L, ps, IPPROTO_IP, IP_MULTICAST_LOOP);
}

int ___pdr_opt_set_ip6_multicast_loop(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_setboolean(L, ps, IPPROTO_IPV6, IPV6_MULTICAST_LOOP);
}

int ___opt_get_ip6_multicast_loop(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_getboolean(L, ps, IPPROTO_IPV6, IPV6_MULTICAST_LOOP);
}

int ___pdr_opt_set_linger(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    struct linger li;                      /* obj, name, table */
    if (!___pdr_lua_istable(L, 3)) ___pdr_auxiliar_typeerror(L,3,___pdr_lua_typename(L, ___PDR_LUA_TTABLE));
    ___pdr_lua_pushstring(L, "on");
    ___pdr_lua_gettable(L, 3);
    if (!___pdr_lua_isboolean(L, -1)) 
        ___pdr_luaL_argerror(L, 3, "boolean 'on' field expected");
    li.l_onoff = (u_short) ___pdr_lua_toboolean(L, -1);
    ___pdr_lua_pushstring(L, "timeout");
    ___pdr_lua_gettable(L, 3);
    if (!___pdr_lua_isnumber(L, -1)) 
        ___pdr_luaL_argerror(L, 3, "number 'timeout' field expected");
    li.l_linger = (u_short) ___pdr_lua_tonumber(L, -1);
    return opt_set(L, ps, SOL_SOCKET, SO_LINGER, (char *) &li, sizeof(li));
}

int ___pdr_opt_get_linger(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    struct linger li;                      /* obj, name */
    int len = sizeof(li);
    int err = opt_get(L, ps, SOL_SOCKET, SO_LINGER, (char *) &li, &len);
    if (err)
        return err;
    ___pdr_lua_newtable(L);
    ___pdr_lua_pushboolean(L, li.l_onoff);
    ___pdr_lua_setfield(L, -2, "on");
    ___pdr_lua_pushinteger(L, li.l_linger);
    ___pdr_lua_setfield(L, -2, "timeout");
    return 1;
}

int ___pdr_opt_set_ip_multicast_ttl(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_setint(L, ps, IPPROTO_IP, IP_MULTICAST_TTL);
}

int ___pdr_opt_set_ip_multicast_if(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    const char *address = ___pdr_luaL_checkstring(L, 3);    /* obj, name, ip */
    struct in_addr val;
    val.s_addr = htonl(INADDR_ANY);
    if (strcmp(address, "*") && !inet_aton(address, &val))
        ___pdr_luaL_argerror(L, 3, "ip expected");
    return opt_set(L, ps, IPPROTO_IP, IP_MULTICAST_IF, 
        (char *) &val, sizeof(val));
}

int ___opt_get_ip_multicast_if(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    struct in_addr val;
    ___pdr_socklen_t len = sizeof(val);
    if (getsockopt(*ps, IPPROTO_IP, IP_MULTICAST_IF, (char *) &val, &len) < 0) {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, "getsockopt failed");
        return 2;
    }
    ___pdr_lua_pushstring(L, inet_ntoa(val));
    return 1;
}

int ___pdr_opt_set_ip_add_membership(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_setmembership(L, ps, IPPROTO_IP, IP_ADD_MEMBERSHIP);
}

int ___pdr_opt_set_ip_drop_membersip(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_setmembership(L, ps, IPPROTO_IP, IP_DROP_MEMBERSHIP);
}

int ___pdr_opt_set_ip6_add_membership(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_ip6_setmembership(L, ps, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP);
}

int ___pdr_opt_set_ip6_drop_membersip(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_ip6_setmembership(L, ps, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP);
}

int ___opt_get_ip6_v6only(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_getboolean(L, ps, IPPROTO_IPV6, IPV6_V6ONLY);
}

int ___pdr_opt_set_ip6_v6only(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    return opt_setboolean(L, ps, IPPROTO_IPV6, IPV6_V6ONLY);
}

/*=========================================================================*\
* Auxiliar functions
\*=========================================================================*/
static int opt_setmembership(___pdr_lua_State *L, ___pdr_p_socket ps, int level, int name)
{
    struct ip_mreq val;                   /* obj, name, table */
    if (!___pdr_lua_istable(L, 3)) ___pdr_auxiliar_typeerror(L,3,___pdr_lua_typename(L, ___PDR_LUA_TTABLE));
    ___pdr_lua_pushstring(L, "multiaddr");
    ___pdr_lua_gettable(L, 3);
    if (!___pdr_lua_isstring(L, -1)) 
        ___pdr_luaL_argerror(L, 3, "string 'multiaddr' field expected");
    if (!inet_aton(___pdr_lua_tostring(L, -1), &val.imr_multiaddr)) 
        ___pdr_luaL_argerror(L, 3, "invalid 'multiaddr' ip address");
    ___pdr_lua_pushstring(L, "interface");
    ___pdr_lua_gettable(L, 3);
    if (!___pdr_lua_isstring(L, -1)) 
        ___pdr_luaL_argerror(L, 3, "string 'interface' field expected");
    val.imr_interface.s_addr = htonl(INADDR_ANY);
    if (strcmp(___pdr_lua_tostring(L, -1), "*") &&
            !inet_aton(___pdr_lua_tostring(L, -1), &val.imr_interface)) 
        ___pdr_luaL_argerror(L, 3, "invalid 'interface' ip address");
    return opt_set(L, ps, level, name, (char *) &val, sizeof(val));
}

static int opt_ip6_setmembership(___pdr_lua_State *L, ___pdr_p_socket ps, int level, int name)
{
    struct ipv6_mreq val;                   /* obj, opt-name, table */
    memset(&val, 0, sizeof(val));
    if (!___pdr_lua_istable(L, 3)) ___pdr_auxiliar_typeerror(L,3,___pdr_lua_typename(L, ___PDR_LUA_TTABLE));
    ___pdr_lua_pushstring(L, "multiaddr");
    ___pdr_lua_gettable(L, 3);
    if (!___pdr_lua_isstring(L, -1)) 
        ___pdr_luaL_argerror(L, 3, "string 'multiaddr' field expected");
    if (!inet_pton(AF_INET6, ___pdr_lua_tostring(L, -1), &val.ipv6mr_multiaddr)) 
        ___pdr_luaL_argerror(L, 3, "invalid 'multiaddr' ip address");
    ___pdr_lua_pushstring(L, "interface");
    ___pdr_lua_gettable(L, 3);
    /* By default we listen to interface on default route
     * (sigh). However, interface= can override it. We should 
     * support either number, or name for it. Waiting for
     * windows port of if_nametoindex */
    if (!___pdr_lua_isnil(L, -1)) {
        if (___pdr_lua_isnumber(L, -1)) {
            val.ipv6mr_interface = (unsigned int) ___pdr_lua_tonumber(L, -1);
        } else
          ___pdr_luaL_argerror(L, -1, "number 'interface' field expected");
    }
    return opt_set(L, ps, level, name, (char *) &val, sizeof(val));
}

static 
int opt_get(___pdr_lua_State *L, ___pdr_p_socket ps, int level, int name, void *val, int* len)
{
    ___pdr_socklen_t socklen = *len;
    if (getsockopt(*ps, level, name, (char *) val, &socklen) < 0) {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, "getsockopt failed");
        return 2;
    }
    *len = socklen;
    return 0;
}

static 
int opt_set(___pdr_lua_State *L, ___pdr_p_socket ps, int level, int name, void *val, int len)
{
    if (setsockopt(*ps, level, name, (char *) val, len) < 0) {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, "setsockopt failed");
        return 2;
    }
    ___pdr_lua_pushnumber(L, 1);
    return 1;
}

static int opt_getboolean(___pdr_lua_State *L, ___pdr_p_socket ps, int level, int name)
{
    int val = 0;
    int len = sizeof(val);
    int err = opt_get(L, ps, level, name, (char *) &val, &len);
    if (err)
        return err;
    ___pdr_lua_pushboolean(L, val);
    return 1;
}

int ___opt_get_error(___pdr_lua_State *L, ___pdr_p_socket ps)
{
    int val = 0;
    ___pdr_socklen_t len = sizeof(val);
    if (getsockopt(*ps, SOL_SOCKET, SO_ERROR, (char *) &val, &len) < 0) {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, "getsockopt failed");
        return 2;
    }
    ___pdr_lua_pushstring(L, ___pdr_socket_strerror(val));
    return 1;
}

static int opt_setboolean(___pdr_lua_State *L, ___pdr_p_socket ps, int level, int name)
{
    int val = ___pdr_auxiliar_checkboolean(L, 3);             /* obj, name, bool */
    return opt_set(L, ps, level, name, (char *) &val, sizeof(val));
}

static int opt_getint(___pdr_lua_State *L, ___pdr_p_socket ps, int level, int name)
{
    int val = 0;
    int len = sizeof(val);
    int err = opt_get(L, ps, level, name, (char *) &val, &len);
    if (err)
        return err;
    ___pdr_lua_pushnumber(L, val);
    return 1;
}

static int opt_setint(___pdr_lua_State *L, ___pdr_p_socket ps, int level, int name)
{
    int val = (int) ___pdr_lua_tonumber(L, 3);             /* obj, name, int */
    return opt_set(L, ps, level, name, (char *) &val, sizeof(val));
}

} // end NS_PDR_SLUA
