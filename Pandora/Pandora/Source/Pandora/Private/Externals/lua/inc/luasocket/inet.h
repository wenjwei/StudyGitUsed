#ifndef ___PDR_INET_H___
#define ___PDR_INET_H___
/*=========================================================================*\
* Internet domain functions
* LuaSocket toolkit
*
* This module implements the creation and connection of internet domain
* sockets, on top of the socket.h interface, and the interface of with the
* resolver. 
*
* The function inet_aton is provided for the platforms where it is not
* available. The module also implements the interface of the internet
* getpeername and getsockname functions as seen by Lua programs.
*
* The Lua functions toip and tohostname are also implemented here.
\*=========================================================================*/
#include "lua.h"
#include "socket.h"
#include "timeout.h"

#ifdef _WIN32
#define ___PDR_LUASOCKET_INET_ATON
#endif

namespace NS_PDR_SLUA {    

int ___pdr_inet_open(___pdr_lua_State *L);

const char *___pdr_inet_trycreate(___pdr_p_socket ps, int family, int type);
const char *___pdr_inet_tryconnect(___pdr_p_socket ps, int *family, const char *address,
        const char *serv, ___pdr_p_timeout tm, struct addrinfo *connecthints);
const char *___pdr_inet_tryconnect_with_ip(___pdr_p_socket ps, int *family, const char *ip,
        int port, int type, ___pdr_p_timeout tm);
const char *___pdr_inet_trybind(___pdr_p_socket ps, const char *address, const char *serv,
        struct addrinfo *bindhints);
const char *___pdr_inet_trydisconnect(___pdr_p_socket ps, int family, ___pdr_p_timeout tm);
const char *___pdr_inet_tryaccept(___pdr_p_socket server, int family, ___pdr_p_socket client, ___pdr_p_timeout tm);

int ___pdr_inet_meth_getpeername(___pdr_lua_State *L, ___pdr_p_socket ps, int family);
int ___pdr_inet_meth_getsockname(___pdr_lua_State *L, ___pdr_p_socket ps, int family);

int ___pdr_inet_optfamily(___pdr_lua_State* L, int narg, const char* def);
int ___pdr_inet_optsocktype(___pdr_lua_State* L, int narg, const char* def);

#ifdef ___PDR_LUASOCKET_INET_ATON
int inet_aton(const char *cp, struct in_addr *inp);
#endif

#ifdef ___PDR_LUASOCKET_INET_PTON
const char *inet_ntop(int af, const void *src, char *dst, ___pdr_socklen_t cnt);
int inet_pton(int af, const char *src, void *dst);
#endif

} // end NS_PDR_SLUA

#endif /* INET_H */
