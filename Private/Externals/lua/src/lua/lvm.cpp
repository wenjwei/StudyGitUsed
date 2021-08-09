/*
** $Id: lvm.c,v 2.268 2016/02/05 19:59:14 roberto Exp $
** Lua virtual machine
** See Copyright Notice in lua.h
*/

#define ___pdr_lvm_c
#define ___PDR_LUA_CORE

#include "lvm.h"
#include "lprefix.h"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "ltm.h"

namespace NS_PDR_SLUA {

/* limit for table tag-method chains (to avoid loops) */
#define ___PDR_MAXTAGLOOP	2000



/*
** 'l_intfitsf' checks whether a given integer can be converted to a
** float without rounding. Used in comparisons. Left undefined if
** all integers fit in a float precisely.
*/
#if !defined(___pdr_l_intfitsf)

/* number of bits in the mantissa of a float */
#define ___PDR_NBM		(___pdr_l_mathlim(MANT_DIG))

/*
** Check whether some integers may not fit in a float, that is, whether
** (maxinteger >> NBM) > 0 (that implies (1 << NBM) <= maxinteger).
** (The shifts are done in parts to avoid shifting by more than the size
** of an integer. In a worst case, NBM == 113 for long double and
** sizeof(integer) == 32.)
*/
#if ((((___PDR_LUA_MAXINTEGER >> (___PDR_NBM / 4)) >> (___PDR_NBM / 4)) >> (___PDR_NBM / 4)) \
	>> (___PDR_NBM - (3 * (___PDR_NBM / 4))))  >  0

#define ___pdr_l_intfitsf(i)  \
  (-((___pdr_lua_Integer)1 << ___PDR_NBM) <= (i) && (i) <= ((___pdr_lua_Integer)1 << ___PDR_NBM))

#endif

#endif



/*
** Try to convert a value to a float. The float case is already handled
** by the macro 'tonumber'.
*/
int ___pdr_luaV_tonumber_ (const ___pdr_TValue *obj, ___pdr_lua_Number *n) {
  ___pdr_TValue v;
  if (___pdr_ttisinteger(obj)) {
    *n = ___pdr_cast_num(___pdr_ivalue(obj));
    return 1;
  }
  else if (___pdr_cvt2num(obj) &&  /* string convertible to number? */
            ___pdr_luaO_str2num(___pdr_svalue(obj), &v) == ___pdr_vslen(obj) + 1) {
    *n = ___pdr_nvalue(&v);  /* convert result of 'luaO_str2num' to a float */
    return 1;
  }
  else
    return 0;  /* conversion failed */
}


/*
** try to convert a value to an integer, rounding according to 'mode':
** mode == 0: accepts only integral values
** mode == 1: takes the floor of the number
** mode == 2: takes the ceil of the number
*/
int ___pdr_luaV_tointeger (const ___pdr_TValue *obj, ___pdr_lua_Integer *p, int mode) {
  ___pdr_TValue v;
 again:
  if (___pdr_ttisfloat(obj)) {
    ___pdr_lua_Number n = ___pdr_fltvalue(obj);
    ___pdr_lua_Number f = ___pdr_l_floor(n);
    if (n != f) {  /* not an integral value? */
      if (mode == 0) return 0;  /* fails if mode demands integral value */
      else if (mode > 1)  /* needs ceil? */
        f += 1;  /* convert floor to ceil (remember: n != f) */
    }
    return ___pdr_lua_numbertointeger(f, p);
  }
  else if (___pdr_ttisinteger(obj)) {
    *p = ___pdr_ivalue(obj);
    return 1;
  }
  else if (___pdr_cvt2num(obj) &&
            ___pdr_luaO_str2num(___pdr_svalue(obj), &v) == ___pdr_vslen(obj) + 1) {
    obj = &v;
    goto again;  /* convert result from 'luaO_str2num' to an integer */
  }
  return 0;  /* conversion failed */
}


/*
** Try to convert a 'for' limit to an integer, preserving the
** semantics of the loop.
** (The following explanation assumes a non-negative step; it is valid
** for negative steps mutatis mutandis.)
** If the limit can be converted to an integer, rounding down, that is
** it.
** Otherwise, check whether the limit can be converted to a number.  If
** the number is too large, it is OK to set the limit as LUA_MAXINTEGER,
** which means no limit.  If the number is too negative, the loop
** should not run, because any initial integer value is larger than the
** limit. So, it sets the limit to LUA_MININTEGER. 'stopnow' corrects
** the extreme case when the initial value is LUA_MININTEGER, in which
** case the LUA_MININTEGER limit would still run the loop once.
*/
static int forlimit (const ___pdr_TValue *obj, ___pdr_lua_Integer *p, ___pdr_lua_Integer step,
                     int *stopnow) {
  *stopnow = 0;  /* usually, let loops run */
  if (!___pdr_luaV_tointeger(obj, p, (step < 0 ? 2 : 1))) {  /* not fit in integer? */
    ___pdr_lua_Number n;  /* try to convert to float */
    if (!___pdr_tonumber(obj, &n)) /* cannot convert to float? */
      return 0;  /* not a number */
    if (___pdr_luai_numlt(0, n)) {  /* if true, float is larger than max integer */
      *p = ___PDR_LUA_MAXINTEGER;
      if (step < 0) *stopnow = 1;
    }
    else {  /* float is smaller than min integer */
      *p = ___PDR_LUA_MININTEGER;
      if (step >= 0) *stopnow = 1;
    }
  }
  return 1;
}


/*
** Finish the table access 'val = t[key]'.
** if 'slot' is NULL, 't' is not a table; otherwise, 'slot' points to
** t[k] entry (which must be nil).
*/
void ___pdr_luaV_finishget (___pdr_lua_State *L, const ___pdr_TValue *t, ___pdr_TValue *key, ___pdr_StkId val,
                      const ___pdr_TValue *slot) {
  int loop;  /* counter to avoid infinite loops */
  const ___pdr_TValue *tm;  /* metamethod */
  for (loop = 0; loop < ___PDR_MAXTAGLOOP; loop++) {
    if (slot == NULL) {  /* 't' is not a table? */
      ___pdr_lua_assert(!___pdr_ttistable(t));
      tm = ___pdr_luaT_gettmbyobj(L, t, PDR_TM_INDEX);
      if (___pdr_ttisnil(tm))
        ___pdr_luaG_typeerror(L, t, "index");  /* no metamethod */
      /* else will try the metamethod */
    }
    else {  /* 't' is a table */
      ___pdr_lua_assert(___pdr_ttisnil(slot));
      tm = ___pdr_fasttm(L, ___pdr_hvalue(t)->metatable, PDR_TM_INDEX);  /* table's metamethod */
      if (tm == NULL) {  /* no metamethod? */
        ___pdr_setnilvalue(val);  /* result is nil */
        return;
      }
      /* else will try the metamethod */
    }
    if (___pdr_ttisfunction(tm)) {  /* is metamethod a function? */
      ___pdr_luaT_callTM(L, tm, t, key, val, 1);  /* call it */
      return;
    }
    t = tm;  /* else try to access 'tm[key]' */
    if (___pdr_luaV_fastget(L,t,key,slot,___pdr_luaH_get)) {  /* fast track? */
      ___pdr_setobj2s(L, val, slot);  /* done */
      return;
    }
    /* else repeat (tail call 'luaV_finishget') */
  }
  ___pdr_luaG_runerror(L, "'__index' chain too long; possible loop");
}


