/*
** $Id: linit.c,v 1.39 2016/12/04 20:17:24 roberto Exp $
** Initialization of libraries for lua.c and other clients
** See Copyright Notice in lua.h
*/


#define ___pdr_linit_c
#define ___PDR_LUA_LIB

/*
** If you embed Lua in your program and need to open the standard
** libraries, call luaL_openlibs in your program. If you need a
** different set of libraries, copy this file to your project and edit
** it to suit your needs.
**
** You can also *preload* libraries, so that a later 'require' can
** open the library, which is already linked to the application.
** For that, do the following code:
**
**  luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
**  lua_pushcfunction(L, luaopen_modname);
**  lua_setfield(L, -2, modname);
**  lua_pop(L, 1);  // remove PRELOAD table
*/

#include "lprefix.h"

#include <stddef.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

namespace NS_PDR_SLUA {

/*
** these libs are loaded by lua.c and are readily available to any Lua
** program
*/
static const ___pdr_luaL_Reg loadedlibs[] = {
  {"_G", ___pdr_luaopen_base},
  {___PDR_LUA_LOADLIBNAME, ___pdr_luaopen_package},
  {___PDR_LUA_COLIBNAME, ___pdr_luaopen_coroutine},
  {___PDR_LUA_TABLIBNAME, ___pdr_luaopen_table},
  {___PDR_LUA_IOLIBNAME, ___pdr_luaopen_io},
  {___PDR_LUA_OSLIBNAME, ___pdr_luaopen_os},
  {___PDR_LUA_STRLIBNAME, ___pdr_luaopen_string},
  {___PDR_LUA_MATHLIBNAME, ___pdr_luaopen_math},
  {___PDR_LUA_UTF8LIBNAME, ___pdr_luaopen_utf8},
  {___PDR_LUA_DBLIBNAME, ___pdr_luaopen_debug},
#if defined(___PDR_LUA_COMPAT_BITLIB)
  {___PDR_LUA_BITLIBNAME, ___pdr_luaopen_bit32},
#endif
  {NULL, NULL}
};


___PDR_LUALIB_API void ___pdr_luaL_openlibs (___pdr_lua_State *L) {
  const ___pdr_luaL_Reg *lib;
  /* "require" functions from 'loadedlibs' and set results to global table */
  for (lib = loadedlibs; lib->func; lib++) {
    ___pdr_luaL_requiref(L, lib->name, lib->func, 1);
    ___pdr_lua_pop(L, 1);  /* remove lib */
  }
}

} // end NS_PDR_SLUA