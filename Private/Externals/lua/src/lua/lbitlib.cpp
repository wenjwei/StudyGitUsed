/*
** $Id: lbitlib.c,v 1.30 2015/11/11 19:08:09 roberto Exp $
** Standard library for bitwise operations
** See Copyright Notice in lua.h
*/

#define ___pdr_lbitlib_c
#define ___PDR_LUA_LIB

#include "lprefix.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

namespace NS_PDR_SLUA {

#if defined(___PDR_LUA_COMPAT_BITLIB)		/* { */


#define ___pdr_pushunsigned(L,n)	___pdr_lua_pushinteger(L, (___pdr_lua_Integer)(n))
#define ___pdr_checkunsigned(L,i)	((___pdr_lua_Unsigned)___pdr_luaL_checkinteger(L,i))


/* number of bits to consider in a number */
#if !defined(___PDR_LUA_NBITS)
#define ___PDR_LUA_NBITS	32
#endif


/*
** a lua_Unsigned with its first LUA_NBITS bits equal to 1. (Shift must
** be made in two parts to avoid problems when LUA_NBITS is equal to the
** number of bits in a lua_Unsigned.)
*/
#define ___PDR_ALLONES		(~(((~(___pdr_lua_Unsigned)0) << (___PDR_LUA_NBITS - 1)) << 1))


/* macro to trim extra bits */
#define ___pdr_trim(x)		((x) & ___PDR_ALLONES)


/* builds a number with 'n' ones (1 <= n <= LUA_NBITS) */
#define ___pdr_mask(n)		(~((___PDR_ALLONES << 1) << ((n) - 1)))

static ___pdr_lua_Unsigned andaux (___pdr_lua_State *L) {
  int i, n = ___pdr_lua_gettop(L);
  ___pdr_lua_Unsigned r = ~(___pdr_lua_Unsigned)0;
  for (i = 1; i <= n; i++)
    r &= ___pdr_checkunsigned(L, i);
  return ___pdr_trim(r);
}


static int b_and (___pdr_lua_State *L) {
  ___pdr_lua_Unsigned r = andaux(L);
  ___pdr_pushunsigned(L, r);
  return 1;
}


static int b_test (___pdr_lua_State *L) {
  ___pdr_lua_Unsigned r = andaux(L);
  ___pdr_lua_pushboolean(L, r != 0);
  return 1;
}


static int b_or (___pdr_lua_State *L) {
  int i, n = ___pdr_lua_gettop(L);
  ___pdr_lua_Unsigned r = 0;
  for (i = 1; i <= n; i++)
    r |= ___pdr_checkunsigned(L, i);
  ___pdr_pushunsigned(L, ___pdr_trim(r));
  return 1;
}


static int b_xor (___pdr_lua_State *L) {
  int i, n = ___pdr_lua_gettop(L);
  ___pdr_lua_Unsigned r = 0;
  for (i = 1; i <= n; i++)
    r ^= ___pdr_checkunsigned(L, i);
  ___pdr_pushunsigned(L, ___pdr_trim(r));
  return 1;
}


static int b_not (___pdr_lua_State *L) {
  ___pdr_lua_Unsigned r = ~___pdr_checkunsigned(L, 1);
  ___pdr_pushunsigned(L, ___pdr_trim(r));
  return 1;
}


static int b_shift (___pdr_lua_State *L, ___pdr_lua_Unsigned r, ___pdr_lua_Integer i) {
  if (i < 0) {  /* shift right? */
    i = -i;
    r = ___pdr_trim(r);
    if (i >= ___PDR_LUA_NBITS) r = 0;
    else r >>= i;
  }
  else {  /* shift left */
    if (i >= ___PDR_LUA_NBITS) r = 0;
    else r <<= i;
    r = ___pdr_trim(r);
  }
  ___pdr_pushunsigned(L, r);
  return 1;
}


static int b_lshift (___pdr_lua_State *L) {
  return b_shift(L, ___pdr_checkunsigned(L, 1), ___pdr_luaL_checkinteger(L, 2));
}


static int b_rshift (___pdr_lua_State *L) {
  return b_shift(L, ___pdr_checkunsigned(L, 1), -___pdr_luaL_checkinteger(L, 2));
}


static int b_arshift (___pdr_lua_State *L) {
  ___pdr_lua_Unsigned r = ___pdr_checkunsigned(L, 1);
  ___pdr_lua_Integer i = ___pdr_luaL_checkinteger(L, 2);
  if (i < 0 || !(r & ((___pdr_lua_Unsigned)1 << (___PDR_LUA_NBITS - 1))))
    return b_shift(L, r, -i);
  else {  /* arithmetic shift for 'negative' number */
    if (i >= ___PDR_LUA_NBITS) r = ___PDR_ALLONES;
    else
      r = ___pdr_trim((r >> i) | ~(___pdr_trim(~(___pdr_lua_Unsigned)0) >> i));  /* add signal bit */
    ___pdr_pushunsigned(L, r);
    return 1;
  }
}


static int b_rot (___pdr_lua_State *L, ___pdr_lua_Integer d) {
  ___pdr_lua_Unsigned r = ___pdr_checkunsigned(L, 1);
  int i = d & (___PDR_LUA_NBITS - 1);  /* i = d % NBITS */
  r = ___pdr_trim(r);
  if (i != 0)  /* avoid undefined shift of LUA_NBITS when i == 0 */
    r = (r << i) | (r >> (___PDR_LUA_NBITS - i));
  ___pdr_pushunsigned(L, ___pdr_trim(r));
  return 1;
}


static int b_lrot (___pdr_lua_State *L) {
  return b_rot(L, ___pdr_luaL_checkinteger(L, 2));
}


static int b_rrot (___pdr_lua_State *L) {
  return b_rot(L, -___pdr_luaL_checkinteger(L, 2));
}


/*
** get field and width arguments for field-manipulation functions,
** checking whether they are valid.
** ('luaL_error' called without 'return' to avoid later warnings about
** 'width' being used uninitialized.)
*/
static int fieldargs (___pdr_lua_State *L, int farg, int *width) {
  ___pdr_lua_Integer f = ___pdr_luaL_checkinteger(L, farg);
  ___pdr_lua_Integer w = ___pdr_luaL_optinteger(L, farg + 1, 1);
  ___pdr_luaL_argcheck(L, 0 <= f, farg, "field cannot be negative");
  ___pdr_luaL_argcheck(L, 0 < w, farg + 1, "width must be positive");
  if (f + w > ___PDR_LUA_NBITS)
    ___pdr_luaL_error(L, "trying to access non-existent bits");
  *width = (int)w;
  return (int)f;
}


static int b_extract (___pdr_lua_State *L) {
  int w;
  ___pdr_lua_Unsigned r = ___pdr_trim(___pdr_checkunsigned(L, 1));
  int f = fieldargs(L, 2, &w);
  r = (r >> f) & ___pdr_mask(w);
  ___pdr_pushunsigned(L, r);
  return 1;
}


static int b_replace (___pdr_lua_State *L) {
  int w;
  ___pdr_lua_Unsigned r = ___pdr_trim(___pdr_checkunsigned(L, 1));
  ___pdr_lua_Unsigned v = ___pdr_trim(___pdr_checkunsigned(L, 2));
  int f = fieldargs(L, 3, &w);
  ___pdr_lua_Unsigned m = ___pdr_mask(w);
  r = (r & ~(m << f)) | ((v & m) << f);
  ___pdr_pushunsigned(L, r);
  return 1;
}


static const ___pdr_luaL_Reg bitlib[] = {
  {"arshift", b_arshift},
  {"band", b_and},
  {"bnot", b_not},
  {"bor", b_or},
  {"bxor", b_xor},
  {"btest", b_test},
  {"extract", b_extract},
  {"lrotate", b_lrot},
  {"lshift", b_lshift},
  {"replace", b_replace},
  {"rrotate", b_rrot},
  {"rshift", b_rshift},
  {NULL, NULL}
};



___PDR_LUAMOD_API int ___pdr_luaopen_bit32 (___pdr_lua_State *L) {
  ___pdr_luaL_newlib(L, bitlib);
  return 1;
}


#else					/* }{ */


___PDR_LUAMOD_API int ___pdr_luaopen_bit32 (___pdr_lua_State *L) {
  return ___pdr_luaL_error(L, "library 'bit32' has been deprecated");
}

#endif					/* } */

} // end NS_PDR_SLUA
