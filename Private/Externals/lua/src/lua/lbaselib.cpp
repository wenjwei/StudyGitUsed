/*
** $Id: lbaselib.c,v 1.314 2016/09/05 19:06:34 roberto Exp $
** Basic library
** See Copyright Notice in lua.h
*/

#define ___pdr_lbaselib_c
#define ___PDR_LUA_LIB

#include "lprefix.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

namespace NS_PDR_SLUA {

static int luaB_print (___pdr_lua_State *L) {
  int n = ___pdr_lua_gettop(L);  /* number of arguments */
  int i;
  ___pdr_lua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    const char *s;
    size_t l;
    ___pdr_lua_pushvalue(L, -1);  /* function to be called */
    ___pdr_lua_pushvalue(L, i);   /* value to print */
    lua_call(L, 1, 1);
    s = ___pdr_lua_tolstring(L, -1, &l);  /* get result */
    if (s == NULL)
      return ___pdr_luaL_error(L, "'tostring' must return a string to 'print'");
    if (i>1) ___pdr_lua_writestring("\t", 1);
    ___pdr_lua_writestring(s, l);
    ___pdr_lua_pop(L, 1);  /* pop result */
  }
  ___pdr_lua_writeline();
  return 0;
}


#define ___PDR_SPACECHARS	" \f\n\r\t\v"

static const char *b_str2int (const char *s, int base, ___pdr_lua_Integer *pn) {
  ___pdr_lua_Unsigned n = 0;
  int neg = 0;
  s += strspn(s, ___PDR_SPACECHARS);  /* skip initial spaces */
  if (*s == '-') { s++; neg = 1; }  /* handle signal */
  else if (*s == '+') s++;
  if (!isalnum((unsigned char)*s))  /* no digit? */
    return NULL;
  do {
    int digit = (isdigit((unsigned char)*s)) ? *s - '0'
                   : (toupper((unsigned char)*s) - 'A') + 10;
    if (digit >= base) return NULL;  /* invalid numeral */
    n = n * base + digit;
    s++;
  } while (isalnum((unsigned char)*s));
  s += strspn(s, ___PDR_SPACECHARS);  /* skip trailing spaces */
  *pn = (___pdr_lua_Integer)((neg) ? (0u - n) : n);
  return s;
}


static int luaB_tonumber (___pdr_lua_State *L) {
  if (___pdr_lua_isnoneornil(L, 2)) {  /* standard conversion? */
    ___pdr_luaL_checkany(L, 1);
    if (___pdr_lua_type(L, 1) == ___PDR_LUA_TNUMBER) {  /* already a number? */
      ___pdr_lua_settop(L, 1);  /* yes; return it */
      return 1;
    }
    else {
      size_t l;
      const char *s = ___pdr_lua_tolstring(L, 1, &l);
      if (s != NULL && ___pdr_lua_stringtonumber(L, s) == l + 1)
        return 1;  /* successful conversion to number */
      /* else not a number */
    }
  }
  else {
    size_t l;
    const char *s;
    ___pdr_lua_Integer n = 0;  /* to avoid warnings */
    ___pdr_lua_Integer base = ___pdr_luaL_checkinteger(L, 2);
    ___pdr_luaL_checktype(L, 1, ___PDR_LUA_TSTRING);  /* no numbers as strings */
    s = ___pdr_lua_tolstring(L, 1, &l);
    ___pdr_luaL_argcheck(L, 2 <= base && base <= 36, 2, "base out of range");
    if (b_str2int(s, (int)base, &n) == s + l) {
      ___pdr_lua_pushinteger(L, n);
      return 1;
    }  /* else not a number */
  }  /* else not a number */
  ___pdr_lua_pushnil(L);  /* not a number */
  return 1;
}


static int luaB_error (___pdr_lua_State *L) {
  int level = (int)___pdr_luaL_optinteger(L, 2, 1);
  ___pdr_lua_settop(L, 1);
  if (___pdr_lua_type(L, 1) == ___PDR_LUA_TSTRING && level > 0) {
    ___pdr_luaL_where(L, level);   /* add extra information */
    ___pdr_lua_pushvalue(L, 1);
    ___pdr_lua_concat(L, 2);
  }
  return ___pdr_lua_error(L);
}


