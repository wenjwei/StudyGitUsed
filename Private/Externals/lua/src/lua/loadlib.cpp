/*
** $Id: loadlib.c,v 1.130 2017/01/12 17:14:26 roberto Exp $
** Dynamic library loader for Lua
** See Copyright Notice in lua.h
**
** This module contains an implementation of loadlib for Unix systems
** that have dlfcn, an implementation for Windows, and a stub for other
** systems.
*/

#define ___pdr_loadlib_c
#define ___PDR_LUA_LIB

#include "lprefix.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#if defined(___PDR_LUA_USE_DLOPEN)
#include <dlfcn.h>
#elif defined(___PDR_LUA_DL_DLL)
#include <windows.h>
#endif


namespace NS_PDR_SLUA {

/*
** LUA_IGMARK is a mark to ignore all before it when building the
** luaopen_ function name.
*/
#if !defined (___PDR_LUA_IGMARK)
#define ___PDR_LUA_IGMARK    "-"
#endif


/*
** LUA_CSUBSEP is the character that replaces dots in submodule names
** when searching for a C loader.
** LUA_LSUBSEP is the character that replaces dots in submodule names
** when searching for a Lua loader.
*/
#if !defined(___PDR_LUA_CSUBSEP)
#define ___PDR_LUA_CSUBSEP   ___PDR_LUA_DIRSEP
#endif

#if !defined(___PDR_LUA_LSUBSEP)
#define ___PDR_LUA_LSUBSEP   ___PDR_LUA_DIRSEP
#endif


/* prefix for open functions in C libraries */
#define ___PDR_LUA_POF   "luaopen_"

/* separator for open functions in C libraries */
#define ___PDR_LUA_OFSEP "_"


/*
** unique key for table in the registry that keeps handles
** for all loaded C libraries
*/
static const int CLIBS = 0;

#define ___PDR_LIB_FAIL  "open"


#define ___pdr_setprogdir(L)           ((void)0)


/*
** system-dependent functions
*/

/*
** unload library 'lib'
*/
static void lsys_unloadlib (void *lib);

/*
** load C library in file 'path'. If 'seeglb', load with all names in
** the library global.
** Returns the library; in case of error, returns NULL plus an
** error string in the stack.
*/
// static void *lsys_load (___pdr_lua_State *L, const char *path, int seeglb);

/*
** Try to find a function named 'sym' in library 'lib'.
** Returns the function; in case of error, returns NULL plus an
** error string in the stack.
*/
// static ___pdr_lua_CFunction lsys_sym (___pdr_lua_State *L, void *lib, const char *sym);


#if defined(___PDR_LUA_USE_DLOPEN) /* { */
/*
** {========================================================================
** This is an implementation of loadlib based on the dlfcn interface.
** The dlfcn interface is available in Linux, SunOS, Solaris, IRIX, FreeBSD,
** NetBSD, AIX 4.2, HPUX 11, and  probably most other Unix flavors, at least
** as an emulation layer on top of native functions.
** =========================================================================
*/

/*
** Macro to convert pointer-to-void* to pointer-to-function. This cast
** is undefined according to ISO C, but POSIX assumes that it works.
** (The '__extension__' in gnu compilers is only to avoid warnings.)
*/
#if defined(__GNUC__)
#define ___pdr_cast_func(p) (__extension__ (___pdr_lua_CFunction)(p))
#else
#define ___pdr_cast_func(p) ((___pdr_lua_CFunction)(p))
#endif


static void lsys_unloadlib (void *lib) {
  // dlclose(lib);
}


// static void *lsys_load (___pdr_lua_State *L, const char *path, int seeglb) {
//   // void *lib = dlopen(path, RTLD_NOW | (seeglb ? RTLD_GLOBAL : RTLD_LOCAL));
//   // if (lib == NULL) lua_pushstring(L, dlerror());
//   // return lib;
//   return NULL;
// }


// static ___pdr_lua_CFunction lsys_sym (___pdr_lua_State *L, void *lib, const char *sym) {
//   // ___pdr_lua_CFunction f = cast_func(dlsym(lib, sym));
//   // if (f == NULL) lua_pushstring(L, dlerror());
//   // return f;
//   return NULL;
// }

/* }====================================================== */



#elif defined(___PDR_LUA_DL_DLL) /* }{ */
/*
** {======================================================================
** This is an implementation of loadlib for Windows using native functions.
** =======================================================================
*/

/*
** optional flags for LoadLibraryEx
*/
#if !defined(___PDR_LUA_LLE_FLAGS)
#define ___PDR_LUA_LLE_FLAGS 0
#endif


#undef ___pdr_setprogdir

/*
** Replace in the path (on the top of the stack) any occurrence
** of LUA_EXEC_DIR with the executable's path.
*/
static void ___pdr_setprogdir (___pdr_lua_State *L) {
  char buff[MAX_PATH + 1];
  char *lb;
  DWORD nsize = sizeof(buff)/sizeof(char);
  DWORD n = GetModuleFileNameA(NULL, buff, nsize);  /* get exec. name */
  if (n == 0 || n == nsize || (lb = strrchr(buff, '\\')) == NULL)
    ___pdr_luaL_error(L, "unable to get ModuleFileName");
  else {
    *lb = '\0';  /* cut name on the last '\\' to get the path */
    ___pdr_luaL_gsub(L, ___pdr_lua_tostring(L, -1), ___PDR_LUA_EXEC_DIR, buff);
    ___pdr_lua_remove(L, -2);  /* remove original string */
  }
}




static void pusherror (___pdr_lua_State *L) {
  int error = GetLastError();
  char buffer[128];
  if (FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL, error, 0, buffer, sizeof(buffer)/sizeof(char), NULL))
    ___pdr_lua_pushstring(L, buffer);
  else
    ___pdr_lua_pushfstring(L, "system error %d\n", error);
}