/*
** Finish a table assignment 't[key] = val'.
** If 'slot' is NULL, 't' is not a table.  Otherwise, 'slot' points
** to the entry 't[key]', or to 'luaO_nilobject' if there is no such
** entry.  (The value at 'slot' must be nil, otherwise 'luaV_fastset'
** would have done the job.)
*/
void ___pdr_luaV_finishset (___pdr_lua_State *L, const ___pdr_TValue *t, ___pdr_TValue *key,
                     ___pdr_StkId val, const ___pdr_TValue *slot) {
  int loop;  /* counter to avoid infinite loops */
  for (loop = 0; loop < ___PDR_MAXTAGLOOP; loop++) {
    const ___pdr_TValue *tm;  /* '__newindex' metamethod */
    if (slot != NULL) {  /* is 't' a table? */
      ___pdr_Table *h = ___pdr_hvalue(t);  /* save 't' table */
      ___pdr_lua_assert(___pdr_ttisnil(slot));  /* old value must be nil */
      tm = ___pdr_fasttm(L, h->metatable, PDR_TM_NEWINDEX);  /* get metamethod */
      if (tm == NULL) {  /* no metamethod? */
        if (slot == ___pdr_luaO_nilobject)  /* no previous entry? */
          slot = ___pdr_luaH_newkey(L, h, key);  /* create one */
        /* no metamethod and (now) there is an entry with given key */
        ___pdr_setobj2t(L, ___pdr_cast(___pdr_TValue *, slot), val);  /* set its new value */
        ___pdr_invalidateTMcache(h);
        ___pdr_luaC_barrierback(L, h, val);
        return;
      }
      /* else will try the metamethod */
    }
    else {  /* not a table; check metamethod */
      if (___pdr_ttisnil(tm = ___pdr_luaT_gettmbyobj(L, t, PDR_TM_NEWINDEX)))
        ___pdr_luaG_typeerror(L, t, "index");
    }
    /* try the metamethod */
    if (___pdr_ttisfunction(tm)) {
      ___pdr_luaT_callTM(L, tm, t, key, val, 0);
      return;
    }
    t = tm;  /* else repeat assignment over 'tm' */
    if (___pdr_luaV_fastset(L, t, key, slot, ___pdr_luaH_get, val))
      return;  /* done */
    /* else loop */
  }
  ___pdr_luaG_runerror(L, "'__newindex' chain too long; possible loop");
}


/*
** Compare two strings 'ls' x 'rs', returning an integer smaller-equal-
** -larger than zero if 'ls' is smaller-equal-larger than 'rs'.
** The code is a little tricky because it allows '\0' in the strings
** and it uses 'strcoll' (to respect locales) for each segments
** of the strings.
*/
static int l_strcmp (const ___pdr_TString *ls, const ___pdr_TString *rs) {
  const char *l = ___pdr_getstr(ls);
  size_t ll = ___pdr_tsslen(ls);
  const char *r = ___pdr_getstr(rs);
  size_t lr = ___pdr_tsslen(rs);
  for (;;) {  /* for each segment */
    int temp = strcoll(l, r);
    if (temp != 0)  /* not equal? */
      return temp;  /* done */
    else {  /* strings are equal up to a '\0' */
      size_t len = strlen(l);  /* index of first '\0' in both strings */
      if (len == lr)  /* 'rs' is finished? */
        return (len == ll) ? 0 : 1;  /* check 'ls' */
      else if (len == ll)  /* 'ls' is finished? */
        return -1;  /* 'ls' is smaller than 'rs' ('rs' is not finished) */
      /* both strings longer than 'len'; go on comparing after the '\0' */
      len++;
      l += len; ll -= len; r += len; lr -= len;
    }
  }
}


/*
** Check whether integer 'i' is less than float 'f'. If 'i' has an
** exact representation as a float ('l_intfitsf'), compare numbers as
** floats. Otherwise, if 'f' is outside the range for integers, result
** is trivial. Otherwise, compare them as integers. (When 'i' has no
** float representation, either 'f' is "far away" from 'i' or 'f' has
** no precision left for a fractional part; either way, how 'f' is
** truncated is irrelevant.) When 'f' is NaN, comparisons must result
** in false.
*/
static int LTintfloat (___pdr_lua_Integer i, ___pdr_lua_Number f) {
#if defined(___pdr_l_intfitsf)
  if (!___pdr_l_intfitsf(i)) {
    if (f >= -___pdr_cast_num(___PDR_LUA_MININTEGER))  /* -minint == maxint + 1 */
      return 1;  /* f >= maxint + 1 > i */
    else if (f > ___pdr_cast_num(___PDR_LUA_MININTEGER))  /* minint < f <= maxint ? */
      return (i < ___pdr_cast(___pdr_lua_Integer, f));  /* compare them as integers */
    else  /* f <= minint <= i (or 'f' is NaN)  -->  not(i < f) */
      return 0;
  }
#endif
  return ___pdr_luai_numlt(___pdr_cast_num(i), f);  /* compare them as floats */
}


/*
** Check whether integer 'i' is less than or equal to float 'f'.
** See comments on previous function.
*/
static int LEintfloat (___pdr_lua_Integer i, ___pdr_lua_Number f) {
#if defined(___pdr_l_intfitsf)
  if (!___pdr_l_intfitsf(i)) {
    if (f >= -___pdr_cast_num(___PDR_LUA_MININTEGER))  /* -minint == maxint + 1 */
      return 1;  /* f >= maxint + 1 > i */
    else if (f >= ___pdr_cast_num(___PDR_LUA_MININTEGER))  /* minint <= f <= maxint ? */
      return (i <= ___pdr_cast(___pdr_lua_Integer, f));  /* compare them as integers */
    else  /* f < minint <= i (or 'f' is NaN)  -->  not(i <= f) */
      return 0;
  }
#endif
  return ___pdr_luai_numle(___pdr_cast_num(i), f);  /* compare them as floats */
}


/*
** Return 'l < r', for numbers.
*/
static int LTnum (const ___pdr_TValue *l, const ___pdr_TValue *r) {
  if (___pdr_ttisinteger(l)) {
    ___pdr_lua_Integer li = ___pdr_ivalue(l);
    if (___pdr_ttisinteger(r))
      return li < ___pdr_ivalue(r);  /* both are integers */
    else  /* 'l' is int and 'r' is float */
      return LTintfloat(li, ___pdr_fltvalue(r));  /* l < r ? */
  }
  else {
    ___pdr_lua_Number lf = ___pdr_fltvalue(l);  /* 'l' must be float */
    if (___pdr_ttisfloat(r))
      return ___pdr_luai_numlt(lf, ___pdr_fltvalue(r));  /* both are float */
    else if (___pdr_luai_numisnan(lf))  /* 'r' is int and 'l' is float */
      return 0;  /* NaN < i is always false */
    else  /* without NaN, (l < r)  <-->  not(r <= l) */
      return !LEintfloat(___pdr_ivalue(r), lf);  /* not (r <= l) ? */
  }
}


/*
** Return 'l <= r', for numbers.
*/
static int LEnum (const ___pdr_TValue *l, const ___pdr_TValue *r) {
  if (___pdr_ttisinteger(l)) {
    ___pdr_lua_Integer li = ___pdr_ivalue(l);
    if (___pdr_ttisinteger(r))
      return li <= ___pdr_ivalue(r);  /* both are integers */
    else  /* 'l' is int and 'r' is float */
      return LEintfloat(li, ___pdr_fltvalue(r));  /* l <= r ? */
  }
  else {
    ___pdr_lua_Number lf = ___pdr_fltvalue(l);  /* 'l' must be float */
    if (___pdr_ttisfloat(r))
      return ___pdr_luai_numle(lf, ___pdr_fltvalue(r));  /* both are float */
    else if (___pdr_luai_numisnan(lf))  /* 'r' is int and 'l' is float */
      return 0;  /*  NaN <= i is always false */
    else  /* without NaN, (l <= r)  <-->  not(r < l) */
      return !LTintfloat(___pdr_ivalue(r), lf);  /* not (r < l) ? */
  }
}


/*
** Main operation less than; return 'l < r'.
*/
int ___pdr_luaV_lessthan (___pdr_lua_State *L, const ___pdr_TValue *l, const ___pdr_TValue *r) {
  int res;
  if (___pdr_ttisnumber(l) && ___pdr_ttisnumber(r))  /* both operands are numbers? */
    return LTnum(l, r);
  else if (___pdr_ttisstring(l) && ___pdr_ttisstring(r))  /* both are strings? */
    return l_strcmp(___pdr_tsvalue(l), ___pdr_tsvalue(r)) < 0;
  else if ((res = ___pdr_luaT_callorderTM(L, l, r, PDR_TM_LT)) < 0)  /* no metamethod? */
    ___pdr_luaG_ordererror(L, l, r);  /* error */
  return res;
}


