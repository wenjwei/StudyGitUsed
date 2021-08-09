/*
** $Id: lauxlib.c,v 1.289 2016/12/20 18:37:00 roberto Exp $
** Auxiliary functions for building Lua libraries
** See Copyright Notice in lua.h
*/

#define ___pdr_lauxlib_c
#define ___PDR_LUA_LIB

#include "lauxlib.h"

#include "lprefix.h"
#include "lua.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#if !defined(___pdr_l_inspectstat)	/* { */
#if defined(___PDR_LUA_USE_POSIX)
#include <sys/wait.h>
/*
** use appropriate macros to interpret 'pclose' return status
*/
#define ___pdr_l_inspectstat(stat,what)  \
   if (WIFEXITED(stat)) { stat = WEXITSTATUS(stat); } \
   else if (WIFSIGNALED(stat)) { stat = WTERMSIG(stat); what = "signal"; }
#else
#define ___pdr_l_inspectstat(stat,what)  /* no op */
#endif
#endif				/* } */

/*
** This file uses only the official API of Lua.
** Any function declared here could be written as an application function.
*/

/*
** {======================================================
** Traceback
** =======================================================
*/


#define ___PDR_LEVELS1	10	/* size of the first part of the stack */
#define ___PDR_LEVELS2	11	/* size of the second part of the stack */

