/*
** $Id: lua.h,v 1.332 2016/12/22 15:51:20 roberto Exp $
** Lua - A Scripting Language
** Lua.org, PUC-Rio, Brazil (http://www.lua.org)
** See Copyright Notice at the end of this file
*/


#ifndef ___pdr_lua_h___
#define ___pdr_lua_h___

#include <stdarg.h>
#include <stddef.h>

#include "luaconf.h"

// #define NS_PDR_SLUA pdrslua

#define ___PDR_LUA_VERSION_MAJOR	"5"
#define ___PDR_LUA_VERSION_MINOR	"3"
#define ___PDR_LUA_VERSION_NUM		503
#define ___PDR_LUA_VERSION_RELEASE	"4"

#define ___PDR_LUA_VERSION	"Lua " ___PDR_LUA_VERSION_MAJOR "." ___PDR_LUA_VERSION_MINOR
#define ___PDR_LUA_RELEASE	___PDR_LUA_VERSION "." ___PDR_LUA_VERSION_RELEASE
#define ___PDR_LUA_COPYRIGHT	___PDR_LUA_RELEASE "  Copyright (C) 1994-2017 Lua.org, PUC-Rio"
#define ___PDR_LUA_AUTHORS	"R. Ierusalimschy, L. H. de Figueiredo, W. Celes"


/* mark for precompiled code ('<esc>Lua') */
#define ___PDR_LUA_SIGNATURE	"\x1bLua"

/* option for multiple returns in 'lua_pcall' and 'lua_call' */
#define ___PDR_LUA_MULTRET	(-1)


/*
** Pseudo-indices
** (-LUAI_MAXSTACK is the minimum valid index; we keep some free empty
** space after that to help overflow detection)
*/
#define ___PDR_LUA_REGISTRYINDEX	(-___PDR_LUAI_MAXSTACK - 1000)
#define ___pdr_lua_upvalueindex(i)	(___PDR_LUA_REGISTRYINDEX - (i))


/* thread status */
#define ___PDR_LUA_OK		0
#define ___PDR_LUA_YIELD	1
#define ___PDR_LUA_ERRRUN	2
#define ___PDR_LUA_ERRSYNTAX	3
#define ___PDR_LUA_ERRMEM	4
#define ___PDR_LUA_ERRGCMM	5
#define ___PDRLUA_ERRERR	6

