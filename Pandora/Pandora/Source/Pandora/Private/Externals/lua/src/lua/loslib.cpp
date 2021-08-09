/*
** $Id: loslib.c,v 1.65 2016/07/18 17:58:58 roberto Exp $
** Standard Operating System library
** See Copyright Notice in lua.h
*/

#define ___pdr_loslib_c
#define ___PDR_LUA_LIB

#include "lprefix.h"


#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"


/*
** {==================================================================
** Configuration for 'tmpnam':
** By default, Lua uses tmpnam except when POSIX is available, where
** it uses mkstemp.
** ===================================================================
*/
#if !defined(___pdr_lua_tmpnam)	/* { */

#if defined(___PDR_LUA_USE_POSIX)	/* { */

#include <unistd.h>

#define ___PDR_LUA_TMPNAMBUFSIZE	32

#if !defined(___PDR_LUA_TMPNAMTEMPLATE)
#define ___PDR_LUA_TMPNAMTEMPLATE	"/tmp/lua_XXXXXX"
#endif

#define ___pdr_lua_tmpnam(b,e) { \
        strcpy(b, ___PDR_LUA_TMPNAMTEMPLATE); \
        e = mkstemp(b); \
        if (e != -1) close(e); \
        e = (e == -1); }

#else				/* }{ */

/* ISO C definitions */
#define ___PDR_LUA_TMPNAMBUFSIZE	L_tmpnam
#define ___pdr_lua_tmpnam(b,e)		{ e = (tmpnam(b) == NULL); }

#endif				/* } */

#endif				/* } */
/* }================================================================== */

/*
** {==================================================================
** List of valid conversion specifiers for the 'strftime' function;
** options are grouped by length; group of length 2 start with '||'.
** ===================================================================
*/
#if !defined(___PDR_LUA_STRFTIMEOPTIONS)	/* { */

/* options for ANSI C 89 (only 1-char options) */
#define ___PDR_L_STRFTIMEC89		"aAbBcdHIjmMpSUwWxXyYZ%"

/* options for ISO C 99 and POSIX */
#define ___PDR_L_STRFTIMEC99 "aAbBcCdDeFgGhHIjmMnprRStTuUVwWxXyYzZ%" \
    "||" "EcECExEXEyEY" "OdOeOHOIOmOMOSOuOUOVOwOWOy"  /* two-char options */

/* options for Windows */
#define ___PDR_L_STRFTIMEWIN "aAbBcdHIjmMpSUwWxXyYzZ%" \
    "||" "#c#x#d#H#I#j#m#M#S#U#w#W#y#Y"  /* two-char options */

#if defined(___PDR_LUA_USE_WINDOWS)
#define ___PDR_LUA_STRFTIMEOPTIONS	___PDR_L_STRFTIMEWIN
#elif defined(___PDR_LUA_USE_C89)
#define ___PDR_LUA_STRFTIMEOPTIONS	___PDR_L_STRFTIMEC89
#else  /* C99 specification */
#define ___PDR_LUA_STRFTIMEOPTIONS	___PDR_L_STRFTIMEC99
#endif

#endif					/* } */
/* }================================================================== */