static int luaB_getmetatable (___pdr_lua_State *L) {
  ___pdr_luaL_checkany(L, 1);
  if (!___pdr_lua_getmetatable(L, 1)) {
    ___pdr_lua_pushnil(L);
    return 1;  /* no metatable */
  }
  ___pdr_luaL_getmetafield(L, 1, "__metatable");
  return 1;  /* returns either __metatable field (if present) or metatable */
}


static int luaB_setmetatable (___pdr_lua_State *L) {
  int t = ___pdr_lua_type(L, 2);
  ___pdr_luaL_checktype(L, 1, ___PDR_LUA_TTABLE);
  ___pdr_luaL_argcheck(L, t == ___PDR_LUA_TNIL || t == ___PDR_LUA_TTABLE, 2,
                    "nil or table expected");
  if (___pdr_luaL_getmetafield(L, 1, "__metatable") != ___PDR_LUA_TNIL)
    return ___pdr_luaL_error(L, "cannot change a protected metatable");
  ___pdr_lua_settop(L, 2);
  ___pdr_lua_setmetatable(L, 1);
  return 1;
}


static int luaB_rawequal (___pdr_lua_State *L) {
  ___pdr_luaL_checkany(L, 1);
  ___pdr_luaL_checkany(L, 2);
  ___pdr_lua_pushboolean(L, ___pdr_lua_rawequal(L, 1, 2));
  return 1;
}


static int luaB_rawlen (___pdr_lua_State *L) {
  int t = ___pdr_lua_type(L, 1);
  ___pdr_luaL_argcheck(L, t == ___PDR_LUA_TTABLE || t == ___PDR_LUA_TSTRING, 1,
                   "table or string expected");
  ___pdr_lua_pushinteger(L, ___pdr_lua_rawlen(L, 1));
  return 1;
}


static int luaB_rawget (___pdr_lua_State *L) {
  ___pdr_luaL_checktype(L, 1, ___PDR_LUA_TTABLE);
  ___pdr_luaL_checkany(L, 2);
  ___pdr_lua_settop(L, 2);
  ___pdr_lua_rawget(L, 1);
  return 1;
}

static int luaB_rawset (___pdr_lua_State *L) {
  ___pdr_luaL_checktype(L, 1, ___PDR_LUA_TTABLE);
  ___pdr_luaL_checkany(L, 2);
  ___pdr_luaL_checkany(L, 3);
  ___pdr_lua_settop(L, 3);
  ___pdr_lua_rawset(L, 1);
  return 1;
}


static int luaB_collectgarbage (___pdr_lua_State *L) {
  static const char *const opts[] = {"stop", "restart", "collect",
    "count", "step", "setpause", "setstepmul",
    "isrunning", NULL};
  static const int optsnum[] = {___PDR_LUA_GCSTOP, ___PDR_LUA_GCRESTART, ___PDR_LUA_GCCOLLECT,
    ___PDR_LUA_GCCOUNT, ___PDR_LUA_GCSTEP, ___PDR_LUA_GCSETPAUSE, ___PDR_LUA_GCSETSTEPMUL,
    ___PDR_LUA_GCISRUNNING};
  int o = optsnum[___pdr_luaL_checkoption(L, 1, "collect", opts)];
  int ex = (int)___pdr_luaL_optinteger(L, 2, 0);
  int res = ___pdr_lua_gc(L, o, ex);
  switch (o) {
    case ___PDR_LUA_GCCOUNT: {
      int b = ___pdr_lua_gc(L, ___PDR_LUA_GCCOUNTB, 0);
      ___pdr_lua_pushnumber(L, (___pdr_lua_Number)res + ((___pdr_lua_Number)b/1024));
      return 1;
    }
    case ___PDR_LUA_GCSTEP: case ___PDR_LUA_GCISRUNNING: {
      ___pdr_lua_pushboolean(L, res);
      return 1;
    }
    default: {
      ___pdr_lua_pushinteger(L, res);
      return 1;
    }
  }
}


