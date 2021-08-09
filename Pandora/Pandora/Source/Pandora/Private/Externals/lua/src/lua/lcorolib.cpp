/*
** $Id: lcorolib.c,v 1.10 2016/04/11 19:19:55 roberto Exp $
** Coroutine Library
** See Copyright Notice in lua.h
*/

#define ___pdr_lcorolib_c
#define ___PDR_LUA_LIB

#include "lprefix.h"

#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

namespace NS_PDR_SLUA {

static ___pdr_lua_State *getco (___pdr_lua_State *L) {
  ___pdr_lua_State *co = ___pdr_lua_tothread(L, 1);
  ___pdr_luaL_argcheck(L, co, 1, "thread expected");
  return co;
}


static int auxresume (___pdr_lua_State *L, ___pdr_lua_State *co, int narg) {
  int status;
  if (!___pdr_lua_checkstack(co, narg)) {
    ___pdr_lua_pushliteral(L, "too many arguments to resume");
    return -1;  /* error flag */
  }
  if (___pdr_lua_status(co) == ___PDR_LUA_OK && ___pdr_lua_gettop(co) == 0) {
    ___pdr_lua_pushliteral(L, "cannot resume dead coroutine");
    return -1;  /* error flag */
  }
  ___pdr_lua_xmove(L, co, narg);
  status = ___pdr_lua_resume(co, L, narg);
  if (status == ___PDR_LUA_OK || status == ___PDR_LUA_YIELD) {
    int nres = ___pdr_lua_gettop(co);
    if (!___pdr_lua_checkstack(L, nres + 1)) {
      ___pdr_lua_pop(co, nres);  /* remove results anyway */
      ___pdr_lua_pushliteral(L, "too many results to resume");
      return -1;  /* error flag */
    }
    ___pdr_lua_xmove(co, L, nres);  /* move yielded values */
    return nres;
  }
  else {
    ___pdr_lua_xmove(co, L, 1);  /* move error message */
    return -1;  /* error flag */
  }
}


static int luaB_coresume (___pdr_lua_State *L) {
  ___pdr_lua_State *co = getco(L);
  int r;
  r = auxresume(L, co, ___pdr_lua_gettop(L) - 1);
  if (r < 0) {
    ___pdr_lua_pushboolean(L, 0);
    ___pdr_lua_insert(L, -2);
    return 2;  /* return false + error message */
  }
  else {
    ___pdr_lua_pushboolean(L, 1);
    ___pdr_lua_insert(L, -(r + 1));
    return r + 1;  /* return true + 'resume' returns */
  }
}


static int luaB_auxwrap (___pdr_lua_State *L) {
  ___pdr_lua_State *co = ___pdr_lua_tothread(L, ___pdr_lua_upvalueindex(1));
  int r = auxresume(L, co, ___pdr_lua_gettop(L));
  if (r < 0) {
    if (___pdr_lua_type(L, -1) == ___PDR_LUA_TSTRING) {  /* error object is a string? */
      ___pdr_luaL_where(L, 1);  /* add extra info */
      ___pdr_lua_insert(L, -2);
      ___pdr_lua_concat(L, 2);
    }
    return ___pdr_lua_error(L);  /* propagate error */
  }
  return r;
}


static int luaB_cocreate (___pdr_lua_State *L) {
  ___pdr_lua_State *NL;
  ___pdr_luaL_checktype(L, 1, ___PDR_LUA_TFUNCTION);
  NL = ___pdr_lua_newthread(L);
  ___pdr_lua_pushvalue(L, 1);  /* move function to top */
  ___pdr_lua_xmove(L, NL, 1);  /* move function from L to NL */
  return 1;
}


static int luaB_cowrap (___pdr_lua_State *L) {
  luaB_cocreate(L);
  ___pdr_lua_pushcclosure(L, luaB_auxwrap, 1);
  return 1;
}


static int luaB_yield (___pdr_lua_State *L) {
  return ___pdr_lua_yield(L, ___pdr_lua_gettop(L));
}


static int luaB_costatus (___pdr_lua_State *L) {
  ___pdr_lua_State *co = getco(L);
  if (L == co) ___pdr_lua_pushliteral(L, "running");
  else {
    switch (___pdr_lua_status(co)) {
      case ___PDR_LUA_YIELD:
        ___pdr_lua_pushliteral(L, "suspended");
        break;
      case ___PDR_LUA_OK: {
        ___pdr_lua_Debug ar;
        if (___pdr_lua_getstack(co, 0, &ar) > 0)  /* does it have frames? */
          ___pdr_lua_pushliteral(L, "normal");  /* it is running */
        else if (___pdr_lua_gettop(co) == 0)
            ___pdr_lua_pushliteral(L, "dead");
        else
          ___pdr_lua_pushliteral(L, "suspended");  /* initial state */
        break;
      }
      default:  /* some error occurred */
        ___pdr_lua_pushliteral(L, "dead");
        break;
    }
  }
  return 1;
}


static int luaB_yieldable (___pdr_lua_State *L) {
  ___pdr_lua_pushboolean(L, ___pdr_lua_isyieldable(L));
  return 1;
}


static int luaB_corunning (___pdr_lua_State *L) {
  int ismain = ___pdr_lua_pushthread(L);
  ___pdr_lua_pushboolean(L, ismain);
  return 2;
}


static const ___pdr_luaL_Reg co_funcs[] = {
  {"create", luaB_cocreate},
  {"resume", luaB_coresume},
  {"running", luaB_corunning},
  {"status", luaB_costatus},
  {"wrap", luaB_cowrap},
  {"yield", luaB_yield},
  {"isyieldable", luaB_yieldable},
  {NULL, NULL}
};



___PDR_LUAMOD_API int ___pdr_luaopen_coroutine (___pdr_lua_State *L) {
  ___pdr_luaL_newlib(L, co_funcs);
  return 1;
}

} // end NS_PDR_SLUA