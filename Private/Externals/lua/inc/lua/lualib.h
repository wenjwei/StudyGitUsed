/*
** $Id: lualib.h,v 1.45 2017/01/12 17:14:26 roberto Exp $
** Lua standard libraries
** See Copyright Notice in lua.h
*/


#ifndef ___pdr_lualib_h___
#define ___pdr_lualib_h___

#include "lua.h"

namespace NS_PDR_SLUA {

/* version suffix for environment variable names */
#define ___PDR_LUA_VERSUFFIX          "_" ___PDR_LUA_VERSION_MAJOR "_" ___PDR_LUA_VERSION_MINOR


___PDR_LUAMOD_API int (___pdr_luaopen_base) (___pdr_lua_State *L);

#define ___PDR_LUA_COLIBNAME	"coroutine"
___PDR_LUAMOD_API int (___pdr_luaopen_coroutine) (___pdr_lua_State *L);

#define ___PDR_LUA_TABLIBNAME	"table"
___PDR_LUAMOD_API int (___pdr_luaopen_table) (___pdr_lua_State *L);

#define ___PDR_LUA_IOLIBNAME	"io"
___PDR_LUAMOD_API int (___pdr_luaopen_io) (___pdr_lua_State *L);

#define ___PDR_LUA_OSLIBNAME	"os"
___PDR_LUAMOD_API int (___pdr_luaopen_os) (___pdr_lua_State *L);

#define ___PDR_LUA_STRLIBNAME	"string"
___PDR_LUAMOD_API int (___pdr_luaopen_string) (___pdr_lua_State *L);

#define ___PDR_LUA_UTF8LIBNAME	"utf8"
___PDR_LUAMOD_API int (___pdr_luaopen_utf8) (___pdr_lua_State *L);

#define ___PDR_LUA_BITLIBNAME	"bit32"
___PDR_LUAMOD_API int (___pdr_luaopen_bit32) (___pdr_lua_State *L);

#define ___PDR_LUA_MATHLIBNAME	"math"
___PDR_LUAMOD_API int (___pdr_luaopen_math) (___pdr_lua_State *L);

#define ___PDR_LUA_DBLIBNAME	"debug"
___PDR_LUAMOD_API int (___pdr_luaopen_debug) (___pdr_lua_State *L);

#define ___PDR_LUA_LOADLIBNAME	"package"
___PDR_LUAMOD_API int (___pdr_luaopen_package) (___pdr_lua_State *L);


/* open all previous libraries */
___PDR_LUALIB_API void (___pdr_luaL_openlibs) (___pdr_lua_State *L);



#if !defined(___pdr_lua_assert)
#define ___pdr_lua_assert(x)	((void)0)
#endif

} // end NS_PDR_SLUA

#endif