static int luaB_type (___pdr_lua_State *L) {
  int t = ___pdr_lua_type(L, 1);
  ___pdr_luaL_argcheck(L, t != ___PDR_LUA_TNONE, 1, "value expected");
  ___pdr_lua_pushstring(L, ___pdr_lua_typename(L, t));
  return 1;
}


static int pairsmeta (___pdr_lua_State *L, const char *method, int iszero,
                      ___pdr_lua_CFunction iter) {
  ___pdr_luaL_checkany(L, 1);
  if (___pdr_luaL_getmetafield(L, 1, method) == ___PDR_LUA_TNIL) {  /* no metamethod? */
    ___pdr_lua_pushcfunction(L, iter);  /* will return generator, */
    ___pdr_lua_pushvalue(L, 1);  /* state, */
    if (iszero) ___pdr_lua_pushinteger(L, 0);  /* and initial value */
    else ___pdr_lua_pushnil(L);
  }
  else {
    ___pdr_lua_pushvalue(L, 1);  /* argument 'self' to metamethod */
    lua_call(L, 1, 3);  /* get 3 values from metamethod */
  }
  return 3;
}


static int luaB_next (___pdr_lua_State *L) {
  ___pdr_luaL_checktype(L, 1, ___PDR_LUA_TTABLE);
  ___pdr_lua_settop(L, 2);  /* create a 2nd argument if there isn't one */
  if (___pdr_lua_next(L, 1))
    return 2;
  else {
    ___pdr_lua_pushnil(L);
    return 1;
  }
}


static int luaB_pairs (___pdr_lua_State *L) {
  return pairsmeta(L, "__pairs", 0, luaB_next);
}


/*
** Traversal function for 'ipairs'
*/
static int ipairsaux (___pdr_lua_State *L) {
  ___pdr_lua_Integer i = ___pdr_luaL_checkinteger(L, 2) + 1;
  ___pdr_lua_pushinteger(L, i);
  return (___pdr_lua_geti(L, 1, i) == ___PDR_LUA_TNIL) ? 1 : 2;
}


/*
** 'ipairs' function. Returns 'ipairsaux', given "table", 0.
** (The given "table" may not be a table.)
*/
static int luaB_ipairs (___pdr_lua_State *L) {
#if defined(___PDR_LUA_COMPAT_IPAIRS)
  return pairsmeta(L, "__ipairs", 1, ipairsaux);
#else
  ___pdr_luaL_checkany(L, 1);
  ___pdr_lua_pushcfunction(L, ipairsaux);  /* iteration function */
  ___pdr_lua_pushvalue(L, 1);  /* state */
  ___pdr_lua_pushinteger(L, 0);  /* initial value */
  return 3;
#endif
}


static int load_aux (___pdr_lua_State *L, int status, int envidx) {
  if (status == ___PDR_LUA_OK) {
    if (envidx != 0) {  /* 'env' parameter? */
      ___pdr_lua_pushvalue(L, envidx);  /* environment for loaded function */
      if (!___pdr_lua_setupvalue(L, -2, 1))  /* set it as 1st upvalue */
        ___pdr_lua_pop(L, 1);  /* remove 'env' if not used by previous call */
    }
    return 1;
  }
  else {  /* error (message is on top of the stack) */
    ___pdr_lua_pushnil(L);
    ___pdr_lua_insert(L, -2);  /* put before error message */
    return 2;  /* return nil plus error message */
  }
}


static int luaB_loadfile (___pdr_lua_State *L) {
  const char *fname = ___pdr_luaL_optstring(L, 1, NULL);
  const char *mode = ___pdr_luaL_optstring(L, 2, NULL);
  int env = (!___pdr_lua_isnone(L, 3) ? 3 : 0);  /* 'env' index or 0 if no 'env' */
  int status = ___pdr_luaL_loadfilex(L, fname, mode);
  return load_aux(L, status, env);
}


/*
** {======================================================
** Generic Read function
** =======================================================
*/


/*
** reserved slot, above all arguments, to hold a copy of the returned
** string to avoid it being collected while parsed. 'load' has four
** optional arguments (chunk, source name, mode, and environment).
*/
#define ___PDR_RESERVEDSLOT	5