static void lsys_unloadlib (void *lib) {
  // FreeLibrary((HMODULE)lib);
}


// static void *lsys_load (___pdr_lua_State *L, const char *path, int seeglb) {
//   // HMODULE lib = LoadLibraryExA(path, NULL, LUA_LLE_FLAGS);
//   // (void)(seeglb);  /* not used: symbols are 'global' by default */
//   // if (lib == NULL) pusherror(L);
//   // return lib;
//   return NULL;
// }


// static ___pdr_lua_CFunction lsys_sym (___pdr_lua_State *L, void *lib, const char *sym) {
//   // ___pdr_lua_CFunction f = (___pdr_lua_CFunction)GetProcAddress((HMODULE)lib, sym);
//   // if (f == NULL) pusherror(L);
//   // return f;
//   return NULL;
// }

/* }====================================================== */


#else       /* }{ */
/*
** {======================================================
** Fallback for other systems
** =======================================================
*/

#undef ___PDR_LIB_FAIL
#define ___PDR_LIB_FAIL  "absent"


#define ___PDR_DLMSG "dynamic libraries not enabled; check your Lua installation"


static void lsys_unloadlib (void *lib) {
  // (void)(lib);  /* not used */
}


// static void *lsys_load (___pdr_lua_State *L, const char *path, int seeglb) {
//   (void)(path); (void)(seeglb);  /* not used */
//   lua_pushliteral(L, DLMSG);
//   return NULL;
// }


// static ___pdr_lua_CFunction lsys_sym (___pdr_lua_State *L, void *lib, const char *sym) {
//   // (void)(lib); (void)(sym);  /* not used */
//   // lua_pushliteral(L, DLMSG);
//   return NULL;
// }

/* }====================================================== */
#endif        /* } */


/*
** {==================================================================
** Set Paths
** ===================================================================
*/

/*
** LUA_PATH_VAR and LUA_CPATH_VAR are the names of the environment
** variables that Lua check to set its paths.
*/
#if !defined(___PDR_LUA_PATH_VAR)
#define ___PDR_LUA_PATH_VAR    "LUA_PATH"
#endif