namespace NS_PDR_SLUA {

/*
** search for 'objidx' in table at index -1.
** return 1 + string at top if find a good name.
*/
static int findfield (___pdr_lua_State *L, int objidx, int level) {
  if (level == 0 || !___pdr_lua_istable(L, -1))
    return 0;  /* not found */
  ___pdr_lua_pushnil(L);  /* start 'next' loop */
  while (___pdr_lua_next(L, -2)) {  /* for each pair in table */
    if (___pdr_lua_type(L, -2) == ___PDR_LUA_TSTRING) {  /* ignore non-string keys */
      if (___pdr_lua_rawequal(L, objidx, -1)) {  /* found object? */
        ___pdr_lua_pop(L, 1);  /* remove value (but keep name) */
        return 1;
      }
      else if (findfield(L, objidx, level - 1)) {  /* try recursively */
        ___pdr_lua_remove(L, -2);  /* remove table (but keep name) */
        ___pdr_lua_pushliteral(L, ".");
        ___pdr_lua_insert(L, -2);  /* place '.' between the two names */
        ___pdr_lua_concat(L, 3);
        return 1;
      }
    }
    ___pdr_lua_pop(L, 1);  /* remove value */
  }
  return 0;  /* not found */
}


/*
** Search for a name for a function in all loaded modules
*/
static int pushglobalfuncname (___pdr_lua_State *L, ___pdr_lua_Debug *ar) {
  int top = ___pdr_lua_gettop(L);
  ___pdr_lua_getinfo(L, "f", ar);  /* push function */
  ___pdr_lua_getfield(L, ___PDR_LUA_REGISTRYINDEX, ___PDR_LUA_LOADED_TABLE);
  if (findfield(L, top + 1, 2)) {
    const char *name = ___pdr_lua_tostring(L, -1);
    if (strncmp(name, "_G.", 3) == 0) {  /* name start with '_G.'? */
      ___pdr_lua_pushstring(L, name + 3);  /* push name without prefix */
      ___pdr_lua_remove(L, -2);  /* remove original name */
    }
    ___pdr_lua_copy(L, -1, top + 1);  /* move name to proper place */
    ___pdr_lua_pop(L, 2);  /* remove pushed values */
    return 1;
  }
  else {
    ___pdr_lua_settop(L, top);  /* remove function and global table */
    return 0;
  }
}


static void pushfuncname (___pdr_lua_State *L, ___pdr_lua_Debug *ar) {
  if (pushglobalfuncname(L, ar)) {  /* try first a global name */
    ___pdr_lua_pushfstring(L, "function '%s'", ___pdr_lua_tostring(L, -1));
    ___pdr_lua_remove(L, -2);  /* remove name */
  }
  else if (*ar->namewhat != '\0')  /* is there a name from code? */
    ___pdr_lua_pushfstring(L, "%s '%s'", ar->namewhat, ar->name);  /* use it */
  else if (*ar->what == 'm')  /* main? */
      ___pdr_lua_pushliteral(L, "main chunk");
  else if (*ar->what != 'C')  /* for Lua functions, use <file:line> */
    ___pdr_lua_pushfstring(L, "function <%s:%d>", ar->short_src, ar->linedefined);
  else  /* nothing left... */
    ___pdr_lua_pushliteral(L, "?");
}


static int lastlevel (___pdr_lua_State *L) {
  ___pdr_lua_Debug ar;
  int li = 1, le = 1;
  /* find an upper bound */
  while (___pdr_lua_getstack(L, le, &ar)) { li = le; le *= 2; }
  /* do a binary search */
  while (li < le) {
    int m = (li + le)/2;
    if (___pdr_lua_getstack(L, m, &ar)) li = m + 1;
    else le = m;
  }
  return le - 1;
}


___PDR_LUALIB_API void ___pdr_luaL_traceback (___pdr_lua_State *L, ___pdr_lua_State *L1,
                                const char *msg, int level) {
  ___pdr_lua_Debug ar;
  int top = ___pdr_lua_gettop(L);
  int last = lastlevel(L1);
  int n1 = (last - level > ___PDR_LEVELS1 + ___PDR_LEVELS2) ? ___PDR_LEVELS1 : -1;
  if (msg)
    ___pdr_lua_pushfstring(L, "%s\n", msg);
  ___pdr_luaL_checkstack(L, 10, NULL);
  ___pdr_lua_pushliteral(L, "stack traceback:");
  while (___pdr_lua_getstack(L1, level++, &ar)) {
    if (n1-- == 0) {  /* too many levels? */
      ___pdr_lua_pushliteral(L, "\n\t...");  /* add a '...' */
      level = last - ___PDR_LEVELS2 + 1;  /* and skip to last ones */
    }
    else {
      ___pdr_lua_getinfo(L1, "Slnt", &ar);
      ___pdr_lua_pushfstring(L, "\n\t%s:", ar.short_src);
      if (ar.currentline > 0)
        ___pdr_lua_pushfstring(L, "%d:", ar.currentline);
      ___pdr_lua_pushliteral(L, " in ");
      pushfuncname(L, &ar);
      if (ar.istailcall)
        ___pdr_lua_pushliteral(L, "\n\t(...tail calls...)");
      ___pdr_lua_concat(L, ___pdr_lua_gettop(L) - top);
    }
  }
  ___pdr_lua_concat(L, ___pdr_lua_gettop(L) - top);
}

/* }====================================================== */


/*
** {======================================================
** Error-report functions
** =======================================================
*/

___PDR_LUALIB_API int ___pdr_luaL_argerror (___pdr_lua_State *L, int arg, const char *extramsg) {
  ___pdr_lua_Debug ar;
  if (!___pdr_lua_getstack(L, 0, &ar))  /* no stack frame? */
    return ___pdr_luaL_error(L, "bad argument #%d (%s)", arg, extramsg);
  ___pdr_lua_getinfo(L, "n", &ar);
  if (strcmp(ar.namewhat, "method") == 0) {
    arg--;  /* do not count 'self' */
    if (arg == 0)  /* error is in the self argument itself? */
      return ___pdr_luaL_error(L, "calling '%s' on bad self (%s)",
                           ar.name, extramsg);
  }
  if (ar.name == NULL)
    ar.name = (pushglobalfuncname(L, &ar)) ? ___pdr_lua_tostring(L, -1) : "?";
  return ___pdr_luaL_error(L, "bad argument #%d to '%s' (%s)",
                        arg, ar.name, extramsg);
}


static int typeerror (___pdr_lua_State *L, int arg, const char *tname) {
  const char *msg;
  const char *typearg;  /* name for the type of the actual argument */
  if (___pdr_luaL_getmetafield(L, arg, "__name") == ___PDR_LUA_TSTRING)
    typearg = ___pdr_lua_tostring(L, -1);  /* use the given type name */
  else if (___pdr_lua_type(L, arg) == ___PDR_LUA_TLIGHTUSERDATA)
    typearg = "light userdata";  /* special name for messages */
  else
    typearg = ___pdr_luaL_typename(L, arg);  /* standard name */
  msg = ___pdr_lua_pushfstring(L, "%s expected, got %s", tname, typearg);
  return ___pdr_luaL_argerror(L, arg, msg);
}


static void tag_error (___pdr_lua_State *L, int arg, int tag) {
  typeerror(L, arg, ___pdr_lua_typename(L, tag));
}


/*
** The use of 'lua_pushfstring' ensures this function does not
** need reserved stack space when called.
*/
___PDR_LUALIB_API void ___pdr_luaL_where (___pdr_lua_State *L, int level) {
  ___pdr_lua_Debug ar;
  if (___pdr_lua_getstack(L, level, &ar)) {  /* check function at level */
    ___pdr_lua_getinfo(L, "Sl", &ar);  /* get info about it */
    if (ar.currentline > 0) {  /* is there info? */
      ___pdr_lua_pushfstring(L, "%s:%d: ", ar.short_src, ar.currentline);
      return;
    }
  }
  ___pdr_lua_pushfstring(L, "");  /* else, no information available... */
}


/*
** Again, the use of 'lua_pushvfstring' ensures this function does
** not need reserved stack space when called. (At worst, it generates
** an error with "stack overflow" instead of the given message.)
*/
___PDR_LUALIB_API int ___pdr_luaL_error (___pdr_lua_State *L, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  ___pdr_luaL_where(L, 1);
  ___pdr_lua_pushvfstring(L, fmt, argp);
  va_end(argp);
  ___pdr_lua_concat(L, 2);
  return ___pdr_lua_error(L);
}


___PDR_LUALIB_API int ___pdr_luaL_fileresult (___pdr_lua_State *L, int stat, const char *fname) {
  int en = errno;  /* calls to Lua API may change this value */
  if (stat) {
    ___pdr_lua_pushboolean(L, 1);
    return 1;
  }
  else {
    ___pdr_lua_pushnil(L);
    if (fname)
      ___pdr_lua_pushfstring(L, "%s: %s", fname, strerror(en));
    else
      ___pdr_lua_pushstring(L, strerror(en));
    ___pdr_lua_pushinteger(L, en);
    return 3;
  }
}

___PDR_LUALIB_API int ___pdr_luaL_execresult (___pdr_lua_State *L, int stat) {
  const char *what = "exit";  /* type of termination */
  if (stat == -1)  /* error? */
    return ___pdr_luaL_fileresult(L, 0, NULL);
  else {
    ___pdr_l_inspectstat(stat, what);  /* interpret result */
    if (*what == 'e' && stat == 0)  /* successful termination? */
      ___pdr_lua_pushboolean(L, 1);
    else
      ___pdr_lua_pushnil(L);
    ___pdr_lua_pushstring(L, what);
    ___pdr_lua_pushinteger(L, stat);
    return 3;  /* return true/nil,what,code */
  }
}

/* }====================================================== */


/*
** {======================================================
** Userdata's metatable manipulation
** =======================================================
*/

___PDR_LUALIB_API int ___pdr_luaL_newmetatable (___pdr_lua_State *L, const char *tname) {
  if (___pdr_luaL_getmetatable(L, tname) != ___PDR_LUA_TNIL)  /* name already in use? */
    return 0;  /* leave previous value on top, but return 0 */
  ___pdr_lua_pop(L, 1);
  ___pdr_lua_createtable(L, 0, 2);  /* create metatable */
  ___pdr_lua_pushstring(L, tname);
  ___pdr_lua_setfield(L, -2, "__name");  /* metatable.__name = tname */
  ___pdr_lua_pushvalue(L, -1);
  ___pdr_lua_setfield(L, ___PDR_LUA_REGISTRYINDEX, tname);  /* registry.name = metatable */
  return 1;
}


___PDR_LUALIB_API void ___pdr_luaL_setmetatable (___pdr_lua_State *L, const char *tname) {
  ___pdr_luaL_getmetatable(L, tname);
  ___pdr_lua_setmetatable(L, -2);
}


___PDR_LUALIB_API void *___pdr_luaL_testudata (___pdr_lua_State *L, int ud, const char *tname) {
  void *p = ___pdr_lua_touserdata(L, ud);
  if (p != NULL) {  /* value is a userdata? */
    if (___pdr_lua_getmetatable(L, ud)) {  /* does it have a metatable? */
      ___pdr_luaL_getmetatable(L, tname);  /* get correct metatable */
      if (!___pdr_lua_rawequal(L, -1, -2))  /* not the same? */
        p = NULL;  /* value is a userdata with wrong metatable */
      ___pdr_lua_pop(L, 2);  /* remove both metatables */
      return p;
    }
  }
  return NULL;  /* value is not a userdata with a metatable */
}


___PDR_LUALIB_API void *___pdr_luaL_checkudata (___pdr_lua_State *L, int ud, const char *tname) {
  void *p = ___pdr_luaL_testudata(L, ud, tname);
  if (p == NULL) typeerror(L, ud, tname);
  return p;
}

/* }====================================================== */


/*
** {======================================================
** Argument check functions
** =======================================================
*/

___PDR_LUALIB_API int ___pdr_luaL_checkoption (___pdr_lua_State *L, int arg, const char *def,
                                 const char *const lst[]) {
  const char *name = (def) ? ___pdr_luaL_optstring(L, arg, def) :
                             ___pdr_luaL_checkstring(L, arg);
  int i;
  for (i=0; lst[i]; i++)
    if (strcmp(lst[i], name) == 0)
      return i;
  return ___pdr_luaL_argerror(L, arg,
                       ___pdr_lua_pushfstring(L, "invalid option '%s'", name));
}


/*
** Ensures the stack has at least 'space' extra slots, raising an error
** if it cannot fulfill the request. (The error handling needs a few
** extra slots to format the error message. In case of an error without
** this extra space, Lua will generate the same 'stack overflow' error,
** but without 'msg'.)
*/
___PDR_LUALIB_API void ___pdr_luaL_checkstack (___pdr_lua_State *L, int space, const char *msg) {
  if (!___pdr_lua_checkstack(L, space)) {
    if (msg)
      ___pdr_luaL_error(L, "stack overflow (%s)", msg);
    else
      ___pdr_luaL_error(L, "stack overflow");
  }
}


___PDR_LUALIB_API void ___pdr_luaL_checktype (___pdr_lua_State *L, int arg, int t) {
  if (___pdr_lua_type(L, arg) != t)
    tag_error(L, arg, t);
}


___PDR_LUALIB_API void ___pdr_luaL_checkany (___pdr_lua_State *L, int arg) {
  if (___pdr_lua_type(L, arg) == ___PDR_LUA_TNONE)
    ___pdr_luaL_argerror(L, arg, "value expected");
}


___PDR_LUALIB_API const char *___pdr_luaL_checklstring (___pdr_lua_State *L, int arg, size_t *len) {
  const char *s = ___pdr_lua_tolstring(L, arg, len);
  if (!s) tag_error(L, arg, ___PDR_LUA_TSTRING);
  return s;
}


___PDR_LUALIB_API const char *___pdr_luaL_optlstring (___pdr_lua_State *L, int arg,
                                        const char *def, size_t *len) {
  if (___pdr_lua_isnoneornil(L, arg)) {
    if (len)
      *len = (def ? strlen(def) : 0);
    return def;
  }
  else return ___pdr_luaL_checklstring(L, arg, len);
}


___PDR_LUALIB_API ___pdr_lua_Number ___pdr_luaL_checknumber (___pdr_lua_State *L, int arg) {
  int isnum;
  ___pdr_lua_Number d = ___pdr_lua_tonumberx(L, arg, &isnum);
  if (!isnum)
    tag_error(L, arg, ___PDR_LUA_TNUMBER);
  return d;
}


___PDR_LUALIB_API ___pdr_lua_Number ___pdr_luaL_optnumber (___pdr_lua_State *L, int arg, ___pdr_lua_Number def) {
  return ___pdr_luaL_opt(L, ___pdr_luaL_checknumber, arg, def);
}


static void interror (___pdr_lua_State *L, int arg) {
  if (___pdr_lua_isnumber(L, arg))
    ___pdr_luaL_argerror(L, arg, "number has no integer representation");
  else
    tag_error(L, arg, ___PDR_LUA_TNUMBER);
}


___PDR_LUALIB_API ___pdr_lua_Integer ___pdr_luaL_checkinteger (___pdr_lua_State *L, int arg) {
  int isnum;
  ___pdr_lua_Integer d = ___pdr_lua_tointegerx(L, arg, &isnum);
  if (!isnum) {
    interror(L, arg);
  }
  return d;
}


___PDR_LUALIB_API ___pdr_lua_Integer ___pdr_luaL_optinteger (___pdr_lua_State *L, int arg,
                                                      ___pdr_lua_Integer def) {
  return ___pdr_luaL_opt(L, ___pdr_luaL_checkinteger, arg, def);
}

/* }====================================================== */


/*
** {======================================================
** Generic Buffer manipulation
** =======================================================
*/

/* userdata to box arbitrary data */
typedef struct UBox {
  void *box;
  size_t bsize;
} UBox;


static void *resizebox (___pdr_lua_State *L, int idx, size_t newsize) {
  void *ud;
  ___pdr_lua_Alloc allocf = ___pdr_lua_getallocf(L, &ud);
  UBox *box = (UBox *)___pdr_lua_touserdata(L, idx);
  void *temp = allocf(ud, box->box, box->bsize, newsize);
  if (temp == NULL && newsize > 0) {  /* allocation error? */
    resizebox(L, idx, 0);  /* free buffer */
    ___pdr_luaL_error(L, "not enough memory for buffer allocation");
  }
  box->box = temp;
  box->bsize = newsize;
  return temp;
}


static int boxgc (___pdr_lua_State *L) {
  resizebox(L, 1, 0);
  return 0;
}


static void *newbox (___pdr_lua_State *L, size_t newsize) {
  UBox *box = (UBox *)___pdr_lua_newuserdata(L, sizeof(UBox));
  box->box = NULL;
  box->bsize = 0;
  if (___pdr_luaL_newmetatable(L, "LUABOX")) {  /* creating metatable? */
    ___pdr_lua_pushcfunction(L, boxgc);
    ___pdr_lua_setfield(L, -2, "__gc");  /* metatable.__gc = boxgc */
  }
  ___pdr_lua_setmetatable(L, -2);
  return resizebox(L, -1, newsize);
}


/*
** check whether buffer is using a userdata on the stack as a temporary
** buffer
*/
#define ___pdr_buffonstack(B)	((B)->b != (B)->initb)


/*
** returns a pointer to a free area with at least 'sz' bytes
*/
___PDR_LUALIB_API char *___pdr_luaL_prepbuffsize (___pdr_luaL_Buffer *B, size_t sz) {
  ___pdr_lua_State *L = B->L;
  if (B->size - B->n < sz) {  /* not enough space? */
    char *newbuff;
    size_t newsize = B->size * 2;  /* double buffer size */
    if (newsize - B->n < sz)  /* not big enough? */
      newsize = B->n + sz;
    if (newsize < B->n || newsize - B->n < sz)
      ___pdr_luaL_error(L, "buffer too large");
    /* create larger buffer */
    if (___pdr_buffonstack(B))
      newbuff = (char *)resizebox(L, -1, newsize);
    else {  /* no buffer yet */
      newbuff = (char *)newbox(L, newsize);
      memcpy(newbuff, B->b, B->n * sizeof(char));  /* copy original content */
    }
    B->b = newbuff;
    B->size = newsize;
  }
  return &B->b[B->n];
}


___PDR_LUALIB_API void ___pdr_luaL_addlstring (___pdr_luaL_Buffer *B, const char *s, size_t l) {
  if (l > 0) {  /* avoid 'memcpy' when 's' can be NULL */
    char *b = ___pdr_luaL_prepbuffsize(B, l);
    memcpy(b, s, l * sizeof(char));
    ___pdr_luaL_addsize(B, l);
  }
}


___PDR_LUALIB_API void ___pdr_luaL_addstring (___pdr_luaL_Buffer *B, const char *s) {
  ___pdr_luaL_addlstring(B, s, strlen(s));
}


___PDR_LUALIB_API void ___pdr_luaL_pushresult (___pdr_luaL_Buffer *B) {
  ___pdr_lua_State *L = B->L;
  ___pdr_lua_pushlstring(L, B->b, B->n);
  if (___pdr_buffonstack(B)) {
    resizebox(L, -2, 0);  /* delete old buffer */
    ___pdr_lua_remove(L, -2);  /* remove its header from the stack */
  }
}


___PDR_LUALIB_API void ___pdr_luaL_pushresultsize (___pdr_luaL_Buffer *B, size_t sz) {
  ___pdr_luaL_addsize(B, sz);
  ___pdr_luaL_pushresult(B);
}


___PDR_LUALIB_API void ___pdr_luaL_addvalue (___pdr_luaL_Buffer *B) {
  ___pdr_lua_State *L = B->L;
  size_t l;
  const char *s = ___pdr_lua_tolstring(L, -1, &l);
  if (___pdr_buffonstack(B))
    ___pdr_lua_insert(L, -2);  /* put value below buffer */
  ___pdr_luaL_addlstring(B, s, l);
  ___pdr_lua_remove(L, (___pdr_buffonstack(B)) ? -2 : -1);  /* remove value */
}


___PDR_LUALIB_API void ___pdr_luaL_buffinit (___pdr_lua_State *L, ___pdr_luaL_Buffer *B) {
  B->L = L;
  B->b = B->initb;
  B->n = 0;
  B->size = ___PDR_LUAL_BUFFERSIZE;
}


___PDR_LUALIB_API char *___pdr_luaL_buffinitsize (___pdr_lua_State *L, ___pdr_luaL_Buffer *B, size_t sz) {
  ___pdr_luaL_buffinit(L, B);
  return ___pdr_luaL_prepbuffsize(B, sz);
}

/* }====================================================== */


/*
** {======================================================
** Reference system
** =======================================================
*/

/* index of free-list header */
#define ___pdr_freelist	0


___PDR_LUALIB_API int ___pdr_luaL_ref (___pdr_lua_State *L, int t) {
  int ref;
  if (___pdr_lua_isnil(L, -1)) {
    ___pdr_lua_pop(L, 1);  /* remove from stack */
    return ___PDR_LUA_REFNIL;  /* 'nil' has a unique fixed reference */
  }
  t = ___pdr_lua_absindex(L, t);
  ___pdr_lua_rawgeti(L, t, ___pdr_freelist);  /* get first free element */
  ref = (int)___pdr_lua_tointeger(L, -1);  /* ref = t[freelist] */
  ___pdr_lua_pop(L, 1);  /* remove it from stack */
  if (ref != 0) {  /* any free element? */
    ___pdr_lua_rawgeti(L, t, ref);  /* remove it from list */
    ___pdr_lua_rawseti(L, t, ___pdr_freelist);  /* (t[freelist] = t[ref]) */
  }
  else  /* no free elements */
    ref = (int)___pdr_lua_rawlen(L, t) + 1;  /* get a new reference */
  ___pdr_lua_rawseti(L, t, ref);
  return ref;
}


___PDR_LUALIB_API void ___pdr_luaL_unref (___pdr_lua_State *L, int t, int ref) {
  if (ref >= 0) {
    t = ___pdr_lua_absindex(L, t);
    ___pdr_lua_rawgeti(L, t, ___pdr_freelist);
    ___pdr_lua_rawseti(L, t, ref);  /* t[ref] = t[freelist] */
    ___pdr_lua_pushinteger(L, ref);
    ___pdr_lua_rawseti(L, t, ___pdr_freelist);  /* t[freelist] = ref */
  }
}

/* }====================================================== */


/*
** {======================================================
** Load functions
** =======================================================
*/

typedef struct LoadF {
  int n;  /* number of pre-read characters */
  FILE *f;  /* file being read */
  char buff[BUFSIZ];  /* area for reading file */
} LoadF;


static const char *getF (___pdr_lua_State *L, void *ud, size_t *size) {
  LoadF *lf = (LoadF *)ud;
  (void)L;  /* not used */
  if (lf->n > 0) {  /* are there pre-read characters to be read? */
    *size = lf->n;  /* return them (chars already in buffer) */
    lf->n = 0;  /* no more pre-read characters */
  }
  else {  /* read a block from file */
    /* 'fread' can return > 0 *and* set the EOF flag. If next call to
       'getF' called 'fread', it might still wait for user input.
       The next check avoids this problem. */
    if (feof(lf->f)) return NULL;
    *size = fread(lf->buff, 1, sizeof(lf->buff), lf->f);  /* read block */
  }
  return lf->buff;
}


static int errfile (___pdr_lua_State *L, const char *what, int fnameindex) {
  const char *serr = strerror(errno);
  const char *filename = ___pdr_lua_tostring(L, fnameindex) + 1;
  ___pdr_lua_pushfstring(L, "cannot %s %s: %s", what, filename, serr);
  ___pdr_lua_remove(L, fnameindex);
  return ___PDR_LUA_ERRFILE;
}


static int skipBOM (LoadF *lf) {
  const char *p = "\xEF\xBB\xBF";  /* UTF-8 BOM mark */
  int c;
  lf->n = 0;
  do {
    c = getc(lf->f);
    if (c == EOF || c != *(const unsigned char *)p++) return c;
    lf->buff[lf->n++] = c;  /* to be read by the parser */
  } while (*p != '\0');
  lf->n = 0;  /* prefix matched; discard it */
  return getc(lf->f);  /* return next character */
}


/*
** reads the first character of file 'f' and skips an optional BOM mark
** in its beginning plus its first line if it starts with '#'. Returns
** true if it skipped the first line.  In any case, '*cp' has the
** first "valid" character of the file (after the optional BOM and
** a first-line comment).
*/
static int skipcomment (LoadF *lf, int *cp) {
  int c = *cp = skipBOM(lf);
  if (c == '#') {  /* first line is a comment (Unix exec. file)? */
    do {  /* skip first line */
      c = getc(lf->f);
    } while (c != EOF && c != '\n');
    *cp = getc(lf->f);  /* skip end-of-line, if present */
    return 1;  /* there was a comment */
  }
  else return 0;  /* no comment */
}


___PDR_LUALIB_API int ___pdr_luaL_loadfilex (___pdr_lua_State *L, const char *filename,
                                             const char *mode) {
  LoadF lf;
  int status, readstatus;
  int c;
  int fnameindex = ___pdr_lua_gettop(L) + 1;  /* index of filename on the stack */
  if (filename == NULL) {
    ___pdr_lua_pushliteral(L, "=stdin");
    lf.f = stdin;
  }
  else {
    ___pdr_lua_pushfstring(L, "@%s", filename);
    lf.f = fopen(filename, "r");
    if (lf.f == NULL) return errfile(L, "open", fnameindex);
  }
  if (skipcomment(&lf, &c))  /* read initial portion */
    lf.buff[lf.n++] = '\n';  /* add line to correct line numbers */
  if (c == ___PDR_LUA_SIGNATURE[0] && filename) {  /* binary file? */
    lf.f = freopen(filename, "rb", lf.f);  /* reopen in binary mode */
    if (lf.f == NULL) return errfile(L, "reopen", fnameindex);
    skipcomment(&lf, &c);  /* re-read initial portion */
  }
  if (c != EOF)
    lf.buff[lf.n++] = c;  /* 'c' is the first character of the stream */
  status = lua_load(L, getF, &lf, ___pdr_lua_tostring(L, -1), mode);
  readstatus = ferror(lf.f);
  if (filename) fclose(lf.f);  /* close file (even in case of errors) */
  if (readstatus) {
    ___pdr_lua_settop(L, fnameindex);  /* ignore results from 'lua_load' */
    return errfile(L, "read", fnameindex);
  }
  ___pdr_lua_remove(L, fnameindex);
  return status;
}


typedef struct LoadS {
  const char *s;
  size_t size;
} LoadS;


static const char *getS (___pdr_lua_State *L, void *ud, size_t *size) {
  LoadS *ls = (LoadS *)ud;
  (void)L;  /* not used */
  if (ls->size == 0) return NULL;
  *size = ls->size;
  ls->size = 0;
  return ls->s;
}


___PDR_LUALIB_API int ___pdr_luaL_loadbufferx (___pdr_lua_State *L, const char *buff, size_t size,
                                 const char *name, const char *mode) {
  LoadS ls;
  ls.s = buff;
  ls.size = size;
  return lua_load(L, getS, &ls, name, mode);
}


___PDR_LUALIB_API int ___pdr_luaL_loadstring (___pdr_lua_State *L, const char *s) {
  return ___pdr_luaL_loadbuffer(L, s, strlen(s), s);
}

/* }====================================================== */


___PDR_LUALIB_API int ___pdr_luaL_getmetafield (___pdr_lua_State *L, int obj, const char *event) {
  if (!___pdr_lua_getmetatable(L, obj))  /* no metatable? */
    return ___PDR_LUA_TNIL;
  else {
    int tt;
    ___pdr_lua_pushstring(L, event);
    tt = ___pdr_lua_rawget(L, -2);
    if (tt == ___PDR_LUA_TNIL)  /* is metafield nil? */
      ___pdr_lua_pop(L, 2);  /* remove metatable and metafield */
    else
      ___pdr_lua_remove(L, -2);  /* remove only metatable */
    return tt;  /* return metafield type */
  }
}


___PDR_LUALIB_API int ___pdr_luaL_callmeta (___pdr_lua_State *L, int obj, const char *event) {
  obj = ___pdr_lua_absindex(L, obj);
  if (___pdr_luaL_getmetafield(L, obj, event) == ___PDR_LUA_TNIL)  /* no metafield? */
    return 0;
  ___pdr_lua_pushvalue(L, obj);
  lua_call(L, 1, 1);
  return 1;
}


___PDR_LUALIB_API ___pdr_lua_Integer ___pdr_luaL_len (___pdr_lua_State *L, int idx) {
  ___pdr_lua_Integer l;
  int isnum;
  ___pdr_lua_len(L, idx);
  l = ___pdr_lua_tointegerx(L, -1, &isnum);
  if (!isnum)
    ___pdr_luaL_error(L, "object length is not an integer");
  ___pdr_lua_pop(L, 1);  /* remove object */
  return l;
}


___PDR_LUALIB_API const char *___pdr_luaL_tolstring (___pdr_lua_State *L, int idx, size_t *len) {
  if (___pdr_luaL_callmeta(L, idx, "__tostring")) {  /* metafield? */
    if (!___pdr_lua_isstring(L, -1))
      ___pdr_luaL_error(L, "'__tostring' must return a string");
  }
  else {
    switch (___pdr_lua_type(L, idx)) {
      case ___PDR_LUA_TNUMBER: {
        if (___pdr_lua_isinteger(L, idx))
          ___pdr_lua_pushfstring(L, "%I", (___PDR_LUAI_UACINT)___pdr_lua_tointeger(L, idx));
        else
          ___pdr_lua_pushfstring(L, "%f", (___PDR_LUAI_UACNUMBER)___pdr_lua_tonumber(L, idx));
        break;
      }
      case ___PDR_LUA_TSTRING:
        ___pdr_lua_pushvalue(L, idx);
        break;
      case ___PDR_LUA_TBOOLEAN:
        ___pdr_lua_pushstring(L, (___pdr_lua_toboolean(L, idx) ? "true" : "false"));
        break;
      case ___PDR_LUA_TNIL:
        ___pdr_lua_pushliteral(L, "nil");
        break;
      default: {
        int tt = ___pdr_luaL_getmetafield(L, idx, "__name");  /* try name */
        const char *kind = (tt == ___PDR_LUA_TSTRING) ? ___pdr_lua_tostring(L, -1) :
                                                 ___pdr_luaL_typename(L, idx);
        ___pdr_lua_pushfstring(L, "%s: %p", kind, ___pdr_lua_topointer(L, idx));
        if (tt != ___PDR_LUA_TNIL)
          ___pdr_lua_remove(L, -2);  /* remove '__name' */
        break;
      }
    }
  }
  return ___pdr_lua_tolstring(L, -1, len);
}


/*
** {======================================================
** Compatibility with 5.1 module functions
** =======================================================
*/
#if defined(___PDR_LUA_COMPAT_MODULE)

static const char *luaL_findtable (___pdr_lua_State *L, int idx,
                                   const char *fname, int szhint) {
  const char *e;
  if (idx) ___pdr_lua_pushvalue(L, idx);
  do {
    e = strchr(fname, '.');
    if (e == NULL) e = fname + strlen(fname);
    ___pdr_lua_pushlstring(L, fname, e - fname);
    if (___pdr_lua_rawget(L, -2) == ___PDR_LUA_TNIL) {  /* no such field? */
      ___pdr_lua_pop(L, 1);  /* remove this nil */
      ___pdr_lua_createtable(L, 0, (*e == '.' ? 1 : szhint)); /* new table for field */
      ___pdr_lua_pushlstring(L, fname, e - fname);
      ___pdr_lua_pushvalue(L, -2);
      ___pdr_lua_settable(L, -4);  /* set new table into field */
    }
    else if (!___pdr_lua_istable(L, -1)) {  /* field has a non-table value? */
      ___pdr_lua_pop(L, 2);  /* remove table and value */
      return fname;  /* return problematic part of the name */
    }
    ___pdr_lua_remove(L, -2);  /* remove previous table */
    fname = e + 1;
  } while (*e == '.');
  return NULL;
}


/*
** Count number of elements in a ___pdr_luaL_Reg list.
*/
static int libsize (const ___pdr_luaL_Reg *l) {
  int size = 0;
  for (; l && l->name; l++) size++;
  return size;
}


/*
** Find or create a module table with a given name. The function
** first looks at the LOADED table and, if that fails, try a
** global variable with that name. In any case, leaves on the stack
** the module table.
*/
___PDR_LUALIB_API void ___pdr_luaL_pushmodule (___pdr_lua_State *L, const char *modname,
                                 int sizehint) {
  luaL_findtable(L, ___PDR_LUA_REGISTRYINDEX, ___PDR_LUA_LOADED_TABLE, 1);
  if (___pdr_lua_getfield(L, -1, modname) != ___PDR_LUA_TTABLE) {  /* no LOADED[modname]? */
    ___pdr_lua_pop(L, 1);  /* remove previous result */
    /* try global variable (and create one if it does not exist) */
    ___pdr_lua_pushglobaltable(L);
    if (luaL_findtable(L, 0, modname, sizehint) != NULL)
      ___pdr_luaL_error(L, "name conflict for module '%s'", modname);
    ___pdr_lua_pushvalue(L, -1);
    ___pdr_lua_setfield(L, -3, modname);  /* LOADED[modname] = new table */
  }
  ___pdr_lua_remove(L, -2);  /* remove LOADED table */
}


___PDR_LUALIB_API void ___pdr_luaL_openlib (___pdr_lua_State *L, const char *libname,
                               const ___pdr_luaL_Reg *l, int nup) {
  ___pdr_luaL_checkversion(L);
  if (libname) {
    ___pdr_luaL_pushmodule(L, libname, libsize(l));  /* get/create library table */
    ___pdr_lua_insert(L, -(nup + 1));  /* move library table to below upvalues */
  }
  if (l)
    ___pdr_luaL_setfuncs(L, l, nup);
  else
    ___pdr_lua_pop(L, nup);  /* remove upvalues */
}

#endif
/* }====================================================== */

/*
** set functions from list 'l' into table at top - 'nup'; each
** function gets the 'nup' elements at the top as upvalues.
** Returns with only the table at the stack.
*/
___PDR_LUALIB_API void ___pdr_luaL_setfuncs (___pdr_lua_State *L, const ___pdr_luaL_Reg *l, int nup) {
  ___pdr_luaL_checkstack(L, nup, "too many upvalues");
  for (; l->name != NULL; l++) {  /* fill the table with given functions */
    int i;
    for (i = 0; i < nup; i++)  /* copy upvalues to the top */
      ___pdr_lua_pushvalue(L, -nup);
    ___pdr_lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
    ___pdr_lua_setfield(L, -(nup + 2), l->name);
  }
  ___pdr_lua_pop(L, nup);  /* remove upvalues */
}


/*
** ensure that stack[idx][fname] has a table and push that table
** into the stack
*/
___PDR_LUALIB_API int ___pdr_luaL_getsubtable (___pdr_lua_State *L, int idx, const char *fname) {
  if (___pdr_lua_getfield(L, idx, fname) == ___PDR_LUA_TTABLE)
    return 1;  /* table already there */
  else {
    ___pdr_lua_pop(L, 1);  /* remove previous result */
    idx = ___pdr_lua_absindex(L, idx);
    ___pdr_lua_newtable(L);
    ___pdr_lua_pushvalue(L, -1);  /* copy to be left at top */
    ___pdr_lua_setfield(L, idx, fname);  /* assign new table to field */
    return 0;  /* false, because did not find table there */
  }
}


/*
** Stripped-down 'require': After checking "loaded" table, calls 'openf'
** to open a module, registers the result in 'package.loaded' table and,
** if 'glb' is true, also registers the result in the global table.
** Leaves resulting module on the top.
*/
___PDR_LUALIB_API void ___pdr_luaL_requiref (___pdr_lua_State *L, const char *modname,
                               ___pdr_lua_CFunction openf, int glb) {
  ___pdr_luaL_getsubtable(L, ___PDR_LUA_REGISTRYINDEX, ___PDR_LUA_LOADED_TABLE);
  ___pdr_lua_getfield(L, -1, modname);  /* LOADED[modname] */
  if (!___pdr_lua_toboolean(L, -1)) {  /* package not already loaded? */
    ___pdr_lua_pop(L, 1);  /* remove field */
    ___pdr_lua_pushcfunction(L, openf);
    ___pdr_lua_pushstring(L, modname);  /* argument to open function */
    lua_call(L, 1, 1);  /* call 'openf' to open module */
    ___pdr_lua_pushvalue(L, -1);  /* make copy of module (call result) */
    ___pdr_lua_setfield(L, -3, modname);  /* LOADED[modname] = module */
  }
  ___pdr_lua_remove(L, -2);  /* remove LOADED table */
  if (glb) {
    ___pdr_lua_pushvalue(L, -1);  /* copy of module */
    ___pdr_lua_setglobal(L, modname);  /* _G[modname] = module */
  }
}


___PDR_LUALIB_API const char *___pdr_luaL_gsub (___pdr_lua_State *L, const char *s, const char *p,
                                                               const char *r) {
  const char *wild;
  size_t l = strlen(p);
  ___pdr_luaL_Buffer b;
  ___pdr_luaL_buffinit(L, &b);
  while ((wild = strstr(s, p)) != NULL) {
    ___pdr_luaL_addlstring(&b, s, wild - s);  /* push prefix */
    ___pdr_luaL_addstring(&b, r);  /* push replacement in place of pattern */
    s = wild + l;  /* continue after 'p' */
  }
  ___pdr_luaL_addstring(&b, s);  /* push last suffix */
  ___pdr_luaL_pushresult(&b);
  return ___pdr_lua_tostring(L, -1);
}


static void *l_alloc (void *ud, void *ptr, size_t osize, size_t nsize) {
  (void)ud; (void)osize;  /* not used */
  if (nsize == 0) {
    free(ptr);
    return NULL;
  }
  else
    return realloc(ptr, nsize);
}


static int panic (___pdr_lua_State *L) {
  ___pdr_lua_writestringerror("PANIC: unprotected error in call to Lua API (%s)\n",
                        ___pdr_lua_tostring(L, -1));
  return 0;  /* return to Lua to abort */
}


___PDR_LUALIB_API ___pdr_lua_State *___pdr_luaL_newstate (void) {
  ___pdr_lua_State *L = ___pdr_lua_newstate(l_alloc, NULL);
  if (L) ___pdr_lua_atpanic(L, &panic);
  return L;
}


___PDR_LUALIB_API void ___pdr_luaL_checkversion_ (___pdr_lua_State *L, ___pdr_lua_Number ver, size_t sz) {
  const ___pdr_lua_Number *v = ___pdr_lua_version(L);
  if (sz != ___PDR_LUAL_NUMSIZES)  /* check numeric types */
    ___pdr_luaL_error(L, "core and library have incompatible numeric types");
  if (v != ___pdr_lua_version(NULL))
    ___pdr_luaL_error(L, "multiple Lua VMs detected");
  else if (*v != ver)
    ___pdr_luaL_error(L, "version mismatch: app. needs %f, Lua core provides %f",
                  (___PDR_LUAI_UACNUMBER)ver, (___PDR_LUAI_UACNUMBER)*v);
}

} // end NS_PDR_SLUA