namespace NS_PDR_SLUA {

typedef struct ___pdr_lua_State ___pdr_lua_State;


/*
** basic types
*/
#define ___PDR_LUA_TNONE		(-1)

#define ___PDR_LUA_TNIL             0
#define ___PDR_LUA_TBOOLEAN         1
#define ___PDR_LUA_TLIGHTUSERDATA   2
#define ___PDR_LUA_TNUMBER		3
#define ___PDR_LUA_TSTRING		4
#define ___PDR_LUA_TTABLE		5
#define ___PDR_LUA_TFUNCTION		6
#define ___PDR_LUA_TUSERDATA		7
#define ___PDR_LUA_TTHREAD		8

#define ___PDR_LUA_NUMTAGS		9



/* minimum Lua stack available to a C function */
#define ___PDR_LUA_MINSTACK	20


/* predefined values in the registry */
#define ___PDR_LUA_RIDX_MAINTHREAD	1
#define ___PDR_LUA_RIDX_GLOBALS	2
#define ___PDR_LUA_RIDX_LAST		___PDR_LUA_RIDX_GLOBALS


/* type of numbers in Lua */
typedef __PDR_LUA_NUMBER ___pdr_lua_Number;


/* type for integer functions */
typedef ___PDR_LUA_INTEGER ___pdr_lua_Integer;

/* unsigned integer type */
typedef ___PDR_LUA_UNSIGNED ___pdr_lua_Unsigned;

/* type for continuation-function contexts */
typedef ___PDR_LUA_KCONTEXT ___pdr_lua_KContext;


/*
** Type for C functions registered with Lua
*/
typedef int (*___pdr_lua_CFunction) (___pdr_lua_State *L);

/*
** Type for continuation functions
*/
typedef int (*___pdr_lua_KFunction) (___pdr_lua_State *L, int status, ___pdr_lua_KContext ctx);


/*
** Type for functions that read/write blocks when loading/dumping Lua chunks
*/
typedef const char * (*___pdr_lua_Reader) (___pdr_lua_State *L, void *ud, size_t *sz);

typedef int (*___pdr_lua_Writer) (___pdr_lua_State *L, const void *p, size_t sz, void *ud);


/*
** Type for memory-allocation functions
*/
typedef void * (*___pdr_lua_Alloc) (void *ud, void *ptr, size_t osize, size_t nsize);



/*
** generic extra include file
*/
#if defined(___PDR_LUA_USER_H)
#include ___PDR_LUA_USER_H
#endif


/*
** RCS ident string
*/
extern const char ___pdr_lua_ident[];


/*
** state manipulation
*/
___PDR_LUA_API ___pdr_lua_State *(___pdr_lua_newstate) (___pdr_lua_Alloc f, void *ud);
___PDR_LUA_API void       (___pdr_lua_close) (___pdr_lua_State *L);
___PDR_LUA_API ___pdr_lua_State *(___pdr_lua_newthread) (___pdr_lua_State *L);

___PDR_LUA_API ___pdr_lua_CFunction (___pdr_lua_atpanic) (___pdr_lua_State *L, ___pdr_lua_CFunction panicf);

___PDR_LUA_API const ___pdr_lua_Number *(___pdr_lua_version) (___pdr_lua_State *L);

___PDR_LUA_API void (___pdr_lua_setonlyluac)(___pdr_lua_State *L, int v);

/*
** basic stack manipulation
*/
___PDR_LUA_API int   (___pdr_lua_absindex) (___pdr_lua_State *L, int idx);
___PDR_LUA_API int   (___pdr_lua_gettop) (___pdr_lua_State *L);
___PDR_LUA_API void  (___pdr_lua_settop) (___pdr_lua_State *L, int idx);
___PDR_LUA_API void  (___pdr_lua_pushvalue) (___pdr_lua_State *L, int idx);
___PDR_LUA_API void  (___pdr_lua_rotate) (___pdr_lua_State *L, int idx, int n);
___PDR_LUA_API void  (___pdr_lua_copy) (___pdr_lua_State *L, int fromidx, int toidx);
___PDR_LUA_API int   (___pdr_lua_checkstack) (___pdr_lua_State *L, int n);

___PDR_LUA_API void  (___pdr_lua_xmove) (___pdr_lua_State *from, ___pdr_lua_State *to, int n);


/*
** access functions (stack -> C)
*/

___PDR_LUA_API int             (___pdr_lua_isnumber) (___pdr_lua_State *L, int idx);
___PDR_LUA_API int             (___pdr_lua_isstring) (___pdr_lua_State *L, int idx);
___PDR_LUA_API int             (___pdr_lua_iscfunction) (___pdr_lua_State *L, int idx);
___PDR_LUA_API int             (___pdr_lua_isinteger) (___pdr_lua_State *L, int idx);
___PDR_LUA_API int             (___pdr_lua_isuserdata) (___pdr_lua_State *L, int idx);
___PDR_LUA_API int             (___pdr_lua_type) (___pdr_lua_State *L, int idx);
___PDR_LUA_API const char     *(___pdr_lua_typename) (___pdr_lua_State *L, int tp);

___PDR_LUA_API ___pdr_lua_Number      (___pdr_lua_tonumberx) (___pdr_lua_State *L, int idx, int *isnum);
___PDR_LUA_API ___pdr_lua_Integer     (___pdr_lua_tointegerx) (___pdr_lua_State *L, int idx, int *isnum);
___PDR_LUA_API int             (___pdr_lua_toboolean) (___pdr_lua_State *L, int idx);
___PDR_LUA_API const char     *(___pdr_lua_tolstring) (___pdr_lua_State *L, int idx, size_t *len);
___PDR_LUA_API size_t          (___pdr_lua_rawlen) (___pdr_lua_State *L, int idx);
___PDR_LUA_API ___pdr_lua_CFunction   (___pdr_lua_tocfunction) (___pdr_lua_State *L, int idx);
___PDR_LUA_API void	       *(___pdr_lua_touserdata) (___pdr_lua_State *L, int idx);
___PDR_LUA_API ___pdr_lua_State      *(___pdr_lua_tothread) (___pdr_lua_State *L, int idx);
___PDR_LUA_API const void     *(___pdr_lua_topointer) (___pdr_lua_State *L, int idx);


/*
** Comparison and arithmetic functions
*/

#define ___PDR_LUA_OPADD	0	/* ORDER TM, ORDER OP */
#define ___PDR_LUA_OPSUB	1
#define ___PDR_LUA_OPMUL	2
#define ___PDR_LUA_OPMOD	3
#define ___PDR_LUA_OPPOW	4
#define ___PDR_LUA_OPDIV	5
#define ___PDR_LUA_OPIDIV	6
#define ___PDR_LUA_OPBAND	7
#define ___PDR_LUA_OPBOR	8
#define ___PDR_LUA_OPBXOR	9
#define ___PDR_LUA_OPSHL	10
#define ___PDR_LUA_OPSHR	11
#define ___PDR_LUA_OPUNM	12
#define ___PDR_LUA_OPBNOT	13

___PDR_LUA_API void  (___pdr_lua_arith) (___pdr_lua_State *L, int op);

#define ___PDR_LUA_OPEQ	0
#define ___PDR_LUA_OPLT	1
#define ___PDR_LUA_OPLE	2

___PDR_LUA_API int   (___pdr_lua_rawequal) (___pdr_lua_State *L, int idx1, int idx2);
___PDR_LUA_API int   (___pdr_lua_compare) (___pdr_lua_State *L, int idx1, int idx2, int op);


/*
** push functions (C -> stack)
*/
___PDR_LUA_API void        (___pdr_lua_pushnil) (___pdr_lua_State *L);
___PDR_LUA_API void        (___pdr_lua_pushnumber) (___pdr_lua_State *L, ___pdr_lua_Number n);
___PDR_LUA_API void        (___pdr_lua_pushinteger) (___pdr_lua_State *L, ___pdr_lua_Integer n);
___PDR_LUA_API const char *(___pdr_lua_pushlstring) (___pdr_lua_State *L, const char *s, size_t len);
___PDR_LUA_API const char *(___pdr_lua_pushstring) (___pdr_lua_State *L, const char *s);
___PDR_LUA_API const char *(___pdr_lua_pushvfstring) (___pdr_lua_State *L, const char *fmt,
                                                      va_list argp);
___PDR_LUA_API const char *(___pdr_lua_pushfstring) (___pdr_lua_State *L, const char *fmt, ...);
___PDR_LUA_API void  (___pdr_lua_pushcclosure) (___pdr_lua_State *L, ___pdr_lua_CFunction fn, int n);
___PDR_LUA_API void  (___pdr_lua_pushboolean) (___pdr_lua_State *L, int b);
___PDR_LUA_API void  (___pdr_lua_pushlightuserdata) (___pdr_lua_State *L, void *p);
___PDR_LUA_API int   (___pdr_lua_pushthread) (___pdr_lua_State *L);


/*
** get functions (Lua -> stack)
*/
___PDR_LUA_API int (___pdr_lua_getglobal) (___pdr_lua_State *L, const char *name);
___PDR_LUA_API int (___pdr_lua_gettable) (___pdr_lua_State *L, int idx);
___PDR_LUA_API int (___pdr_lua_getfield) (___pdr_lua_State *L, int idx, const char *k);
___PDR_LUA_API int (___pdr_lua_geti) (___pdr_lua_State *L, int idx, ___pdr_lua_Integer n);
___PDR_LUA_API int (___pdr_lua_rawget) (___pdr_lua_State *L, int idx);
___PDR_LUA_API int (___pdr_lua_rawgeti) (___pdr_lua_State *L, int idx, ___pdr_lua_Integer n);
___PDR_LUA_API int (___pdr_lua_rawgetp) (___pdr_lua_State *L, int idx, const void *p);

___PDR_LUA_API void  (___pdr_lua_createtable) (___pdr_lua_State *L, int narr, int nrec);
___PDR_LUA_API void *(___pdr_lua_newuserdata) (___pdr_lua_State *L, size_t sz);
___PDR_LUA_API int   (___pdr_lua_getmetatable) (___pdr_lua_State *L, int objindex);
___PDR_LUA_API int  (___pdr_lua_getuservalue) (___pdr_lua_State *L, int idx);


/*
** set functions (stack -> Lua)
*/
___PDR_LUA_API void  (___pdr_lua_setglobal) (___pdr_lua_State *L, const char *name);
___PDR_LUA_API void  (___pdr_lua_settable) (___pdr_lua_State *L, int idx);
___PDR_LUA_API void  (___pdr_lua_setfield) (___pdr_lua_State *L, int idx, const char *k);
___PDR_LUA_API void  (___pdr_lua_seti) (___pdr_lua_State *L, int idx, ___pdr_lua_Integer n);
___PDR_LUA_API void  (___pdr_lua_rawset) (___pdr_lua_State *L, int idx);
___PDR_LUA_API void  (___pdr_lua_rawseti) (___pdr_lua_State *L, int idx, ___pdr_lua_Integer n);
___PDR_LUA_API void  (___pdr_lua_rawsetp) (___pdr_lua_State *L, int idx, const void *p);
___PDR_LUA_API int   (___pdr_lua_setmetatable) (___pdr_lua_State *L, int objindex);
___PDR_LUA_API void  (___pdr_lua_setuservalue) (___pdr_lua_State *L, int idx);


/*
** 'load' and 'call' functions (load and run Lua code)
*/
___PDR_LUA_API void  (___pdr_lua_callk) (___pdr_lua_State *L, int nargs, int nresults,
                           ___pdr_lua_KContext ctx, ___pdr_lua_KFunction k);
#define lua_call(L,n,r)		___pdr_lua_callk(L, (n), (r), 0, NULL)

___PDR_LUA_API int   (___pdr_lua_pcallk) (___pdr_lua_State *L, int nargs, int nresults, int errfunc,
                            ___pdr_lua_KContext ctx, ___pdr_lua_KFunction k);
#define ___pdr_lua_pcall(L,n,r,f)	___pdr_lua_pcallk(L, (n), (r), (f), 0, NULL)

___PDR_LUA_API int   (lua_load) (___pdr_lua_State *L, ___pdr_lua_Reader reader, void *dt,
                          const char *chunkname, const char *mode);

#ifdef ___PDR_LUAC
___PDR_LUA_API int (___pdr_lua_dump) (___pdr_lua_State *L, ___pdr_lua_Writer writer, void *data, int strip);
#endif // end ___PDR_LUAC


/*
** coroutine functions
*/
___PDR_LUA_API int  (___pdr_lua_yieldk)     (___pdr_lua_State *L, int nresults, ___pdr_lua_KContext ctx,
                               ___pdr_lua_KFunction k);
___PDR_LUA_API int  (___pdr_lua_resume)     (___pdr_lua_State *L, ___pdr_lua_State *from, int narg);
___PDR_LUA_API int  (___pdr_lua_status)     (___pdr_lua_State *L);
___PDR_LUA_API int (___pdr_lua_isyieldable) (___pdr_lua_State *L);

#define ___pdr_lua_yield(L,n)		___pdr_lua_yieldk(L, (n), 0, NULL)


/*
** garbage-collection function and options
*/

#define ___PDR_LUA_GCSTOP		0
#define ___PDR_LUA_GCRESTART		1
#define ___PDR_LUA_GCCOLLECT		2
#define ___PDR_LUA_GCCOUNT		3
#define ___PDR_LUA_GCCOUNTB		4
#define ___PDR_LUA_GCSTEP		5
#define ___PDR_LUA_GCSETPAUSE		6
#define ___PDR_LUA_GCSETSTEPMUL	7
#define ___PDR_LUA_GCISRUNNING		9

___PDR_LUA_API int (___pdr_lua_gc) (___pdr_lua_State *L, int what, int data);


/*
** miscellaneous functions
*/

___PDR_LUA_API int   (___pdr_lua_error) (___pdr_lua_State *L);

___PDR_LUA_API int   (___pdr_lua_next) (___pdr_lua_State *L, int idx);

___PDR_LUA_API void  (___pdr_lua_concat) (___pdr_lua_State *L, int n);
___PDR_LUA_API void  (___pdr_lua_len)    (___pdr_lua_State *L, int idx);

___PDR_LUA_API size_t   (___pdr_lua_stringtonumber) (___pdr_lua_State *L, const char *s);

___PDR_LUA_API ___pdr_lua_Alloc (___pdr_lua_getallocf) (___pdr_lua_State *L, void **ud);
___PDR_LUA_API void      (___pdr_lua_setallocf) (___pdr_lua_State *L, ___pdr_lua_Alloc f, void *ud);



/*
** {==============================================================
** some useful macros
** ===============================================================
*/

#define ___pdr_lua_getextraspace(L)	((void *)((char *)(L) - ___PDR_LUA_EXTRASPACE))

#define ___pdr_lua_tonumber(L,i)	___pdr_lua_tonumberx(L,(i),NULL)
#define ___pdr_lua_tointeger(L,i)	___pdr_lua_tointegerx(L,(i),NULL)

#define ___pdr_lua_pop(L,n)		___pdr_lua_settop(L, -(n)-1)

#define ___pdr_lua_newtable(L)		___pdr_lua_createtable(L, 0, 0)

#define ___pdr_lua_register(L,n,f) (___pdr_lua_pushcfunction(L, (f)), ___pdr_lua_setglobal(L, (n)))

#define ___pdr_lua_pushcfunction(L,f)	___pdr_lua_pushcclosure(L, (f), 0)

#define ___pdr_lua_isfunction(L,n)	(___pdr_lua_type(L, (n)) == ___PDR_LUA_TFUNCTION)
#define ___pdr_lua_istable(L,n)	(___pdr_lua_type(L, (n)) == ___PDR_LUA_TTABLE)
#define ___pdr_lua_islightuserdata(L,n)	(___pdr_lua_type(L, (n)) == ___PDR_LUA_TLIGHTUSERDATA)
#define ___pdr_lua_isnil(L,n)		(___pdr_lua_type(L, (n)) == ___PDR_LUA_TNIL)
#define ___pdr_lua_isboolean(L,n)	(___pdr_lua_type(L, (n)) == ___PDR_LUA_TBOOLEAN)
#define ___pdr_lua_isthread(L,n)	(___pdr_lua_type(L, (n)) == ___PDR_LUA_TTHREAD)
#define ___pdr_lua_isnone(L,n)		(___pdr_lua_type(L, (n)) == ___PDR_LUA_TNONE)
#define ___pdr_lua_isnoneornil(L, n)	(___pdr_lua_type(L, (n)) <= 0)

#define ___pdr_lua_pushliteral(L, s)	___pdr_lua_pushstring(L, "" s)

#define ___pdr_lua_pushglobaltable(L)  \
	((void)___pdr_lua_rawgeti(L, ___PDR_LUA_REGISTRYINDEX, ___PDR_LUA_RIDX_GLOBALS))

#define ___pdr_lua_tostring(L,i)	___pdr_lua_tolstring(L, (i), NULL)


#define ___pdr_lua_insert(L,idx)	___pdr_lua_rotate(L, (idx), 1)

#define ___pdr_lua_remove(L,idx)	(___pdr_lua_rotate(L, (idx), -1), ___pdr_lua_pop(L, 1))

#define ___pdr_lua_replace(L,idx)	(___pdr_lua_copy(L, -1, (idx)), ___pdr_lua_pop(L, 1))

/* }============================================================== */


/*
** {==============================================================
** compatibility macros for unsigned conversions
** ===============================================================
*/
#if defined(___PDR_LUA_COMPAT_APIINTCASTS)

#define ___pdr_lua_pushunsigned(L,n)	___pdr_lua_pushinteger(L, (___pdr_lua_Integer)(n))
#define ___pdr_lua_tounsignedx(L,i,is)	((___pdr_lua_Unsigned)___pdr_lua_tointegerx(L,i,is))
#define ___pdr_lua_tounsigned(L,i)	___pdr_lua_tounsignedx(L,(i),NULL)

#endif
/* }============================================================== */

/*
** {======================================================================
** Debug API
** =======================================================================
*/


/*
** Event codes
*/
#define ___PDR_LUA_HOOKCALL	0
#define ___PDR_LUA_HOOKRET	1
#define ___PDR_LUA_HOOKLINE	2
#define ___PDR_LUA_HOOKCOUNT	3
#define ___PDR_LUA_HOOKTAILCALL 4


/*
** Event masks
*/
#define ___PDR_LUA_MASKCALL	(1 << ___PDR_LUA_HOOKCALL)
#define ___PDR_LUA_MASKRET	(1 << ___PDR_LUA_HOOKRET)
#define ___PDR_LUA_MASKLINE	(1 << ___PDR_LUA_HOOKLINE)
#define ___PDR_LUA_MASKCOUNT	(1 << ___PDR_LUA_HOOKCOUNT)

typedef struct ___pdr_lua_Debug ___pdr_lua_Debug;  /* activation record */


/* Functions to be called by the debugger in specific events */
typedef void (*___pdr_lua_Hook) (___pdr_lua_State *L, ___pdr_lua_Debug *ar);


___PDR_LUA_API int (___pdr_lua_getstack) (___pdr_lua_State *L, int level, ___pdr_lua_Debug *ar);
___PDR_LUA_API int (___pdr_lua_getinfo) (___pdr_lua_State *L, const char *what, ___pdr_lua_Debug *ar);
___PDR_LUA_API const char *(___pdr_lua_getlocal) (___pdr_lua_State *L, const ___pdr_lua_Debug *ar, int n);
___PDR_LUA_API const char *(___pdr_lua_setlocal) (___pdr_lua_State *L, const ___pdr_lua_Debug *ar, int n);
___PDR_LUA_API const char *(___pdr_lua_getupvalue) (___pdr_lua_State *L, int funcindex, int n);
___PDR_LUA_API const char *(___pdr_lua_setupvalue) (___pdr_lua_State *L, int funcindex, int n);

___PDR_LUA_API void *(___pdr_lua_upvalueid) (___pdr_lua_State *L, int fidx, int n);
___PDR_LUA_API void  (___pdr_lua_upvaluejoin) (___pdr_lua_State *L, int fidx1, int n1,
                                               int fidx2, int n2);

___PDR_LUA_API void (___pdr_lua_sethook) (___pdr_lua_State *L, ___pdr_lua_Hook func, int mask, int count);
___PDR_LUA_API ___pdr_lua_Hook (___pdr_lua_gethook) (___pdr_lua_State *L);
___PDR_LUA_API int (___pdr_lua_gethookmask) (___pdr_lua_State *L);
___PDR_LUA_API int (___pdr_lua_gethookcount) (___pdr_lua_State *L);


struct ___pdr_lua_Debug {
  int event;
  const char *name;	/* (n) */
  const char *namewhat;	/* (n) 'global', 'local', 'field', 'method' */
  const char *what;	/* (S) 'Lua', 'C', 'main', 'tail' */
  const char *source;	/* (S) */
  int currentline;	/* (l) */
  int linedefined;	/* (S) */
  int lastlinedefined;	/* (S) */
  unsigned char nups;	/* (u) number of upvalues */
  unsigned char nparams;/* (u) number of parameters */
  char isvararg;        /* (u) */
  char istailcall;	/* (t) */
  char short_src[___PDR_LUA_IDSIZE]; /* (S) */
  /* private part */
  struct ___pdr_CallInfo *i_ci;  /* active function */
};

/* }====================================================================== */


/******************************************************************************
* Copyright (C) 1994-2017 Lua.org, PUC-Rio.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/

} // end NS_PDR_SLUA

#endif
