/*
** $Id: ltable.h,v 2.23 2016/12/22 13:08:50 roberto Exp $
** Lua tables (hash)
** See Copyright Notice in lua.h
*/

#ifndef ___pdr_ltable_h___
#define ___pdr_ltable_h___

#include "lobject.h"

namespace NS_PDR_SLUA {

#define ___pdr_gnode(t,i)	(&(t)->node[i])
#define ___pdr_gval(n)		(&(n)->i_val)
#define ___pdr_gnext(n)	((n)->i_key.nk.next)


/* 'const' to avoid wrong writings that can mess up field 'next' */
#define ___pdr_gkey(n)		___pdr_cast(const ___pdr_TValue*, (&(n)->i_key.tvk))

/*
** writable version of 'gkey'; allows updates to individual fields,
** but not to the whole (which has incompatible type)
*/
#define ___pdr_wgkey(n)		(&(n)->i_key.nk)

#define ___pdr_invalidateTMcache(t)	((t)->flags = 0)


/* true when 't' is using 'dummynode' as its hash part */
#define ___pdr_isdummy(t)		((t)->lastfree == NULL)


/* allocated size for hash nodes */
#define ___pdr_allocsizenode(t)	(___pdr_isdummy(t) ? 0 : ___pdr_sizenode(t))


/* returns the key, given the value of a table entry */
#define ___pdr_keyfromval(v) \
  (___pdr_gkey(___pdr_cast(___pdr_Node *, ___pdr_cast(char *, (v)) - offsetof(___pdr_Node, i_val))))


___PDR_LUAI_FUNC const ___pdr_TValue *___pdr_luaH_getint (___pdr_Table *t, ___pdr_lua_Integer key);
___PDR_LUAI_FUNC void ___pdr_luaH_setint (___pdr_lua_State *L, ___pdr_Table *t, ___pdr_lua_Integer key,
                                                    ___pdr_TValue *value);
___PDR_LUAI_FUNC const ___pdr_TValue *___pdr_luaH_getshortstr (___pdr_Table *t, ___pdr_TString *key);
___PDR_LUAI_FUNC const ___pdr_TValue *___pdr_luaH_getstr (___pdr_Table *t, ___pdr_TString *key);
___PDR_LUAI_FUNC const ___pdr_TValue *___pdr_luaH_get (___pdr_Table *t, const ___pdr_TValue *key);
___PDR_LUAI_FUNC ___pdr_TValue *___pdr_luaH_newkey (___pdr_lua_State *L, ___pdr_Table *t, const ___pdr_TValue *key);
___PDR_LUAI_FUNC ___pdr_TValue *___pdr_luaH_set (___pdr_lua_State *L, ___pdr_Table *t, const ___pdr_TValue *key);
___PDR_LUAI_FUNC ___pdr_Table *___pdr_luaH_new (___pdr_lua_State *L);
___PDR_LUAI_FUNC void ___pdr_luaH_resize (___pdr_lua_State *L, ___pdr_Table *t, unsigned int nasize,
                                                    unsigned int nhsize);
___PDR_LUAI_FUNC void ___pdr_luaH_resizearray (___pdr_lua_State *L, ___pdr_Table *t, unsigned int nasize);
___PDR_LUAI_FUNC void ___pdr_luaH_free (___pdr_lua_State *L, ___pdr_Table *t);
___PDR_LUAI_FUNC int ___pdr_luaH_next (___pdr_lua_State *L, ___pdr_Table *t, ___pdr_StkId key);
___PDR_LUAI_FUNC int ___pdr_luaH_getn (___pdr_Table *t);


#if defined(___PDR_LUA_DEBUG)
___PDR_LUAI_FUNC ___pdr_Node *___pdr_luaH_mainposition (const ___pdr_Table *t, const ___pdr_TValue *key);
___PDR_LUAI_FUNC int ___pdr_luaH_isdummy (const ___pdr_Table *t);
#endif

} // end NS_PDR_SLUA

#endif