namespace NS_PDR_SLUA {


/*
** {==================================================================
** Configuration for time-related stuff
** ===================================================================
*/

#if !defined(l_time_t)		/* { */
/*
** type to represent time_t in Lua
*/
#define ___pdr_l_timet			___pdr_lua_Integer
#define ___pdr_l_pushtime(L,t)		___pdr_lua_pushinteger(L,(___pdr_lua_Integer)(t))

static time_t l_checktime (___pdr_lua_State *L, int arg) {
  ___pdr_lua_Integer t = ___pdr_luaL_checkinteger(L, arg);
  ___pdr_luaL_argcheck(L, (time_t)t == t, arg, "time out-of-bounds");
  return (time_t)t;
}

#endif				/* } */


#if !defined(___pdr_l_gmtime)		/* { */
/*
** By default, Lua uses gmtime/localtime, except when POSIX is available,
** where it uses gmtime_r/localtime_r
*/

#if defined(___PDR_LUA_USE_POSIX)	/* { */

#define ___pdr_l_gmtime(t,r)		gmtime_r(t,r)
#define ___pdr_l_localtime(t,r)	localtime_r(t,r)

#else				/* }{ */

/* ISO C definitions */
#define ___pdr_l_gmtime(t,r)		((void)(r)->tm_sec, gmtime(t))
#define ___pdr_l_localtime(t,r)  	((void)(r)->tm_sec, localtime(t))

#endif				/* } */

#endif				/* } */

/* }================================================================== */

static int os_execute (___pdr_lua_State *L) {
  // const char *cmd = luaL_optstring(L, 1, NULL);
  // int stat = system(cmd);
  // if (cmd != NULL)
  //   return luaL_execresult(L, stat);
  // else {
  //   lua_pushboolean(L, stat);  /* true if there is a shell */
  //   return 1;
  // }
  return 0;
}


static int os_remove (___pdr_lua_State *L) {
  const char *filename = ___pdr_luaL_checkstring(L, 1);
  return ___pdr_luaL_fileresult(L, remove(filename) == 0, filename);
}


static int os_rename (___pdr_lua_State *L) {
  const char *fromname = ___pdr_luaL_checkstring(L, 1);
  const char *toname = ___pdr_luaL_checkstring(L, 2);
  return ___pdr_luaL_fileresult(L, rename(fromname, toname) == 0, NULL);
}


static int os_tmpname (___pdr_lua_State *L) {
  // char buff[LUA_TMPNAMBUFSIZE];
  // int err;
  // lua_tmpnam(buff, err);
  // if (err)
  //   return luaL_error(L, "unable to generate a unique filename");
  // lua_pushstring(L, buff);
  // return 1;
  return 0;
}


static int os_getenv (___pdr_lua_State *L) {
  ___pdr_lua_pushstring(L, getenv(___pdr_luaL_checkstring(L, 1)));  /* if NULL push nil */
  return 1;
}


static int os_clock (___pdr_lua_State *L) {
  ___pdr_lua_pushnumber(L, ((___pdr_lua_Number)clock())/(___pdr_lua_Number)CLOCKS_PER_SEC);
  return 1;
}


/*
** {======================================================
** Time/Date operations
** { year=%Y, month=%m, day=%d, hour=%H, min=%M, sec=%S,
**   wday=%w+1, yday=%j, isdst=? }
** =======================================================
*/

static void setfield (___pdr_lua_State *L, const char *key, int value) {
  ___pdr_lua_pushinteger(L, value);
  ___pdr_lua_setfield(L, -2, key);
}

static void setboolfield (___pdr_lua_State *L, const char *key, int value) {
  if (value < 0)  /* undefined? */
    return;  /* does not set field */
  ___pdr_lua_pushboolean(L, value);
  ___pdr_lua_setfield(L, -2, key);
}


/*
** Set all fields from structure 'tm' in the table on top of the stack
*/
static void setallfields (___pdr_lua_State *L, struct tm *stm) {
  setfield(L, "sec", stm->tm_sec);
  setfield(L, "min", stm->tm_min);
  setfield(L, "hour", stm->tm_hour);
  setfield(L, "day", stm->tm_mday);
  setfield(L, "month", stm->tm_mon + 1);
  setfield(L, "year", stm->tm_year + 1900);
  setfield(L, "wday", stm->tm_wday + 1);
  setfield(L, "yday", stm->tm_yday + 1);
  setboolfield(L, "isdst", stm->tm_isdst);
}


static int getboolfield (___pdr_lua_State *L, const char *key) {
  int res;
  res = (___pdr_lua_getfield(L, -1, key) == ___PDR_LUA_TNIL) ? -1 : ___pdr_lua_toboolean(L, -1);
  ___pdr_lua_pop(L, 1);
  return res;
}


/* maximum value for date fields (to avoid arithmetic overflows with 'int') */
#if !defined(___PDR_L_MAXDATEFIELD)
#define ___PDR_L_MAXDATEFIELD	(INT_MAX / 2)
#endif

static int getfield (___pdr_lua_State *L, const char *key, int d, int delta) {
  int isnum;
  int t = ___pdr_lua_getfield(L, -1, key);  /* get field and its type */
  ___pdr_lua_Integer res = ___pdr_lua_tointegerx(L, -1, &isnum);
  if (!isnum) {  /* field is not an integer? */
    if (t != ___PDR_LUA_TNIL)  /* some other value? */
      return ___pdr_luaL_error(L, "field '%s' is not an integer", key);
    else if (d < 0)  /* absent field; no default? */
      return ___pdr_luaL_error(L, "field '%s' missing in date table", key);
    res = d;
  }
  else {
    if (!(-___PDR_L_MAXDATEFIELD <= res && res <= ___PDR_L_MAXDATEFIELD))
      return ___pdr_luaL_error(L, "field '%s' is out-of-bound", key);
    res -= delta;
  }
  ___pdr_lua_pop(L, 1);
  return (int)res;
}


static const char *checkoption (___pdr_lua_State *L, const char *conv,
                                ptrdiff_t convlen, char *buff) {
  const char *option = ___PDR_LUA_STRFTIMEOPTIONS;
  int oplen = 1;  /* length of options being checked */
  for (; *option != '\0' && oplen <= convlen; option += oplen) {
    if (*option == '|')  /* next block? */
      oplen++;  /* will check options with next length (+1) */
    else if (memcmp(conv, option, oplen) == 0) {  /* match? */
      memcpy(buff, conv, oplen);  /* copy valid option to buffer */
      buff[oplen] = '\0';
      return conv + oplen;  /* return next item */
    }
  }
  ___pdr_luaL_argerror(L, 1,
    ___pdr_lua_pushfstring(L, "invalid conversion specifier '%%%s'", conv));
  return conv;  /* to avoid warnings */
}


/* maximum size for an individual 'strftime' item */
#define ___PDR_SIZETIMEFMT	250


static int os_date (___pdr_lua_State *L) {
  size_t slen;
  const char *s = ___pdr_luaL_optlstring(L, 1, "%c", &slen);
  time_t t = ___pdr_luaL_opt(L, l_checktime, 2, time(NULL));
  const char *se = s + slen;  /* 's' end */
  struct tm tmr, *stm;
  if (*s == '!') {  /* UTC? */
    stm = ___pdr_l_gmtime(&t, &tmr);
    s++;  /* skip '!' */
  }
  else
    stm = ___pdr_l_localtime(&t, &tmr);
  if (stm == NULL)  /* invalid date? */
    ___pdr_luaL_error(L, "time result cannot be represented in this installation");
  if (strcmp(s, "*t") == 0) {
    ___pdr_lua_createtable(L, 0, 9);  /* 9 = number of fields */
    setallfields(L, stm);
  }
  else {
    char cc[4];  /* buffer for individual conversion specifiers */
    ___pdr_luaL_Buffer b;
    cc[0] = '%';
    ___pdr_luaL_buffinit(L, &b);
    while (s < se) {
      if (*s != '%')  /* not a conversion specifier? */
        ___pdr_luaL_addchar(&b, *s++);
      else {
        size_t reslen;
        char *buff = ___pdr_luaL_prepbuffsize(&b, ___PDR_SIZETIMEFMT);
        s++;  /* skip '%' */
        s = checkoption(L, s, se - s, cc + 1);  /* copy specifier to 'cc' */
        reslen = strftime(buff, ___PDR_SIZETIMEFMT, cc, stm);
        ___pdr_luaL_addsize(&b, reslen);
      }
    }
    ___pdr_luaL_pushresult(&b);
  }
  return 1;
}


static int os_time (___pdr_lua_State *L) {
  time_t t;
  if (___pdr_lua_isnoneornil(L, 1))  /* called without args? */
    t = time(NULL);  /* get current time */
  else {
    struct tm ts;
    ___pdr_luaL_checktype(L, 1, ___PDR_LUA_TTABLE);
    ___pdr_lua_settop(L, 1);  /* make sure table is at the top */
    ts.tm_sec = getfield(L, "sec", 0, 0);
    ts.tm_min = getfield(L, "min", 0, 0);
    ts.tm_hour = getfield(L, "hour", 12, 0);
    ts.tm_mday = getfield(L, "day", -1, 0);
    ts.tm_mon = getfield(L, "month", -1, 1);
    ts.tm_year = getfield(L, "year", -1, 1900);
    ts.tm_isdst = getboolfield(L, "isdst");
    t = mktime(&ts);
    setallfields(L, &ts);  /* update fields with normalized values */
  }
  if (t != (time_t)(___pdr_l_timet)t || t == (time_t)(-1))
    ___pdr_luaL_error(L, "time result cannot be represented in this installation");
  ___pdr_l_pushtime(L, t);
  return 1;
}


static int os_difftime (___pdr_lua_State *L) {
  time_t t1 = l_checktime(L, 1);
  time_t t2 = l_checktime(L, 2);
  ___pdr_lua_pushnumber(L, (___pdr_lua_Number)difftime(t1, t2));
  return 1;
}

/* }====================================================== */


static int os_setlocale (___pdr_lua_State *L) {
  static const int cat[] = {LC_ALL, LC_COLLATE, LC_CTYPE, LC_MONETARY,
                      LC_NUMERIC, LC_TIME};
  static const char *const catnames[] = {"all", "collate", "ctype", "monetary",
     "numeric", "time", NULL};
  const char *l = ___pdr_luaL_optstring(L, 1, NULL);
  int op = ___pdr_luaL_checkoption(L, 2, "all", catnames);
  ___pdr_lua_pushstring(L, setlocale(cat[op], l));
  return 1;
}


static int os_exit (___pdr_lua_State *L) {
  int status;
  if (___pdr_lua_isboolean(L, 1))
    status = (___pdr_lua_toboolean(L, 1) ? EXIT_SUCCESS : EXIT_FAILURE);
  else
    status = (int)___pdr_luaL_optinteger(L, 1, EXIT_SUCCESS);
  if (___pdr_lua_toboolean(L, 2))
    ___pdr_lua_close(L);
  if (L) exit(status);  /* 'if' to avoid warnings for unreachable 'return' */
  return 0;
}


static const ___pdr_luaL_Reg syslib[] = {
  {"clock",     os_clock},
  {"date",      os_date},
  {"difftime",  os_difftime},
  {"execute",   os_execute},
  {"exit",      os_exit},
  {"getenv",    os_getenv},
  {"remove",    os_remove},
  {"rename",    os_rename},
  {"setlocale", os_setlocale},
  {"time",      os_time},
  {"tmpname",   os_tmpname},
  {NULL, NULL}
};

/* }====================================================== */



___PDR_LUAMOD_API int ___pdr_luaopen_os (___pdr_lua_State *L) {
  ___pdr_luaL_newlib(L, syslib);
  return 1;
}

} // end NS_PDR_SLUA
