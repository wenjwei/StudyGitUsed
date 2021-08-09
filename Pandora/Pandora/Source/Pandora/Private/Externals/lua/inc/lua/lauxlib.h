/*
** $Id: lauxlib.h,v 1.131 2016/12/06 14:54:31 roberto Exp $
** Auxiliary functions for building Lua libraries
** See Copyright Notice in lua.h
*/


#ifndef ___pdr_lauxlib_h___
#define ___pdr_lauxlib_h___

#include <stddef.h>
#include <stdio.h>

#include "lua.h"

/* extra error code for 'luaL_loadfilex' */
#define ___PDR_LUA_ERRFILE     (___PDRLUA_ERRERR+1)


/* key, in the registry, for table of loaded modules */
#define ___PDR_LUA_LOADED_TABLE	"_LOADED"


/* key, in the registry, for table of preloaded loaders */
#define ___PDR_LUA_PRELOAD_TABLE	"_PRELOAD"

namespace NS_PDR_SLUA {

typedef struct ___pdr_luaL_Reg {
  const char *name;
  ___pdr_lua_CFunction func;
} ___pdr_luaL_Reg;


#define ___PDR_LUAL_NUMSIZES	(sizeof(___pdr_lua_Integer)*16 + sizeof(___pdr_lua_Number))

___PDR_LUALIB_API void (___pdr_luaL_checkversion_) (___pdr_lua_State *L, ___pdr_lua_Number ver, size_t sz);
#define ___pdr_luaL_checkversion(L)  \
	  ___pdr_luaL_checkversion_(L, ___PDR_LUA_VERSION_NUM, ___PDR_LUAL_NUMSIZES)

___PDR_LUALIB_API int (___pdr_luaL_getmetafield) (___pdr_lua_State *L, int obj, const char *e);
___PDR_LUALIB_API int (___pdr_luaL_callmeta) (___pdr_lua_State *L, int obj, const char *e);
___PDR_LUALIB_API const char *(___pdr_luaL_tolstring) (___pdr_lua_State *L, int idx, size_t *len);
___PDR_LUALIB_API int (___pdr_luaL_argerror) (___pdr_lua_State *L, int arg, const char *extramsg);
___PDR_LUALIB_API const char *(___pdr_luaL_checklstring) (___pdr_lua_State *L, int arg,
                                                          size_t *l);
___PDR_LUALIB_API const char *(___pdr_luaL_optlstring) (___pdr_lua_State *L, int arg,
                                          const char *def, size_t *l);
___PDR_LUALIB_API ___pdr_lua_Number (___pdr_luaL_checknumber) (___pdr_lua_State *L, int arg);
___PDR_LUALIB_API ___pdr_lua_Number (___pdr_luaL_optnumber) (___pdr_lua_State *L, int arg, ___pdr_lua_Number def);

___PDR_LUALIB_API ___pdr_lua_Integer (___pdr_luaL_checkinteger) (___pdr_lua_State *L, int arg);
___PDR_LUALIB_API ___pdr_lua_Integer (___pdr_luaL_optinteger) (___pdr_lua_State *L, int arg,
                                          ___pdr_lua_Integer def);

___PDR_LUALIB_API void (___pdr_luaL_checkstack) (___pdr_lua_State *L, int sz, const char *msg);
___PDR_LUALIB_API void (___pdr_luaL_checktype) (___pdr_lua_State *L, int arg, int t);
___PDR_LUALIB_API void (___pdr_luaL_checkany) (___pdr_lua_State *L, int arg);

___PDR_LUALIB_API int   (___pdr_luaL_newmetatable) (___pdr_lua_State *L, const char *tname);
___PDR_LUALIB_API void  (___pdr_luaL_setmetatable) (___pdr_lua_State *L, const char *tname);
___PDR_LUALIB_API void *(___pdr_luaL_testudata) (___pdr_lua_State *L, int ud, const char *tname);
___PDR_LUALIB_API void *(___pdr_luaL_checkudata) (___pdr_lua_State *L, int ud, const char *tname);

___PDR_LUALIB_API void (___pdr_luaL_where) (___pdr_lua_State *L, int lvl);
___PDR_LUALIB_API int (___pdr_luaL_error) (___pdr_lua_State *L, const char *fmt, ...);

___PDR_LUALIB_API int (___pdr_luaL_checkoption) (___pdr_lua_State *L, int arg, const char *def,
                                   const char *const lst[]);

___PDR_LUALIB_API int (___pdr_luaL_fileresult) (___pdr_lua_State *L, int stat, const char *fname);
___PDR_LUALIB_API int (___pdr_luaL_execresult) (___pdr_lua_State *L, int stat);

/* predefined references */
#define ___PDR_LUA_NOREF       (-2)
#define ___PDR_LUA_REFNIL      (-1)

___PDR_LUALIB_API int (___pdr_luaL_ref) (___pdr_lua_State *L, int t);
___PDR_LUALIB_API void (___pdr_luaL_unref) (___pdr_lua_State *L, int t, int ref);

___PDR_LUALIB_API int (___pdr_luaL_loadfilex) (___pdr_lua_State *L, const char *filename,
                                               const char *mode);

#define ___pdr_luaL_loadfile(L,f)	___pdr_luaL_loadfilex(L,f,NULL)

___PDR_LUALIB_API int (___pdr_luaL_loadbufferx) (___pdr_lua_State *L, const char *buff, size_t sz,
                                   const char *name, const char *mode);
___PDR_LUALIB_API int (___pdr_luaL_loadstring) (___pdr_lua_State *L, const char *s);

___PDR_LUALIB_API ___pdr_lua_State *(___pdr_luaL_newstate) (void);

___PDR_LUALIB_API ___pdr_lua_Integer (___pdr_luaL_len) (___pdr_lua_State *L, int idx);

___PDR_LUALIB_API const char *(___pdr_luaL_gsub) (___pdr_lua_State *L, const char *s, const char *p,
                                                  const char *r);

___PDR_LUALIB_API void (___pdr_luaL_setfuncs) (___pdr_lua_State *L, const ___pdr_luaL_Reg *l, int nup);

___PDR_LUALIB_API int (___pdr_luaL_getsubtable) (___pdr_lua_State *L, int idx, const char *fname);

___PDR_LUALIB_API void (___pdr_luaL_traceback) (___pdr_lua_State *L, ___pdr_lua_State *L1,
                                  const char *msg, int level);

___PDR_LUALIB_API void (___pdr_luaL_requiref) (___pdr_lua_State *L, const char *modname,
                                 ___pdr_lua_CFunction openf, int glb);

/*
** ===============================================================
** some useful macros
** ===============================================================
*/


#define ___pdr_luaL_newlibtable(L,l)	\
  ___pdr_lua_createtable(L, 0, sizeof(l)/sizeof((l)[0]) - 1)

#define ___pdr_luaL_newlib(L,l)  \
  (___pdr_luaL_checkversion(L), ___pdr_luaL_newlibtable(L,l), ___pdr_luaL_setfuncs(L,l,0))

#define ___pdr_luaL_argcheck(L, cond,arg,extramsg)	\
		((void)((cond) || ___pdr_luaL_argerror(L, (arg), (extramsg))))
#define ___pdr_luaL_checkstring(L,n)	(___pdr_luaL_checklstring(L, (n), NULL))
#define ___pdr_luaL_optstring(L,n,d)	(___pdr_luaL_optlstring(L, (n), (d), NULL))

#define ___pdr_luaL_typename(L,i)	___pdr_lua_typename(L, ___pdr_lua_type(L,(i)))

#define ___pdr_luaL_dofile(L, fn) \
	(___pdr_luaL_loadfile(L, fn) || ___pdr_lua_pcall(L, 0, ___PDR_LUA_MULTRET, 0))

#define ___pdr_luaL_dostring(L, s) \
	(___pdr_luaL_loadstring(L, s) || ___pdr_lua_pcall(L, 0, ___PDR_LUA_MULTRET, 0))

#define ___pdr_luaL_getmetatable(L,n)	(___pdr_lua_getfield(L, ___PDR_LUA_REGISTRYINDEX, (n)))

#define ___pdr_luaL_opt(L,f,n,d)	(___pdr_lua_isnoneornil(L,(n)) ? (d) : f(L,(n)))

#define ___pdr_luaL_loadbuffer(L,s,sz,n)	___pdr_luaL_loadbufferx(L,s,sz,n,NULL)


/*
** {======================================================
** Generic Buffer manipulation
** =======================================================
*/

typedef struct ___pdr_luaL_Buffer {
  char *b;  /* buffer address */
  size_t size;  /* buffer size */
  size_t n;  /* number of characters in buffer */
  ___pdr_lua_State *L;
  char initb[___PDR_LUAL_BUFFERSIZE];  /* initial buffer */
} ___pdr_luaL_Buffer;


#define ___pdr_luaL_addchar(B,c) \
  ((void)((B)->n < (B)->size || ___pdr_luaL_prepbuffsize((B), 1)), \
   ((B)->b[(B)->n++] = (c)))

#define ___pdr_luaL_addsize(B,s)	((B)->n += (s))

___PDR_LUALIB_API void (___pdr_luaL_buffinit) (___pdr_lua_State *L, ___pdr_luaL_Buffer *B);
___PDR_LUALIB_API char *(___pdr_luaL_prepbuffsize) (___pdr_luaL_Buffer *B, size_t sz);
___PDR_LUALIB_API void (___pdr_luaL_addlstring) (___pdr_luaL_Buffer *B, const char *s, size_t l);
___PDR_LUALIB_API void (___pdr_luaL_addstring) (___pdr_luaL_Buffer *B, const char *s);
___PDR_LUALIB_API void (___pdr_luaL_addvalue) (___pdr_luaL_Buffer *B);
___PDR_LUALIB_API void (___pdr_luaL_pushresult) (___pdr_luaL_Buffer *B);
___PDR_LUALIB_API void (___pdr_luaL_pushresultsize) (___pdr_luaL_Buffer *B, size_t sz);
___PDR_LUALIB_API char *(___pdr_luaL_buffinitsize) (___pdr_lua_State *L, ___pdr_luaL_Buffer *B, size_t sz);

#define ___pdr_luaL_prepbuffer(B)	___pdr_luaL_prepbuffsize(B, ___PDR_LUAL_BUFFERSIZE)

/* }====================================================== */



/*
** {======================================================
** File handles for IO library
** =======================================================
*/

/*
** A file handle is a userdata with metatable 'LUA_FILEHANDLE' and
** initial structure 'luaL_Stream' (it may contain other fields
** after that initial structure).
*/

#define ___PDR_LUA_FILEHANDLE          "FILE*"


typedef struct ___pdr_luaL_Stream {
  FILE *f;  /* stream (NULL for incompletely created streams) */
  ___pdr_lua_CFunction closef;  /* to close stream (NULL for closed streams) */
} ___pdr_luaL_Stream;

/* }====================================================== */



/* compatibility with old module system */
#if defined(___PDR_LUA_COMPAT_MODULE)

___PDR_LUALIB_API void (___pdr_luaL_pushmodule) (___pdr_lua_State *L, const char *modname,
                                   int sizehint);
___PDR_LUALIB_API void (___pdr_luaL_openlib) (___pdr_lua_State *L, const char *libname,
                                const ___pdr_luaL_Reg *l, int nup);

#define ___pdr_luaL_register(L,n,l)	(___pdr_luaL_openlib(L,(n),(l),0))

#endif


/*
** {==================================================================
** "Abstraction Layer" for basic report of messages and errors
** ===================================================================
*/

/* print a string */
#if !defined(___pdr_lua_writestring)
#define ___pdr_lua_writestring(s,l)   fwrite((s), sizeof(char), (l), stdout)
#endif

/* print a newline and flush the output */
#if !defined(___pdr_lua_writeline)
#define ___pdr_lua_writeline()        (___pdr_lua_writestring("\n", 1), fflush(stdout))
#endif

/* print an error message */
#if !defined(___pdr_lua_writestringerror)
#define ___pdr_lua_writestringerror(s,p) \
        (fprintf(stderr, (s), (p)), fflush(stderr))
#endif

/* }================================================================== */


/*
** {============================================================
** Compatibility with deprecated conversions
** =============================================================
*/
#if defined(___PDR_LUA_COMPAT_APIINTCASTS)

#define ___pdr_luaL_checkunsigned(L,a)	((___pdr_lua_Unsigned)___pdr_luaL_checkinteger(L,a))
#define ___pdr_luaL_optunsigned(L,a,d)	\
	((___pdr_lua_Unsigned)___pdr_luaL_optinteger(L,a,(___pdr_lua_Integer)(d)))

#define ___pdr_luaL_checkint(L,n)	((int)___pdr_luaL_checkinteger(L, (n)))
#define ___pdr_luaL_optint(L,n,d)	((int)___pdr_luaL_optinteger(L, (n), (d)))

#define ___pdr_luaL_checklong(L,n)	((long)___pdr_luaL_checkinteger(L, (n)))
#define ___pdr_luaL_optlong(L,n,d)	((long)___pdr_luaL_optinteger(L, (n), (d)))

#endif
/* }============================================================ */

} // end NS_PDR_SLUA

#endif


