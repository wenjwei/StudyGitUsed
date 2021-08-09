#ifndef ___PDR_UNIX_H___
#define ___PDR_UNIX_H___
/*=========================================================================*\
* Unix domain object
* LuaSocket toolkit
*
* This module is just an example of how to extend LuaSocket with a new 
* domain.
\*=========================================================================*/
#include "lua.h"

#include "buffer.h"
#include "timeout.h"
#include "socket.h"

#ifndef ___PDR_UNIX_API
#define ___PDR_UNIX_API
#endif

namespace NS_PDR_SLUA {    

typedef struct ___pdr_t_unix_ {
    ___pdr_t_socket sock;
    ___pdr_t_io io;
    ___pdr_t_buffer buf;
    ___pdr_t_timeout tm;
} ___pdr_t_unix;
typedef ___pdr_t_unix *___pdr_p_unix;

___PDR_UNIX_API int ___pdr_luaopen_socket_unix(___pdr_lua_State *L);

} // end NS_PDR_SLUA

#endif /* UNIX_H */
