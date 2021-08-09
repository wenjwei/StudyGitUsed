/*
** $Id: lstring.c,v 2.56 2015/11/23 11:32:51 roberto Exp $
** String table (keeps all strings handled by Lua)
** See Copyright Notice in lua.h
*/

#define ___pdr_lstring_c
#define ___PDR_LUA_CORE

#include "lstring.h"
#include "lprefix.h"

#include <string.h>

#include "lua.h"
#include "ldebug.h"
#include "ldo.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"

namespace NS_PDR_SLUA {

#define ___PDR_MEMERRMSG       "not enough memory"


/*
** Lua will use at most ~(2^LUAI_HASHLIMIT) bytes from a string to
** compute its hash
*/
#if !defined(___PDR_LUAI_HASHLIMIT)
#define ___PDR_LUAI_HASHLIMIT		5
#endif


/*
** equality for long strings
*/
int ___pdr_luaS_eqlngstr (___pdr_TString *a, ___pdr_TString *b) {
  size_t len = a->u.lnglen;
  ___pdr_lua_assert(a->tt == ___PDR_LUA_TLNGSTR && b->tt == ___PDR_LUA_TLNGSTR);
  return (a == b) ||  /* same instance or... */
    ((len == b->u.lnglen) &&  /* equal length and ... */
     (memcmp(___pdr_getstr(a), ___pdr_getstr(b), len) == 0));  /* equal contents */
}


unsigned int ___pdr_luaS_hash (const char *str, size_t l, unsigned int seed) {
  unsigned int h = seed ^ ___pdr_cast(unsigned int, l);
  size_t step = (l >> ___PDR_LUAI_HASHLIMIT) + 1;
  for (; l >= step; l -= step)
    h ^= ((h<<5) + (h>>2) + ___pdr_cast_byte(str[l - 1]));
  return h;
}


unsigned int ___pdr_luaS_hashlongstr (___pdr_TString *ts) {
  ___pdr_lua_assert(ts->tt == ___PDR_LUA_TLNGSTR);
  if (ts->extra == 0) {  /* no hash? */
    ts->hash = ___pdr_luaS_hash(___pdr_getstr(ts), ts->u.lnglen, ts->hash);
    ts->extra = 1;  /* now it has its hash */
  }
  return ts->hash;
}


/*
** resizes the string table
*/
void ___pdr_luaS_resize (___pdr_lua_State *L, int newsize) {
  int i;
  ___pdr_stringtable *tb = &___pdr_G(L)->strt;
  if (newsize > tb->size) {  /* grow table if needed */
    ___pdr_luaM_reallocvector(L, tb->hash, tb->size, newsize, ___pdr_TString *);
    for (i = tb->size; i < newsize; i++)
      tb->hash[i] = NULL;
  }
  for (i = 0; i < tb->size; i++) {  /* rehash */
    ___pdr_TString *p = tb->hash[i];
    tb->hash[i] = NULL;
    while (p) {  /* for each node in the list */
      ___pdr_TString *hnext = p->u.hnext;  /* save next */
      unsigned int h = ___pdr_lmod(p->hash, newsize);  /* new position */
      p->u.hnext = tb->hash[h];  /* chain it */
      tb->hash[h] = p;
      p = hnext;
    }
  }
  if (newsize < tb->size) {  /* shrink table if needed */
    /* vanishing slice should be empty */
    ___pdr_lua_assert(tb->hash[newsize] == NULL && tb->hash[tb->size - 1] == NULL);
    ___pdr_luaM_reallocvector(L, tb->hash, tb->size, newsize, ___pdr_TString *);
  }
  tb->size = newsize;
}


/*
** Clear API string cache. (Entries cannot be empty, so fill them with
** a non-collectable string.)
*/
void ___pdr_luaS_clearcache (___pdr_global_State *g) {
  int i, j;
  for (i = 0; i < ___PDR_STRCACHE_N; i++)
    for (j = 0; j < ___PDR_STRCACHE_M; j++) {
    if (___pdr_iswhite(g->strcache[i][j]))  /* will entry be collected? */
      g->strcache[i][j] = g->memerrmsg;  /* replace it with something fixed */
    }
}


/*
** Initialize the string table and the string cache
*/
void ___pdr_luaS_init (___pdr_lua_State *L) {
  ___pdr_global_State *g = ___pdr_G(L);
  int i, j;
  ___pdr_luaS_resize(L, ___PDR_MINSTRTABSIZE);  /* initial size of string table */
  /* pre-create memory-error message */
  g->memerrmsg = ___pdr_luaS_newliteral(L, ___PDR_MEMERRMSG);
  ___pdr_luaC_fix(L, ___pdr_obj2gco(g->memerrmsg));  /* it should never be collected */
  for (i = 0; i < ___PDR_STRCACHE_N; i++)  /* fill cache with valid strings */
    for (j = 0; j < ___PDR_STRCACHE_M; j++)
      g->strcache[i][j] = g->memerrmsg;
}



/*
** creates a new string object
*/
static ___pdr_TString *createstrobj (___pdr_lua_State *L, size_t l, int tag, unsigned int h) {
  ___pdr_TString *ts;
  ___pdr_GCObject *o;
  size_t totalsize;  /* total size of TString object */
  totalsize = ___pdr_sizelstring(l);
  o = ___pdr_luaC_newobj(L, tag, totalsize);
  ts = ___pdr_gco2ts(o);
  ts->hash = h;
  ts->extra = 0;
  ___pdr_getstr(ts)[l] = '\0';  /* ending 0 */
  return ts;
}


___pdr_TString *___pdr_luaS_createlngstrobj (___pdr_lua_State *L, size_t l) {
  ___pdr_TString *ts = createstrobj(L, l, ___PDR_LUA_TLNGSTR, ___pdr_G(L)->seed);
  ts->u.lnglen = l;
  return ts;
}


void ___pdr_luaS_remove (___pdr_lua_State *L, ___pdr_TString *ts) {
  ___pdr_stringtable *tb = &___pdr_G(L)->strt;
  ___pdr_TString **p = &tb->hash[___pdr_lmod(ts->hash, tb->size)];
  while (*p != ts)  /* find previous element */
    p = &(*p)->u.hnext;
  *p = (*p)->u.hnext;  /* remove element from its list */
  tb->nuse--;
}


/*
** checks whether short string exists and reuses it or creates a new one
*/
static ___pdr_TString *internshrstr (___pdr_lua_State *L, const char *str, size_t l) {
  ___pdr_TString *ts;
  ___pdr_global_State *g = ___pdr_G(L);
  unsigned int h = ___pdr_luaS_hash(str, l, g->seed);
  ___pdr_TString **list = &g->strt.hash[___pdr_lmod(h, g->strt.size)];
  ___pdr_lua_assert(str != NULL);  /* otherwise 'memcmp'/'memcpy' are undefined */
  for (ts = *list; ts != NULL; ts = ts->u.hnext) {
    if (l == ts->shrlen &&
        (memcmp(str, ___pdr_getstr(ts), l * sizeof(char)) == 0)) {
      /* found! */
      if (___pdr_isdead(g, ts))  /* dead (but not collected yet)? */
        ___pdr_changewhite(ts);  /* resurrect it */
      return ts;
    }
  }
  if (g->strt.nuse >= g->strt.size && g->strt.size <= ___PDR_MAX_INT/2) {
    ___pdr_luaS_resize(L, g->strt.size * 2);
    list = &g->strt.hash[___pdr_lmod(h, g->strt.size)];  /* recompute with new size */
  }
  ts = createstrobj(L, l, ___PDR_LUA_TSHRSTR, h);
  memcpy(___pdr_getstr(ts), str, l * sizeof(char));
  ts->shrlen = ___pdr_cast_byte(l);
  ts->u.hnext = *list;
  *list = ts;
  g->strt.nuse++;
  return ts;
}


/*
** new string (with explicit length)
*/
___pdr_TString *___pdr_luaS_newlstr (___pdr_lua_State *L, const char *str, size_t l) {
  if (l <= ___PDR_LUAI_MAXSHORTLEN)  /* short string? */
    return internshrstr(L, str, l);
  else {
    ___pdr_TString *ts;
    if (l >= (___PDR_MAX_SIZE - sizeof(___pdr_TString))/sizeof(char))
      ___pdr_luaM_toobig(L);
    ts = ___pdr_luaS_createlngstrobj(L, l);
    memcpy(___pdr_getstr(ts), str, l * sizeof(char));
    return ts;
  }
}


/*
** Create or reuse a zero-terminated string, first checking in the
** cache (using the string address as a key). The cache can contain
** only zero-terminated strings, so it is safe to use 'strcmp' to
** check hits.
*/
___pdr_TString *___pdr_luaS_new (___pdr_lua_State *L, const char *str) {
  unsigned int i = ___pdr_point2uint(str) % ___PDR_STRCACHE_N;  /* hash */
  int j;
  ___pdr_TString **p = ___pdr_G(L)->strcache[i];
  for (j = 0; j < ___PDR_STRCACHE_M; j++) {
    if (strcmp(str, ___pdr_getstr(p[j])) == 0)  /* hit? */
      return p[j];  /* that is it */
  }
  /* normal route */
  for (j = ___PDR_STRCACHE_M - 1; j > 0; j--)
    p[j] = p[j - 1];  /* move out last element */
  /* new element is first in the list */
  p[0] = ___pdr_luaS_newlstr(L, str, strlen(str));
  return p[0];
}


___pdr_Udata *___pdr_luaS_newudata (___pdr_lua_State *L, size_t s) {
  ___pdr_Udata *u;
  ___pdr_GCObject *o;
  if (s > ___PDR_MAX_SIZE - sizeof(___pdr_Udata))
    ___pdr_luaM_toobig(L);
  o = ___pdr_luaC_newobj(L, ___PDR_LUA_TUSERDATA, ___pdr_sizeludata(s));
  u = ___pdr_gco2u(o);
  u->len = s;
  u->metatable = NULL;
  ___pdr_setuservalue(L, u, ___pdr_luaO_nilobject);
  return u;
}

} // end NS_PDR_SLUA