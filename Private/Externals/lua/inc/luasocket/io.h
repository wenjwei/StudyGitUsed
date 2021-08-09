#ifndef ___PDR_IO_H___
#define ___PDR_IO_H___
/*=========================================================================*\
* Input/Output abstraction
* LuaSocket toolkit
*
* This module defines the interface that LuaSocket expects from the
* transport layer for streamed input/output. The idea is that if any
* transport implements this interface, then the buffer.c functions
* automatically work on it.
*
* The module socket.h implements this interface, and thus the module tcp.h
* is very simple.
\*=========================================================================*/
#include <stdio.h>
#include "lua.h"

#include "timeout.h"

namespace NS_PDR_SLUA {    

/* IO error codes */
enum {
    PDR_IO_DONE = 0,        /* operation completed successfully */
    PDR_IO_TIMEOUT = -1,    /* operation timed out */
    PDR_IO_CLOSED = -2,     /* the connection has been closed */
	PDR_IO_UNKNOWN = -3     
};

/* interface to error message function */
typedef const char *(*___pdr_p_error) (
    void *ctx,          /* context needed by send */
    int err             /* error code */
);

/* interface to send function */
typedef int (*___pdr_p_send) (
    void *ctx,          /* context needed by send */
    const char *data,   /* pointer to buffer with data to send */
    size_t count,       /* number of bytes to send from buffer */
    size_t *sent,       /* number of bytes sent uppon return */
    ___pdr_p_timeout tm        /* timeout control */
);

/* interface to recv function */
typedef int (*___pdr_p_recv) (
    void *ctx,          /* context needed by recv */
    char *data,         /* pointer to buffer where data will be writen */
    size_t count,       /* number of bytes to receive into buffer */
    size_t *got,        /* number of bytes received uppon return */
    ___pdr_p_timeout tm        /* timeout control */
);

/* IO driver definition */
typedef struct ___pdr_t_io_ {
    void *ctx;          /* context needed by send/recv */
    ___pdr_p_send send;        /* send function pointer */
    ___pdr_p_recv recv;        /* receive function pointer */
    ___pdr_p_error error;      /* strerror function */
} ___pdr_t_io;
typedef ___pdr_t_io *___pdr_p_io;

void ___pdr_io_init(___pdr_p_io io, ___pdr_p_send send, ___pdr_p_recv recv, ___pdr_p_error error, void *ctx);
const char *___pdr_io_strerror(int err);

} // end NS_PDR_SLUA

#endif /* IO_H */

