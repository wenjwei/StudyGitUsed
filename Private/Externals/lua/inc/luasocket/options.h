#ifndef ___PDR_OPTIONS_H___
#define ___PDR_OPTIONS_H___
/*=========================================================================*\
* Common option interface 
* LuaSocket toolkit
*
* This module provides a common interface to socket options, used mainly by
* modules UDP and TCP. 
\*=========================================================================*/

#include "lua.h"
#include "socket.h"

namespace NS_PDR_SLUA {    

/* option registry */
typedef struct ___pdr_t_opt {
  const char *name;
  int (*func)(___pdr_lua_State *L, ___pdr_p_socket ps);
} ___pdr_t_opt;
typedef ___pdr_t_opt *___pdr_p_opt;

/* supported options for setoption */
int ___pdr_opt_set_dontroute(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_set_broadcast(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_set_reuseaddr(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_set_tcp_nodelay(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_set_keepalive(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_set_linger(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_set_reuseaddr(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_set_reuseport(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_set_ip_multicast_if(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_set_ip_multicast_ttl(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_set_ip_multicast_loop(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_set_ip_add_membership(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_set_ip_drop_membersip(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_set_ip6_unicast_hops(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_set_ip6_multicast_hops(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_set_ip6_multicast_loop(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_set_ip6_add_membership(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_set_ip6_drop_membersip(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_set_ip6_v6only(___pdr_lua_State *L, ___pdr_p_socket ps);

/* supported options for getoption */
int ___pdr_opt_get_reuseaddr(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_get_tcp_nodelay(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_get_keepalive(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_get_linger(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_get_reuseaddr(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___pdr_opt_get_ip_multicast_loop(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___opt_get_ip_multicast_if(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___opt_get_error(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___opt_get_ip6_multicast_loop(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___opt_get_ip6_multicast_hops(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___opt_get_ip6_unicast_hops(___pdr_lua_State *L, ___pdr_p_socket ps);
int ___opt_get_ip6_v6only(___pdr_lua_State *L, ___pdr_p_socket ps); 

/* invokes the appropriate option handler */
int ___opt_meth_setoption(___pdr_lua_State *L, ___pdr_p_opt opt, ___pdr_p_socket ps);
int ___opt_meth_getoption(___pdr_lua_State *L, ___pdr_p_opt opt, ___pdr_p_socket ps);

} // end NS_PDR_SLUA

#endif
