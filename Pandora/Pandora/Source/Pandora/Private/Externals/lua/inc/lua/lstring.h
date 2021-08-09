/*
** $Id: lstring.h,v 1.61 2015/11/03 15:36:01 roberto Exp $
** String table (keep all strings handled by Lua)
** See Copyright Notice in lua.h
*/

#ifndef ___pdr_lstring_h___
#define ___pdr_lstring_h___

#include "lgc.h"
#include "lobject.h"
#include "lstate.h"

#define ___pdr_sizelstring(l)  (sizeof(union ___pdr_UTString) + ((l) + 1) * sizeof(char))

#define ___pdr_sizeludata(l)	(sizeof(union ___pdr_UUdata) + (l))
#define ___pdr_sizeudata(u)	___pdr_sizeludata((u)->len)

#define ___pdr_luaS_newliteral(L, s)	(___pdr_luaS_newlstr(L, "" s, \
                                 (sizeof(s)/sizeof(char))-1))


/*
** test whether a string is a reserved word
*/
#define ___pdr_isreserved(s)	((s)->tt == ___PDR_LUA_TSHRSTR && (s)->extra > 0)


/*
** equality for short strings, which are always internalized
*/
#define ___pdr_eqshrstr(a,b)	___pdr_check_exp((a)->tt == ___PDR_LUA_TSHRSTR, (a) == (b))

namespace NS_PDR_SLUA {

___PDR_LUAI_FUNC unsigned int ___pdr_luaS_hash (const char *str, size_t l, unsigned int seed);
___PDR_LUAI_FUNC unsigned int ___pdr_luaS_hashlongstr (___pdr_TString *ts);
___PDR_LUAI_FUNC int ___pdr_luaS_eqlngstr (___pdr_TString *a, ___pdr_TString *b);
___PDR_LUAI_FUNC void ___pdr_luaS_resize (___pdr_lua_State *L, int newsize);
___PDR_LUAI_FUNC void ___pdr_luaS_clearcache (___pdr_global_State *g);
___PDR_LUAI_FUNC void ___pdr_luaS_init (___pdr_lua_State *L);
___PDR_LUAI_FUNC void ___pdr_luaS_remove (___pdr_lua_State *L, ___pdr_TString *ts);
___PDR_LUAI_FUNC ___pdr_Udata *___pdr_luaS_newudata (___pdr_lua_State *L, size_t s);
___PDR_LUAI_FUNC ___pdr_TString *___pdr_luaS_newlstr (___pdr_lua_State *L, const char *str, size_t l);
___PDR_LUAI_FUNC ___pdr_TString *___pdr_luaS_new (___pdr_lua_State *L, const char *str);
___PDR_LUAI_FUNC ___pdr_TString *___pdr_luaS_createlngstrobj (___pdr_lua_State *L, size_t l);

} // end NS_PDR_SLUA

#endif
