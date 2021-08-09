/*=========================================================================*\
* Input/Output abstraction
* LuaSocket toolkit
\*=========================================================================*/
#include "io.h"

namespace NS_PDR_SLUA {    

/*=========================================================================*\
* Exported functions
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Initializes C structure
\*-------------------------------------------------------------------------*/
void ___pdr_io_init(___pdr_p_io io, ___pdr_p_send send, ___pdr_p_recv recv, ___pdr_p_error error, void *ctx) {
    io->send = send;
    io->recv = recv;
    io->error = error;
    io->ctx = ctx;
}

/*-------------------------------------------------------------------------*\
* I/O error strings
\*-------------------------------------------------------------------------*/
const char *___pdr_io_strerror(int err) {
    switch (err) {
        case PDR_IO_DONE: return NULL;
        case PDR_IO_CLOSED: return "closed";
        case PDR_IO_TIMEOUT: return "timeout";
        default: return "unknown error"; 
    }
}

} // end NS_PDR_SLUA
