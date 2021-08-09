/*
** $Id: lapi.c,v 2.259 2016/02/29 14:27:14 roberto Exp $
** Lua API
** See Copyright Notice in lua.h
*/

#define __pdr_lapi_c
#define ___PDR_LUA_CORE

#include "lapi.h"
#include "lprefix.h"
#include "lua.h"

#include <stdarg.h>
#include <string.h>

#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "ltm.h"
#include "lundump.h"
#include "lvm.h"

namespace NS_PDR_SLUA {

const char ___pdr_lua_ident[] =
  "$LuaVersion: " ___PDR_LUA_COPYRIGHT " $"
  "$LuaAuthors: " ___PDR_LUA_AUTHORS " $";


/* value at a non-valid index */
#define ___PDR_NONVALIDVALUE		___pdr_cast(___pdr_TValue *, ___pdr_luaO_nilobject)

/* corresponding test */
#define ___pdr_isvalid(o)	((o) != ___pdr_luaO_nilobject)

/* test for pseudo index */
#define ___pdr_ispseudo(i)		((i) <= ___PDR_LUA_REGISTRYINDEX)

/* test for upvalue */
#define ___pdr_isupvalue(i)		((i) < ___PDR_LUA_REGISTRYINDEX)

/* test for valid but not pseudo index */
#define ___pdr_isstackindex(i, o)	(___pdr_isvalid(o) && !___pdr_ispseudo(i))

#define ___pdr_api_checkvalidindex(l,o)  ___pdr_api_check(l, ___pdr_isvalid(o), "invalid index")

#define ___pdr_api_checkstackindex(l, i, o)  \
	___pdr_api_check(l, ___pdr_isstackindex(i, o), "index not in the stack")


static ___pdr_TValue *index2addr (___pdr_lua_State *L, int idx) {
  ___pdr_CallInfo *ci = L->ci;
  if (idx > 0) {
    ___pdr_TValue *o = ci->func + idx;
    ___pdr_api_check(L, idx <= ci->top - (ci->func + 1), "unacceptable index");
    if (o >= L->top) return ___PDR_NONVALIDVALUE;
    else return o;
  }
  else if (!___pdr_ispseudo(idx)) {  /* negative index */
    ___pdr_api_check(L, idx != 0 && -idx <= L->top - (ci->func + 1), "invalid index");
    return L->top + idx;
  }
  else if (idx == ___PDR_LUA_REGISTRYINDEX)
    return &___pdr_G(L)->l_registry;
  else {  /* upvalues */
    idx = ___PDR_LUA_REGISTRYINDEX - idx;
    ___pdr_api_check(L, idx <= ___PDR_MAXUPVAL + 1, "upvalue index too large");
    if (___pdr_ttislcf(ci->func))  /* light C function? */
      return ___PDR_NONVALIDVALUE;  /* it has no upvalues */
    else {
      ___pdr_CClosure *func = ___pdr_clCvalue(ci->func);
      return (idx <= func->nupvalues) ? &func->upvalue[idx-1] : ___PDR_NONVALIDVALUE;
    }
  }
}


/*
** to be called by 'lua_checkstack' in protected mode, to grow stack
** capturing memory errors
*/
static void growstack (___pdr_lua_State *L, void *ud) {
  int size = *(int *)ud;
  ___pdr_luaD_growstack(L, size);
}


___PDR_LUA_API int ___pdr_lua_checkstack (___pdr_lua_State *L, int n) {
  int res;
  ___pdr_CallInfo *ci = L->ci;
  ___pdr_lua_lock(L);
  ___pdr_api_check(L, n >= 0, "negative 'n'");
  if (L->stack_last - L->top > n)  /* stack large enough? */
    res = 1;  /* yes; check is OK */
  else {  /* no; need to grow stack */
    int inuse = ___pdr_cast_int(L->top - L->stack) + ___PDR_EXTRA_STACK;
    if (inuse > ___PDR_LUAI_MAXSTACK - n)  /* can grow without overflow? */
      res = 0;  /* no */
    else  /* try to grow stack */
      res = (___pdr_luaD_rawrunprotected(L, &growstack, &n) == ___PDR_LUA_OK);
  }
  if (res && ci->top < L->top + n)
    ci->top = L->top + n;  /* adjust frame top */
  ___pdr_lua_unlock(L);
  return res;
}


___PDR_LUA_API void ___pdr_lua_xmove (___pdr_lua_State *from, ___pdr_lua_State *to, int n) {
  int i;
  if (from == to) return;
  ___pdr_lua_lock(to);
  ___pdr_api_checknelems(from, n);
  ___pdr_api_check(from, ___pdr_G(from) == ___pdr_G(to), "moving among independent states");
  ___pdr_api_check(from, to->ci->top - to->top >= n, "stack overflow");
  from->top -= n;
  for (i = 0; i < n; i++) {
    ___pdr_setobj2s(to, to->top, from->top + i);
    to->top++;  /* stack already checked by previous 'api_check' */
  }
  ___pdr_lua_unlock(to);
}


___PDR_LUA_API ___pdr_lua_CFunction ___pdr_lua_atpanic (___pdr_lua_State *L, ___pdr_lua_CFunction panicf) {
  ___pdr_lua_CFunction old;
  ___pdr_lua_lock(L);
  old = ___pdr_G(L)->panic;
  ___pdr_G(L)->panic = panicf;
  ___pdr_lua_unlock(L);
  return old;
}


___PDR_LUA_API const ___pdr_lua_Number *___pdr_lua_version (___pdr_lua_State *L) {
  static const ___pdr_lua_Number version = ___PDR_LUA_VERSION_NUM;
  if (L == NULL) return &version;
  else return ___pdr_G(L)->version;
}



/*
** basic stack manipulation
*/


/*
** convert an acceptable stack index into an absolute index
*/
___PDR_LUA_API int ___pdr_lua_absindex (___pdr_lua_State *L, int idx) {
  return (idx > 0 || ___pdr_ispseudo(idx))
         ? idx
         : ___pdr_cast_int(L->top - L->ci->func) + idx;
}


___PDR_LUA_API int ___pdr_lua_gettop (___pdr_lua_State *L) {
  return ___pdr_cast_int(L->top - (L->ci->func + 1));
}


___PDR_LUA_API void ___pdr_lua_settop (___pdr_lua_State *L, int idx) {
  ___pdr_StkId func = L->ci->func;
  ___pdr_lua_lock(L);
  if (idx >= 0) {
    ___pdr_api_check(L, idx <= L->stack_last - (func + 1), "new top too large");
    while (L->top < (func + 1) + idx)
      ___pdr_setnilvalue(L->top++);
    L->top = (func + 1) + idx;
  }
  else {
    ___pdr_api_check(L, -(idx+1) <= (L->top - (func + 1)), "invalid new top");
    L->top += idx+1;  /* 'subtract' index (index is negative) */
  }
  ___pdr_lua_unlock(L);
}


/*
** Reverse the stack segment from 'from' to 'to'
** (auxiliary to 'lua_rotate')
*/
static void reverse (___pdr_lua_State *L, ___pdr_StkId from, ___pdr_StkId to) {
  for (; from < to; from++, to--) {
    ___pdr_TValue temp;
    ___pdr_setobj(L, &temp, from);
    ___pdr_setobjs2s(L, from, to);
    ___pdr_setobj2s(L, to, &temp);
  }
}


/*
** Let x = AB, where A is a prefix of length 'n'. Then,
** rotate x n == BA. But BA == (A^r . B^r)^r.
*/
___PDR_LUA_API void ___pdr_lua_rotate (___pdr_lua_State *L, int idx, int n) {
  ___pdr_StkId p, t, m;
  ___pdr_lua_lock(L);
  t = L->top - 1;  /* end of stack segment being rotated */
  p = index2addr(L, idx);  /* start of segment */
  ___pdr_api_checkstackindex(L, idx, p);
  ___pdr_api_check(L, (n >= 0 ? n : -n) <= (t - p + 1), "invalid 'n'");
  m = (n >= 0 ? t - n : p - n - 1);  /* end of prefix */
  reverse(L, p, m);  /* reverse the prefix with length 'n' */
  reverse(L, m + 1, t);  /* reverse the suffix */
  reverse(L, p, t);  /* reverse the entire segment */
  ___pdr_lua_unlock(L);
}


___PDR_LUA_API void ___pdr_lua_copy (___pdr_lua_State *L, int fromidx, int toidx) {
  ___pdr_TValue *fr, *to;
  ___pdr_lua_lock(L);
  fr = index2addr(L, fromidx);
  to = index2addr(L, toidx);
  ___pdr_api_checkvalidindex(L, to);
  ___pdr_setobj(L, to, fr);
  if (___pdr_isupvalue(toidx))  /* function upvalue? */
    ___pdr_luaC_barrier(L, ___pdr_clCvalue(L->ci->func), fr);
  /* LUA_REGISTRYINDEX does not need gc barrier
     (collector revisits it before finishing collection) */
  ___pdr_lua_unlock(L);
}


___PDR_LUA_API void ___pdr_lua_pushvalue (___pdr_lua_State *L, int idx) {
  ___pdr_lua_lock(L);
  ___pdr_setobj2s(L, L->top, index2addr(L, idx));
  ___pdr_api_incr_top(L);
  ___pdr_lua_unlock(L);
}



/*
** access functions (stack -> C)
*/


___PDR_LUA_API int ___pdr_lua_type (___pdr_lua_State *L, int idx) {
  ___pdr_StkId o = index2addr(L, idx);
  return (___pdr_isvalid(o) ? ___pdr_ttnov(o) : ___PDR_LUA_TNONE);
}


___PDR_LUA_API const char *___pdr_lua_typename (___pdr_lua_State *L, int t) {
  ___PDR_UNUSED(L);
  ___pdr_api_check(L, ___PDR_LUA_TNONE <= t && t < ___PDR_LUA_NUMTAGS, "invalid tag");
  return ___pdr_ttypename(t);
}


___PDR_LUA_API int ___pdr_lua_iscfunction (___pdr_lua_State *L, int idx) {
  ___pdr_StkId o = index2addr(L, idx);
  return (___pdr_ttislcf(o) || (___pdr_ttisCclosure(o)));
}


___PDR_LUA_API int ___pdr_lua_isinteger (___pdr_lua_State *L, int idx) {
  ___pdr_StkId o = index2addr(L, idx);
  return ___pdr_ttisinteger(o);
}


___PDR_LUA_API int ___pdr_lua_isnumber (___pdr_lua_State *L, int idx) {
  ___pdr_lua_Number n;
  const ___pdr_TValue *o = index2addr(L, idx);
  return ___pdr_tonumber(o, &n);
}


___PDR_LUA_API int ___pdr_lua_isstring (___pdr_lua_State *L, int idx) {
  const ___pdr_TValue *o = index2addr(L, idx);
  return (___pdr_ttisstring(o) || ___pdr_cvt2str(o));
}


___PDR_LUA_API int ___pdr_lua_isuserdata (___pdr_lua_State *L, int idx) {
  const ___pdr_TValue *o = index2addr(L, idx);
  return (___pdr_ttisfulluserdata(o) || ___pdr_ttislightuserdata(o));
}


___PDR_LUA_API int ___pdr_lua_rawequal (___pdr_lua_State *L, int index1, int index2) {
  ___pdr_StkId o1 = index2addr(L, index1);
  ___pdr_StkId o2 = index2addr(L, index2);
  return (___pdr_isvalid(o1) && ___pdr_isvalid(o2)) ? ___pdr_luaV_rawequalobj(o1, o2) : 0;
}


___PDR_LUA_API void ___pdr_lua_arith (___pdr_lua_State *L, int op) {
  ___pdr_lua_lock(L);
  if (op != ___PDR_LUA_OPUNM && op != ___PDR_LUA_OPBNOT)
    ___pdr_api_checknelems(L, 2);  /* all other operations expect two operands */
  else {  /* for unary operations, add fake 2nd operand */
    ___pdr_api_checknelems(L, 1);
    ___pdr_setobjs2s(L, L->top, L->top - 1);
    ___pdr_api_incr_top(L);
  }
  /* first operand at top - 2, second at top - 1; result go to top - 2 */
  ___pdr_luaO_arith(L, op, L->top - 2, L->top - 1, L->top - 2);
  L->top--;  /* remove second operand */
  ___pdr_lua_unlock(L);
}


___PDR_LUA_API int ___pdr_lua_compare (___pdr_lua_State *L, int index1, int index2, int op) {
  ___pdr_StkId o1, o2;
  int i = 0;
  ___pdr_lua_lock(L);  /* may call tag method */
  o1 = index2addr(L, index1);
  o2 = index2addr(L, index2);
  if (___pdr_isvalid(o1) && ___pdr_isvalid(o2)) {
    switch (op) {
      case ___PDR_LUA_OPEQ: i = ___pdr_luaV_equalobj(L, o1, o2); break;
      case ___PDR_LUA_OPLT: i = ___pdr_luaV_lessthan(L, o1, o2); break;
      case ___PDR_LUA_OPLE: i = ___pdr_luaV_lessequal(L, o1, o2); break;
      default: ___pdr_api_check(L, 0, "invalid option");
    }
  }
  ___pdr_lua_unlock(L);
  return i;
}


___PDR_LUA_API size_t ___pdr_lua_stringtonumber (___pdr_lua_State *L, const char *s) {
  size_t sz = ___pdr_luaO_str2num(s, L->top);
  if (sz != 0)
    ___pdr_api_incr_top(L);
  return sz;
}


___PDR_LUA_API ___pdr_lua_Number ___pdr_lua_tonumberx (___pdr_lua_State *L, int idx, int *pisnum) {
  ___pdr_lua_Number n;
  const ___pdr_TValue *o = index2addr(L, idx);
  int isnum = ___pdr_tonumber(o, &n);
  if (!isnum)
    n = 0;  /* call to 'tonumber' may change 'n' even if it fails */
  if (pisnum) *pisnum = isnum;
  return n;
}


___PDR_LUA_API ___pdr_lua_Integer ___pdr_lua_tointegerx (___pdr_lua_State *L, int idx, int *pisnum) {
  ___pdr_lua_Integer res;
  const ___pdr_TValue *o = index2addr(L, idx);
  int isnum = ___pdr_tointeger(o, &res);
  if (!isnum)
    res = 0;  /* call to 'tointeger' may change 'n' even if it fails */
  if (pisnum) *pisnum = isnum;
  return res;
}


___PDR_LUA_API int ___pdr_lua_toboolean (___pdr_lua_State *L, int idx) {
  const ___pdr_TValue *o = index2addr(L, idx);
  return !___pdr_l_isfalse(o);
}


___PDR_LUA_API const char *___pdr_lua_tolstring (___pdr_lua_State *L, int idx, size_t *len) {
  ___pdr_StkId o = index2addr(L, idx);
  if (!___pdr_ttisstring(o)) {
    if (!___pdr_cvt2str(o)) {  /* not convertible? */
      if (len != NULL) *len = 0;
      return NULL;
    }
    ___pdr_lua_lock(L);  /* 'luaO_tostring' may create a new string */
    ___pdr_luaO_tostring(L, o);
    ___pdr_luaC_checkGC(L);
    o = index2addr(L, idx);  /* previous call may reallocate the stack */
    ___pdr_lua_unlock(L);
  }
  if (len != NULL)
    *len = ___pdr_vslen(o);
  return ___pdr_svalue(o);
}


___PDR_LUA_API size_t ___pdr_lua_rawlen (___pdr_lua_State *L, int idx) {
  ___pdr_StkId o = index2addr(L, idx);
  switch (___pdr_ttype(o)) {
    case ___PDR_LUA_TSHRSTR: return ___pdr_tsvalue(o)->shrlen;
    case ___PDR_LUA_TLNGSTR: return ___pdr_tsvalue(o)->u.lnglen;
    case ___PDR_LUA_TUSERDATA: return ___pdr_uvalue(o)->len;
    case ___PDR_LUA_TTABLE: return ___pdr_luaH_getn(___pdr_hvalue(o));
    default: return 0;
  }
}


___PDR_LUA_API ___pdr_lua_CFunction ___pdr_lua_tocfunction (___pdr_lua_State *L, int idx) {
  ___pdr_StkId o = index2addr(L, idx);
  if (___pdr_ttislcf(o)) return ___pdr_fvalue(o);
  else if (___pdr_ttisCclosure(o))
    return ___pdr_clCvalue(o)->f;
  else return NULL;  /* not a C function */
}


___PDR_LUA_API void *___pdr_lua_touserdata (___pdr_lua_State *L, int idx) {
  ___pdr_StkId o = index2addr(L, idx);
  switch (___pdr_ttnov(o)) {
    case ___PDR_LUA_TUSERDATA: return ___pdr_getudatamem(___pdr_uvalue(o));
    case ___PDR_LUA_TLIGHTUSERDATA: return ___pdr_pvalue(o);
    default: return NULL;
  }
}


___PDR_LUA_API ___pdr_lua_State *___pdr_lua_tothread (___pdr_lua_State *L, int idx) {
  ___pdr_StkId o = index2addr(L, idx);
  return (!___pdr_ttisthread(o)) ? NULL : ___pdr_thvalue(o);
}


___PDR_LUA_API const void *___pdr_lua_topointer (___pdr_lua_State *L, int idx) {
  ___pdr_StkId o = index2addr(L, idx);
  switch (___pdr_ttype(o)) {
    case ___PDR_LUA_TTABLE: return ___pdr_hvalue(o);
    case ___PDR_LUA_TLCL: return ___pdr_clLvalue(o);
    case ___PDR_LUA_TCCL: return ___pdr_clCvalue(o);
    case ___PDR_LUA_TLCF: return ___pdr_cast(void *, ___pdr_cast(size_t, ___pdr_fvalue(o)));
    case ___PDR_LUA_TTHREAD: return ___pdr_thvalue(o);
    case ___PDR_LUA_TUSERDATA: return ___pdr_getudatamem(___pdr_uvalue(o));
    case ___PDR_LUA_TLIGHTUSERDATA: return ___pdr_pvalue(o);
    default: return NULL;
  }
}



/*
** push functions (C -> stack)
*/


___PDR_LUA_API void ___pdr_lua_pushnil (___pdr_lua_State *L) {
  ___pdr_lua_lock(L);
  ___pdr_setnilvalue(L->top);
  ___pdr_api_incr_top(L);
  ___pdr_lua_unlock(L);
}


___PDR_LUA_API void ___pdr_lua_pushnumber (___pdr_lua_State *L, ___pdr_lua_Number n) {
  ___pdr_lua_lock(L);
  ___pdr_setfltvalue(L->top, n);
  ___pdr_api_incr_top(L);
  ___pdr_lua_unlock(L);
}


___PDR_LUA_API void ___pdr_lua_pushinteger (___pdr_lua_State *L, ___pdr_lua_Integer n) {
  ___pdr_lua_lock(L);
  ___pdr_setivalue(L->top, n);
  ___pdr_api_incr_top(L);
  ___pdr_lua_unlock(L);
}


/*
** Pushes on the stack a string with given length. Avoid using 's' when
** 'len' == 0 (as 's' can be NULL in that case), due to later use of
** 'memcmp' and 'memcpy'.
*/
___PDR_LUA_API const char *___pdr_lua_pushlstring (___pdr_lua_State *L, const char *s, size_t len) {
  ___pdr_TString *ts;
  ___pdr_lua_lock(L);
  ts = (len == 0) ? ___pdr_luaS_new(L, "") : ___pdr_luaS_newlstr(L, s, len);
  ___pdr_setsvalue2s(L, L->top, ts);
  ___pdr_api_incr_top(L);
  ___pdr_luaC_checkGC(L);
  ___pdr_lua_unlock(L);
  return ___pdr_getstr(ts);
}


___PDR_LUA_API const char *___pdr_lua_pushstring (___pdr_lua_State *L, const char *s) {
  ___pdr_lua_lock(L);
  if (s == NULL)
    ___pdr_setnilvalue(L->top);
  else {
    ___pdr_TString *ts;
    ts = ___pdr_luaS_new(L, s);
    ___pdr_setsvalue2s(L, L->top, ts);
    s = ___pdr_getstr(ts);  /* internal copy's address */
  }
  ___pdr_api_incr_top(L);
  ___pdr_luaC_checkGC(L);
  ___pdr_lua_unlock(L);
  return s;
}


___PDR_LUA_API const char *___pdr_lua_pushvfstring (___pdr_lua_State *L, const char *fmt,
                                      va_list argp) {
  const char *ret;
  ___pdr_lua_lock(L);
  ret = ___pdr_luaO_pushvfstring(L, fmt, argp);
  ___pdr_luaC_checkGC(L);
  ___pdr_lua_unlock(L);
  return ret;
}


___PDR_LUA_API const char *___pdr_lua_pushfstring (___pdr_lua_State *L, const char *fmt, ...) {
  const char *ret;
  va_list argp;
  ___pdr_lua_lock(L);
  va_start(argp, fmt);
  ret = ___pdr_luaO_pushvfstring(L, fmt, argp);
  va_end(argp);
  ___pdr_luaC_checkGC(L);
  ___pdr_lua_unlock(L);
  return ret;
}


___PDR_LUA_API void ___pdr_lua_pushcclosure (___pdr_lua_State *L, ___pdr_lua_CFunction fn, int n) {
  ___pdr_lua_lock(L);
  if (n == 0) {
    ___pdr_setfvalue(L->top, fn);
  }
  else {
    ___pdr_CClosure *cl;
    ___pdr_api_checknelems(L, n);
    ___pdr_api_check(L, n <= ___PDR_MAXUPVAL, "upvalue index too large");
    cl = ___pdr_luaF_newCclosure(L, n);
    cl->f = fn;
    L->top -= n;
    while (n--) {
      ___pdr_setobj2n(L, &cl->upvalue[n], L->top + n);
      /* does not need barrier because closure is white */
    }
    ___pdr_setclCvalue(L, L->top, cl);
  }
  ___pdr_api_incr_top(L);
  ___pdr_luaC_checkGC(L);
  ___pdr_lua_unlock(L);
}


___PDR_LUA_API void ___pdr_lua_pushboolean (___pdr_lua_State *L, int b) {
  ___pdr_lua_lock(L);
  ___pdr_setbvalue(L->top, (b != 0));  /* ensure that true is 1 */
  ___pdr_api_incr_top(L);
  ___pdr_lua_unlock(L);
}


___PDR_LUA_API void ___pdr_lua_pushlightuserdata (___pdr_lua_State *L, void *p) {
  ___pdr_lua_lock(L);
  ___pdr_setpvalue(L->top, p);
  ___pdr_api_incr_top(L);
  ___pdr_lua_unlock(L);
}


___PDR_LUA_API int ___pdr_lua_pushthread (___pdr_lua_State *L) {
  ___pdr_lua_lock(L);
  ___pdr_setthvalue(L, L->top, L);
  ___pdr_api_incr_top(L);
  ___pdr_lua_unlock(L);
  return (___pdr_G(L)->mainthread == L);
}



/*
** get functions (Lua -> stack)
*/


static int auxgetstr (___pdr_lua_State *L, const ___pdr_TValue *t, const char *k) {
  const ___pdr_TValue *slot;
  ___pdr_TString *str = ___pdr_luaS_new(L, k);
  if (___pdr_luaV_fastget(L, t, str, slot, ___pdr_luaH_getstr)) {
    ___pdr_setobj2s(L, L->top, slot);
    ___pdr_api_incr_top(L);
  }
  else {
    ___pdr_setsvalue2s(L, L->top, str);
    ___pdr_api_incr_top(L);
    ___pdr_luaV_finishget(L, t, L->top - 1, L->top - 1, slot);
  }
  ___pdr_lua_unlock(L);
  return ___pdr_ttnov(L->top - 1);
}


___PDR_LUA_API int ___pdr_lua_getglobal (___pdr_lua_State *L, const char *name) {
  ___pdr_Table *reg = ___pdr_hvalue(&___pdr_G(L)->l_registry);
  ___pdr_lua_lock(L);
  return auxgetstr(L, ___pdr_luaH_getint(reg, ___PDR_LUA_RIDX_GLOBALS), name);
}


___PDR_LUA_API int ___pdr_lua_gettable (___pdr_lua_State *L, int idx) {
  ___pdr_StkId t;
  ___pdr_lua_lock(L);
  t = index2addr(L, idx);
  ___pdr_luaV_gettable(L, t, L->top - 1, L->top - 1);
  ___pdr_lua_unlock(L);
  return ___pdr_ttnov(L->top - 1);
}


___PDR_LUA_API int ___pdr_lua_getfield (___pdr_lua_State *L, int idx, const char *k) {
  ___pdr_lua_lock(L);
  return auxgetstr(L, index2addr(L, idx), k);
}


___PDR_LUA_API int ___pdr_lua_geti (___pdr_lua_State *L, int idx, ___pdr_lua_Integer n) {
  ___pdr_StkId t;
  const ___pdr_TValue *slot;
  ___pdr_lua_lock(L);
  t = index2addr(L, idx);
  if (___pdr_luaV_fastget(L, t, n, slot, ___pdr_luaH_getint)) {
    ___pdr_setobj2s(L, L->top, slot);
    ___pdr_api_incr_top(L);
  }
  else {
    ___pdr_setivalue(L->top, n);
    ___pdr_api_incr_top(L);
    ___pdr_luaV_finishget(L, t, L->top - 1, L->top - 1, slot);
  }
  ___pdr_lua_unlock(L);
  return ___pdr_ttnov(L->top - 1);
}


___PDR_LUA_API int ___pdr_lua_rawget (___pdr_lua_State *L, int idx) {
  ___pdr_StkId t;
  ___pdr_lua_lock(L);
  t = index2addr(L, idx);
  ___pdr_api_check(L, ___pdr_ttistable(t), "table expected");
  ___pdr_setobj2s(L, L->top - 1, ___pdr_luaH_get(___pdr_hvalue(t), L->top - 1));
  ___pdr_lua_unlock(L);
  return ___pdr_ttnov(L->top - 1);
}


___PDR_LUA_API int ___pdr_lua_rawgeti (___pdr_lua_State *L, int idx, ___pdr_lua_Integer n) {
  ___pdr_StkId t;
  ___pdr_lua_lock(L);
  t = index2addr(L, idx);
  ___pdr_api_check(L, ___pdr_ttistable(t), "table expected");
  ___pdr_setobj2s(L, L->top, ___pdr_luaH_getint(___pdr_hvalue(t), n));
  ___pdr_api_incr_top(L);
  ___pdr_lua_unlock(L);
  return ___pdr_ttnov(L->top - 1);
}


___PDR_LUA_API int ___pdr_lua_rawgetp (___pdr_lua_State *L, int idx, const void *p) {
  ___pdr_StkId t;
  ___pdr_TValue k;
  ___pdr_lua_lock(L);
  t = index2addr(L, idx);
  ___pdr_api_check(L, ___pdr_ttistable(t), "table expected");
  ___pdr_setpvalue(&k, ___pdr_cast(void *, p));
  ___pdr_setobj2s(L, L->top, ___pdr_luaH_get(___pdr_hvalue(t), &k));
  ___pdr_api_incr_top(L);
  ___pdr_lua_unlock(L);
  return ___pdr_ttnov(L->top - 1);
}


___PDR_LUA_API void ___pdr_lua_createtable (___pdr_lua_State *L, int narray, int nrec) {
  ___pdr_Table *t;
  ___pdr_lua_lock(L);
  t = ___pdr_luaH_new(L);
  ___pdr_sethvalue(L, L->top, t);
  ___pdr_api_incr_top(L);
  if (narray > 0 || nrec > 0)
    ___pdr_luaH_resize(L, t, narray, nrec);
  ___pdr_luaC_checkGC(L);
  ___pdr_lua_unlock(L);
}


___PDR_LUA_API int ___pdr_lua_getmetatable (___pdr_lua_State *L, int objindex) {
  const ___pdr_TValue *obj;
  ___pdr_Table *mt;
  int res = 0;
  ___pdr_lua_lock(L);
  obj = index2addr(L, objindex);
  switch (___pdr_ttnov(obj)) {
    case ___PDR_LUA_TTABLE:
      mt = ___pdr_hvalue(obj)->metatable;
      break;
    case ___PDR_LUA_TUSERDATA:
      mt = ___pdr_uvalue(obj)->metatable;
      break;
    default:
      mt = ___pdr_G(L)->mt[___pdr_ttnov(obj)];
      break;
  }
  if (mt != NULL) {
    ___pdr_sethvalue(L, L->top, mt);
    ___pdr_api_incr_top(L);
    res = 1;
  }
  ___pdr_lua_unlock(L);
  return res;
}


___PDR_LUA_API int ___pdr_lua_getuservalue (___pdr_lua_State *L, int idx) {
  ___pdr_StkId o;
  ___pdr_lua_lock(L);
  o = index2addr(L, idx);
  ___pdr_api_check(L, ___pdr_ttisfulluserdata(o), "full userdata expected");
  ___pdr_getuservalue(L, ___pdr_uvalue(o), L->top);
  ___pdr_api_incr_top(L);
  ___pdr_lua_unlock(L);
  return ___pdr_ttnov(L->top - 1);
}


/*
** set functions (stack -> Lua)
*/

/*
** t[k] = value at the top of the stack (where 'k' is a string)
*/
static void auxsetstr (___pdr_lua_State *L, const ___pdr_TValue *t, const char *k) {
  const ___pdr_TValue *slot;
  ___pdr_TString *str = ___pdr_luaS_new(L, k);
  ___pdr_api_checknelems(L, 1);
  if (___pdr_luaV_fastset(L, t, str, slot, ___pdr_luaH_getstr, L->top - 1))
    L->top--;  /* pop value */
  else {
    ___pdr_setsvalue2s(L, L->top, str);  /* push 'str' (to make it a TValue) */
    ___pdr_api_incr_top(L);
    ___pdr_luaV_finishset(L, t, L->top - 1, L->top - 2, slot);
    L->top -= 2;  /* pop value and key */
  }
  ___pdr_lua_unlock(L);  /* lock done by caller */
}


___PDR_LUA_API void ___pdr_lua_setglobal (___pdr_lua_State *L, const char *name) {
  ___pdr_Table *reg = ___pdr_hvalue(&___pdr_G(L)->l_registry);
  ___pdr_lua_lock(L);  /* unlock done in 'auxsetstr' */
  auxsetstr(L, ___pdr_luaH_getint(reg, ___PDR_LUA_RIDX_GLOBALS), name);
}


___PDR_LUA_API void ___pdr_lua_settable (___pdr_lua_State *L, int idx) {
  ___pdr_StkId t;
  ___pdr_lua_lock(L);
  ___pdr_api_checknelems(L, 2);
  t = index2addr(L, idx);
  ___pdr_luaV_settable(L, t, L->top - 2, L->top - 1);
  L->top -= 2;  /* pop index and value */
  ___pdr_lua_unlock(L);
}


___PDR_LUA_API void ___pdr_lua_setfield (___pdr_lua_State *L, int idx, const char *k) {
  ___pdr_lua_lock(L);  /* unlock done in 'auxsetstr' */
  auxsetstr(L, index2addr(L, idx), k);
}


___PDR_LUA_API void ___pdr_lua_seti (___pdr_lua_State *L, int idx, ___pdr_lua_Integer n) {
  ___pdr_StkId t;
  const ___pdr_TValue *slot;
  ___pdr_lua_lock(L);
  ___pdr_api_checknelems(L, 1);
  t = index2addr(L, idx);
  if (___pdr_luaV_fastset(L, t, n, slot, ___pdr_luaH_getint, L->top - 1))
    L->top--;  /* pop value */
  else {
    ___pdr_setivalue(L->top, n);
    ___pdr_api_incr_top(L);
    ___pdr_luaV_finishset(L, t, L->top - 1, L->top - 2, slot);
    L->top -= 2;  /* pop value and key */
  }
  ___pdr_lua_unlock(L);
}


___PDR_LUA_API void ___pdr_lua_rawset (___pdr_lua_State *L, int idx) {
  ___pdr_StkId o;
  ___pdr_TValue *slot;
  ___pdr_lua_lock(L);
  ___pdr_api_checknelems(L, 2);
  o = index2addr(L, idx);
  ___pdr_api_check(L, ___pdr_ttistable(o), "table expected");
  slot = ___pdr_luaH_set(L, ___pdr_hvalue(o), L->top - 2);
  ___pdr_setobj2t(L, slot, L->top - 1);
  ___pdr_invalidateTMcache(___pdr_hvalue(o));
  ___pdr_luaC_barrierback(L, ___pdr_hvalue(o), L->top-1);
  L->top -= 2;
  ___pdr_lua_unlock(L);
}


___PDR_LUA_API void ___pdr_lua_rawseti (___pdr_lua_State *L, int idx, ___pdr_lua_Integer n) {
  ___pdr_StkId o;
  ___pdr_lua_lock(L);
  ___pdr_api_checknelems(L, 1);
  o = index2addr(L, idx);
  ___pdr_api_check(L, ___pdr_ttistable(o), "table expected");
  ___pdr_luaH_setint(L, ___pdr_hvalue(o), n, L->top - 1);
  ___pdr_luaC_barrierback(L, ___pdr_hvalue(o), L->top-1);
  L->top--;
  ___pdr_lua_unlock(L);
}


___PDR_LUA_API void ___pdr_lua_rawsetp (___pdr_lua_State *L, int idx, const void *p) {
  ___pdr_StkId o;
  ___pdr_TValue k, *slot;
  ___pdr_lua_lock(L);
  ___pdr_api_checknelems(L, 1);
  o = index2addr(L, idx);
  ___pdr_api_check(L, ___pdr_ttistable(o), "table expected");
  ___pdr_setpvalue(&k, ___pdr_cast(void *, p));
  slot = ___pdr_luaH_set(L, ___pdr_hvalue(o), &k);
  ___pdr_setobj2t(L, slot, L->top - 1);
  ___pdr_luaC_barrierback(L, ___pdr_hvalue(o), L->top - 1);
  L->top--;
  ___pdr_lua_unlock(L);
}


___PDR_LUA_API int ___pdr_lua_setmetatable (___pdr_lua_State *L, int objindex) {
  ___pdr_TValue *obj;
  ___pdr_Table *mt;
  ___pdr_lua_lock(L);
  ___pdr_api_checknelems(L, 1);
  obj = index2addr(L, objindex);
  if (___pdr_ttisnil(L->top - 1))
    mt = NULL;
  else {
    ___pdr_api_check(L, ___pdr_ttistable(L->top - 1), "table expected");
    mt = ___pdr_hvalue(L->top - 1);
  }
  switch (___pdr_ttnov(obj)) {
    case ___PDR_LUA_TTABLE: {
      ___pdr_hvalue(obj)->metatable = mt;
      if (mt) {
        ___pdr_luaC_objbarrier(L, ___pdr_gcvalue(obj), mt);
        ___pdr_luaC_checkfinalizer(L, ___pdr_gcvalue(obj), mt);
      }
      break;
    }
    case ___PDR_LUA_TUSERDATA: {
      ___pdr_uvalue(obj)->metatable = mt;
      if (mt) {
        ___pdr_luaC_objbarrier(L, ___pdr_uvalue(obj), mt);
        ___pdr_luaC_checkfinalizer(L, ___pdr_gcvalue(obj), mt);
      }
      break;
    }
    default: {
      ___pdr_G(L)->mt[___pdr_ttnov(obj)] = mt;
      break;
    }
  }
  L->top--;
  ___pdr_lua_unlock(L);
  return 1;
}


___PDR_LUA_API void ___pdr_lua_setuservalue (___pdr_lua_State *L, int idx) {
  ___pdr_StkId o;
  ___pdr_lua_lock(L);
  ___pdr_api_checknelems(L, 1);
  o = index2addr(L, idx);
  ___pdr_api_check(L, ___pdr_ttisfulluserdata(o), "full userdata expected");
  ___pdr_setuservalue(L, ___pdr_uvalue(o), L->top - 1);
  ___pdr_luaC_barrier(L, ___pdr_gcvalue(o), L->top - 1);
  L->top--;
  ___pdr_lua_unlock(L);
}


/*
** 'load' and 'call' functions (run Lua code)
*/


#define ___pdr_checkresults(L,na,nr) \
    ___pdr_api_check(L, (nr) == ___PDR_LUA_MULTRET || (L->ci->top - L->top >= (nr) - (na)), \
	"results from function overflow current stack size")


___PDR_LUA_API void ___pdr_lua_callk (___pdr_lua_State *L, int nargs, int nresults,
                        ___pdr_lua_KContext ctx, ___pdr_lua_KFunction k) {
  ___pdr_StkId func;
  ___pdr_lua_lock(L);
  ___pdr_api_check(L, k == NULL || !___pdr_isLua(L->ci),
    "cannot use continuations inside hooks");
  ___pdr_api_checknelems(L, nargs+1);
  ___pdr_api_check(L, L->status == ___PDR_LUA_OK, "cannot do calls on non-normal thread");
  ___pdr_checkresults(L, nargs, nresults);
  func = L->top - (nargs+1);
  if (k != NULL && L->nny == 0) {  /* need to prepare continuation? */
    L->ci->u.c.k = k;  /* save continuation */
    L->ci->u.c.ctx = ctx;  /* save context */
    ___pdr_luaD_call(L, func, nresults);  /* do the call */
  }
  else  /* no continuation or no yieldable */
    ___pdr_luaD_callnoyield(L, func, nresults);  /* just do the call */
  ___pdr_adjustresults(L, nresults);
  ___pdr_lua_unlock(L);
}



/*
** Execute a protected call.
*/
struct CallS {  /* data to 'f_call' */
  ___pdr_StkId func;
  int nresults;
};


static void f_call (___pdr_lua_State *L, void *ud) {
  struct CallS *c = ___pdr_cast(struct CallS *, ud);
  ___pdr_luaD_callnoyield(L, c->func, c->nresults);
}



___PDR_LUA_API int ___pdr_lua_pcallk (___pdr_lua_State *L, int nargs, int nresults, int errfunc,
                        ___pdr_lua_KContext ctx, ___pdr_lua_KFunction k) {
  struct CallS c;
  int status;
  ptrdiff_t func;
  ___pdr_lua_lock(L);
  ___pdr_api_check(L, k == NULL || !___pdr_isLua(L->ci),
    "cannot use continuations inside hooks");
  ___pdr_api_checknelems(L, nargs+1);
  ___pdr_api_check(L, L->status == ___PDR_LUA_OK, "cannot do calls on non-normal thread");
  ___pdr_checkresults(L, nargs, nresults);
  if (errfunc == 0)
    func = 0;
  else {
    ___pdr_StkId o = index2addr(L, errfunc);
    ___pdr_api_checkstackindex(L, errfunc, o);
    func = ___pdr_savestack(L, o);
  }
  c.func = L->top - (nargs+1);  /* function to be called */
  if (k == NULL || L->nny > 0) {  /* no continuation or no yieldable? */
    c.nresults = nresults;  /* do a 'conventional' protected call */
    status = ___pdr_luaD_pcall(L, f_call, &c, ___pdr_savestack(L, c.func), func);
  }
  else {  /* prepare continuation (call is already protected by 'resume') */
    ___pdr_CallInfo *ci = L->ci;
    ci->u.c.k = k;  /* save continuation */
    ci->u.c.ctx = ctx;  /* save context */
    /* save information for error recovery */
    ci->extra = ___pdr_savestack(L, c.func);
    ci->u.c.old_errfunc = L->errfunc;
    L->errfunc = func;
    ___pdr_setoah(ci->callstatus, L->allowhook);  /* save value of 'allowhook' */
    ci->callstatus |= ___PDR_CIST_YPCALL;  /* function can do error recovery */
    ___pdr_luaD_call(L, c.func, nresults);  /* do the call */
    ci->callstatus &= ~___PDR_CIST_YPCALL;
    L->errfunc = ci->u.c.old_errfunc;
    status = ___PDR_LUA_OK;  /* if it is here, there were no errors */
  }
  ___pdr_adjustresults(L, nresults);
  ___pdr_lua_unlock(L);
  return status;
}


___PDR_LUA_API int lua_load (___pdr_lua_State *L, ___pdr_lua_Reader reader, void *data,
                      const char *chunkname, const char *mode) {
  ___pdr_ZIO z;
  int status;
  ___pdr_lua_lock(L);
  if (!chunkname) chunkname = "?";
  ___pdr_luaZ_init(L, &z, reader, data);
  status = ___pdr_luaD_protectedparser(L, &z, chunkname, mode);
  if (status == ___PDR_LUA_OK) {  /* no errors? */
    ___pdr_LClosure *f = ___pdr_clLvalue(L->top - 1);  /* get newly created function */
    if (f->nupvalues >= 1) {  /* does it have an upvalue? */
      /* get global table from registry */
      ___pdr_Table *reg = ___pdr_hvalue(&___pdr_G(L)->l_registry);
      const ___pdr_TValue *gt = ___pdr_luaH_getint(reg, ___PDR_LUA_RIDX_GLOBALS);
      /* set global table as 1st upvalue of 'f' (may be LUA_ENV) */
      ___pdr_setobj(L, f->upvals[0]->v, gt);
      ___pdr_luaC_upvalbarrier(L, f->upvals[0]);
    }
  }
  ___pdr_lua_unlock(L);
  return status;
}


#ifdef ___PDR_LUAC
___PDR_LUA_API int ___pdr_lua_dump (___pdr_lua_State *L, ___pdr_lua_Writer writer, void *data, int strip) {
  int status;
  ___pdr_TValue *o;
  ___pdr_lua_lock(L);
  ___pdr_api_checknelems(L, 1);
  o = L->top - 1;
  if (___pdr_isLfunction(o))
    status = ___pdr_luaU_dump(L, ___pdr_getproto(o), writer, data, strip);
  else
    status = 1;
  ___pdr_lua_unlock(L);
  return status;
}
#endif // end ___PDR_LUAC


___PDR_LUA_API int ___pdr_lua_status (___pdr_lua_State *L) {
  return L->status;
}


/*
** Garbage-collection function
*/

___PDR_LUA_API int ___pdr_lua_gc (___pdr_lua_State *L, int what, int data) {
  int res = 0;
  ___pdr_global_State *g;
  ___pdr_lua_lock(L);
  g = ___pdr_G(L);
  switch (what) {
    case ___PDR_LUA_GCSTOP: {
      g->gcrunning = 0;
      break;
    }
    case ___PDR_LUA_GCRESTART: {
      ___pdr_luaE_setdebt(g, 0);
      g->gcrunning = 1;
      break;
    }
    case ___PDR_LUA_GCCOLLECT: {
      ___pdr_luaC_fullgc(L, 0);
      break;
    }
    case ___PDR_LUA_GCCOUNT: {
      /* GC values are expressed in Kbytes: #bytes/2^10 */
      res = ___pdr_cast_int(___pdr_gettotalbytes(g) >> 10);
      break;
    }
    case ___PDR_LUA_GCCOUNTB: {
      res = ___pdr_cast_int(___pdr_gettotalbytes(g) & 0x3ff);
      break;
    }
    case ___PDR_LUA_GCSTEP: {
      ___pdr_l_mem debt = 1;  /* =1 to signal that it did an actual step */
      ___pdr_lu_byte oldrunning = g->gcrunning;
      g->gcrunning = 1;  /* allow GC to run */
      if (data == 0) {
        ___pdr_luaE_setdebt(g, -___PDR_GCSTEPSIZE);  /* to do a "small" step */
        ___pdr_luaC_step(L);
      }
      else {  /* add 'data' to total debt */
        debt = ___pdr_cast(___pdr_l_mem, data) * 1024 + g->GCdebt;
        ___pdr_luaE_setdebt(g, debt);
        ___pdr_luaC_checkGC(L);
      }
      g->gcrunning = oldrunning;  /* restore previous state */
      if (debt > 0 && g->gcstate == ___pdr_GCSpause)  /* end of cycle? */
        res = 1;  /* signal it */
      break;
    }
    case ___PDR_LUA_GCSETPAUSE: {
      res = g->gcpause;
      g->gcpause = data;
      break;
    }
    case ___PDR_LUA_GCSETSTEPMUL: {
      res = g->gcstepmul;
      if (data < 40) data = 40;  /* avoid ridiculous low values (and 0) */
      g->gcstepmul = data;
      break;
    }
    case ___PDR_LUA_GCISRUNNING: {
      res = g->gcrunning;
      break;
    }
    default: res = -1;  /* invalid option */
  }
  ___pdr_lua_unlock(L);
  return res;
}



/*
** miscellaneous functions
*/


___PDR_LUA_API int ___pdr_lua_error (___pdr_lua_State *L) {
  ___pdr_lua_lock(L);
  ___pdr_api_checknelems(L, 1);
  ___pdr_luaG_errormsg(L);
  /* code unreachable; will unlock when control actually leaves the kernel */
  return 0;  /* to avoid warnings */
}


___PDR_LUA_API int ___pdr_lua_next (___pdr_lua_State *L, int idx) {
  ___pdr_StkId t;
  int more;
  ___pdr_lua_lock(L);
  t = index2addr(L, idx);
  ___pdr_api_check(L, ___pdr_ttistable(t), "table expected");
  more = ___pdr_luaH_next(L, ___pdr_hvalue(t), L->top - 1);
  if (more) {
    ___pdr_api_incr_top(L);
  }
  else  /* no more elements */
    L->top -= 1;  /* remove key */
  ___pdr_lua_unlock(L);
  return more;
}


___PDR_LUA_API void ___pdr_lua_concat (___pdr_lua_State *L, int n) {
  ___pdr_lua_lock(L);
  ___pdr_api_checknelems(L, n);
  if (n >= 2) {
    ___pdr_luaV_concat(L, n);
  }
  else if (n == 0) {  /* push empty string */
    ___pdr_setsvalue2s(L, L->top, ___pdr_luaS_newlstr(L, "", 0));
    ___pdr_api_incr_top(L);
  }
  /* else n == 1; nothing to do */
  ___pdr_luaC_checkGC(L);
  ___pdr_lua_unlock(L);
}


___PDR_LUA_API void ___pdr_lua_len (___pdr_lua_State *L, int idx) {
  ___pdr_StkId t;
  ___pdr_lua_lock(L);
  t = index2addr(L, idx);
  ___pdr_luaV_objlen(L, L->top, t);
  ___pdr_api_incr_top(L);
  ___pdr_lua_unlock(L);
}


___PDR_LUA_API ___pdr_lua_Alloc ___pdr_lua_getallocf (___pdr_lua_State *L, void **ud) {
  ___pdr_lua_Alloc f;
  ___pdr_lua_lock(L);
  if (ud) *ud = ___pdr_G(L)->ud;
  f = ___pdr_G(L)->frealloc;
  ___pdr_lua_unlock(L);
  return f;
}


___PDR_LUA_API void ___pdr_lua_setallocf (___pdr_lua_State *L, ___pdr_lua_Alloc f, void *ud) {
  ___pdr_lua_lock(L);
  ___pdr_G(L)->ud = ud;
  ___pdr_G(L)->frealloc = f;
  ___pdr_lua_unlock(L);
}


___PDR_LUA_API void *___pdr_lua_newuserdata (___pdr_lua_State *L, size_t size) {
  ___pdr_Udata *u;
  ___pdr_lua_lock(L);
  u = ___pdr_luaS_newudata(L, size);
  ___pdr_setuvalue(L, L->top, u);
  ___pdr_api_incr_top(L);
  ___pdr_luaC_checkGC(L);
  ___pdr_lua_unlock(L);
  return ___pdr_getudatamem(u);
}



static const char *aux_upvalue (___pdr_StkId fi, int n, ___pdr_TValue **val,
                                ___pdr_CClosure **owner, ___pdr_UpVal **uv) {
  switch (___pdr_ttype(fi)) {
    case ___PDR_LUA_TCCL: {  /* C closure */
      ___pdr_CClosure *f = ___pdr_clCvalue(fi);
      if (!(1 <= n && n <= f->nupvalues)) return NULL;
      *val = &f->upvalue[n-1];
      if (owner) *owner = f;
      return "";
    }
    case ___PDR_LUA_TLCL: {  /* Lua closure */
      ___pdr_LClosure *f = ___pdr_clLvalue(fi);
      ___pdr_TString *name;
      ___pdr_Proto *p = f->p;
      if (!(1 <= n && n <= p->sizeupvalues)) return NULL;
      *val = f->upvals[n-1]->v;
      if (uv) *uv = f->upvals[n - 1];
      name = p->upvalues[n-1].name;
      return (name == NULL) ? "(*no name)" : ___pdr_getstr(name);
    }
    default: return NULL;  /* not a closure */
  }
}


___PDR_LUA_API const char *___pdr_lua_getupvalue (___pdr_lua_State *L, int funcindex, int n) {
  const char *name;
  ___pdr_TValue *val = NULL;  /* to avoid warnings */
  ___pdr_lua_lock(L);
  name = aux_upvalue(index2addr(L, funcindex), n, &val, NULL, NULL);
  if (name) {
    ___pdr_setobj2s(L, L->top, val);
    ___pdr_api_incr_top(L);
  }
  ___pdr_lua_unlock(L);
  return name;
}


___PDR_LUA_API const char *___pdr_lua_setupvalue (___pdr_lua_State *L, int funcindex, int n) {
  const char *name;
  ___pdr_TValue *val = NULL;  /* to avoid warnings */
  ___pdr_CClosure *owner = NULL;
  ___pdr_UpVal *uv = NULL;
  ___pdr_StkId fi;
  ___pdr_lua_lock(L);
  fi = index2addr(L, funcindex);
  ___pdr_api_checknelems(L, 1);
  name = aux_upvalue(fi, n, &val, &owner, &uv);
  if (name) {
    L->top--;
    ___pdr_setobj(L, val, L->top);
    if (owner) { ___pdr_luaC_barrier(L, owner, L->top); }
    else if (uv) { ___pdr_luaC_upvalbarrier(L, uv); }
  }
  ___pdr_lua_unlock(L);
  return name;
}


static ___pdr_UpVal **getupvalref (___pdr_lua_State *L, int fidx, int n, ___pdr_LClosure **pf) {
  ___pdr_LClosure *f;
  ___pdr_StkId fi = index2addr(L, fidx);
  ___pdr_api_check(L, ___pdr_ttisLclosure(fi), "Lua function expected");
  f = ___pdr_clLvalue(fi);
  ___pdr_api_check(L, (1 <= n && n <= f->p->sizeupvalues), "invalid upvalue index");
  if (pf) *pf = f;
  return &f->upvals[n - 1];  /* get its upvalue pointer */
}


___PDR_LUA_API void *___pdr_lua_upvalueid (___pdr_lua_State *L, int fidx, int n) {
  ___pdr_StkId fi = index2addr(L, fidx);
  switch (___pdr_ttype(fi)) {
    case ___PDR_LUA_TLCL: {  /* lua closure */
      return *getupvalref(L, fidx, n, NULL);
    }
    case ___PDR_LUA_TCCL: {  /* C closure */
      ___pdr_CClosure *f = ___pdr_clCvalue(fi);
      ___pdr_api_check(L, 1 <= n && n <= f->nupvalues, "invalid upvalue index");
      return &f->upvalue[n - 1];
    }
    default: {
      ___pdr_api_check(L, 0, "closure expected");
      return NULL;
    }
  }
}


___PDR_LUA_API void ___pdr_lua_upvaluejoin (___pdr_lua_State *L, int fidx1, int n1,
                                            int fidx2, int n2) {
  ___pdr_LClosure *f1;
  ___pdr_UpVal **up1 = getupvalref(L, fidx1, n1, &f1);
  ___pdr_UpVal **up2 = getupvalref(L, fidx2, n2, NULL);
  ___pdr_luaC_upvdeccount(L, *up1);
  *up1 = *up2;
  (*up1)->refcount++;
  if (___pdr_upisopen(*up1)) (*up1)->u.open.touched = 1;
  ___pdr_luaC_upvalbarrier(L, *up1);
}

} // end NS_PDR_SLUA
