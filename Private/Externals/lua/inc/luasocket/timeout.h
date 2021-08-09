#ifndef ___PDR_TIMEOUT_H___
#define ___PDR_TIMEOUT_H___
/*=========================================================================*\
* Timeout management functions
* LuaSocket toolkit
\*=========================================================================*/
#include "lua.h"

namespace NS_PDR_SLUA {    

/* timeout control structure */
typedef struct ___pdr_t_timeout_ {
    double block;          /* maximum time for blocking calls */
    double total;          /* total number of miliseconds for operation */
    double start;          /* time of start of operation */
} ___pdr_t_timeout;
typedef ___pdr_t_timeout *___pdr_p_timeout;

int ___pdr_timeout_open(___pdr_lua_State *L);
void ___pdr_timeout_init(___pdr_p_timeout tm, double block, double total);
double ___pdr_timeout_get(___pdr_p_timeout tm);
double ___pdr_timeout_getretry(___pdr_p_timeout tm);
___pdr_p_timeout ___pdr_timeout_markstart(___pdr_p_timeout tm);
double ___pdr_timeout_getstart(___pdr_p_timeout tm);
double ___pdr_timeout_gettime(void);
int ___pdr_timeout_meth_settimeout(___pdr_lua_State *L, ___pdr_p_timeout tm);

#define timeout_iszero(tm)   ((tm)->block == 0.0)

} // end NS_PDR_SLUA

#endif /* TIMEOUT_H */
