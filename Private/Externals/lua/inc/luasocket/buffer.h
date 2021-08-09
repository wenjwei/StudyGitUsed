#ifndef ___PDR_BUF_H___
#define ___PDR_BUF_H___
/*=========================================================================*\
* Input/Output interface for Lua programs
* LuaSocket toolkit
*
* Line patterns require buffering. Reading one character at a time involves
* too many system calls and is very slow. This module implements the
* LuaSocket interface for input/output on connected objects, as seen by 
* Lua programs. 
*
* Input is buffered. Output is *not* buffered because there was no simple
* way of making sure the buffered output data would ever be sent.
*
* The module is built on top of the I/O abstraction defined in io.h and the
* timeout management is done with the timeout.h interface.
\*=========================================================================*/
#include "lua.h"

#include "io.h"
#include "timeout.h"

/* buffer size in bytes */
#define ___PDR_BUF_SIZE 8192

namespace NS_PDR_SLUA {

/* buffer control structure */
typedef struct ___pdr_t_buffer_ {
    double birthday;        /* throttle support info: creation time, */
    size_t sent, received;  /* bytes sent, and bytes received */
    ___pdr_p_io io;                /* IO driver used for this buffer */
    ___pdr_p_timeout tm;           /* timeout management for this buffer */
    size_t first, last;     /* index of first and last bytes of stored data */
    char data[___PDR_BUF_SIZE];    /* storage space for buffer data */
} ___pdr_t_buffer;
typedef ___pdr_t_buffer *___pdr_p_buffer;

int ___pdr_buffer_open(___pdr_lua_State *L);
void ___pdr_buffer_init(___pdr_p_buffer buf, ___pdr_p_io io, ___pdr_p_timeout tm);
int ___pdr_buffer_meth_send(___pdr_lua_State *L, ___pdr_p_buffer buf);
int ___pdr_buffer_meth_receive(___pdr_lua_State *L, ___pdr_p_buffer buf);
int ___pdr_buffer_meth_getstats(___pdr_lua_State *L, ___pdr_p_buffer buf);
int ___pdr_buffer_meth_setstats(___pdr_lua_State *L, ___pdr_p_buffer buf);
int ___pdr_buffer_isempty(___pdr_p_buffer buf);

} // end NS_PDR_SLUA

#endif /* BUF_H */
