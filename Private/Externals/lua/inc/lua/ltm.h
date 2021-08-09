/*
** $Id: ltm.h,v 2.22 2016/02/26 19:20:15 roberto Exp $
** Tag methods
** See Copyright Notice in lua.h
*/

#ifndef ___pdr_ltm_h___
#define ___pdr_ltm_h___


#include "lobject.h"

namespace NS_PDR_SLUA {

/*
* WARNING: if you change the order of this enumeration,
* grep "ORDER TM" and "ORDER OP"
*/
typedef enum {
  PDR_TM_INDEX,
  PDR_TM_NEWINDEX,
  PDR_TM_GC,
  PDR_TM_MODE,
  PDR_TM_LEN,
  PDR_TM_EQ,  /* last tag method with fast access */
  PDR_TM_ADD,
  PDR_TM_SUB,
  PDR_TM_MUL,
  PDR_TM_MOD,
  PDR_TM_POW,
  PDR_TM_DIV,
  PDR_TM_IDIV,
  PDR_TM_BAND,
  PDR_TM_BOR,
  PDR_TM_BXOR,
  PDR_TM_SHL,
  PDR_TM_SHR,
  PDR_TM_UNM,
  PDR_TM_BNOT,
  PDR_TM_LT,
  PDR_TM_LE,
  PDR_TM_CONCAT,
  PDR_TM_CALL,
  PDR_TM_N		/* number of elements in the enum */
} ___pdr_TMS;



#define ___pdr_gfasttm(g,et,e) ((et) == NULL ? NULL : \
  ((et)->flags & (1u<<(e))) ? NULL : ___pdr_luaT_gettm(et, e, (g)->tmname[e]))

#define ___pdr_fasttm(l,et,e)	___pdr_gfasttm(___pdr_G(l), et, e)

#define ___pdr_ttypename(x)	___pdr_luaT_typenames_[(x) + 1]

___PDR_LUAI_DDEC const char *const ___pdr_luaT_typenames_[___PDR_LUA_TOTALTAGS];


___PDR_LUAI_FUNC const char *___pdr_luaT_objtypename (___pdr_lua_State *L, const ___pdr_TValue *o);

___PDR_LUAI_FUNC const ___pdr_TValue *___pdr_luaT_gettm (___pdr_Table *events, ___pdr_TMS event, ___pdr_TString *ename);
___PDR_LUAI_FUNC const ___pdr_TValue *___pdr_luaT_gettmbyobj (___pdr_lua_State *L, const ___pdr_TValue *o,
                                                       ___pdr_TMS event);
___PDR_LUAI_FUNC void ___pdr_luaT_init (___pdr_lua_State *L);

___PDR_LUAI_FUNC void ___pdr_luaT_callTM (___pdr_lua_State *L, const ___pdr_TValue *f, const ___pdr_TValue *p1,
                            const ___pdr_TValue *p2, ___pdr_TValue *p3, int hasres);
___PDR_LUAI_FUNC int ___pdr_luaT_callbinTM (___pdr_lua_State *L, const ___pdr_TValue *p1, const ___pdr_TValue *p2,
                              ___pdr_StkId res, ___pdr_TMS event);
___PDR_LUAI_FUNC void ___pdr_luaT_trybinTM (___pdr_lua_State *L, const ___pdr_TValue *p1, const ___pdr_TValue *p2,
                              ___pdr_StkId res, ___pdr_TMS event);
___PDR_LUAI_FUNC int ___pdr_luaT_callorderTM (___pdr_lua_State *L, const ___pdr_TValue *p1,
                                const ___pdr_TValue *p2, ___pdr_TMS event);

} // end NS_PDR_SLUA

#endif
