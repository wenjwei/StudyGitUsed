#ifndef ___PDR_TCP_H___
#define ___PDR_TCP_H___
/*=========================================================================*\
* TCP object
* LuaSocket toolkit
*
* The tcp.h module is basicly a glue that puts together modules buffer.h,
* timeout.h socket.h and inet.h to provide the LuaSocket TCP (AF_INET,
* SOCK_STREAM) support.
*
* Three classes are defined: master, client and server. The master class is
* a newly created tcp object, that has not been bound or connected. Server
* objects are tcp objects bound to some local address. Client objects are
* tcp objects either connected to some address or returned by the accept
* method of a server object.
\*=========================================================================*/
#include "lua.h"

#include "buffer.h"
#include "timeout.h"
#include "socket.h"

namespace NS_PDR_SLUA {    

typedef struct ___pdr_t_tcp_ {
    ___pdr_t_socket sock;
    ___pdr_t_io io;
    ___pdr_t_buffer buf;
    ___pdr_t_timeout tm;
    int family;
} ___pdr_t_tcp;

typedef ___pdr_t_tcp *___pdr_p_tcp;

int tcp_open(___pdr_lua_State *L);

} // end NS_PDR_SLUA

#endif /* TCP_H */
