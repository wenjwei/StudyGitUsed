/*
** $Id: ldebug.h,v 2.14 2015/05/22 17:45:56 roberto Exp $
** Auxiliary functions from Debug Interface module
** See Copyright Notice in lua.h
*/

#ifndef ___pdr_ldebug_h___
#define ___pdr_ldebug_h___

#include "lstate.h"

namespace NS_PDR_SLUA {

#define ___pdr_pcRel(pc, p)	(___pdr_cast(int, (pc) - (p)->code) - 1)

#define ___pdr_getfuncline(f,pc)	(((f)->lineinfo) ? (f)->lineinfo[pc] : -1)

#define ___pdr_resethookcount(L)	(L->hookcount = L->basehookcount)


___PDR_LUAI_FUNC ___pdr_l_noret ___pdr_luaG_typeerror (___pdr_lua_State *L, const ___pdr_TValue *o,
                                                const char *opname);
___PDR_LUAI_FUNC ___pdr_l_noret ___pdr_luaG_concaterror (___pdr_lua_State *L, const ___pdr_TValue *p1,
                                                  const ___pdr_TValue *p2);
___PDR_LUAI_FUNC ___pdr_l_noret ___pdr_luaG_opinterror (___pdr_lua_State *L, const ___pdr_TValue *p1,
                                                 const ___pdr_TValue *p2,
                                                 const char *msg);
___PDR_LUAI_FUNC ___pdr_l_noret ___pdr_luaG_tointerror (___pdr_lua_State *L, const ___pdr_TValue *p1,
                                                 const ___pdr_TValue *p2);
___PDR_LUAI_FUNC ___pdr_l_noret ___pdr_luaG_ordererror (___pdr_lua_State *L, const ___pdr_TValue *p1,
                                                 const ___pdr_TValue *p2);
___PDR_LUAI_FUNC ___pdr_l_noret ___pdr_luaG_runerror (___pdr_lua_State *L, const char *fmt, ...);
___PDR_LUAI_FUNC const char *___pdr_luaG_addinfo (___pdr_lua_State *L, const char *msg,
                                                  ___pdr_TString *src, int line);
___PDR_LUAI_FUNC ___pdr_l_noret ___pdr_luaG_errormsg (___pdr_lua_State *L);
___PDR_LUAI_FUNC void ___pdr_luaG_traceexec (___pdr_lua_State *L);

} // end NS_PDR_SLUA

#endif