/*
** Main operation less than or equal to; return 'l <= r'. If it needs
** a metamethod and there is no '__le', try '__lt', based on
** l <= r iff !(r < l) (assuming a total order). If the metamethod
** yields during this substitution, the continuation has to know
** about it (to negate the result of r<l); bit CIST_LEQ in the call
** status keeps that information.
*/
int ___pdr_luaV_lessequal (___pdr_lua_State *L, const ___pdr_TValue *l, const ___pdr_TValue *r) {
  int res;
  if (___pdr_ttisnumber(l) && ___pdr_ttisnumber(r))  /* both operands are numbers? */
    return LEnum(l, r);
  else if (___pdr_ttisstring(l) && ___pdr_ttisstring(r))  /* both are strings? */
    return l_strcmp(___pdr_tsvalue(l), ___pdr_tsvalue(r)) <= 0;
  else if ((res = ___pdr_luaT_callorderTM(L, l, r, PDR_TM_LE)) >= 0)  /* try 'le' */
    return res;
  else {  /* try 'lt': */
    L->ci->callstatus |= ___PDR_CIST_LEQ;  /* mark it is doing 'lt' for 'le' */
    res = ___pdr_luaT_callorderTM(L, r, l, PDR_TM_LT);
    L->ci->callstatus ^= ___PDR_CIST_LEQ;  /* clear mark */
    if (res < 0)
      ___pdr_luaG_ordererror(L, l, r);
    return !res;  /* result is negated */
  }
}


/*
** Main operation for equality of Lua values; return 't1 == t2'.
** L == NULL means raw equality (no metamethods)
*/
int ___pdr_luaV_equalobj (___pdr_lua_State *L, const ___pdr_TValue *t1, const ___pdr_TValue *t2) {
  const ___pdr_TValue *tm;
  if (___pdr_ttype(t1) != ___pdr_ttype(t2)) {  /* not the same variant? */
    if (___pdr_ttnov(t1) != ___pdr_ttnov(t2) || ___pdr_ttnov(t1) != ___PDR_LUA_TNUMBER)
      return 0;  /* only numbers can be equal with different variants */
    else {  /* two numbers with different variants */
      ___pdr_lua_Integer i1, i2;  /* compare them as integers */
      return (___pdr_tointeger(t1, &i1) && ___pdr_tointeger(t2, &i2) && i1 == i2);
    }
  }
  /* values have same type and same variant */
  switch (___pdr_ttype(t1)) {
    case ___PDR_LUA_TNIL: return 1;
    case ___PDR_LUA_TNUMINT: return (___pdr_ivalue(t1) == ___pdr_ivalue(t2));
    case ___PDR_LUA_TNUMFLT: return ___pdr_luai_numeq(___pdr_fltvalue(t1), ___pdr_fltvalue(t2));
    case ___PDR_LUA_TBOOLEAN: return ___pdr_bvalue(t1) == ___pdr_bvalue(t2);  /* true must be 1 !! */
    case ___PDR_LUA_TLIGHTUSERDATA: return ___pdr_pvalue(t1) == ___pdr_pvalue(t2);
    case ___PDR_LUA_TLCF: return ___pdr_fvalue(t1) == ___pdr_fvalue(t2);
    case ___PDR_LUA_TSHRSTR: return ___pdr_eqshrstr(___pdr_tsvalue(t1), ___pdr_tsvalue(t2));
    case ___PDR_LUA_TLNGSTR: return ___pdr_luaS_eqlngstr(___pdr_tsvalue(t1), ___pdr_tsvalue(t2));
    case ___PDR_LUA_TUSERDATA: {
      if (___pdr_uvalue(t1) == ___pdr_uvalue(t2)) return 1;
      else if (L == NULL) return 0;
      tm = ___pdr_fasttm(L, ___pdr_uvalue(t1)->metatable, PDR_TM_EQ);
      if (tm == NULL)
        tm = ___pdr_fasttm(L, ___pdr_uvalue(t2)->metatable, PDR_TM_EQ);
      break;  /* will try TM */
    }
    case ___PDR_LUA_TTABLE: {
      if (___pdr_hvalue(t1) == ___pdr_hvalue(t2)) return 1;
      else if (L == NULL) return 0;
      tm = ___pdr_fasttm(L, ___pdr_hvalue(t1)->metatable, PDR_TM_EQ);
      if (tm == NULL)
        tm = ___pdr_fasttm(L, ___pdr_hvalue(t2)->metatable, PDR_TM_EQ);
      break;  /* will try TM */
    }
    default:
      return ___pdr_gcvalue(t1) == ___pdr_gcvalue(t2);
  }
  if (tm == NULL)  /* no TM? */
    return 0;  /* objects are different */
  ___pdr_luaT_callTM(L, tm, t1, t2, L->top, 1);  /* call TM */
  return !___pdr_l_isfalse(L->top);
}


/* macro used by 'luaV_concat' to ensure that element at 'o' is a string */
#define ___pdr_tostring(L,o)  \
	(___pdr_ttisstring(o) || (___pdr_cvt2str(o) && (___pdr_luaO_tostring(L, o), 1)))

#define ___pdr_isemptystr(o)	(___pdr_ttisshrstring(o) && ___pdr_tsvalue(o)->shrlen == 0)

/* copy strings in stack from top - n up to top - 1 to buffer */
static void copy2buff (___pdr_StkId top, int n, char *buff) {
  size_t tl = 0;  /* size already copied */
  do {
    size_t l = ___pdr_vslen(top - n);  /* length of string being copied */
    memcpy(buff + tl, ___pdr_svalue(top - n), l * sizeof(char));
    tl += l;
  } while (--n > 0);
}


/*
** Main operation for concatenation: concat 'total' values in the stack,
** from 'L->top - total' up to 'L->top - 1'.
*/
void ___pdr_luaV_concat (___pdr_lua_State *L, int total) {
  ___pdr_lua_assert(total >= 2);
  do {
    ___pdr_StkId top = L->top;
    int n = 2;  /* number of elements handled in this pass (at least 2) */
    if (!(___pdr_ttisstring(top-2) || ___pdr_cvt2str(top-2)) || !___pdr_tostring(L, top-1))
      ___pdr_luaT_trybinTM(L, top-2, top-1, top-2, PDR_TM_CONCAT);
    else if (___pdr_isemptystr(top - 1))  /* second operand is empty? */
      ___pdr_cast_void(___pdr_tostring(L, top - 2));  /* result is first operand */
    else if (___pdr_isemptystr(top - 2)) {  /* first operand is an empty string? */
      ___pdr_setobjs2s(L, top - 2, top - 1);  /* result is second op. */
    }
    else {
      /* at least two non-empty string values; get as many as possible */
      size_t tl = ___pdr_vslen(top - 1);
      ___pdr_TString *ts;
      /* collect total length and number of strings */
      for (n = 1; n < total && ___pdr_tostring(L, top - n - 1); n++) {
        size_t l = ___pdr_vslen(top - n - 1);
        if (l >= (___PDR_MAX_SIZE/sizeof(char)) - tl)
          ___pdr_luaG_runerror(L, "string length overflow");
        tl += l;
      }
      if (tl <= ___PDR_LUAI_MAXSHORTLEN) {  /* is result a short string? */
        char buff[___PDR_LUAI_MAXSHORTLEN];
        copy2buff(top, n, buff);  /* copy strings to buffer */
        ts = ___pdr_luaS_newlstr(L, buff, tl);
      }
      else {  /* long string; copy strings directly to final result */
        ts = ___pdr_luaS_createlngstrobj(L, tl);
        copy2buff(top, n, ___pdr_getstr(ts));
      }
      ___pdr_setsvalue2s(L, top - n, ts);  /* create result */
    }
    total -= n-1;  /* got 'n' strings to create 1 new */
    L->top -= n-1;  /* popped 'n' strings and pushed one */
  } while (total > 1);  /* repeat until only 1 result left */
}