#if !defined(___PDR_LUA_CPATH_VAR)
#define ___PDR_LUA_CPATH_VAR   "LUA_CPATH"
#endif


#define ___PDR_AUXMARK         "\1"  /* auxiliary mark */


/*
** return registry.LUA_NOENV as a boolean
*/
static int noenv (___pdr_lua_State *L) {
  int b;
  ___pdr_lua_getfield(L, ___PDR_LUA_REGISTRYINDEX, "LUA_NOENV");
  b = ___pdr_lua_toboolean(L, -1);
  ___pdr_lua_pop(L, 1);  /* remove value */
  return b;
}


/*
** Set a path
*/
static void setpath (___pdr_lua_State *L, const char *fieldname,
                                   const char *envname,
                                   const char *dft) {
  const char *nver = ___pdr_lua_pushfstring(L, "%s%s", envname, ___PDR_LUA_VERSUFFIX);
  const char *path = getenv(nver);  /* use versioned name */
  if (path == NULL)  /* no environment variable? */
    path = getenv(envname);  /* try unversioned name */
  if (path == NULL || noenv(L))  /* no environment variable? */
    ___pdr_lua_pushstring(L, dft);  /* use default */
  else {
    /* replace ";;" by ";AUXMARK;" and then AUXMARK by default path */
    path = ___pdr_luaL_gsub(L, path, ___PDR_LUA_PATH_SEP ___PDR_LUA_PATH_SEP,
                              ___PDR_LUA_PATH_SEP ___PDR_AUXMARK ___PDR_LUA_PATH_SEP);
    ___pdr_luaL_gsub(L, path, ___PDR_AUXMARK, dft);
    ___pdr_lua_remove(L, -2); /* remove result from 1st 'gsub' */
  }
  ___pdr_setprogdir(L);
  ___pdr_lua_setfield(L, -3, fieldname);  /* package[fieldname] = path value */
  ___pdr_lua_pop(L, 1);  /* pop versioned variable name */
}

/* }================================================================== */


/*
** return registry.CLIBS[path]
*/
static void *checkclib (___pdr_lua_State *L, const char *path) {
  void *plib;
  ___pdr_lua_rawgetp(L, ___PDR_LUA_REGISTRYINDEX, &CLIBS);
  ___pdr_lua_getfield(L, -1, path);
  plib = ___pdr_lua_touserdata(L, -1);  /* plib = CLIBS[path] */
  ___pdr_lua_pop(L, 2);  /* pop CLIBS table and 'plib' */
  return plib;
}


/*
** registry.CLIBS[path] = plib        -- for queries
** registry.CLIBS[#CLIBS + 1] = plib  -- also keep a list of all libraries
*/
static void addtoclib (___pdr_lua_State *L, const char *path, void *plib) {
  ___pdr_lua_rawgetp(L, ___PDR_LUA_REGISTRYINDEX, &CLIBS);
  ___pdr_lua_pushlightuserdata(L, plib);
  ___pdr_lua_pushvalue(L, -1);
  ___pdr_lua_setfield(L, -3, path);  /* CLIBS[path] = plib */
  ___pdr_lua_rawseti(L, -2, ___pdr_luaL_len(L, -2) + 1);  /* CLIBS[#CLIBS + 1] = plib */
  ___pdr_lua_pop(L, 1);  /* pop CLIBS table */
}


/*
** __gc tag method for CLIBS table: calls 'lsys_unloadlib' for all lib
** handles in list CLIBS
*/
static int gctm (___pdr_lua_State *L) {
  ___pdr_lua_Integer n = ___pdr_luaL_len(L, 1);
  for (; n >= 1; n--) {  /* for each handle, in reverse order */
    ___pdr_lua_rawgeti(L, 1, n);  /* get handle CLIBS[n] */
    lsys_unloadlib(___pdr_lua_touserdata(L, -1));
    ___pdr_lua_pop(L, 1);  /* pop handle */
  }
  return 0;
}