/*
** Reader for generic 'load' function: 'lua_load' uses the
** stack for internal stuff, so the reader cannot change the
** stack top. Instead, it keeps its resulting string in a
** reserved slot inside the stack.
*/
static const char *generic_reader (___pdr_lua_State *L, void *ud, size_t *size) {
  (void)(ud);  /* not used */
  ___pdr_luaL_checkstack(L, 2, "too many nested functions");
  ___pdr_lua_pushvalue(L, 1);  /* get function */
  lua_call(L, 0, 1);  /* call it */
  if (___pdr_lua_isnil(L, -1)) {
    ___pdr_lua_pop(L, 1);  /* pop result */
    *size = 0;
    return NULL;
  }
  else if (!___pdr_lua_isstring(L, -1))
    ___pdr_luaL_error(L, "reader function must return a string");
  ___pdr_lua_replace(L, ___PDR_RESERVEDSLOT);  /* save string in reserved slot */
  return ___pdr_lua_tolstring(L, ___PDR_RESERVEDSLOT, size);
}


static int luaB_load (___pdr_lua_State *L) {
  int status;
  size_t l;
  const char *s = ___pdr_lua_tolstring(L, 1, &l);
  const char *mode = ___pdr_luaL_optstring(L, 3, "bt");
  int env = (!___pdr_lua_isnone(L, 4) ? 4 : 0);  /* 'env' index or 0 if no 'env' */
  if (s != NULL) {  /* loading a string? */
    const char *chunkname = ___pdr_luaL_optstring(L, 2, s);
    status = ___pdr_luaL_loadbufferx(L, s, l, chunkname, mode);
  }
  else {  /* loading from a reader function */
    const char *chunkname = ___pdr_luaL_optstring(L, 2, "=(load)");
    ___pdr_luaL_checktype(L, 1, ___PDR_LUA_TFUNCTION);
    ___pdr_lua_settop(L, ___PDR_RESERVEDSLOT);  /* create reserved slot */
    status = lua_load(L, generic_reader, NULL, chunkname, mode);
  }
  return load_aux(L, status, env);
}

/* }====================================================== */


static int dofilecont (___pdr_lua_State *L, int d1, ___pdr_lua_KContext d2) {
  (void)d1;  (void)d2;  /* only to match 'lua_Kfunction' prototype */
  return ___pdr_lua_gettop(L) - 1;
}


static int luaB_dofile (___pdr_lua_State *L) {
  const char *fname = ___pdr_luaL_optstring(L, 1, NULL);
  ___pdr_lua_settop(L, 1);
  if (___pdr_luaL_loadfile(L, fname) != ___PDR_LUA_OK)
    return ___pdr_lua_error(L);
  ___pdr_lua_callk(L, 0, ___PDR_LUA_MULTRET, 0, dofilecont);
  return dofilecont(L, 0, 0);
}


static int luaB_assert (___pdr_lua_State *L) {
  if (___pdr_lua_toboolean(L, 1))  /* condition is true? */
    return ___pdr_lua_gettop(L);  /* return all arguments */
  else {  /* error */
    ___pdr_luaL_checkany(L, 1);  /* there must be a condition */
    ___pdr_lua_remove(L, 1);  /* remove it */
    ___pdr_lua_pushliteral(L, "assertion failed!");  /* default message */
    ___pdr_lua_settop(L, 1);  /* leave only message (default if no other one) */
    return luaB_error(L);  /* call 'error' */
  }
}


static int luaB_select (___pdr_lua_State *L) {
  int n = ___pdr_lua_gettop(L);
  if (___pdr_lua_type(L, 1) == ___PDR_LUA_TSTRING && *___pdr_lua_tostring(L, 1) == '#') {
    ___pdr_lua_pushinteger(L, n-1);
    return 1;
  }
  else {
    ___pdr_lua_Integer i = ___pdr_luaL_checkinteger(L, 1);
    if (i < 0) i = n + i;
    else if (i > n) i = n;
    ___pdr_luaL_argcheck(L, 1 <= i, 1, "index out of range");
    return n - (int)i;
  }
}


