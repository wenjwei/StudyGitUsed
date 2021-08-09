#ifndef ___PDR_UDP_H___
#define ___PDR_UDP_H___
/*=========================================================================*\
* UDP object
* LuaSocket toolkit
*
* The udp.h module provides LuaSocket with support for UDP protocol
* (AF_INET, SOCK_DGRAM).
*
* Two classes are defined: connected and unconnected. UDP objects are
* originally unconnected. They can be "connected" to a given address 
* with a call to the setpeername function. The same function can be used to
* break the connection.
\*=========================================================================*/
#include "lua.h"

#include "timeout.h"
#include "socket.h"

/* can't be larger than wsocket.c MAXCHUNK!!! */
#define ___PDR_UDP_DATAGRAMSIZE 8192

namespace NS_PDR_SLUA {    

typedef struct ___pdr_t_udp_ {
    ___pdr_t_socket sock;
    ___pdr_t_timeout tm;
    int family;
} ___pdr_t_udp;
typedef ___pdr_t_udp *___pdr_p_udp;

int ___pdr_udp_open(___pdr_lua_State *L);

} // end NS_PDR_SLUA

#endif /* UDP_H */