/* error codes for 'lookforfunc' */
#define ___PDR_ERRLIB    1
#define ___PDR_ERRFUNC   2

/*
** Look for a C function named 'sym' in a dynamically loaded library
** 'path'.
** First, check whether the library is already loaded; if not, try
** to load it.
** Then, if 'sym' is '*', return true (as library has been loaded).
** Otherwise, look for symbol 'sym' in the library and push a
** C function with that symbol.
** Return 0 and 'true' or a function in the stack; in case of
** errors, return an error code and an error message in the stack.
*/
static int lookforfunc (___pdr_lua_State *L, const char *path, const char *sym) {
  void *reg = checkclib(L, path);  /* check loaded C libraries */
  if (reg == NULL) {  /* must load library? */
    reg = NULL; // lsys_load(L, path, *sym == '*');  /* global symbols if 'sym'=='*' */
    if (reg == NULL) return ___PDR_ERRLIB;  /* unable to load library */
    addtoclib(L, path, reg);
  }
  if (*sym == '*') {  /* loading only library (no function)? */
    ___pdr_lua_pushboolean(L, 1);  /* return 'true' */
    return 0;  /* no errors */
  }
  else {
    ___pdr_lua_CFunction f = NULL; // lsys_sym(L, reg, sym);
    if (f == NULL)
      return ___PDR_ERRFUNC;  /* unable to find function */
    ___pdr_lua_pushcfunction(L, f);  /* else create new function */
    return 0;  /* no errors */
  }
}


static int ll_loadlib (___pdr_lua_State *L) {
  const char *path = ___pdr_luaL_checkstring(L, 1);
  const char *init = ___pdr_luaL_checkstring(L, 2);
  int stat = lookforfunc(L, path, init);
  if (stat == 0)  /* no errors? */
    return 1;  /* return the loaded function */
  else {  /* error; error message is on stack top */
    ___pdr_lua_pushnil(L);
    ___pdr_lua_insert(L, -2);
    ___pdr_lua_pushstring(L, (stat == ___PDR_ERRLIB) ?  ___PDR_LIB_FAIL : "init");
    return 3;  /* return nil, error message, and where */
  }
}



/*
** {======================================================
** 'require' function
** =======================================================
*/


static int readable (const char *filename) {
  FILE *f = fopen(filename, "r");  /* try to open file */
  if (f == NULL) return 0;  /* open failed */
  fclose(f);
  return 1;
}


static const char *pushnexttemplate (___pdr_lua_State *L, const char *path) {
  const char *l;
  while (*path == *___PDR_LUA_PATH_SEP) path++;  /* skip separators */
  if (*path == '\0') return NULL;  /* no more templates */
  l = strchr(path, *___PDR_LUA_PATH_SEP);  /* find next separator */
  if (l == NULL) l = path + strlen(path);
  ___pdr_lua_pushlstring(L, path, l - path);  /* template */
  return l;
}


static const char *searchpath (___pdr_lua_State *L, const char *name,
                                             const char *path,
                                             const char *sep,
                                             const char *dirsep) {
  ___pdr_luaL_Buffer msg;  /* to build error message */
  ___pdr_luaL_buffinit(L, &msg);
  if (*sep != '\0')  /* non-empty separator? */
    name = ___pdr_luaL_gsub(L, name, sep, dirsep);  /* replace it by 'dirsep' */
  while ((path = pushnexttemplate(L, path)) != NULL) {
    const char *filename = ___pdr_luaL_gsub(L, ___pdr_lua_tostring(L, -1),
                                     ___PDR_LUA_PATH_MARK, name);
    ___pdr_lua_remove(L, -2);  /* remove path template */
    if (readable(filename))  /* does file exist and is readable? */
      return filename;  /* return that file name */
    ___pdr_lua_pushfstring(L, "\n\tno file '%s'", filename);
    ___pdr_lua_remove(L, -2);  /* remove file name */
    ___pdr_luaL_addvalue(&msg);  /* concatenate error msg. entry */
  }
  ___pdr_luaL_pushresult(&msg);  /* create error message */
  return NULL;  /* not found */
}