/*
** Main operation 'ra' = #rb'.
*/
void ___pdr_luaV_objlen (___pdr_lua_State *L, ___pdr_StkId ra, const ___pdr_TValue *rb) {
  const ___pdr_TValue *tm;
  switch (___pdr_ttype(rb)) {
    case ___PDR_LUA_TTABLE: {
      ___pdr_Table *h = ___pdr_hvalue(rb);
      tm = ___pdr_fasttm(L, h->metatable, PDR_TM_LEN);
      if (tm) break;  /* metamethod? break switch to call it */
      ___pdr_setivalue(ra, ___pdr_luaH_getn(h));  /* else primitive len */
      return;
    }
    case ___PDR_LUA_TSHRSTR: {
      ___pdr_setivalue(ra, ___pdr_tsvalue(rb)->shrlen);
      return;
    }
    case ___PDR_LUA_TLNGSTR: {
      ___pdr_setivalue(ra, ___pdr_tsvalue(rb)->u.lnglen);
      return;
    }
    default: {  /* try metamethod */
      tm = ___pdr_luaT_gettmbyobj(L, rb, PDR_TM_LEN);
      if (___pdr_ttisnil(tm))  /* no metamethod? */
        ___pdr_luaG_typeerror(L, rb, "get length of");
      break;
    }
  }
  ___pdr_luaT_callTM(L, tm, rb, rb, ra, 1);
}


/*
** Integer division; return 'm // n', that is, floor(m/n).
** C division truncates its result (rounds towards zero).
** 'floor(q) == trunc(q)' when 'q >= 0' or when 'q' is integer,
** otherwise 'floor(q) == trunc(q) - 1'.
*/
___pdr_lua_Integer ___pdr_luaV_div (___pdr_lua_State *L, ___pdr_lua_Integer m, ___pdr_lua_Integer n) {
  if (___pdr_l_castS2U(n) + 1u <= 1u) {  /* special cases: -1 or 0 */
    if (n == 0)
      ___pdr_luaG_runerror(L, "attempt to divide by zero");
    return ___pdr_intop(-, 0, m);   /* n==-1; avoid overflow with 0x80000...//-1 */
  }
  else {
    ___pdr_lua_Integer q = m / n;  /* perform C division */
    if ((m ^ n) < 0 && m % n != 0)  /* 'm/n' would be negative non-integer? */
      q -= 1;  /* correct result for different rounding */
    return q;
  }
}


/*
** Integer modulus; return 'm % n'. (Assume that C '%' with
** negative operands follows C99 behavior. See previous comment
** about luaV_div.)
*/
___pdr_lua_Integer ___pdr_luaV_mod (___pdr_lua_State *L, ___pdr_lua_Integer m, ___pdr_lua_Integer n) {
  if (___pdr_l_castS2U(n) + 1u <= 1u) {  /* special cases: -1 or 0 */
    if (n == 0)
      ___pdr_luaG_runerror(L, "attempt to perform 'n%%0'");
    return 0;   /* m % -1 == 0; avoid overflow with 0x80000...%-1 */
  }
  else {
    ___pdr_lua_Integer r = m % n;
    if (r != 0 && (m ^ n) < 0)  /* 'm/n' would be non-integer negative? */
      r += n;  /* correct result for different rounding */
    return r;
  }
}


/* number of bits in an integer */
#define ___PDR_NBITS	___pdr_cast_int(sizeof(___pdr_lua_Integer) * CHAR_BIT)

/*
** Shift left operation. (Shift right just negates 'y'.)
*/
___pdr_lua_Integer ___pdr_luaV_shiftl (___pdr_lua_Integer x, ___pdr_lua_Integer y) {
  if (y < 0) {  /* shift right? */
    if (y <= -___PDR_NBITS) return 0;
    else return ___pdr_intop(>>, x, -y);
  }
  else {  /* shift left */
    if (y >= ___PDR_NBITS) return 0;
    else return ___pdr_intop(<<, x, y);
  }
}


/*
** check whether cached closure in prototype 'p' may be reused, that is,
** whether there is a cached closure with the same upvalues needed by
** new closure to be created.
*/
static ___pdr_LClosure *getcached (___pdr_Proto *p, ___pdr_UpVal **encup, ___pdr_StkId base) {
  ___pdr_LClosure *c = p->cache;
  if (c != NULL) {  /* is there a cached closure? */
    int nup = p->sizeupvalues;
    ___pdr_Upvaldesc *uv = p->upvalues;
    int i;
    for (i = 0; i < nup; i++) {  /* check whether it has right upvalues */
      ___pdr_TValue *v = uv[i].instack ? base + uv[i].idx : encup[uv[i].idx]->v;
      if (c->upvals[i]->v != v)
        return NULL;  /* wrong upvalue; cannot reuse closure */
    }
  }
  return c;  /* return cached closure (or NULL if no cached closure) */
}


/*
** create a new Lua closure, push it in the stack, and initialize
** its upvalues. Note that the closure is not cached if prototype is
** already black (which means that 'cache' was already cleared by the
** GC).
*/
static void pushclosure (___pdr_lua_State *L, ___pdr_Proto *p, ___pdr_UpVal **encup, ___pdr_StkId base,
                         ___pdr_StkId ra) {
  int nup = p->sizeupvalues;
  ___pdr_Upvaldesc *uv = p->upvalues;
  int i;
  ___pdr_LClosure *ncl = ___pdr_luaF_newLclosure(L, nup);
  ncl->p = p;
  ___pdr_setclLvalue(L, ra, ncl);  /* anchor new closure in stack */
  for (i = 0; i < nup; i++) {  /* fill in its upvalues */
    if (uv[i].instack)  /* upvalue refers to local variable? */
      ncl->upvals[i] = ___pdr_luaF_findupval(L, base + uv[i].idx);
    else  /* get upvalue from enclosing function */
      ncl->upvals[i] = encup[uv[i].idx];
    ncl->upvals[i]->refcount++;
    /* new closure is white, so we do not need a barrier here */
  }
  if (!___pdr_isblack(p))  /* cache will not break GC invariant? */
    p->cache = ncl;  /* save it on cache for reuse */
}


/*
** finish execution of an opcode interrupted by an yield
*/
void ___pdr_luaV_finishOp (___pdr_lua_State *L) {
  ___pdr_CallInfo *ci = L->ci;
  ___pdr_StkId base = ci->u.l.base;
  ___pdr_Instruction inst = *(ci->u.l.savedpc - 1);  /* interrupted instruction */
  ___pdr_OpCode op = ___PDR_GET_OPCODE(inst);
  switch (op) {  /* finish its execution */
    case PDR_OP_ADD: case PDR_OP_SUB: case PDR_OP_MUL: case PDR_OP_DIV: case PDR_OP_IDIV:
    case PDR_OP_BAND: case PDR_OP_BOR: case PDR_OP_BXOR: case PDR_OP_SHL: case PDR_OP_SHR:
    case PDR_OP_MOD: case PDR_OP_POW:
    case PDR_OP_UNM: case PDR_OP_BNOT: case PDR_OP_LEN:
    case PDR_OP_GETTABUP: case PDR_OP_GETTABLE: case PDR_OP_SELF: {
      ___pdr_setobjs2s(L, base + ___PDR_GETARG_A(inst), --L->top);
      break;
    }
    case PDR_OP_LE: case PDR_OP_LT: case PDR_OP_EQ: {
      int res = !___pdr_l_isfalse(L->top - 1);
      L->top--;
      if (ci->callstatus & ___PDR_CIST_LEQ) {  /* "<=" using "<" instead? */
        ___pdr_lua_assert(op == PDR_OP_LE);
        ci->callstatus ^= ___PDR_CIST_LEQ;  /* clear mark */
        res = !res;  /* negate result */
      }
      ___pdr_lua_assert(___PDR_GET_OPCODE(*ci->u.l.savedpc) == PDR_OP_JMP);
      if (res != ___PDR_GETARG_A(inst))  /* condition failed? */
        ci->u.l.savedpc++;  /* skip jump instruction */
      break;
    }
    case PDR_OP_CONCAT: {
      ___pdr_StkId top = L->top - 1;  /* top when 'luaT_trybinTM' was called */
      int b = ___PDR_GETARG_B(inst);      /* first element to concatenate */
      int total = ___pdr_cast_int(top - 1 - (base + b));  /* yet to concatenate */
      ___pdr_setobj2s(L, top - 2, top);  /* put TM result in proper position */
      if (total > 1) {  /* are there elements to concat? */
        L->top = top - 1;  /* top is one after last element (at top-2) */
        ___pdr_luaV_concat(L, total);  /* concat them (may yield again) */
      }
      /* move final result to final position */
      ___pdr_setobj2s(L, ci->u.l.base + ___PDR_GETARG_A(inst), L->top - 1);
      L->top = ci->top;  /* restore top */
      break;
    }
    case PDR_OP_TFORCALL: {
      ___pdr_lua_assert(___PDR_GET_OPCODE(*ci->u.l.savedpc) == PDR_OP_TFORLOOP);
      L->top = ci->top;  /* correct top */
      break;
    }
    case PDR_OP_CALL: {
      if (___PDR_GETARG_C(inst) - 1 >= 0)  /* nresults >= 0? */
        L->top = ci->top;  /* adjust results */
      break;
    }
    case PDR_OP_TAILCALL: case PDR_OP_SETTABUP: case PDR_OP_SETTABLE:
      break;
    default: ___pdr_lua_assert(0);
  }
}




