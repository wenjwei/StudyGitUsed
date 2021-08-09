/*
** $Id: lmathlib.c,v 1.119 2016/12/22 13:08:50 roberto Exp $
** Standard mathematical library
** See Copyright Notice in lua.h
*/

#define ___pdr_lmathlib_c
#define ___PDR_LUA_LIB

#include "lprefix.h"

#include <stdlib.h>
#include <math.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

namespace NS_PDR_SLUA {

#undef ___PDR_PI
#define ___PDR_PI	(___pdr_l_mathop(3.141592653589793238462643383279502884))


#if !defined(___pdr_l_rand)		/* { */
#if defined(___PDR_LUA_USE_POSIX)
#define ___pdr_l_rand()	random()
#define ___pdr_l_srand(x)	srandom(x)
#define ___PDR_L_RANDMAX	2147483647	/* (2^31 - 1), following POSIX */
#else
#define ___pdr_l_rand()	rand()
#define ___pdr_l_srand(x)	srand(x)
#define ___PDR_L_RANDMAX	RAND_MAX
#endif
#endif				/* } */


static int math_abs (___pdr_lua_State *L) {
  if (___pdr_lua_isinteger(L, 1)) {
    ___pdr_lua_Integer n = ___pdr_lua_tointeger(L, 1);
    if (n < 0) n = (___pdr_lua_Integer)(0u - (___pdr_lua_Unsigned)n);
    ___pdr_lua_pushinteger(L, n);
  }
  else
    ___pdr_lua_pushnumber(L, ___pdr_l_mathop(fabs)(___pdr_luaL_checknumber(L, 1)));
  return 1;
}

static int math_sin (___pdr_lua_State *L) {
  ___pdr_lua_pushnumber(L, ___pdr_l_mathop(sin)(___pdr_luaL_checknumber(L, 1)));
  return 1;
}

static int math_cos (___pdr_lua_State *L) {
  ___pdr_lua_pushnumber(L, ___pdr_l_mathop(cos)(___pdr_luaL_checknumber(L, 1)));
  return 1;
}

static int math_tan (___pdr_lua_State *L) {
  ___pdr_lua_pushnumber(L, ___pdr_l_mathop(tan)(___pdr_luaL_checknumber(L, 1)));
  return 1;
}

static int math_asin (___pdr_lua_State *L) {
  ___pdr_lua_pushnumber(L, ___pdr_l_mathop(asin)(___pdr_luaL_checknumber(L, 1)));
  return 1;
}

static int math_acos (___pdr_lua_State *L) {
  ___pdr_lua_pushnumber(L, ___pdr_l_mathop(acos)(___pdr_luaL_checknumber(L, 1)));
  return 1;
}

static int math_atan (___pdr_lua_State *L) {
  ___pdr_lua_Number y = ___pdr_luaL_checknumber(L, 1);
  ___pdr_lua_Number x = ___pdr_luaL_optnumber(L, 2, 1);
  ___pdr_lua_pushnumber(L, ___pdr_l_mathop(atan2)(y, x));
  return 1;
}


static int math_toint (___pdr_lua_State *L) {
  int valid;
  ___pdr_lua_Integer n = ___pdr_lua_tointegerx(L, 1, &valid);
  if (valid)
    ___pdr_lua_pushinteger(L, n);
  else {
    ___pdr_luaL_checkany(L, 1);
    ___pdr_lua_pushnil(L);  /* value is not convertible to integer */
  }
  return 1;
}


static void pushnumint (___pdr_lua_State *L, ___pdr_lua_Number d) {
  ___pdr_lua_Integer n;
  if (___pdr_lua_numbertointeger(d, &n))  /* does 'd' fit in an integer? */
    ___pdr_lua_pushinteger(L, n);  /* result is integer */
  else
    ___pdr_lua_pushnumber(L, d);  /* result is float */
}


static int math_floor (___pdr_lua_State *L) {
  if (___pdr_lua_isinteger(L, 1))
    ___pdr_lua_settop(L, 1);  /* integer is its own floor */
  else {
    ___pdr_lua_Number d = ___pdr_l_mathop(floor)(___pdr_luaL_checknumber(L, 1));
    pushnumint(L, d);
  }
  return 1;
}


static int math_ceil (___pdr_lua_State *L) {
  if (___pdr_lua_isinteger(L, 1))
    ___pdr_lua_settop(L, 1);  /* integer is its own ceil */
  else {
    ___pdr_lua_Number d = ___pdr_l_mathop(ceil)(___pdr_luaL_checknumber(L, 1));
    pushnumint(L, d);
  }
  return 1;
}


static int math_fmod (___pdr_lua_State *L) {
  if (___pdr_lua_isinteger(L, 1) && ___pdr_lua_isinteger(L, 2)) {
    ___pdr_lua_Integer d = ___pdr_lua_tointeger(L, 2);
    if ((___pdr_lua_Unsigned)d + 1u <= 1u) {  /* special cases: -1 or 0 */
      ___pdr_luaL_argcheck(L, d != 0, 2, "zero");
      ___pdr_lua_pushinteger(L, 0);  /* avoid overflow with 0x80000... / -1 */
    }
    else
      ___pdr_lua_pushinteger(L, ___pdr_lua_tointeger(L, 1) % d);
  }
  else
    ___pdr_lua_pushnumber(L, ___pdr_l_mathop(fmod)(___pdr_luaL_checknumber(L, 1),
                                     ___pdr_luaL_checknumber(L, 2)));
  return 1;
}


/*
** next function does not use 'modf', avoiding problems with 'double*'
** (which is not compatible with 'float*') when lua_Number is not
** 'double'.
*/
static int math_modf (___pdr_lua_State *L) {
  if (___pdr_lua_isinteger(L ,1)) {
    ___pdr_lua_settop(L, 1);  /* number is its own integer part */
    ___pdr_lua_pushnumber(L, 0);  /* no fractional part */
  }
  else {
    ___pdr_lua_Number n = ___pdr_luaL_checknumber(L, 1);
    /* integer part (rounds toward zero) */
    ___pdr_lua_Number ip = (n < 0) ? ___pdr_l_mathop(ceil)(n) : ___pdr_l_mathop(floor)(n);
    pushnumint(L, ip);
    /* fractional part (test needed for inf/-inf) */
    ___pdr_lua_pushnumber(L, (n == ip) ? ___pdr_l_mathop(0.0) : (n - ip));
  }
  return 2;
}


static int math_sqrt (___pdr_lua_State *L) {
  ___pdr_lua_pushnumber(L, ___pdr_l_mathop(sqrt)(___pdr_luaL_checknumber(L, 1)));
  return 1;
}


static int math_ult (___pdr_lua_State *L) {
  ___pdr_lua_Integer a = ___pdr_luaL_checkinteger(L, 1);
  ___pdr_lua_Integer b = ___pdr_luaL_checkinteger(L, 2);
  ___pdr_lua_pushboolean(L, (___pdr_lua_Unsigned)a < (___pdr_lua_Unsigned)b);
  return 1;
}

static int math_log (___pdr_lua_State *L) {
  ___pdr_lua_Number x = ___pdr_luaL_checknumber(L, 1);
  ___pdr_lua_Number res;
  if (___pdr_lua_isnoneornil(L, 2))
    res = ___pdr_l_mathop(log)(x);
  else {
    ___pdr_lua_Number base = ___pdr_luaL_checknumber(L, 2);
    if (base == ___pdr_l_mathop(10.0))
      res = ___pdr_l_mathop(log10)(x);
    else
      res = ___pdr_l_mathop(log)(x)/___pdr_l_mathop(log)(base);
  }
  ___pdr_lua_pushnumber(L, res);
  return 1;
}

static int math_exp (___pdr_lua_State *L) {
  ___pdr_lua_pushnumber(L, ___pdr_l_mathop(exp)(___pdr_luaL_checknumber(L, 1)));
  return 1;
}

static int math_deg (___pdr_lua_State *L) {
  ___pdr_lua_pushnumber(L, ___pdr_luaL_checknumber(L, 1) * (___pdr_l_mathop(180.0) / ___PDR_PI));
  return 1;
}

static int math_rad (___pdr_lua_State *L) {
  ___pdr_lua_pushnumber(L, ___pdr_luaL_checknumber(L, 1) * (___PDR_PI / ___pdr_l_mathop(180.0)));
  return 1;
}


static int math_min (___pdr_lua_State *L) {
  int n = ___pdr_lua_gettop(L);  /* number of arguments */
  int imin = 1;  /* index of current minimum value */
  int i;
  ___pdr_luaL_argcheck(L, n >= 1, 1, "value expected");
  for (i = 2; i <= n; i++) {
    if (___pdr_lua_compare(L, i, imin, ___PDR_LUA_OPLT))
      imin = i;
  }
  ___pdr_lua_pushvalue(L, imin);
  return 1;
}


static int math_max (___pdr_lua_State *L) {
  int n = ___pdr_lua_gettop(L);  /* number of arguments */
  int imax = 1;  /* index of current maximum value */
  int i;
  ___pdr_luaL_argcheck(L, n >= 1, 1, "value expected");
  for (i = 2; i <= n; i++) {
    if (___pdr_lua_compare(L, imax, i, ___PDR_LUA_OPLT))
      imax = i;
  }
  ___pdr_lua_pushvalue(L, imax);
  return 1;
}

/*
** This function uses 'double' (instead of 'lua_Number') to ensure that
** all bits from 'l_rand' can be represented, and that 'RANDMAX + 1.0'
** will keep full precision (ensuring that 'r' is always less than 1.0.)
*/
static int math_random (___pdr_lua_State *L) {
  ___pdr_lua_Integer low, up;
  double r = (double)___pdr_l_rand() * (1.0 / ((double)___PDR_L_RANDMAX + 1.0));
  switch (___pdr_lua_gettop(L)) {  /* check number of arguments */
    case 0: {  /* no arguments */
      ___pdr_lua_pushnumber(L, (___pdr_lua_Number)r);  /* Number between 0 and 1 */
      return 1;
    }
    case 1: {  /* only upper limit */
      low = 1;
      up = ___pdr_luaL_checkinteger(L, 1);
      break;
    }
    case 2: {  /* lower and upper limits */
      low = ___pdr_luaL_checkinteger(L, 1);
      up = ___pdr_luaL_checkinteger(L, 2);
      break;
    }
    default: return ___pdr_luaL_error(L, "wrong number of arguments");
  }
  /* random integer in the interval [low, up] */
  ___pdr_luaL_argcheck(L, low <= up, 1, "interval is empty");
  ___pdr_luaL_argcheck(L, low >= 0 || up <= ___PDR_LUA_MAXINTEGER + low, 1,
                   "interval too large");
  r *= (double)(up - low) + 1.0;
  ___pdr_lua_pushinteger(L, (___pdr_lua_Integer)r + low);
  return 1;
}


static int math_randomseed (___pdr_lua_State *L) {
  ___pdr_l_srand((unsigned int)(___pdr_lua_Integer)___pdr_luaL_checknumber(L, 1));
  (void)___pdr_l_rand(); /* discard first value to avoid undesirable correlations */
  return 0;
}


static int math_type (___pdr_lua_State *L) {
  if (___pdr_lua_type(L, 1) == ___PDR_LUA_TNUMBER) {
      if (___pdr_lua_isinteger(L, 1))
        ___pdr_lua_pushliteral(L, "integer");
      else
        ___pdr_lua_pushliteral(L, "float");
  }
  else {
    ___pdr_luaL_checkany(L, 1);
    ___pdr_lua_pushnil(L);
  }
  return 1;
}


/*
** {==================================================================
** Deprecated functions (for compatibility only)
** ===================================================================
*/
#if defined(___PDR_LUA_COMPAT_MATHLIB)

static int math_cosh (___pdr_lua_State *L) {
  ___pdr_lua_pushnumber(L, ___pdr_l_mathop(cosh)(___pdr_luaL_checknumber(L, 1)));
  return 1;
}

static int math_sinh (___pdr_lua_State *L) {
  ___pdr_lua_pushnumber(L, ___pdr_l_mathop(sinh)(___pdr_luaL_checknumber(L, 1)));
  return 1;
}

static int math_tanh (___pdr_lua_State *L) {
  ___pdr_lua_pushnumber(L, ___pdr_l_mathop(tanh)(___pdr_luaL_checknumber(L, 1)));
  return 1;
}

static int math_pow (___pdr_lua_State *L) {
  ___pdr_lua_Number x = ___pdr_luaL_checknumber(L, 1);
  ___pdr_lua_Number y = ___pdr_luaL_checknumber(L, 2);
  ___pdr_lua_pushnumber(L, ___pdr_l_mathop(pow)(x, y));
  return 1;
}

static int math_frexp (___pdr_lua_State *L) {
  int e;
  ___pdr_lua_pushnumber(L, ___pdr_l_mathop(frexp)(___pdr_luaL_checknumber(L, 1), &e));
  ___pdr_lua_pushinteger(L, e);
  return 2;
}

static int math_ldexp (___pdr_lua_State *L) {
  ___pdr_lua_Number x = ___pdr_luaL_checknumber(L, 1);
  int ep = (int)___pdr_luaL_checkinteger(L, 2);
  ___pdr_lua_pushnumber(L, ___pdr_l_mathop(ldexp)(x, ep));
  return 1;
}

static int math_log10 (___pdr_lua_State *L) {
  ___pdr_lua_pushnumber(L, ___pdr_l_mathop(log10)(___pdr_luaL_checknumber(L, 1)));
  return 1;
}

#endif
/* }================================================================== */



static const ___pdr_luaL_Reg mathlib[] = {
  {"abs",   math_abs},
  {"acos",  math_acos},
  {"asin",  math_asin},
  {"atan",  math_atan},
  {"ceil",  math_ceil},
  {"cos",   math_cos},
  {"deg",   math_deg},
  {"exp",   math_exp},
  {"tointeger", math_toint},
  {"floor", math_floor},
  {"fmod",   math_fmod},
  {"ult",   math_ult},
  {"log",   math_log},
  {"max",   math_max},
  {"min",   math_min},
  {"modf",   math_modf},
  {"rad",   math_rad},
  {"random",     math_random},
  {"randomseed", math_randomseed},
  {"sin",   math_sin},
  {"sqrt",  math_sqrt},
  {"tan",   math_tan},
  {"type", math_type},
#if defined(___PDR_LUA_COMPAT_MATHLIB)
  {"atan2", math_atan},
  {"cosh",   math_cosh},
  {"sinh",   math_sinh},
  {"tanh",   math_tanh},
  {"pow",   math_pow},
  {"frexp", math_frexp},
  {"ldexp", math_ldexp},
  {"log10", math_log10},
#endif
  /* placeholders */
  {"pi", NULL},
  {"huge", NULL},
  {"maxinteger", NULL},
  {"mininteger", NULL},
  {NULL, NULL}
};


/*
** Open math library
*/
___PDR_LUAMOD_API int ___pdr_luaopen_math (___pdr_lua_State *L) {
  ___pdr_luaL_newlib(L, mathlib);
  ___pdr_lua_pushnumber(L, ___PDR_PI);
  ___pdr_lua_setfield(L, -2, "pi");
  ___pdr_lua_pushnumber(L, (___pdr_lua_Number)HUGE_VAL);
  ___pdr_lua_setfield(L, -2, "huge");
  ___pdr_lua_pushinteger(L, ___PDR_LUA_MAXINTEGER);
  ___pdr_lua_setfield(L, -2, "maxinteger");
  ___pdr_lua_pushinteger(L, ___PDR_LUA_MININTEGER);
  ___pdr_lua_setfield(L, -2, "mininteger");
  return 1;
}

} // end NS_PDR_SLUA