static int ll_searchpath (___pdr_lua_State *L) {
  const char *f = searchpath(L, ___pdr_luaL_checkstring(L, 1),
                                ___pdr_luaL_checkstring(L, 2),
                                ___pdr_luaL_optstring(L, 3, "."),
                                ___pdr_luaL_optstring(L, 4, ___PDR_LUA_DIRSEP));
  if (f != NULL) return 1;
  else {  /* error message is on top of the stack */
    ___pdr_lua_pushnil(L);
    ___pdr_lua_insert(L, -2);
    return 2;  /* return nil + error message */
  }
}


static const char *findfile (___pdr_lua_State *L, const char *name,
                                           const char *pname,
                                           const char *dirsep) {
  const char *path;
  ___pdr_lua_getfield(L, ___pdr_lua_upvalueindex(1), pname);
  path = ___pdr_lua_tostring(L, -1);
  if (path == NULL)
    ___pdr_luaL_error(L, "'package.%s' must be a string", pname);
  return searchpath(L, name, path, ".", dirsep);
}


static int checkload (___pdr_lua_State *L, int stat, const char *filename) {
  if (stat) {  /* module loaded successfully? */
    ___pdr_lua_pushstring(L, filename);  /* will be 2nd argument to module */
    return 2;  /* return open function and file name */
  }
  else
    return ___pdr_luaL_error(L, "error loading module '%s' from file '%s':\n\t%s",
                          ___pdr_lua_tostring(L, 1), filename, ___pdr_lua_tostring(L, -1));
}


static int searcher_Lua (___pdr_lua_State *L) {
  const char *filename;
  const char *name = ___pdr_luaL_checkstring(L, 1);
  filename = findfile(L, name, "path", ___PDR_LUA_LSUBSEP);
  if (filename == NULL) return 1;  /* module not found in this path */
  return checkload(L, (___pdr_luaL_loadfile(L, filename) == ___PDR_LUA_OK), filename);
}


/*
** Try to find a load function for module 'modname' at file 'filename'.
** First, change '.' to '_' in 'modname'; then, if 'modname' has
** the form X-Y (that is, it has an "ignore mark"), build a function
** name "luaopen_X" and look for it. (For compatibility, if that
** fails, it also tries "luaopen_Y".) If there is no ignore mark,
** look for a function named "luaopen_modname".
*/
static int loadfunc (___pdr_lua_State *L, const char *filename, const char *modname) {
  const char *openfunc;
  const char *mark;
  modname = ___pdr_luaL_gsub(L, modname, ".", ___PDR_LUA_OFSEP);
  mark = strchr(modname, *___PDR_LUA_IGMARK);
  if (mark) {
    int stat;
    openfunc = ___pdr_lua_pushlstring(L, modname, mark - modname);
    openfunc = ___pdr_lua_pushfstring(L, ___PDR_LUA_POF"%s", openfunc);
    stat = lookforfunc(L, filename, openfunc);
    if (stat != ___PDR_ERRFUNC) return stat;
    modname = mark + 1;  /* else go ahead and try old-style name */
  }
  openfunc = ___pdr_lua_pushfstring(L, ___PDR_LUA_POF"%s", modname);
  return lookforfunc(L, filename, openfunc);
}


static int searcher_C (___pdr_lua_State *L) {
  const char *name = ___pdr_luaL_checkstring(L, 1);
  const char *filename = findfile(L, name, "cpath", ___PDR_LUA_CSUBSEP);
  if (filename == NULL) return 1;  /* module not found in this path */
  return checkload(L, (loadfunc(L, filename, name) == 0), filename);
}


