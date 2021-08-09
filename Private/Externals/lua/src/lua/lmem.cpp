/*
** $Id: lmem.c,v 1.91 2015/03/06 19:45:54 roberto Exp $
** Interface to Memory Manager
** See Copyright Notice in lua.h
*/

#define ___pdr_lmem_c
#define ___PDR_LUA_CORE

#include "lmem.h"
#include "lprefix.h"

#include <stddef.h>

#include "lua.h"
#include "ldebug.h"
#include "ldo.h"
#include "lgc.h"
#include "lobject.h"
#include "lstate.h"

namespace NS_PDR_SLUA {

/*
** About the realloc function:
** void * frealloc (void *ud, void *ptr, size_t osize, size_t nsize);
** ('osize' is the old size, 'nsize' is the new size)
**
** * frealloc(ud, NULL, x, s) creates a new block of size 's' (no
** matter 'x').
**
** * frealloc(ud, p, x, 0) frees the block 'p'
** (in this specific case, frealloc must return NULL);
** particularly, frealloc(ud, NULL, 0, 0) does nothing
** (which is equivalent to free(NULL) in ISO C)
**
** frealloc returns NULL if it cannot create or reallocate the area
** (any reallocation to an equal or smaller size cannot fail!)
*/



#define ___PDR_MINSIZEARRAY	4


void *___pdr_luaM_growaux_ (___pdr_lua_State *L, void *block, int *size, size_t size_elems,
                     int limit, const char *what) {
  void *newblock;
  int newsize;
  if (*size >= limit/2) {  /* cannot double it? */
    if (*size >= limit)  /* cannot grow even a little? */
      ___pdr_luaG_runerror(L, "too many %s (limit is %d)", what, limit);
    newsize = limit;  /* still have at least one free place */
  }
  else {
    newsize = (*size)*2;
    if (newsize < ___PDR_MINSIZEARRAY)
      newsize = ___PDR_MINSIZEARRAY;  /* minimum size */
  }
  newblock = ___pdr_luaM_reallocv(L, block, *size, newsize, size_elems);
  *size = newsize;  /* update only when everything else is OK */
  return newblock;
}


___pdr_l_noret ___pdr_luaM_toobig (___pdr_lua_State *L) {
  ___pdr_luaG_runerror(L, "memory allocation error: block too big");
}



/*
** generic allocation routine.
*/
void *___pdr_luaM_realloc_ (___pdr_lua_State *L, void *block, size_t osize, size_t nsize) {
  void *newblock;
  ___pdr_global_State *g = ___pdr_G(L);
  size_t realosize = (block) ? osize : 0;
  ___pdr_lua_assert((realosize == 0) == (block == NULL));
#if defined(___PDR_HARDMEMTESTS)
  if (nsize > realosize && g->gcrunning)
    ___pdr_luaC_fullgc(L, 1);  /* force a GC whenever possible */
#endif
  newblock = (*g->frealloc)(g->ud, block, osize, nsize);
  if (newblock == NULL && nsize > 0) {
    ___pdr_lua_assert(nsize > realosize);  /* cannot fail when shrinking a block */
    if (g->version) {  /* is state fully built? */
      ___pdr_luaC_fullgc(L, 1);  /* try to free some memory... */
      newblock = (*g->frealloc)(g->ud, block, osize, nsize);  /* try again */
    }
    if (newblock == NULL)
      ___pdr_luaD_throw(L, ___PDR_LUA_ERRMEM);
  }
  ___pdr_lua_assert((nsize == 0) == (newblock == NULL));
  g->GCdebt = (g->GCdebt + nsize) - realosize;
  return newblock;
}

} // end NS_PDR_SLUA