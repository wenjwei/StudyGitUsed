/*
** $Id: ltablib.c,v 1.93 2016/02/25 19:41:54 roberto Exp $
** Library for Table Manipulation
** See Copyright Notice in lua.h
*/

#define ___pdr_ltablib_c
#define ___PDR_LUA_LIB

#include "lprefix.h"


#include <limits.h>
#include <stddef.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"


#if !defined(___pdr_l_randomizePivot)
#include <time.h>
#endif

namespace NS_PDR_SLUA {

/*
** Operations that an object must define to mimic a table
** (some functions only need some of them)
*/
#define ___PDR_TAB_R	1			/* read */
#define ___PDR_TAB_W	2			/* write */
#define ___PDR_TAB_L	4			/* length */
#define ___PDR_TAB_RW	(___PDR_TAB_R | ___PDR_TAB_W)		/* read/write */


#define ___pdr_aux_getn(L,n,w)	(checktab(L, n, (w) | ___PDR_TAB_L), ___pdr_luaL_len(L, n))


static int checkfield (___pdr_lua_State *L, const char *key, int n) {
  ___pdr_lua_pushstring(L, key);
  return (___pdr_lua_rawget(L, -n) != ___PDR_LUA_TNIL);
}


/*
** Check that 'arg' either is a table or can behave like one (that is,
** has a metatable with the required metamethods)
*/
static void checktab (___pdr_lua_State *L, int arg, int what) {
  if (___pdr_lua_type(L, arg) != ___PDR_LUA_TTABLE) {  /* is it not a table? */
    int n = 1;  /* number of elements to pop */
    if (___pdr_lua_getmetatable(L, arg) &&  /* must have metatable */
        (!(what & ___PDR_TAB_R) || checkfield(L, "__index", ++n)) &&
        (!(what & ___PDR_TAB_W) || checkfield(L, "__newindex", ++n)) &&
        (!(what & ___PDR_TAB_L) || checkfield(L, "__len", ++n))) {
      ___pdr_lua_pop(L, n);  /* pop metatable and tested metamethods */
    }
    else
      ___pdr_luaL_checktype(L, arg, ___PDR_LUA_TTABLE);  /* force an error */
  }
}


#if defined(___PDR_LUA_COMPAT_MAXN)
static int maxn (___pdr_lua_State *L) {
  ___pdr_lua_Number max = 0;
  ___pdr_luaL_checktype(L, 1, ___PDR_LUA_TTABLE);
  ___pdr_lua_pushnil(L);  /* first key */
  while (___pdr_lua_next(L, 1)) {
    ___pdr_lua_pop(L, 1);  /* remove value */
    if (___pdr_lua_type(L, -1) == ___PDR_LUA_TNUMBER) {
      ___pdr_lua_Number v = ___pdr_lua_tonumber(L, -1);
      if (v > max) max = v;
    }
  }
  ___pdr_lua_pushnumber(L, max);
  return 1;
}
#endif


static int tinsert (___pdr_lua_State *L) {
  ___pdr_lua_Integer e = ___pdr_aux_getn(L, 1, ___PDR_TAB_RW) + 1;  /* first empty element */
  ___pdr_lua_Integer pos;  /* where to insert new element */
  switch (___pdr_lua_gettop(L)) {
    case 2: {  /* called with only 2 arguments */
      pos = e;  /* insert new element at the end */
      break;
    }
    case 3: {
      ___pdr_lua_Integer i;
      pos = ___pdr_luaL_checkinteger(L, 2);  /* 2nd argument is the position */
      ___pdr_luaL_argcheck(L, 1 <= pos && pos <= e, 2, "position out of bounds");
      for (i = e; i > pos; i--) {  /* move up elements */
        ___pdr_lua_geti(L, 1, i - 1);
        ___pdr_lua_seti(L, 1, i);  /* t[i] = t[i - 1] */
      }
      break;
    }
    default: {
      return ___pdr_luaL_error(L, "wrong number of arguments to 'insert'");
    }
  }
  ___pdr_lua_seti(L, 1, pos);  /* t[pos] = v */
  return 0;
}


static int tremove (___pdr_lua_State *L) {
  ___pdr_lua_Integer size = ___pdr_aux_getn(L, 1, ___PDR_TAB_RW);
  ___pdr_lua_Integer pos = ___pdr_luaL_optinteger(L, 2, size);
  if (pos != size)  /* validate 'pos' if given */
    ___pdr_luaL_argcheck(L, 1 <= pos && pos <= size + 1, 1, "position out of bounds");
  ___pdr_lua_geti(L, 1, pos);  /* result = t[pos] */
  for ( ; pos < size; pos++) {
    ___pdr_lua_geti(L, 1, pos + 1);
    ___pdr_lua_seti(L, 1, pos);  /* t[pos] = t[pos + 1] */
  }
  ___pdr_lua_pushnil(L);
  ___pdr_lua_seti(L, 1, pos);  /* t[pos] = nil */
  return 1;
}


/*
** Copy elements (1[f], ..., 1[e]) into (tt[t], tt[t+1], ...). Whenever
** possible, copy in increasing order, which is better for rehashing.
** "possible" means destination after original range, or smaller
** than origin, or copying to another table.
*/
static int tmove (___pdr_lua_State *L) {
  ___pdr_lua_Integer f = ___pdr_luaL_checkinteger(L, 2);
  ___pdr_lua_Integer e = ___pdr_luaL_checkinteger(L, 3);
  ___pdr_lua_Integer t = ___pdr_luaL_checkinteger(L, 4);
  int tt = !___pdr_lua_isnoneornil(L, 5) ? 5 : 1;  /* destination table */
  checktab(L, 1, ___PDR_TAB_R);
  checktab(L, tt, ___PDR_TAB_W);
  if (e >= f) {  /* otherwise, nothing to move */
    ___pdr_lua_Integer n, i;
    ___pdr_luaL_argcheck(L, f > 0 || e < ___PDR_LUA_MAXINTEGER + f, 3,
                  "too many elements to move");
    n = e - f + 1;  /* number of elements to move */
    ___pdr_luaL_argcheck(L, t <= ___PDR_LUA_MAXINTEGER - n + 1, 4,
                  "destination wrap around");
    if (t > e || t <= f || (tt != 1 && !___pdr_lua_compare(L, 1, tt, ___PDR_LUA_OPEQ))) {
      for (i = 0; i < n; i++) {
        ___pdr_lua_geti(L, 1, f + i);
        ___pdr_lua_seti(L, tt, t + i);
      }
    }
    else {
      for (i = n - 1; i >= 0; i--) {
        ___pdr_lua_geti(L, 1, f + i);
        ___pdr_lua_seti(L, tt, t + i);
      }
    }
  }
  ___pdr_lua_pushvalue(L, tt);  /* return destination table */
  return 1;
}


static void addfield (___pdr_lua_State *L, ___pdr_luaL_Buffer *b, ___pdr_lua_Integer i) {
  ___pdr_lua_geti(L, 1, i);
  if (!___pdr_lua_isstring(L, -1))
    ___pdr_luaL_error(L, "invalid value (%s) at index %d in table for 'concat'",
                  ___pdr_luaL_typename(L, -1), i);
  ___pdr_luaL_addvalue(b);
}


static int tconcat (___pdr_lua_State *L) {
  ___pdr_luaL_Buffer b;
  ___pdr_lua_Integer last = ___pdr_aux_getn(L, 1, ___PDR_TAB_R);
  size_t lsep;
  const char *sep = ___pdr_luaL_optlstring(L, 2, "", &lsep);
  ___pdr_lua_Integer i = ___pdr_luaL_optinteger(L, 3, 1);
  last = ___pdr_luaL_optinteger(L, 4, last);
  ___pdr_luaL_buffinit(L, &b);
  for (; i < last; i++) {
    addfield(L, &b, i);
    ___pdr_luaL_addlstring(&b, sep, lsep);
  }
  if (i == last)  /* add last value (if interval was not empty) */
    addfield(L, &b, i);
  ___pdr_luaL_pushresult(&b);
  return 1;
}


/*
** {======================================================
** Pack/unpack
** =======================================================
*/

static int pack (___pdr_lua_State *L) {
  int i;
  int n = ___pdr_lua_gettop(L);  /* number of elements to pack */
  ___pdr_lua_createtable(L, n, 1);  /* create result table */
  ___pdr_lua_insert(L, 1);  /* put it at index 1 */
  for (i = n; i >= 1; i--)  /* assign elements */
    ___pdr_lua_seti(L, 1, i);
  ___pdr_lua_pushinteger(L, n);
  ___pdr_lua_setfield(L, 1, "n");  /* t.n = number of elements */
  return 1;  /* return table */
}


static int unpack (___pdr_lua_State *L) {
  ___pdr_lua_Unsigned n;
  ___pdr_lua_Integer i = ___pdr_luaL_optinteger(L, 2, 1);
  ___pdr_lua_Integer e = ___pdr_luaL_opt(L, ___pdr_luaL_checkinteger, 3, ___pdr_luaL_len(L, 1));
  if (i > e) return 0;  /* empty range */
  n = (___pdr_lua_Unsigned)e - i;  /* number of elements minus 1 (avoid overflows) */
  if (n >= (unsigned int)INT_MAX  || !___pdr_lua_checkstack(L, (int)(++n)))
    return ___pdr_luaL_error(L, "too many results to unpack");
  for (; i < e; i++) {  /* push arg[i..e - 1] (to avoid overflows) */
    ___pdr_lua_geti(L, 1, i);
  }
  ___pdr_lua_geti(L, 1, e);  /* push last element */
  return (int)n;
}

/* }====================================================== */



/*
** {======================================================
** Quicksort
** (based on 'Algorithms in MODULA-3', Robert Sedgewick;
**  Addison-Wesley, 1993.)
** =======================================================
*/


/* type for array indices */
typedef unsigned int IdxT;


/*
** Produce a "random" 'unsigned int' to randomize pivot choice. This
** macro is used only when 'sort' detects a big imbalance in the result
** of a partition. (If you don't want/need this "randomness", ~0 is a
** good choice.)
*/
#if !defined(___pdr_l_randomizePivot)		/* { */

/* size of 'e' measured in number of 'unsigned int's */
#define ___pdr_sof(e)		(sizeof(e) / sizeof(unsigned int))

/*
** Use 'time' and 'clock' as sources of "randomness". Because we don't
** know the types 'clock_t' and 'time_t', we cannot cast them to
** anything without risking overflows. A safe way to use their values
** is to copy them to an array of a known type and use the array values.
*/
static unsigned int ___pdr_l_randomizePivot (void) {
  clock_t c = clock();
  time_t t = time(NULL);
  unsigned int buff[___pdr_sof(c) + ___pdr_sof(t)];
  unsigned int i, rnd = 0;
  memcpy(buff, &c, ___pdr_sof(c) * sizeof(unsigned int));
  memcpy(buff + ___pdr_sof(c), &t, ___pdr_sof(t) * sizeof(unsigned int));
  for (i = 0; i < ___pdr_sof(buff); i++)
    rnd += buff[i];
  return rnd;
}

#endif					/* } */


/* arrays larger than 'RANLIMIT' may use randomized pivots */
#define ___PDR_RANLIMIT	100u


static void set2 (___pdr_lua_State *L, IdxT i, IdxT j) {
  ___pdr_lua_seti(L, 1, i);
  ___pdr_lua_seti(L, 1, j);
}


/*
** Return true iff value at stack index 'a' is less than the value at
** index 'b' (according to the order of the sort).
*/
static int sort_comp (___pdr_lua_State *L, int a, int b) {
  if (___pdr_lua_isnil(L, 2))  /* no function? */
    return ___pdr_lua_compare(L, a, b, ___PDR_LUA_OPLT);  /* a < b */
  else {  /* function */
    int res;
    ___pdr_lua_pushvalue(L, 2);    /* push function */
    ___pdr_lua_pushvalue(L, a-1);  /* -1 to compensate function */
    ___pdr_lua_pushvalue(L, b-2);  /* -2 to compensate function and 'a' */
    lua_call(L, 2, 1);      /* call function */
    res = ___pdr_lua_toboolean(L, -1);  /* get result */
    ___pdr_lua_pop(L, 1);          /* pop result */
    return res;
  }
}


/*
** Does the partition: Pivot P is at the top of the stack.
** precondition: a[lo] <= P == a[up-1] <= a[up],
** so it only needs to do the partition from lo + 1 to up - 2.
** Pos-condition: a[lo .. i - 1] <= a[i] == P <= a[i + 1 .. up]
** returns 'i'.
*/
static IdxT partition (___pdr_lua_State *L, IdxT lo, IdxT up) {
  IdxT i = lo;  /* will be incremented before first use */
  IdxT j = up - 1;  /* will be decremented before first use */
  /* loop invariant: a[lo .. i] <= P <= a[j .. up] */
  for (;;) {
    /* next loop: repeat ++i while a[i] < P */
    while (___pdr_lua_geti(L, 1, ++i), sort_comp(L, -1, -2)) {
      if (i == up - 1)  /* a[i] < P  but a[up - 1] == P  ?? */
        ___pdr_luaL_error(L, "invalid order function for sorting");
      ___pdr_lua_pop(L, 1);  /* remove a[i] */
    }
    /* after the loop, a[i] >= P and a[lo .. i - 1] < P */
    /* next loop: repeat --j while P < a[j] */
    while (___pdr_lua_geti(L, 1, --j), sort_comp(L, -3, -1)) {
      if (j < i)  /* j < i  but  a[j] > P ?? */
        ___pdr_luaL_error(L, "invalid order function for sorting");
      ___pdr_lua_pop(L, 1);  /* remove a[j] */
    }
    /* after the loop, a[j] <= P and a[j + 1 .. up] >= P */
    if (j < i) {  /* no elements out of place? */
      /* a[lo .. i - 1] <= P <= a[j + 1 .. i .. up] */
      ___pdr_lua_pop(L, 1);  /* pop a[j] */
      /* swap pivot (a[up - 1]) with a[i] to satisfy pos-condition */
      set2(L, up - 1, i);
      return i;
    }
    /* otherwise, swap a[i] - a[j] to restore invariant and repeat */
    set2(L, i, j);
  }
}


/*
** Choose an element in the middle (2nd-3th quarters) of [lo,up]
** "randomized" by 'rnd'
*/
static IdxT choosePivot (IdxT lo, IdxT up, unsigned int rnd) {
  IdxT r4 = (up - lo) / 4;  /* range/4 */
  IdxT p = rnd % (r4 * 2) + (lo + r4);
  ___pdr_lua_assert(lo + r4 <= p && p <= up - r4);
  return p;
}


/*
** QuickSort algorithm (recursive function)
*/
static void auxsort (___pdr_lua_State *L, IdxT lo, IdxT up,
                                   unsigned int rnd) {
  while (lo < up) {  /* loop for tail recursion */
    IdxT p;  /* Pivot index */
    IdxT n;  /* to be used later */
    /* sort elements 'lo', 'p', and 'up' */
    ___pdr_lua_geti(L, 1, lo);
    ___pdr_lua_geti(L, 1, up);
    if (sort_comp(L, -1, -2))  /* a[up] < a[lo]? */
      set2(L, lo, up);  /* swap a[lo] - a[up] */
    else
      ___pdr_lua_pop(L, 2);  /* remove both values */
    if (up - lo == 1)  /* only 2 elements? */
      return;  /* already sorted */
    if (up - lo < ___PDR_RANLIMIT || rnd == 0)  /* small interval or no randomize? */
      p = (lo + up)/2;  /* middle element is a good pivot */
    else  /* for larger intervals, it is worth a random pivot */
      p = choosePivot(lo, up, rnd);
    ___pdr_lua_geti(L, 1, p);
    ___pdr_lua_geti(L, 1, lo);
    if (sort_comp(L, -2, -1))  /* a[p] < a[lo]? */
      set2(L, p, lo);  /* swap a[p] - a[lo] */
    else {
      ___pdr_lua_pop(L, 1);  /* remove a[lo] */
      ___pdr_lua_geti(L, 1, up);
      if (sort_comp(L, -1, -2))  /* a[up] < a[p]? */
        set2(L, p, up);  /* swap a[up] - a[p] */
      else
        ___pdr_lua_pop(L, 2);
    }
    if (up - lo == 2)  /* only 3 elements? */
      return;  /* already sorted */
    ___pdr_lua_geti(L, 1, p);  /* get middle element (Pivot) */
    ___pdr_lua_pushvalue(L, -1);  /* push Pivot */
    ___pdr_lua_geti(L, 1, up - 1);  /* push a[up - 1] */
    set2(L, p, up - 1);  /* swap Pivot (a[p]) with a[up - 1] */
    p = partition(L, lo, up);
    /* a[lo .. p - 1] <= a[p] == P <= a[p + 1 .. up] */
    if (p - lo < up - p) {  /* lower interval is smaller? */
      auxsort(L, lo, p - 1, rnd);  /* call recursively for lower interval */
      n = p - lo;  /* size of smaller interval */
      lo = p + 1;  /* tail call for [p + 1 .. up] (upper interval) */
    }
    else {
      auxsort(L, p + 1, up, rnd);  /* call recursively for upper interval */
      n = up - p;  /* size of smaller interval */
      up = p - 1;  /* tail call for [lo .. p - 1]  (lower interval) */
    }
    if ((up - lo) / 128 > n) /* partition too imbalanced? */
      rnd = ___pdr_l_randomizePivot();  /* try a new randomization */
  }  /* tail call auxsort(L, lo, up, rnd) */
}


static int sort (___pdr_lua_State *L) {
  ___pdr_lua_Integer n = ___pdr_aux_getn(L, 1, ___PDR_TAB_RW);
  if (n > 1) {  /* non-trivial interval? */
    ___pdr_luaL_argcheck(L, n < INT_MAX, 1, "array too big");
    if (!___pdr_lua_isnoneornil(L, 2))  /* is there a 2nd argument? */
      ___pdr_luaL_checktype(L, 2, ___PDR_LUA_TFUNCTION);  /* must be a function */
    ___pdr_lua_settop(L, 2);  /* make sure there are two arguments */
    auxsort(L, 1, (IdxT)n, 0);
  }
  return 0;
}

/* }====================================================== */


static const ___pdr_luaL_Reg tab_funcs[] = {
  {"concat", tconcat},
#if defined(___PDR_LUA_COMPAT_MAXN)
  {"maxn", maxn},
#endif
  {"insert", tinsert},
  {"pack", pack},
  {"unpack", unpack},
  {"remove", tremove},
  {"move", tmove},
  {"sort", sort},
  {NULL, NULL}
};


___PDR_LUAMOD_API int ___pdr_luaopen_table (___pdr_lua_State *L) {
  ___pdr_luaL_newlib(L, tab_funcs);
#if defined(___PDR_LUA_COMPAT_UNPACK)
  /* _G.unpack = table.unpack */
  ___pdr_lua_getfield(L, -1, "unpack");
  ___pdr_lua_setglobal(L, "unpack");
#endif
  return 1;
}

} // end NS_PDR_SLUA