static int searcher_Croot (___pdr_lua_State *L) {
  const char *filename;
  const char *name = ___pdr_luaL_checkstring(L, 1);
  const char *p = strchr(name, '.');
  int stat;
  if (p == NULL) return 0;  /* is root */
  ___pdr_lua_pushlstring(L, name, p - name);
  filename = findfile(L, ___pdr_lua_tostring(L, -1), "cpath", ___PDR_LUA_CSUBSEP);
  if (filename == NULL) return 1;  /* root not found */
  if ((stat = loadfunc(L, filename, name)) != 0) {
    if (stat != ___PDR_ERRFUNC)
      return checkload(L, 0, filename);  /* real error */
    else {  /* open function not found */
      ___pdr_lua_pushfstring(L, "\n\tno module '%s' in file '%s'", name, filename);
      return 1;
    }
  }
  ___pdr_lua_pushstring(L, filename);  /* will be 2nd argument to module */
  return 2;
}


static int searcher_preload (___pdr_lua_State *L) {
  const char *name = ___pdr_luaL_checkstring(L, 1);
  ___pdr_lua_getfield(L, ___PDR_LUA_REGISTRYINDEX, ___PDR_LUA_PRELOAD_TABLE);
  if (___pdr_lua_getfield(L, -1, name) == ___PDR_LUA_TNIL)  /* not found? */
    ___pdr_lua_pushfstring(L, "\n\tno field package.preload['%s']", name);
  return 1;
}


static void findloader (___pdr_lua_State *L, const char *name) {
  int i;
  ___pdr_luaL_Buffer msg;  /* to build error message */
  ___pdr_luaL_buffinit(L, &msg);
  /* push 'package.searchers' to index 3 in the stack */
  if (___pdr_lua_getfield(L, ___pdr_lua_upvalueindex(1), "searchers") != ___PDR_LUA_TTABLE)
    ___pdr_luaL_error(L, "'package.searchers' must be a table");
  /*  iterate over available searchers to find a loader */
  for (i = 1; ; i++) {
    if (___pdr_lua_rawgeti(L, 3, i) == ___PDR_LUA_TNIL) {  /* no more searchers? */
      ___pdr_lua_pop(L, 1);  /* remove nil */
      ___pdr_luaL_pushresult(&msg);  /* create error message */
      ___pdr_luaL_error(L, "module '%s' not found:%s", name, ___pdr_lua_tostring(L, -1));
    }
    ___pdr_lua_pushstring(L, name);
    lua_call(L, 1, 2);  /* call it */
    if (___pdr_lua_isfunction(L, -2))  /* did it find a loader? */
      return;  /* module loader found */
    else if (___pdr_lua_isstring(L, -2)) {  /* searcher returned error message? */
      ___pdr_lua_pop(L, 1);  /* remove extra return */
      ___pdr_luaL_addvalue(&msg);  /* concatenate error message */
    }
    else
      ___pdr_lua_pop(L, 2);  /* remove both returns */
  }
}


static int ll_require (___pdr_lua_State *L) {
  const char *name = ___pdr_luaL_checkstring(L, 1);
  ___pdr_lua_settop(L, 1);  /* LOADED table will be at index 2 */
  ___pdr_lua_getfield(L, ___PDR_LUA_REGISTRYINDEX, ___PDR_LUA_LOADED_TABLE);
  ___pdr_lua_getfield(L, 2, name);  /* LOADED[name] */
  if (___pdr_lua_toboolean(L, -1))  /* is it there? */
    return 1;  /* package is already loaded */
  /* else must load package */
  ___pdr_lua_pop(L, 1);  /* remove 'getfield' result */
  findloader(L, name);
  ___pdr_lua_pushstring(L, name);  /* pass name as argument to module loader */
  ___pdr_lua_insert(L, -2);  /* name is 1st argument (before search data) */
  lua_call(L, 2, 1);  /* run loader to load module */
  if (!___pdr_lua_isnil(L, -1))  /* non-nil return? */
    ___pdr_lua_setfield(L, 2, name);  /* LOADED[name] = returned value */
  if (___pdr_lua_getfield(L, 2, name) == ___PDR_LUA_TNIL) {   /* module set no value? */
    ___pdr_lua_pushboolean(L, 1);  /* use true as result */
    ___pdr_lua_pushvalue(L, -1);  /* extra copy to be returned */
    ___pdr_lua_setfield(L, 2, name);  /* LOADED[name] = true */
  }
  return 1;
}