/*
** {==================================================================
** Function 'luaV_execute': main interpreter loop
** ===================================================================
*/


/*
** some macros for common tasks in 'luaV_execute'
*/


#define ___PDR_RA(i)	(base+___PDR_GETARG_A(i))
#define ___PDR_RB(i)	___pdr_check_exp(___pdr_getBMode(___PDR_GET_OPCODE(i)) == PDR_OpArgR, base+___PDR_GETARG_B(i))
#define ___PDR_RC(i)	___pdr_check_exp(___pdr_getCMode(___PDR_GET_OPCODE(i)) == PDR_OpArgR, base+___PDR_GETARG_C(i))
#define ___PDR_RKB(i)	___pdr_check_exp(___pdr_getBMode(___PDR_GET_OPCODE(i)) == PDR_OpArgK, \
	___PDR_ISK(___PDR_GETARG_B(i)) ? k+___PDR_INDEXK(___PDR_GETARG_B(i)) : base+___PDR_GETARG_B(i))
#define ___PDR_RKC(i)	___pdr_check_exp(___pdr_getCMode(___PDR_GET_OPCODE(i)) == PDR_OpArgK, \
	___PDR_ISK(___PDR_GETARG_C(i)) ? k+___PDR_INDEXK(___PDR_GETARG_C(i)) : base+___PDR_GETARG_C(i))


/* execute a jump instruction */
#define ___pdr_dojump(ci,i,e) \
  { int a = ___PDR_GETARG_A(i); \
    if (a != 0) ___pdr_luaF_close(L, ci->u.l.base + a - 1); \
    ci->u.l.savedpc += ___PDR_GETARG_sBx(i) + e; }

/* for test instructions, execute the jump instruction that follows it */
#define ___pdr_donextjump(ci)	{ i = *ci->u.l.savedpc; ___pdr_dojump(ci, i, 1); }


#define ___pdr_Protect(x)	{ {x;}; base = ci->u.l.base; }

#define ___pdr_checkGC(L,c)  \
	{ ___pdr_luaC_condGC(L, L->top = (c),  /* limit of live values */ \
                         ___pdr_Protect(L->top = ci->top));  /* restore top */ \
           ___pdr_luai_threadyield(L); }


/* fetch an instruction and prepare its execution */
#define ___pdr_vmfetch()	{ \
  i = *(ci->u.l.savedpc++); \
  if (L->hookmask & (___PDR_LUA_MASKLINE | ___PDR_LUA_MASKCOUNT)) \
    ___pdr_Protect(___pdr_luaG_traceexec(L)); \
  ra = ___PDR_RA(i); /* WARNING: any stack reallocation invalidates 'ra' */ \
  ___pdr_lua_assert(base == ci->u.l.base); \
  ___pdr_lua_assert(base <= L->top && L->top < L->stack + L->stacksize); \
}

#define ___pdr_vmdispatch(o)	switch(o)
#define ___pdr_vmcase(l)	case l:
#define ___pdr_vmbreak		break


/*
** copy of 'luaV_gettable', but protecting the call to potential
** metamethod (which can reallocate the stack)
*/
#define ___pdr_gettableProtected(L,t,k,v)  { const ___pdr_TValue *slot; \
  if (___pdr_luaV_fastget(L,t,k,slot,___pdr_luaH_get)) { ___pdr_setobj2s(L, v, slot); } \
  else ___pdr_Protect(___pdr_luaV_finishget(L,t,k,v,slot)); }


/* same for 'luaV_settable' */
#define ___pdr_settableProtected(L,t,k,v) { const ___pdr_TValue *slot; \
  if (!___pdr_luaV_fastset(L,t,k,slot,___pdr_luaH_get,v)) \
    ___pdr_Protect(___pdr_luaV_finishset(L,t,k,v,slot)); }



