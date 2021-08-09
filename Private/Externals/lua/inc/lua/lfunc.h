/*
** $Id: lfunc.h,v 2.15 2015/01/13 15:49:11 roberto Exp $
** Auxiliary functions to manipulate prototypes and closures
** See Copyright Notice in lua.h
*/

#ifndef ___pdr_lfunc_h___
#define ___pdr_lfunc_h___

#include "lobject.h"

#define ___pdr_sizeCclosure(n)	(___pdr_cast(int, sizeof(___pdr_CClosure)) + \
                                 ___pdr_cast(int, sizeof(___pdr_TValue)*((n)-1)))

#define ___pdr_sizeLclosure(n)	(___pdr_cast(int, sizeof(___pdr_LClosure)) + \
                                 ___pdr_cast(int, sizeof(___pdr_TValue *)*((n)-1)))


/* test whether thread is in 'twups' list */
#define ___pdr_isintwups(L)	(L->twups != L)


/*
** maximum number of upvalues in a closure (both C and Lua). (Value
** must fit in a VM register.)
*/
#define ___PDR_MAXUPVAL	255

namespace NS_PDR_SLUA {

/*
** Upvalues for Lua closures
*/
struct ___pdr_UpVal {
  ___pdr_TValue *v;  /* points to stack or to its own value */
  ___pdr_lu_mem refcount;  /* reference counter */
  union {
    struct {  /* (when open) */
      ___pdr_UpVal *next;  /* linked list */
      int touched;  /* mark to avoid cycles with dead threads */
    } open;
    ___pdr_TValue value;  /* the value (when closed) */
  } u;
};

#define ___pdr_upisopen(up)	((up)->v != &(up)->u.value)


___PDR_LUAI_FUNC ___pdr_Proto *___pdr_luaF_newproto (___pdr_lua_State *L);
___PDR_LUAI_FUNC ___pdr_CClosure *___pdr_luaF_newCclosure (___pdr_lua_State *L, int nelems);
___PDR_LUAI_FUNC ___pdr_LClosure *___pdr_luaF_newLclosure (___pdr_lua_State *L, int nelems);
___PDR_LUAI_FUNC void ___pdr_luaF_initupvals (___pdr_lua_State *L, ___pdr_LClosure *cl);
___PDR_LUAI_FUNC ___pdr_UpVal *___pdr_luaF_findupval (___pdr_lua_State *L, ___pdr_StkId level);
___PDR_LUAI_FUNC void ___pdr_luaF_close (___pdr_lua_State *L, ___pdr_StkId level);
___PDR_LUAI_FUNC void ___pdr_luaF_freeproto (___pdr_lua_State *L, ___pdr_Proto *f);
___PDR_LUAI_FUNC const char *___pdr_luaF_getlocalname (const ___pdr_Proto *func, int local_number,
                                         int pc);

} // end NS_PDR_SLUA

#endif
