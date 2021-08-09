/*
** $Id: lundump.h,v 1.45 2015/09/08 15:41:05 roberto Exp $
** load precompiled Lua chunks
** See Copyright Notice in lua.h
*/

#ifndef ___pdr_lundump_h___
#define ___pdr_lundump_h___

#include "llimits.h"
#include "lobject.h"
#include "lzio.h"

namespace NS_PDR_SLUA {

/* data to catch conversion errors */
#define ___PDR_LUAC_DATA	"\x19\x93\r\n\x1a\n"

#define ___PDR_LUAC_INT	0x5678
#define ___PDR_LUAC_NUM	___pdr_cast_num(370.5)

#define ___PDR_MYINT(s)	(s[0]-'0')
#define ___PDR_LUAC_VERSION	(___PDR_MYINT(___PDR_LUA_VERSION_MAJOR)*16+___PDR_MYINT(___PDR_LUA_VERSION_MINOR))
#define ___PDR_LUAC_FORMAT	0	/* this is the official format */

/* load one chunk; from lundump.c */
___PDR_LUAI_FUNC ___pdr_LClosure* ___pdr_luaU_undump (___pdr_lua_State* L, ___pdr_ZIO* Z, const char* name);

#ifdef ___PDR_LUAC
/* dump one chunk; from ldump.c */
___PDR_LUAI_FUNC int ___pdr_luaU_dump (___pdr_lua_State* L, const ___pdr_Proto* f, ___pdr_lua_Writer w, void* data, int strip);
#endif // end ___PDR_LUAC

} // end NS_PDR_SLUA

#endif
