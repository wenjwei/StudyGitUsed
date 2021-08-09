/*
** $Id: ltm.c,v 2.38 2016/12/22 13:08:50 roberto Exp $
** Tag methods
** See Copyright Notice in lua.h
*/

#define ___pdr_ltm_c
#define ___PDR_LUA_CORE

#include "ltm.h"
#include "lprefix.h"

#include <string.h>

#include "lua.h"
#include "ldebug.h"
#include "ldo.h"
#include "lobject.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "lvm.h"

namespace NS_PDR_SLUA {

static const char udatatypename[] = "userdata";

___PDR_LUAI_DDEF const char *const ___pdr_luaT_typenames_[___PDR_LUA_TOTALTAGS] = {
  "no value",
  "nil", "boolean", udatatypename, "number",
  "string", "table", "function", udatatypename, "thread",
  "proto" /* this last case is used for tests only */
};


void ___pdr_luaT_init (___pdr_lua_State *L) {
  static const char *const luaT_eventname[] = {  /* ORDER TM */
    "__index", "__newindex",
    "__gc", "__mode", "__len", "__eq",
    "__add", "__sub", "__mul", "__mod", "__pow",
    "__div", "__idiv",
    "__band", "__bor", "__bxor", "__shl", "__shr",
    "__unm", "__bnot", "__lt", "__le",
    "__concat", "__call"
  };
  int i;
  for (i=0; i<PDR_TM_N; i++) {
    ___pdr_G(L)->tmname[i] = ___pdr_luaS_new(L, luaT_eventname[i]);
    ___pdr_luaC_fix(L, ___pdr_obj2gco(___pdr_G(L)->tmname[i]));  /* never collect these names */
  }
}


/*
** function to be used with macro "fasttm": optimized for absence of
** tag methods
*/
const ___pdr_TValue *___pdr_luaT_gettm (___pdr_Table *events, ___pdr_TMS event, ___pdr_TString *ename) {
  const ___pdr_TValue *tm = ___pdr_luaH_getshortstr(events, ename);
  ___pdr_lua_assert(event <= PDR_TM_EQ);
  if (___pdr_ttisnil(tm)) {  /* no tag method? */
    events->flags |= ___pdr_cast_byte(1u<<event);  /* cache this fact */
    return NULL;
  }
  else return tm;
}


const ___pdr_TValue *___pdr_luaT_gettmbyobj (___pdr_lua_State *L, const ___pdr_TValue *o, ___pdr_TMS event) {
  ___pdr_Table *mt;
  switch (___pdr_ttnov(o)) {
    case ___PDR_LUA_TTABLE:
      mt = ___pdr_hvalue(o)->metatable;
      break;
    case ___PDR_LUA_TUSERDATA:
      mt = ___pdr_uvalue(o)->metatable;
      break;
    default:
      mt = ___pdr_G(L)->mt[___pdr_ttnov(o)];
  }
  return (mt ? ___pdr_luaH_getshortstr(mt, ___pdr_G(L)->tmname[event]) : ___pdr_luaO_nilobject);
}


/*
** Return the name of the type of an object. For tables and userdata
** with metatable, use their '__name' metafield, if present.
*/
const char *___pdr_luaT_objtypename (___pdr_lua_State *L, const ___pdr_TValue *o) {
  ___pdr_Table *mt;
  if ((___pdr_ttistable(o) && (mt = ___pdr_hvalue(o)->metatable) != NULL) ||
      (___pdr_ttisfulluserdata(o) && (mt = ___pdr_uvalue(o)->metatable) != NULL)) {
    const ___pdr_TValue *name = ___pdr_luaH_getshortstr(mt, ___pdr_luaS_new(L, "__name"));
    if (___pdr_ttisstring(name))  /* is '__name' a string? */
      return ___pdr_getstr(___pdr_tsvalue(name));  /* use it as type name */
  }
  return ___pdr_ttypename(___pdr_ttnov(o));  /* else use standard type name */
}


void ___pdr_luaT_callTM (___pdr_lua_State *L, const ___pdr_TValue *f, const ___pdr_TValue *p1,
                  const ___pdr_TValue *p2, ___pdr_TValue *p3, int hasres) {
  ptrdiff_t result = ___pdr_savestack(L, p3);
  ___pdr_StkId func = L->top;
  ___pdr_setobj2s(L, func, f);  /* push function (assume EXTRA_STACK) */
  ___pdr_setobj2s(L, func + 1, p1);  /* 1st argument */
  ___pdr_setobj2s(L, func + 2, p2);  /* 2nd argument */
  L->top += 3;
  if (!hasres)  /* no result? 'p3' is third argument */
    ___pdr_setobj2s(L, L->top++, p3);  /* 3rd argument */
  /* metamethod may yield only when called from Lua code */
  if (___pdr_isLua(L->ci))
    ___pdr_luaD_call(L, func, hasres);
  else
    ___pdr_luaD_callnoyield(L, func, hasres);
  if (hasres) {  /* if has result, move it to its place */
    p3 = ___pdr_restorestack(L, result);
    ___pdr_setobjs2s(L, p3, --L->top);
  }
}


int ___pdr_luaT_callbinTM (___pdr_lua_State *L, const ___pdr_TValue *p1, const ___pdr_TValue *p2,
                    ___pdr_StkId res, ___pdr_TMS event) {
  const ___pdr_TValue *tm = ___pdr_luaT_gettmbyobj(L, p1, event);  /* try first operand */
  if (___pdr_ttisnil(tm))
    tm = ___pdr_luaT_gettmbyobj(L, p2, event);  /* try second operand */
  if (___pdr_ttisnil(tm)) return 0;
  ___pdr_luaT_callTM(L, tm, p1, p2, res, 1);
  return 1;
}


void ___pdr_luaT_trybinTM (___pdr_lua_State *L, const ___pdr_TValue *p1, const ___pdr_TValue *p2,
                    ___pdr_StkId res, ___pdr_TMS event) {
  if (!___pdr_luaT_callbinTM(L, p1, p2, res, event)) {
    switch (event) {
      case PDR_TM_CONCAT:
        ___pdr_luaG_concaterror(L, p1, p2);
      /* call never returns, but to avoid warnings: *//* FALLTHROUGH */
      case PDR_TM_BAND: case PDR_TM_BOR: case PDR_TM_BXOR:
      case PDR_TM_SHL: case PDR_TM_SHR: case PDR_TM_BNOT: {
        ___pdr_lua_Number dummy;
        if (___pdr_tonumber(p1, &dummy) && ___pdr_tonumber(p2, &dummy))
          ___pdr_luaG_tointerror(L, p1, p2);
        else
          ___pdr_luaG_opinterror(L, p1, p2, "perform bitwise operation on");
      }
      /* calls never return, but to avoid warnings: *//* FALLTHROUGH */
      default:
        ___pdr_luaG_opinterror(L, p1, p2, "perform arithmetic on");
    }
  }
}


int ___pdr_luaT_callorderTM (___pdr_lua_State *L, const ___pdr_TValue *p1, const ___pdr_TValue *p2,
                      ___pdr_TMS event) {
  if (!___pdr_luaT_callbinTM(L, p1, p2, L->top, event))
    return -1;  /* no metamethod */
  else
    return !___pdr_l_isfalse(L->top);
}

} // end NS_PDR_SLUA