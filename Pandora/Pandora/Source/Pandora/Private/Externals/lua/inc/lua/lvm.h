/*
** $Id: lvm.h,v 2.41 2016/12/22 13:08:50 roberto Exp $
** Lua virtual machine
** See Copyright Notice in lua.h
*/

#ifndef lvm_h
#define lvm_h


#include "ldo.h"
#include "lobject.h"
#include "ltm.h"

#if !defined(___PDR_LUA_NOCVTN2S)
#define ___pdr_cvt2str(o)	___pdr_ttisnumber(o)
#else
#define ___pdr_cvt2str(o)	0	/* no conversion from numbers to strings */
#endif


#if !defined(___PDR_LUA_NOCVTS2N)
#define ___pdr_cvt2num(o)	___pdr_ttisstring(o)
#else
#define ___pdr_cvt2num(o)	0	/* no conversion from strings to numbers */
#endif


/*
** You can define LUA_FLOORN2I if you want to convert floats to integers
** by flooring them (instead of raising an error if they are not
** integral values)
*/
#if !defined(___PDR_LUA_FLOORN2I)
#define ___PDR_LUA_FLOORN2I		0
#endif


#define ___pdr_tonumber(o,n) \
	(___pdr_ttisfloat(o) ? (*(n) = ___pdr_fltvalue(o), 1) : ___pdr_luaV_tonumber_(o,n))

#define ___pdr_tointeger(o,i) \
    (___pdr_ttisinteger(o) ? (*(i) = ___pdr_ivalue(o), 1) : ___pdr_luaV_tointeger(o,i,___PDR_LUA_FLOORN2I))

#define ___pdr_intop(op,v1,v2) ___pdr_l_castU2S(___pdr_l_castS2U(v1) op ___pdr_l_castS2U(v2))

#define ___pdr_luaV_rawequalobj(t1,t2)		___pdr_luaV_equalobj(NULL,t1,t2)


/*
** fast track for 'gettable': if 't' is a table and 't[k]' is not nil,
** return 1 with 'slot' pointing to 't[k]' (final result).  Otherwise,
** return 0 (meaning it will have to check metamethod) with 'slot'
** pointing to a nil 't[k]' (if 't' is a table) or NULL (otherwise).
** 'f' is the raw get function to use.
*/
#define ___pdr_luaV_fastget(L,t,k,slot,f) \
  (!___pdr_ttistable(t)  \
   ? (slot = NULL, 0)  /* not a table; 'slot' is NULL and result is 0 */  \
   : (slot = f(___pdr_hvalue(t), k),  /* else, do raw access */  \
      !___pdr_ttisnil(slot)))  /* result not nil? */

/*
** standard implementation for 'gettable'
*/
#define ___pdr_luaV_gettable(L,t,k,v) { const ___pdr_TValue *slot; \
  if (___pdr_luaV_fastget(L,t,k,slot,___pdr_luaH_get)) { ___pdr_setobj2s(L, v, slot); } \
  else ___pdr_luaV_finishget(L,t,k,v,slot); }


/*
** Fast track for set table. If 't' is a table and 't[k]' is not nil,
** call GC barrier, do a raw 't[k]=v', and return true; otherwise,
** return false with 'slot' equal to NULL (if 't' is not a table) or
** 'nil'. (This is needed by 'luaV_finishget'.) Note that, if the macro
** returns true, there is no need to 'invalidateTMcache', because the
** call is not creating a new entry.
*/
#define ___pdr_luaV_fastset(L,t,k,slot,f,v) \
  (!___pdr_ttistable(t) \
   ? (slot = NULL, 0) \
   : (slot = f(___pdr_hvalue(t), k), \
     ___pdr_ttisnil(slot) ? 0 \
     : (___pdr_luaC_barrierback(L, ___pdr_hvalue(t), v), \
        ___pdr_setobj2t(L, ___pdr_cast(___pdr_TValue *,slot), v), \
        1)))


#define ___pdr_luaV_settable(L,t,k,v) { const ___pdr_TValue *slot; \
  if (!___pdr_luaV_fastset(L,t,k,slot,___pdr_luaH_get,v)) \
    ___pdr_luaV_finishset(L,t,k,v,slot); }

namespace NS_PDR_SLUA {

___PDR_LUAI_FUNC int ___pdr_luaV_equalobj (___pdr_lua_State *L, const ___pdr_TValue *t1, const ___pdr_TValue *t2);
___PDR_LUAI_FUNC int ___pdr_luaV_lessthan (___pdr_lua_State *L, const ___pdr_TValue *l, const ___pdr_TValue *r);
___PDR_LUAI_FUNC int ___pdr_luaV_lessequal (___pdr_lua_State *L, const ___pdr_TValue *l, const ___pdr_TValue *r);
___PDR_LUAI_FUNC int ___pdr_luaV_tonumber_ (const ___pdr_TValue *obj, ___pdr_lua_Number *n);
___PDR_LUAI_FUNC int ___pdr_luaV_tointeger (const ___pdr_TValue *obj, ___pdr_lua_Integer *p, int mode);
___PDR_LUAI_FUNC void ___pdr_luaV_finishget (___pdr_lua_State *L, const ___pdr_TValue *t, ___pdr_TValue *key,
                               ___pdr_StkId val, const ___pdr_TValue *slot);
___PDR_LUAI_FUNC void ___pdr_luaV_finishset (___pdr_lua_State *L, const ___pdr_TValue *t, ___pdr_TValue *key,
                               ___pdr_StkId val, const ___pdr_TValue *slot);
___PDR_LUAI_FUNC void ___pdr_luaV_finishOp (___pdr_lua_State *L);
___PDR_LUAI_FUNC void ___pdr_luaV_execute (___pdr_lua_State *L);
___PDR_LUAI_FUNC void ___pdr_luaV_concat (___pdr_lua_State *L, int total);
___PDR_LUAI_FUNC ___pdr_lua_Integer ___pdr_luaV_div (___pdr_lua_State *L, ___pdr_lua_Integer x, ___pdr_lua_Integer y);
___PDR_LUAI_FUNC ___pdr_lua_Integer ___pdr_luaV_mod (___pdr_lua_State *L, ___pdr_lua_Integer x, ___pdr_lua_Integer y);
___PDR_LUAI_FUNC ___pdr_lua_Integer ___pdr_luaV_shiftl (___pdr_lua_Integer x, ___pdr_lua_Integer y);
___PDR_LUAI_FUNC void ___pdr_luaV_objlen (___pdr_lua_State *L, ___pdr_StkId ra, const ___pdr_TValue *rb);

} // end NS_PDR_SLUA

#endif
