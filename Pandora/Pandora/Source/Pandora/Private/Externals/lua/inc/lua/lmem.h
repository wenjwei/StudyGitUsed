/*
** $Id: lmem.h,v 1.43 2014/12/19 17:26:14 roberto Exp $
** Interface to Memory Manager
** See Copyright Notice in lua.h
*/

#ifndef ___pdr_lmem_h___
#define ___pdr_lmem_h___


#include <stddef.h>

#include "llimits.h"
#include "lua.h"

namespace NS_PDR_SLUA {

/*
** This macro reallocs a vector 'b' from 'on' to 'n' elements, where
** each element has size 'e'. In case of arithmetic overflow of the
** product 'n'*'e', it raises an error (calling 'luaM_toobig'). Because
** 'e' is always constant, it avoids the runtime division MAX_SIZET/(e).
**
** (The macro is somewhat complex to avoid warnings:  The 'sizeof'
** comparison avoids a runtime comparison when overflow cannot occur.
** The compiler should be able to optimize the real test by itself, but
** when it does it, it may give a warning about "comparison is always
** false due to limited range of data type"; the +1 tricks the compiler,
** avoiding this warning but also this optimization.)
*/
#define ___pdr_luaM_reallocv(L,b,on,n,e) \
  (((sizeof(n) >= sizeof(size_t) && ___pdr_cast(size_t, (n)) + 1 > ___PDR_MAX_SIZET/(e)) \
      ? ___pdr_luaM_toobig(L) : ___pdr_cast_void(0)) , \
   ___pdr_luaM_realloc_(L, (b), (on)*(e), (n)*(e)))

/*
** Arrays of chars do not need any test
*/
#define ___pdr_luaM_reallocvchar(L,b,on,n)  \
    ___pdr_cast(char *, ___pdr_luaM_realloc_(L, (b), (on)*sizeof(char), (n)*sizeof(char)))

#define ___pdr_luaM_freemem(L, b, s)	___pdr_luaM_realloc_(L, (b), (s), 0)
#define ___pdr_luaM_free(L, b)		___pdr_luaM_realloc_(L, (b), sizeof(*(b)), 0)
#define ___pdr_luaM_freearray(L, b, n)   ___pdr_luaM_realloc_(L, (b), (n)*sizeof(*(b)), 0)

#define ___pdr_luaM_malloc(L,s)	___pdr_luaM_realloc_(L, NULL, 0, (s))
#define ___pdr_luaM_new(L,t)		___pdr_cast(t *, ___pdr_luaM_malloc(L, sizeof(t)))
#define ___pdr_luaM_newvector(L,n,t) \
		___pdr_cast(t *, ___pdr_luaM_reallocv(L, NULL, 0, n, sizeof(t)))

#define ___pdr_luaM_newobject(L,tag,s)	___pdr_luaM_realloc_(L, NULL, tag, (s))

#define ___pdr_luaM_growvector(L,v,nelems,size,t,limit,e) \
          if ((nelems)+1 > (size)) \
            ((v)=___pdr_cast(t *, ___pdr_luaM_growaux_(L,v,&(size),sizeof(t),limit,e)))

#define ___pdr_luaM_reallocvector(L, v,oldn,n,t) \
   ((v)=___pdr_cast(t *, ___pdr_luaM_reallocv(L, v, oldn, n, sizeof(t))))

___PDR_LUAI_FUNC ___pdr_l_noret ___pdr_luaM_toobig (___pdr_lua_State *L);

/* not to be called directly */
___PDR_LUAI_FUNC void *___pdr_luaM_realloc_ (___pdr_lua_State *L, void *block, size_t oldsize,
                                                          size_t size);
___PDR_LUAI_FUNC void *___pdr_luaM_growaux_ (___pdr_lua_State *L, void *block, int *size,
                               size_t size_elem, int limit,
                               const char *what);

} // end NS_PDR_SLUA

#endif