void ___pdr_luaV_execute (___pdr_lua_State *L) {
  ___pdr_CallInfo *ci = L->ci;
  ___pdr_LClosure *cl;
  ___pdr_TValue *k;
  ___pdr_StkId base;
  ci->callstatus |= ___PDR_CIST_FRESH;  /* fresh invocation of 'luaV_execute" */
 newframe:  /* reentry point when frame changes (call/return) */
  ___pdr_lua_assert(ci == L->ci);
  cl = ___pdr_clLvalue(ci->func);  /* local reference to function's closure */
  k = cl->p->k;  /* local reference to function's constant table */
  base = ci->u.l.base;  /* local copy of function's base */
  /* main loop of interpreter */
  for (;;) {
    ___pdr_Instruction i;
    ___pdr_StkId ra;
    ___pdr_vmfetch();
    ___pdr_vmdispatch (___PDR_GET_OPCODE(i)) {
      ___pdr_vmcase(PDR_OP_MOVE) {
        ___pdr_setobjs2s(L, ra, ___PDR_RB(i));
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_LOADK) {
        ___pdr_TValue *rb = k + ___PDR_GETARG_Bx(i);
        ___pdr_setobj2s(L, ra, rb);
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_LOADKX) {
        ___pdr_TValue *rb;
        ___pdr_lua_assert(___PDR_GET_OPCODE(*ci->u.l.savedpc) == PDR_OP_EXTRAARG);
        rb = k + ___PDR_GETARG_Ax(*ci->u.l.savedpc++);
        ___pdr_setobj2s(L, ra, rb);
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_LOADBOOL) {
        ___pdr_setbvalue(ra, ___PDR_GETARG_B(i));
        if (___PDR_GETARG_C(i)) ci->u.l.savedpc++;  /* skip next instruction (if C) */
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_LOADNIL) {
        int b = ___PDR_GETARG_B(i);
        do {
          ___pdr_setnilvalue(ra++);
        } while (b--);
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_GETUPVAL) {
        int b = ___PDR_GETARG_B(i);
        ___pdr_setobj2s(L, ra, cl->upvals[b]->v);
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_GETTABUP) {
        ___pdr_TValue *upval = cl->upvals[___PDR_GETARG_B(i)]->v;
        ___pdr_TValue *rc = ___PDR_RKC(i);
        ___pdr_gettableProtected(L, upval, rc, ra);
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_GETTABLE) {
        ___pdr_StkId rb = ___PDR_RB(i);
        ___pdr_TValue *rc = ___PDR_RKC(i);
        ___pdr_gettableProtected(L, rb, rc, ra);
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_SETTABUP) {
        ___pdr_TValue *upval = cl->upvals[___PDR_GETARG_A(i)]->v;
        ___pdr_TValue *rb = ___PDR_RKB(i);
        ___pdr_TValue *rc = ___PDR_RKC(i);
        ___pdr_settableProtected(L, upval, rb, rc);
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_SETUPVAL) {
        ___pdr_UpVal *uv = cl->upvals[___PDR_GETARG_B(i)];
        ___pdr_setobj(L, uv->v, ra);
        ___pdr_luaC_upvalbarrier(L, uv);
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_SETTABLE) {
        ___pdr_TValue *rb = ___PDR_RKB(i);
        ___pdr_TValue *rc = ___PDR_RKC(i);
        ___pdr_settableProtected(L, ra, rb, rc);
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_NEWTABLE) {
        int b = ___PDR_GETARG_B(i);
        int c = ___PDR_GETARG_C(i);
        ___pdr_Table *t = ___pdr_luaH_new(L);
        ___pdr_sethvalue(L, ra, t);
        if (b != 0 || c != 0)
          ___pdr_luaH_resize(L, t, ___pdr_luaO_fb2int(b), ___pdr_luaO_fb2int(c));
        ___pdr_checkGC(L, ra + 1);
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_SELF) {
        const ___pdr_TValue *aux;
        ___pdr_StkId rb = ___PDR_RB(i);
        ___pdr_TValue *rc = ___PDR_RKC(i);
        ___pdr_TString *key = ___pdr_tsvalue(rc);  /* key must be a string */
        ___pdr_setobjs2s(L, ra + 1, rb);
        if (___pdr_luaV_fastget(L, rb, key, aux, ___pdr_luaH_getstr)) {
          ___pdr_setobj2s(L, ra, aux);
        }
        else ___pdr_Protect(___pdr_luaV_finishget(L, rb, rc, ra, aux));
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_ADD) {
        ___pdr_TValue *rb = ___PDR_RKB(i);
        ___pdr_TValue *rc = ___PDR_RKC(i);
        ___pdr_lua_Number nb; ___pdr_lua_Number nc;
        if (___pdr_ttisinteger(rb) && ___pdr_ttisinteger(rc)) {
          ___pdr_lua_Integer ib = ___pdr_ivalue(rb); ___pdr_lua_Integer ic = ___pdr_ivalue(rc);
          ___pdr_setivalue(ra, ___pdr_intop(+, ib, ic));
        }
        else if (___pdr_tonumber(rb, &nb) && ___pdr_tonumber(rc, &nc)) {
          ___pdr_setfltvalue(ra, ___pdr_luai_numadd(L, nb, nc));
        }
        else { ___pdr_Protect(___pdr_luaT_trybinTM(L, rb, rc, ra, PDR_TM_ADD)); }
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_SUB) {
        ___pdr_TValue *rb = ___PDR_RKB(i);
        ___pdr_TValue *rc = ___PDR_RKC(i);
        ___pdr_lua_Number nb; ___pdr_lua_Number nc;
        if (___pdr_ttisinteger(rb) && ___pdr_ttisinteger(rc)) {
          ___pdr_lua_Integer ib = ___pdr_ivalue(rb); ___pdr_lua_Integer ic = ___pdr_ivalue(rc);
          ___pdr_setivalue(ra, ___pdr_intop(-, ib, ic));
        }
        else if (___pdr_tonumber(rb, &nb) && ___pdr_tonumber(rc, &nc)) {
          ___pdr_setfltvalue(ra, ___pdr_luai_numsub(L, nb, nc));
        }
        else { ___pdr_Protect(___pdr_luaT_trybinTM(L, rb, rc, ra, PDR_TM_SUB)); }
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_MUL) {
        ___pdr_TValue *rb = ___PDR_RKB(i);
        ___pdr_TValue *rc = ___PDR_RKC(i);
        ___pdr_lua_Number nb; ___pdr_lua_Number nc;
        if (___pdr_ttisinteger(rb) && ___pdr_ttisinteger(rc)) {
          ___pdr_lua_Integer ib = ___pdr_ivalue(rb); ___pdr_lua_Integer ic = ___pdr_ivalue(rc);
          ___pdr_setivalue(ra, ___pdr_intop(*, ib, ic));
        }
        else if (___pdr_tonumber(rb, &nb) && ___pdr_tonumber(rc, &nc)) {
          ___pdr_setfltvalue(ra, ___pdr_luai_nummul(L, nb, nc));
        }
        else { ___pdr_Protect(___pdr_luaT_trybinTM(L, rb, rc, ra, PDR_TM_MUL)); }
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_DIV) {  /* float division (always with floats) */
        ___pdr_TValue *rb = ___PDR_RKB(i);
        ___pdr_TValue *rc = ___PDR_RKC(i);
        ___pdr_lua_Number nb; ___pdr_lua_Number nc;
        if (___pdr_tonumber(rb, &nb) && ___pdr_tonumber(rc, &nc)) {
          ___pdr_setfltvalue(ra, ___pdr_luai_numdiv(L, nb, nc));
        }
        else { ___pdr_Protect(___pdr_luaT_trybinTM(L, rb, rc, ra, PDR_TM_DIV)); }
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_BAND) {
        ___pdr_TValue *rb = ___PDR_RKB(i);
        ___pdr_TValue *rc = ___PDR_RKC(i);
        ___pdr_lua_Integer ib; ___pdr_lua_Integer ic;
        if (___pdr_tointeger(rb, &ib) && ___pdr_tointeger(rc, &ic)) {
          ___pdr_setivalue(ra, ___pdr_intop(&, ib, ic));
        }
        else { ___pdr_Protect(___pdr_luaT_trybinTM(L, rb, rc, ra, PDR_TM_BAND)); }
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_BOR) {
        ___pdr_TValue *rb = ___PDR_RKB(i);
        ___pdr_TValue *rc = ___PDR_RKC(i);
        ___pdr_lua_Integer ib; ___pdr_lua_Integer ic;
        if (___pdr_tointeger(rb, &ib) && ___pdr_tointeger(rc, &ic)) {
          ___pdr_setivalue(ra, ___pdr_intop(|, ib, ic));
        }
        else { ___pdr_Protect(___pdr_luaT_trybinTM(L, rb, rc, ra, PDR_TM_BOR)); }
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_BXOR) {
        ___pdr_TValue *rb = ___PDR_RKB(i);
        ___pdr_TValue *rc = ___PDR_RKC(i);
        ___pdr_lua_Integer ib; ___pdr_lua_Integer ic;
        if (___pdr_tointeger(rb, &ib) && ___pdr_tointeger(rc, &ic)) {
          ___pdr_setivalue(ra, ___pdr_intop(^, ib, ic));
        }
        else { ___pdr_Protect(___pdr_luaT_trybinTM(L, rb, rc, ra, PDR_TM_BXOR)); }
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_SHL) {
        ___pdr_TValue *rb = ___PDR_RKB(i);
        ___pdr_TValue *rc = ___PDR_RKC(i);
        ___pdr_lua_Integer ib; ___pdr_lua_Integer ic;
        if (___pdr_tointeger(rb, &ib) && ___pdr_tointeger(rc, &ic)) {
          ___pdr_setivalue(ra, ___pdr_luaV_shiftl(ib, ic));
        }
        else { ___pdr_Protect(___pdr_luaT_trybinTM(L, rb, rc, ra, PDR_TM_SHL)); }
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_SHR) {
        ___pdr_TValue *rb = ___PDR_RKB(i);
        ___pdr_TValue *rc = ___PDR_RKC(i);
        ___pdr_lua_Integer ib; ___pdr_lua_Integer ic;
        if (___pdr_tointeger(rb, &ib) && ___pdr_tointeger(rc, &ic)) {
          ___pdr_setivalue(ra, ___pdr_luaV_shiftl(ib, -ic));
        }
        else { ___pdr_Protect(___pdr_luaT_trybinTM(L, rb, rc, ra, PDR_TM_SHR)); }
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_MOD) {
        ___pdr_TValue *rb = ___PDR_RKB(i);
        ___pdr_TValue *rc = ___PDR_RKC(i);
        ___pdr_lua_Number nb; ___pdr_lua_Number nc;
        if (___pdr_ttisinteger(rb) && ___pdr_ttisinteger(rc)) {
          ___pdr_lua_Integer ib = ___pdr_ivalue(rb); ___pdr_lua_Integer ic = ___pdr_ivalue(rc);
          ___pdr_setivalue(ra, ___pdr_luaV_mod(L, ib, ic));
        }
        else if (___pdr_tonumber(rb, &nb) && ___pdr_tonumber(rc, &nc)) {
          ___pdr_lua_Number m;
          ___pdr_luai_nummod(L, nb, nc, m);
          ___pdr_setfltvalue(ra, m);
        }
        else { ___pdr_Protect(___pdr_luaT_trybinTM(L, rb, rc, ra, PDR_TM_MOD)); }
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_IDIV) {  /* floor division */
        ___pdr_TValue *rb = ___PDR_RKB(i);
        ___pdr_TValue *rc = ___PDR_RKC(i);
        ___pdr_lua_Number nb; ___pdr_lua_Number nc;
        if (___pdr_ttisinteger(rb) && ___pdr_ttisinteger(rc)) {
          ___pdr_lua_Integer ib = ___pdr_ivalue(rb); ___pdr_lua_Integer ic = ___pdr_ivalue(rc);
          ___pdr_setivalue(ra, ___pdr_luaV_div(L, ib, ic));
        }
        else if (___pdr_tonumber(rb, &nb) && ___pdr_tonumber(rc, &nc)) {
          ___pdr_setfltvalue(ra, ___pdr_luai_numidiv(L, nb, nc));
        }
        else { ___pdr_Protect(___pdr_luaT_trybinTM(L, rb, rc, ra, PDR_TM_IDIV)); }
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_POW) {
        ___pdr_TValue *rb = ___PDR_RKB(i);
        ___pdr_TValue *rc = ___PDR_RKC(i);
        ___pdr_lua_Number nb; ___pdr_lua_Number nc;
        if (___pdr_tonumber(rb, &nb) && ___pdr_tonumber(rc, &nc)) {
          ___pdr_setfltvalue(ra, ___pdr_luai_numpow(L, nb, nc));
        }
        else { ___pdr_Protect(___pdr_luaT_trybinTM(L, rb, rc, ra, PDR_TM_POW)); }
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_UNM) {
        ___pdr_TValue *rb = ___PDR_RB(i);
        ___pdr_lua_Number nb;
        if (___pdr_ttisinteger(rb)) {
          ___pdr_lua_Integer ib = ___pdr_ivalue(rb);
          ___pdr_setivalue(ra, ___pdr_intop(-, 0, ib));
        }
        else if (___pdr_tonumber(rb, &nb)) {
          ___pdr_setfltvalue(ra, ___pdr_luai_numunm(L, nb));
        }
        else {
          ___pdr_Protect(___pdr_luaT_trybinTM(L, rb, rb, ra, PDR_TM_UNM));
        }
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_BNOT) {
        ___pdr_TValue *rb = ___PDR_RB(i);
        ___pdr_lua_Integer ib;
        if (___pdr_tointeger(rb, &ib)) {
          ___pdr_setivalue(ra, ___pdr_intop(^, ~___pdr_l_castS2U(0), ib));
        }
        else {
          ___pdr_Protect(___pdr_luaT_trybinTM(L, rb, rb, ra, PDR_TM_BNOT));
        }
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_NOT) {
        ___pdr_TValue *rb = ___PDR_RB(i);
        int res = ___pdr_l_isfalse(rb);  /* next assignment may change this value */
        ___pdr_setbvalue(ra, res);
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_LEN) {
        ___pdr_Protect(___pdr_luaV_objlen(L, ra, ___PDR_RB(i)));
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_CONCAT) {
        int b = ___PDR_GETARG_B(i);
        int c = ___PDR_GETARG_C(i);
        ___pdr_StkId rb;
        L->top = base + c + 1;  /* mark the end of concat operands */
        ___pdr_Protect(___pdr_luaV_concat(L, c - b + 1));
        ra = ___PDR_RA(i);  /* 'luaV_concat' may invoke TMs and move the stack */
        rb = base + b;
        ___pdr_setobjs2s(L, ra, rb);
        ___pdr_checkGC(L, (ra >= rb ? ra + 1 : rb));
        L->top = ci->top;  /* restore top */
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_JMP) {
        ___pdr_dojump(ci, i, 0);
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_EQ) {
        ___pdr_TValue *rb = ___PDR_RKB(i);
        ___pdr_TValue *rc = ___PDR_RKC(i);
        ___pdr_Protect(
          if (___pdr_luaV_equalobj(L, rb, rc) != ___PDR_GETARG_A(i))
            ci->u.l.savedpc++;
          else
            ___pdr_donextjump(ci);
        )
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_LT) {
        ___pdr_Protect(
          if (___pdr_luaV_lessthan(L, ___PDR_RKB(i), ___PDR_RKC(i)) != ___PDR_GETARG_A(i))
            ci->u.l.savedpc++;
          else
            ___pdr_donextjump(ci);
        )
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_LE) {
        ___pdr_Protect(
          if (___pdr_luaV_lessequal(L, ___PDR_RKB(i), ___PDR_RKC(i)) != ___PDR_GETARG_A(i))
            ci->u.l.savedpc++;
          else
            ___pdr_donextjump(ci);
        )
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_TEST) {
        if (___PDR_GETARG_C(i) ? ___pdr_l_isfalse(ra) : !___pdr_l_isfalse(ra))
            ci->u.l.savedpc++;
          else
          ___pdr_donextjump(ci);
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_TESTSET) {
        ___pdr_TValue *rb = ___PDR_RB(i);
        if (___PDR_GETARG_C(i) ? ___pdr_l_isfalse(rb) : !___pdr_l_isfalse(rb))
          ci->u.l.savedpc++;
        else {
          ___pdr_setobjs2s(L, ra, rb);
          ___pdr_donextjump(ci);
        }
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_CALL) {
        int b = ___PDR_GETARG_B(i);
        int nresults = ___PDR_GETARG_C(i) - 1;
        if (b != 0) L->top = ra+b;  /* else previous instruction set top */
        if (___pdr_luaD_precall(L, ra, nresults)) {  /* C function? */
          if (nresults >= 0)
            L->top = ci->top;  /* adjust results */
          ___pdr_Protect((void)0);  /* update 'base' */
        }
        else {  /* Lua function */
          ci = L->ci;
          goto newframe;  /* restart luaV_execute over new Lua function */
        }
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_TAILCALL) {
        int b = ___PDR_GETARG_B(i);
        if (b != 0) L->top = ra+b;  /* else previous instruction set top */
        ___pdr_lua_assert(___PDR_GETARG_C(i) - 1 == ___PDR_LUA_MULTRET);
        if (___pdr_luaD_precall(L, ra, ___PDR_LUA_MULTRET)) {  /* C function? */
          ___pdr_Protect((void)0);  /* update 'base' */
        }
        else {
          /* tail call: put called frame (n) in place of caller one (o) */
          ___pdr_CallInfo *nci = L->ci;  /* called frame */
          ___pdr_CallInfo *oci = nci->previous;  /* caller frame */
          ___pdr_StkId nfunc = nci->func;  /* called function */
          ___pdr_StkId ofunc = oci->func;  /* caller function */
          /* last stack slot filled by 'precall' */
          ___pdr_StkId lim = nci->u.l.base + ___pdr_getproto(nfunc)->numparams;
          int aux;
          /* close all upvalues from previous call */
          if (cl->p->sizep > 0) ___pdr_luaF_close(L, oci->u.l.base);
          /* move new frame into old one */
          for (aux = 0; nfunc + aux < lim; aux++)
            ___pdr_setobjs2s(L, ofunc + aux, nfunc + aux);
          oci->u.l.base = ofunc + (nci->u.l.base - nfunc);  /* correct base */
          oci->top = L->top = ofunc + (L->top - nfunc);  /* correct top */
          oci->u.l.savedpc = nci->u.l.savedpc;
          oci->callstatus |= ___PDR_CIST_TAIL;  /* function was tail called */
          ci = L->ci = oci;  /* remove new frame */
          ___pdr_lua_assert(L->top == oci->u.l.base + ___pdr_getproto(ofunc)->maxstacksize);
          goto newframe;  /* restart luaV_execute over new Lua function */
        }
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_RETURN) {
        int b = ___PDR_GETARG_B(i);
        if (cl->p->sizep > 0) ___pdr_luaF_close(L, base);
        b = ___pdr_luaD_poscall(L, ci, ra, (b != 0 ? b - 1 : ___pdr_cast_int(L->top - ra)));
        if (ci->callstatus & ___PDR_CIST_FRESH)  /* local 'ci' still from callee */
          return;  /* external invocation: return */
        else {  /* invocation via reentry: continue execution */
          ci = L->ci;
          if (b) L->top = ci->top;
          ___pdr_lua_assert(___pdr_isLua(ci));
          ___pdr_lua_assert(___PDR_GET_OPCODE(*((ci)->u.l.savedpc - 1)) == PDR_OP_CALL);
          goto newframe;  /* restart luaV_execute over new Lua function */
        }
      }
      ___pdr_vmcase(PDR_OP_FORLOOP) {
        if (___pdr_ttisinteger(ra)) {  /* integer loop? */
          ___pdr_lua_Integer step = ___pdr_ivalue(ra + 2);
          ___pdr_lua_Integer idx = ___pdr_intop(+, ___pdr_ivalue(ra), step); /* increment index */
          ___pdr_lua_Integer limit = ___pdr_ivalue(ra + 1);
          if ((0 < step) ? (idx <= limit) : (limit <= idx)) {
            ci->u.l.savedpc += ___PDR_GETARG_sBx(i);  /* jump back */
            ___pdr_chgivalue(ra, idx);  /* update internal index... */
            ___pdr_setivalue(ra + 3, idx);  /* ...and external index */
          }
        }
        else {  /* floating loop */
          ___pdr_lua_Number step = ___pdr_fltvalue(ra + 2);
          ___pdr_lua_Number idx = ___pdr_luai_numadd(L, ___pdr_fltvalue(ra), step); /* inc. index */
          ___pdr_lua_Number limit = ___pdr_fltvalue(ra + 1);
          if (___pdr_luai_numlt(0, step) ? ___pdr_luai_numle(idx, limit)
                                  : ___pdr_luai_numle(limit, idx)) {
            ci->u.l.savedpc += ___PDR_GETARG_sBx(i);  /* jump back */
            ___pdr_chgfltvalue(ra, idx);  /* update internal index... */
            ___pdr_setfltvalue(ra + 3, idx);  /* ...and external index */
          }
        }
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_FORPREP) {
        ___pdr_TValue *init = ra;
        ___pdr_TValue *plimit = ra + 1;
        ___pdr_TValue *pstep = ra + 2;
        ___pdr_lua_Integer ilimit;
        int stopnow;
        if (___pdr_ttisinteger(init) && ___pdr_ttisinteger(pstep) &&
            forlimit(plimit, &ilimit, ___pdr_ivalue(pstep), &stopnow)) {
          /* all values are integer */
          ___pdr_lua_Integer initv = (stopnow ? 0 : ___pdr_ivalue(init));
          ___pdr_setivalue(plimit, ilimit);
          ___pdr_setivalue(init, ___pdr_intop(-, initv, ___pdr_ivalue(pstep)));
        }
        else {  /* try making all values floats */
          ___pdr_lua_Number ninit; ___pdr_lua_Number nlimit; ___pdr_lua_Number nstep;
          if (!___pdr_tonumber(plimit, &nlimit))
            ___pdr_luaG_runerror(L, "'for' limit must be a number");
          ___pdr_setfltvalue(plimit, nlimit);
          if (!___pdr_tonumber(pstep, &nstep))
            ___pdr_luaG_runerror(L, "'for' step must be a number");
          ___pdr_setfltvalue(pstep, nstep);
          if (!___pdr_tonumber(init, &ninit))
            ___pdr_luaG_runerror(L, "'for' initial value must be a number");
          ___pdr_setfltvalue(init, ___pdr_luai_numsub(L, ninit, nstep));
        }
        ci->u.l.savedpc += ___PDR_GETARG_sBx(i);
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_TFORCALL) {
        ___pdr_StkId cb = ra + 3;  /* call base */
        ___pdr_setobjs2s(L, cb+2, ra+2);
        ___pdr_setobjs2s(L, cb+1, ra+1);
        ___pdr_setobjs2s(L, cb, ra);
        L->top = cb + 3;  /* func. + 2 args (state and index) */
        ___pdr_Protect(___pdr_luaD_call(L, cb, ___PDR_GETARG_C(i)));
        L->top = ci->top;
        i = *(ci->u.l.savedpc++);  /* go to next instruction */
        ra = ___PDR_RA(i);
        ___pdr_lua_assert(___PDR_GET_OPCODE(i) == PDR_OP_TFORLOOP);
        goto l_tforloop;
      }
      ___pdr_vmcase(PDR_OP_TFORLOOP) {
        l_tforloop:
        if (!___pdr_ttisnil(ra + 1)) {  /* continue loop? */
          ___pdr_setobjs2s(L, ra, ra + 1);  /* save control variable */
           ci->u.l.savedpc += ___PDR_GETARG_sBx(i);  /* jump back */
        }
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_SETLIST) {
        int n = ___PDR_GETARG_B(i);
        int c = ___PDR_GETARG_C(i);
        unsigned int last;
        ___pdr_Table *h;
        if (n == 0) n = ___pdr_cast_int(L->top - ra) - 1;
        if (c == 0) {
          ___pdr_lua_assert(___PDR_GET_OPCODE(*ci->u.l.savedpc) == PDR_OP_EXTRAARG);
          c = ___PDR_GETARG_Ax(*ci->u.l.savedpc++);
        }
        h = ___pdr_hvalue(ra);
        last = ((c-1)*___PDR_LFIELDS_PER_FLUSH) + n;
        if (last > h->sizearray)  /* needs more space? */
          ___pdr_luaH_resizearray(L, h, last);  /* preallocate it at once */
        for (; n > 0; n--) {
          ___pdr_TValue *val = ra+n;
          ___pdr_luaH_setint(L, h, last--, val);
          ___pdr_luaC_barrierback(L, h, val);
        }
        L->top = ci->top;  /* correct top (in case of previous open call) */
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_CLOSURE) {
        ___pdr_Proto *p = cl->p->p[___PDR_GETARG_Bx(i)];
        ___pdr_LClosure *ncl = getcached(p, cl->upvals, base);  /* cached closure */
        if (ncl == NULL)  /* no match? */
          pushclosure(L, p, cl->upvals, base, ra);  /* create a new one */
        else
          ___pdr_setclLvalue(L, ra, ncl);  /* push cashed closure */
        ___pdr_checkGC(L, ra + 1);
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_VARARG) {
        int b = ___PDR_GETARG_B(i) - 1;  /* required results */
        int j;
        int n = ___pdr_cast_int(base - ci->func) - cl->p->numparams - 1;
        if (n < 0)  /* less arguments than parameters? */
          n = 0;  /* no vararg arguments */
        if (b < 0) {  /* B == 0? */
          b = n;  /* get all var. arguments */
          ___pdr_Protect(___pdr_luaD_checkstack(L, n));
          ra = ___PDR_RA(i);  /* previous call may change the stack */
          L->top = ra + n;
        }
        for (j = 0; j < b && j < n; j++)
          ___pdr_setobjs2s(L, ra + j, base - n + j);
        for (; j < b; j++)  /* complete required results with nil */
          ___pdr_setnilvalue(ra + j);
        ___pdr_vmbreak;
      }
      ___pdr_vmcase(PDR_OP_EXTRAARG) {
        ___pdr_lua_assert(0);
        ___pdr_vmbreak;
      }
    }
  }
}

/* }================================================================== */

} // end NS_PDR_SLUA