/* }====================================================== */



/*
** {======================================================
** 'module' function
** =======================================================
*/
#if defined(___PDR_LUA_COMPAT_MODULE)

/*
** changes the environment variable of calling function
*/
static void set_env (___pdr_lua_State *L) {
  ___pdr_lua_Debug ar;
  if (___pdr_lua_getstack(L, 1, &ar) == 0 ||
      ___pdr_lua_getinfo(L, "f", &ar) == 0 ||  /* get calling function */
      ___pdr_lua_iscfunction(L, -1))
    ___pdr_luaL_error(L, "'module' not called from a Lua function");
  ___pdr_lua_pushvalue(L, -2);  /* copy new environment table to top */
  ___pdr_lua_setupvalue(L, -2, 1);
  ___pdr_lua_pop(L, 1);  /* remove function */
}


static void dooptions (___pdr_lua_State *L, int n) {
  int i;
  for (i = 2; i <= n; i++) {
    if (___pdr_lua_isfunction(L, i)) {  /* avoid 'calling' extra info. */
      ___pdr_lua_pushvalue(L, i);  /* get option (a function) */
      ___pdr_lua_pushvalue(L, -2);  /* module */
      lua_call(L, 1, 0);
    }
  }
}


static void modinit (___pdr_lua_State *L, const char *modname) {
  const char *dot;
  ___pdr_lua_pushvalue(L, -1);
  ___pdr_lua_setfield(L, -2, "_M");  /* module._M = module */
  ___pdr_lua_pushstring(L, modname);
  ___pdr_lua_setfield(L, -2, "_NAME");
  dot = strrchr(modname, '.');  /* look for last dot in module name */
  if (dot == NULL) dot = modname;
  else dot++;
  /* set _PACKAGE as package name (full module name minus last part) */
  ___pdr_lua_pushlstring(L, modname, dot - modname);
  ___pdr_lua_setfield(L, -2, "_PACKAGE");
}


static int ll_module (___pdr_lua_State *L) {
  const char *modname = ___pdr_luaL_checkstring(L, 1);
  int lastarg = ___pdr_lua_gettop(L);  /* last parameter */
  ___pdr_luaL_pushmodule(L, modname, 1);  /* get/create module table */
  /* check whether table already has a _NAME field */
  if (___pdr_lua_getfield(L, -1, "_NAME") != ___PDR_LUA_TNIL)
    ___pdr_lua_pop(L, 1);  /* table is an initialized module */
  else {  /* no; initialize it */
    ___pdr_lua_pop(L, 1);
    modinit(L, modname);
  }
  ___pdr_lua_pushvalue(L, -1);
  set_env(L);
  dooptions(L, lastarg);
  return 1;
}


static int ll_seeall (___pdr_lua_State *L) {
  ___pdr_luaL_checktype(L, 1, ___PDR_LUA_TTABLE);
  if (!___pdr_lua_getmetatable(L, 1)) {
    ___pdr_lua_createtable(L, 0, 1); /* create new metatable */
    ___pdr_lua_pushvalue(L, -1);
    ___pdr_lua_setmetatable(L, 1);
  }
  ___pdr_lua_pushglobaltable(L);
  ___pdr_lua_setfield(L, -2, "__index");  /* mt.__index = _G */
  return 0;
}

#endif
/* }====================================================== */



static const ___pdr_luaL_Reg pk_funcs[] = {
  {"loadlib", ll_loadlib},
  {"searchpath", ll_searchpath},
#if defined(___PDR_LUA_COMPAT_MODULE)
  {"seeall", ll_seeall},
#endif
  /* placeholders */
  {"preload", NULL},
  {"cpath", NULL},
  {"path", NULL},
  {"searchers", NULL},
  {"loaded", NULL},
  {NULL, NULL}
};


