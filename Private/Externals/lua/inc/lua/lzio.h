/*
** $Id: lzio.h,v 1.31 2015/09/08 15:41:05 roberto Exp $
** Buffered streams
** See Copyright Notice in lua.h
*/


#ifndef ___pdr_lzio_h___
#define ___pdr_lzio_h___

#include "lua.h"
#include "lmem.h"

namespace NS_PDR_SLUA {

#define ___PDR_EOZ	(-1)			/* end of stream */

typedef struct ___pdr_Zio ___pdr_ZIO;

#define ___pdr_zgetc(z)  (((z)->n--)>0 ?  ___pdr_cast_uchar(*(z)->p++) : ___pdr_luaZ_fill(z))


typedef struct ___pdr_Mbuffer {
  char *buffer;
  size_t n;
  size_t buffsize;
} ___pdr_Mbuffer;

#define ___pdr_luaZ_initbuffer(L, buff) ((buff)->buffer = NULL, (buff)->buffsize = 0)

#define ___pdr_luaZ_buffer(buff)	((buff)->buffer)
#define ___pdr_luaZ_sizebuffer(buff)	((buff)->buffsize)
#define ___pdr_luaZ_bufflen(buff)	((buff)->n)

#define ___pdr_luaZ_buffremove(buff,i)	((buff)->n -= (i))
#define ___pdr_luaZ_resetbuffer(buff) ((buff)->n = 0)


#define ___pdr_luaZ_resizebuffer(L, buff, size) \
	((buff)->buffer = ___pdr_luaM_reallocvchar(L, (buff)->buffer, \
				(buff)->buffsize, size), \
	(buff)->buffsize = size)

#define ___pdr_luaZ_freebuffer(L, buff)	___pdr_luaZ_resizebuffer(L, buff, 0)


___PDR_LUAI_FUNC void ___pdr_luaZ_init (___pdr_lua_State *L, ___pdr_ZIO *z, ___pdr_lua_Reader reader,
                                        void *data);
___PDR_LUAI_FUNC size_t ___pdr_luaZ_read (___pdr_ZIO* z, void *b, size_t n);	/* read next n bytes */



/* --------- Private Part ------------------ */

struct ___pdr_Zio {
  size_t n;			/* bytes still unread */
  const char *p;		/* current position in buffer */
  ___pdr_lua_Reader reader;		/* reader function */
  void *data;			/* additional data */
  ___pdr_lua_State *L;			/* Lua state (for reader) */
};


___PDR_LUAI_FUNC int ___pdr_luaZ_fill (___pdr_ZIO *z);

} // end NS_PDR_SLUA

#endif
