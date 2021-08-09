/*
** $Id: lapi.h,v 2.9 2015/03/06 19:49:50 roberto Exp $
** Auxiliary functions from Lua API
** See Copyright Notice in lua.h
*/

#ifndef ___pdr_lapi_h___
#define ___pdr_lapi_h___

#include "llimits.h"
#include "lstate.h"

#define ___pdr_api_incr_top(L)   {L->top++; ___pdr_api_check(L, L->top <= L->ci->top, \
				"stack overflow");}

#define ___pdr_adjustresults(L,nres) \
    { if ((nres) == ___PDR_LUA_MULTRET && L->ci->top < L->top) L->ci->top = L->top; }

#define ___pdr_api_checknelems(L,n)	___pdr_api_check(L, (n) < (L->top - L->ci->func), \
				  "not enough elements in the stack")


#endif
