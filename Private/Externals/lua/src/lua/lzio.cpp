/*
** $Id: lzio.c,v 1.37 2015/09/08 15:41:05 roberto Exp $
** Buffered streams
** See Copyright Notice in lua.h
*/

#define ___pdr_lzio_c
#define ___PDR_LUA_CORE

#include "lzio.h"
#include "lprefix.h"

#include <string.h>

#include "lua.h"
#include "llimits.h"
#include "lmem.h"
#include "lstate.h"

namespace NS_PDR_SLUA {

int ___pdr_luaZ_fill (___pdr_ZIO *z) {
  size_t size;
  ___pdr_lua_State *L = z->L;
  const char *buff;
  ___pdr_lua_unlock(L);
  buff = z->reader(L, z->data, &size);
  ___pdr_lua_lock(L);
  if (buff == NULL || size == 0)
    return ___PDR_EOZ;
  z->n = size - 1;  /* discount char being returned */
  z->p = buff;
  return ___pdr_cast_uchar(*(z->p++));
}


void ___pdr_luaZ_init (___pdr_lua_State *L, ___pdr_ZIO *z, ___pdr_lua_Reader reader, void *data) {
  z->L = L;
  z->reader = reader;
  z->data = data;
  z->n = 0;
  z->p = NULL;
}


/* --------------------------------------------------------------- read --- */
size_t ___pdr_luaZ_read (___pdr_ZIO *z, void *b, size_t n) {
  while (n) {
    size_t m;
    if (z->n == 0) {  /* no bytes in buffer? */
      if (___pdr_luaZ_fill(z) == ___PDR_EOZ)  /* try to read more */
        return n;  /* no more input; return number of missing bytes */
      else {
        z->n++;  /* luaZ_fill consumed first byte; put it back */
        z->p--;
      }
    }
    m = (n <= z->n) ? n : z->n;  /* min. between n and z->n */
    memcpy(b, z->p, m);
    z->n -= m;
    z->p += m;
    b = (char *)b + m;
    n -= m;
  }
  return 0;
}

} // end NS_PDR_SLUA