/*
** Continuation function for 'pcall' and 'xpcall'. Both functions
** already pushed a 'true' before doing the call, so in case of success
** 'finishpcall' only has to return everything in the stack minus
** 'extra' values (where 'extra' is exactly the number of items to be
** ignored).
*/
static int finishpcall (___pdr_lua_State *L, int status, ___pdr_lua_KContext extra) {
  if (status != ___PDR_LUA_OK && status != ___PDR_LUA_YIELD) {  /* error? */
    ___pdr_lua_pushboolean(L, 0);  /* first result (false) */
    ___pdr_lua_pushvalue(L, -2);  /* error message */
    return 2;  /* return false, msg */
  }
  else
    return ___pdr_lua_gettop(L) - (int)extra;  /* return all results */
}


static int luaB_pcall (___pdr_lua_State *L) {
  int status;
  ___pdr_luaL_checkany(L, 1);
  ___pdr_lua_pushboolean(L, 1);  /* first result if no errors */
  ___pdr_lua_insert(L, 1);  /* put it in place */
  status = ___pdr_lua_pcallk(L, ___pdr_lua_gettop(L) - 2, ___PDR_LUA_MULTRET, 0, 0, finishpcall);
  return finishpcall(L, status, 0);
}


/*
** Do a protected call with error handling. After 'lua_rotate', the
** stack will have <f, err, true, f, [args...]>; so, the function passes
** 2 to 'finishpcall' to skip the 2 first values when returning results.
*/
static int luaB_xpcall (___pdr_lua_State *L) {
  int status;
  int n = ___pdr_lua_gettop(L);
  ___pdr_luaL_checktype(L, 2, ___PDR_LUA_TFUNCTION);  /* check error function */
  ___pdr_lua_pushboolean(L, 1);  /* first result */
  ___pdr_lua_pushvalue(L, 1);  /* function */
  ___pdr_lua_rotate(L, 3, 2);  /* move them below function's arguments */
  status = ___pdr_lua_pcallk(L, n - 2, ___PDR_LUA_MULTRET, 2, 2, finishpcall);
  return finishpcall(L, status, 2);
}


static int luaB_tostring (___pdr_lua_State *L) {
  ___pdr_luaL_checkany(L, 1);
  ___pdr_luaL_tolstring(L, 1, NULL);
  return 1;
}


static const ___pdr_luaL_Reg base_funcs[] = {
  {"assert", luaB_assert},
  {"collectgarbage", luaB_collectgarbage},
  {"dofile", luaB_dofile},
  {"error", luaB_error},
  {"getmetatable", luaB_getmetatable},
  {"ipairs", luaB_ipairs},
  {"loadfile", luaB_loadfile},
  {"load", luaB_load},
#if defined(___PDR_LUA_COMPAT_LOADSTRING)
  {"loadstring", luaB_load},
#endif
  {"next", luaB_next},
  {"pairs", luaB_pairs},
  {"pcall", luaB_pcall},
  {"print", luaB_print},
  {"rawequal", luaB_rawequal},
  {"rawlen", luaB_rawlen},
  {"rawget", luaB_rawget},
  {"rawset", luaB_rawset},
  {"select", luaB_select},
  {"setmetatable", luaB_setmetatable},
  {"tonumber", luaB_tonumber},
  {"tostring", luaB_tostring},
  {"type", luaB_type},
  {"xpcall", luaB_xpcall},
  /* placeholders */
  {"_G", NULL},
  {"_VERSION", NULL},
  {NULL, NULL}
};


___PDR_LUAMOD_API int ___pdr_luaopen_base (___pdr_lua_State *L) {
  /* open lib into global table */
  ___pdr_lua_pushglobaltable(L);
  ___pdr_luaL_setfuncs(L, base_funcs, 0);
  /* set global _G */
  ___pdr_lua_pushvalue(L, -1);
  ___pdr_lua_setfield(L, -2, "_G");
  /* set global _VERSION */
  ___pdr_lua_pushliteral(L, ___PDR_LUA_VERSION);
  ___pdr_lua_setfield(L, -2, "_VERSION");
  return 1;
}

} // end NS_PDR_SLUA