static const ___pdr_luaL_Reg ll_funcs[] = {
#if defined(___PDR_LUA_COMPAT_MODULE)
  {"module", ll_module},
#endif
  {"require", ll_require},
  {NULL, NULL}
};


static void createsearcherstable (___pdr_lua_State *L) {
  static const ___pdr_lua_CFunction searchers[] =
    {searcher_preload, searcher_Lua, searcher_C, searcher_Croot, NULL};
  int i;
  /* create 'searchers' table */
  ___pdr_lua_createtable(L, sizeof(searchers)/sizeof(searchers[0]) - 1, 0);
  /* fill it with predefined searchers */
  for (i=0; searchers[i] != NULL; i++) {
    ___pdr_lua_pushvalue(L, -2);  /* set 'package' as upvalue for all searchers */
    ___pdr_lua_pushcclosure(L, searchers[i], 1);
    ___pdr_lua_rawseti(L, -2, i+1);
  }
#if defined(___PDR_LUA_COMPAT_LOADERS)
  ___pdr_lua_pushvalue(L, -1);  /* make a copy of 'searchers' table */
  ___pdr_lua_setfield(L, -3, "loaders");  /* put it in field 'loaders' */
#endif
  ___pdr_lua_setfield(L, -2, "searchers");  /* put it in field 'searchers' */
}


/*
** create table CLIBS to keep track of loaded C libraries,
** setting a finalizer to close all libraries when closing state.
*/
static void createclibstable (___pdr_lua_State *L) {
  ___pdr_lua_newtable(L);  /* create CLIBS table */
  ___pdr_lua_createtable(L, 0, 1);  /* create metatable for CLIBS */
  ___pdr_lua_pushcfunction(L, gctm);
  ___pdr_lua_setfield(L, -2, "__gc");  /* set finalizer for CLIBS table */
  ___pdr_lua_setmetatable(L, -2);
  ___pdr_lua_rawsetp(L, ___PDR_LUA_REGISTRYINDEX, &CLIBS);  /* set CLIBS table in registry */
}


___PDR_LUAMOD_API int ___pdr_luaopen_package (___pdr_lua_State *L) {
  createclibstable(L);
  ___pdr_luaL_newlib(L, pk_funcs);  /* create 'package' table */
  createsearcherstable(L);
  /* set paths */
  setpath(L, "path", ___PDR_LUA_PATH_VAR, ___PDR_LUA_PATH_DEFAULT);
  setpath(L, "cpath", ___PDR_LUA_CPATH_VAR, LUA_CPATH_DEFAULT);
  /* store config information */
  ___pdr_lua_pushliteral(L, ___PDR_LUA_DIRSEP "\n" ___PDR_LUA_PATH_SEP "\n" ___PDR_LUA_PATH_MARK "\n"
                     ___PDR_LUA_EXEC_DIR "\n" ___PDR_LUA_IGMARK "\n");
  ___pdr_lua_setfield(L, -2, "config");
  /* set field 'loaded' */
  ___pdr_luaL_getsubtable(L, ___PDR_LUA_REGISTRYINDEX, ___PDR_LUA_LOADED_TABLE);
  ___pdr_lua_setfield(L, -2, "loaded");
  /* set field 'preload' */
  ___pdr_luaL_getsubtable(L, ___PDR_LUA_REGISTRYINDEX, ___PDR_LUA_PRELOAD_TABLE);
  ___pdr_lua_setfield(L, -2, "preload");
  ___pdr_lua_pushglobaltable(L);
  ___pdr_lua_pushvalue(L, -2);  /* set 'package' as upvalue for next lib */
  ___pdr_luaL_setfuncs(L, ll_funcs, 1);  /* open lib into global table */
  ___pdr_lua_pop(L, 1);  /* pop global table */
  return 1;  /* return 'package' table */
}

} // end NS_PDR_SLUA