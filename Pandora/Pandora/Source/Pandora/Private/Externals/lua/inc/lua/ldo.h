/*
** $Id: ldo.h,v 2.29 2015/12/21 13:02:14 roberto Exp $
** Stack and Call structure of Lua
** See Copyright Notice in lua.h
*/

#ifndef ___pdr_ldo_h___
#define ___pdr_ldo_h___


#include "lobject.h"
#include "lstate.h"
#include "lzio.h"

/*
** Macro to check stack size and grow stack if needed.  Parameters
** 'pre'/'pos' allow the macro to preserve a pointer into the
** stack across reallocations, doing the work only when needed.
** 'condmovestack' is used in heavy tests to force a stack reallocation
** at every check.
*/
#define ___pdr_luaD_checkstackaux(L,n,pre,pos)  \
	if (L->stack_last - L->top <= (n)) \
	  { pre; ___pdr_luaD_growstack(L, n); pos; } else { ___pdr_condmovestack(L,pre,pos); }

/* In general, 'pre'/'pos' are empty (nothing to save) */
#define ___pdr_luaD_checkstack(L,n)	___pdr_luaD_checkstackaux(L,n,(void)0,(void)0)



#define ___pdr_savestack(L,p)		((char *)(p) - (char *)L->stack)
#define ___pdr_restorestack(L,n)	((___pdr_TValue *)((char *)L->stack + (n)))

namespace NS_PDR_SLUA {

/* type of protected functions, to be ran by 'runprotected' */
typedef void (*___pdr_Pfunc) (___pdr_lua_State *L, void *ud);

___PDR_LUAI_FUNC int ___pdr_luaD_protectedparser (___pdr_lua_State *L, ___pdr_ZIO *z, const char *name,
                                                  const char *mode);
___PDR_LUAI_FUNC void ___pdr_luaD_hook (___pdr_lua_State *L, int event, int line);
___PDR_LUAI_FUNC int ___pdr_luaD_precall (___pdr_lua_State *L, ___pdr_StkId func, int nresults);
___PDR_LUAI_FUNC void ___pdr_luaD_call (___pdr_lua_State *L, ___pdr_StkId func, int nResults);
___PDR_LUAI_FUNC void ___pdr_luaD_callnoyield (___pdr_lua_State *L, ___pdr_StkId func, int nResults);
___PDR_LUAI_FUNC int ___pdr_luaD_pcall (___pdr_lua_State *L, ___pdr_Pfunc func, void *u,
                                        ptrdiff_t oldtop, ptrdiff_t ef);
___PDR_LUAI_FUNC int ___pdr_luaD_poscall (___pdr_lua_State *L, ___pdr_CallInfo *ci, ___pdr_StkId firstResult,
                                          int nres);
___PDR_LUAI_FUNC void ___pdr_luaD_reallocstack (___pdr_lua_State *L, int newsize);
___PDR_LUAI_FUNC void ___pdr_luaD_growstack (___pdr_lua_State *L, int n);
___PDR_LUAI_FUNC void ___pdr_luaD_shrinkstack (___pdr_lua_State *L);
___PDR_LUAI_FUNC void ___pdr_luaD_inctop (___pdr_lua_State *L);

___PDR_LUAI_FUNC ___pdr_l_noret ___pdr_luaD_throw (___pdr_lua_State *L, int errcode);
___PDR_LUAI_FUNC int ___pdr_luaD_rawrunprotected (___pdr_lua_State *L, ___pdr_Pfunc f, void *ud);

} // end NS_PDR_SLUA

#endif

