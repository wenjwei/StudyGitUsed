/*
** $Id: liolib.c,v 2.151 2016/12/20 18:37:00 roberto Exp $
** Standard I/O (and system) library
** See Copyright Notice in lua.h
*/

#define ___pdr_liolib_c
#define ___PDR_LUA_LIB

#include "lprefix.h"

#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"


/*
** {======================================================
** l_fseek: configuration for longer offsets
** =======================================================
*/

#if !defined(___pdr_l_fseek)		/* { */

#if defined(___PDR_LUA_USE_POSIX)	/* { */

#include <sys/types.h>

#define ___pdr_l_fseek(f,o,w)		fseeko(f,o,w)
#define ___pdr_l_ftell(f)		ftello(f)
#define ___pdr_l_seeknum		off_t

#elif defined(___PDR_LUA_USE_WINDOWS) && !defined(_CRTIMP_TYPEINFO) \
   && defined(_MSC_VER) && (_MSC_VER >= 1400)	/* }{ */

/* Windows (but not DDK) and Visual C++ 2005 or higher */
#define ___pdr_l_fseek(f,o,w)		_fseeki64(f,o,w)
#define ___pdr_l_ftell(f)		_ftelli64(f)
#define ___pdr_l_seeknum		__int64

#else				/* }{ */

/* ISO C definitions */
#define ___pdr_l_fseek(f,o,w)		fseek(f,o,w)
#define ___pdr_l_ftell(f)		ftell(f)
#define ___pdr_l_seeknum		long

#endif				/* } */

#endif				/* } */

/* }====================================================== */


namespace NS_PDR_SLUA {
/*
** Change this macro to accept other modes for 'fopen' besides
** the standard ones.
*/
#if !defined(__pdr_l_checkmode)

/* accepted extensions to 'mode' in 'fopen' */
#if !defined(___PDR_L_MODEEXT)
#define ___PDR_L_MODEEXT	"b"
#endif

/* Check whether 'mode' matches '[rwa]%+?[L_MODEEXT]*' */
static int __pdr_l_checkmode (const char *mode) {
  return (*mode != '\0' && strchr("rwa", *(mode++)) != NULL &&
         (*mode != '+' || (++mode, 1)) &&  /* skip if char is '+' */
         (strspn(mode, ___PDR_L_MODEEXT) == strlen(mode)));  /* check extensions */
}

#endif

/*
** {======================================================
** l_popen spawns a new process connected to the current
** one through the file streams.
** =======================================================
*/

#if !defined(___pdr_l_popen)		/* { */

#if defined(___PDR_LUA_USE_POSIX)	/* { */

#define ___pdr_l_popen(L,c,m)		(fflush(NULL), popen(c,m))
#define ___pdr_l_pclose(L,file)	(pclose(file))

#elif defined(___PDR_LUA_USE_WINDOWS)	/* }{ */

#define ___pdr_l_popen(L,c,m)		(_popen(c,m))
#define ___pdr_l_pclose(L,file)	(_pclose(file))

#else				/* }{ */

/* ISO C definitions */
#define ___pdr_l_popen(L,c,m)  \
	  ((void)((void)c, m), \
	  ___pdr_luaL_error(L, "'popen' not supported"), \
	  (FILE*)0)
#define ___pdr_l_pclose(L,file)		((void)L, (void)file, -1)

#endif				/* } */

#endif				/* } */

/* }====================================================== */


#if !defined(___pdr_l_getc)		/* { */

#if defined(___PDR_LUA_USE_POSIX)
#define ___pdr_l_getc(f)		getc_unlocked(f)
#define ___pdr_l_lockfile(f)		flockfile(f)
#define ___pdr_l_unlockfile(f)		funlockfile(f)
#else
#define ___pdr_l_getc(f)		getc(f)
#define ___pdr_l_lockfile(f)		((void)0)
#define ___pdr_l_unlockfile(f)		((void)0)
#endif

#endif				/* } */

#define ___PDR_IO_PREFIX	"_IO_"
#define ___PDR_IOPREF_LEN	(sizeof(___PDR_IO_PREFIX)/sizeof(char) - 1)
#define ___PDR_IO_INPUT	    (___PDR_IO_PREFIX "input")
#define ___PDR_IO_OUTPUT	(___PDR_IO_PREFIX "output")


typedef ___pdr_luaL_Stream LStream;


#define ___pdr_tolstream(L)	((LStream *)___pdr_luaL_checkudata(L, 1, ___PDR_LUA_FILEHANDLE))

#define ___pdr_isclosed(p)	((p)->closef == NULL)


static int io_type (___pdr_lua_State *L) {
  LStream *p;
  ___pdr_luaL_checkany(L, 1);
  p = (LStream *)___pdr_luaL_testudata(L, 1, ___PDR_LUA_FILEHANDLE);
  if (p == NULL)
    ___pdr_lua_pushnil(L);  /* not a file */
  else if (___pdr_isclosed(p))
    ___pdr_lua_pushliteral(L, "closed file");
  else
    ___pdr_lua_pushliteral(L, "file");
  return 1;
}


static int f_tostring (___pdr_lua_State *L) {
  LStream *p = ___pdr_tolstream(L);
  if (___pdr_isclosed(p))
    ___pdr_lua_pushliteral(L, "file (closed)");
  else
    ___pdr_lua_pushfstring(L, "file (%p)", p->f);
  return 1;
}


static FILE *tofile (___pdr_lua_State *L) {
  LStream *p = ___pdr_tolstream(L);
  if (___pdr_isclosed(p))
    ___pdr_luaL_error(L, "attempt to use a closed file");
  ___pdr_lua_assert(p->f);
  return p->f;
}


/*
** When creating file handles, always creates a 'closed' file handle
** before opening the actual file; so, if there is a memory error, the
** handle is in a consistent state.
*/
static LStream *newprefile (___pdr_lua_State *L) {
  LStream *p = (LStream *)___pdr_lua_newuserdata(L, sizeof(LStream));
  p->closef = NULL;  /* mark file handle as 'closed' */
  ___pdr_luaL_setmetatable(L, ___PDR_LUA_FILEHANDLE);
  return p;
}


/*
** Calls the 'close' function from a file handle. The 'volatile' avoids
** a bug in some versions of the Clang compiler (e.g., clang 3.0 for
** 32 bits).
*/
static int aux_close (___pdr_lua_State *L) {
  LStream *p = ___pdr_tolstream(L);
  volatile ___pdr_lua_CFunction cf = p->closef;
  p->closef = NULL;  /* mark stream as closed */
  return (*cf)(L);  /* close it */
}


static int io_close (___pdr_lua_State *L) {
  if (___pdr_lua_isnone(L, 1))  /* no argument? */
    ___pdr_lua_getfield(L, ___PDR_LUA_REGISTRYINDEX, ___PDR_IO_OUTPUT);  /* use standard output */
  tofile(L);  /* make sure argument is an open stream */
  return aux_close(L);
}


static int f_gc (___pdr_lua_State *L) {
  LStream *p = ___pdr_tolstream(L);
  if (!___pdr_isclosed(p) && p->f != NULL)
    aux_close(L);  /* ignore closed and incompletely open files */
  return 0;
}


/*
** function to close regular files
*/
static int io_fclose (___pdr_lua_State *L) {
  LStream *p = ___pdr_tolstream(L);
  int res = fclose(p->f);
  return ___pdr_luaL_fileresult(L, (res == 0), NULL);
}


static LStream *newfile (___pdr_lua_State *L) {
  LStream *p = newprefile(L);
  p->f = NULL;
  p->closef = &io_fclose;
  return p;
}


static void opencheck (___pdr_lua_State *L, const char *fname, const char *mode) {
  LStream *p = newfile(L);
  p->f = fopen(fname, mode);
  if (p->f == NULL)
    ___pdr_luaL_error(L, "cannot open file '%s' (%s)", fname, strerror(errno));
}


static int io_open (___pdr_lua_State *L) {
  const char *filename = ___pdr_luaL_checkstring(L, 1);
  const char *mode = ___pdr_luaL_optstring(L, 2, "r");
  LStream *p = newfile(L);
  const char *md = mode;  /* to traverse/check mode */
  ___pdr_luaL_argcheck(L, __pdr_l_checkmode(md), 2, "invalid mode");
  p->f = fopen(filename, mode);
  return (p->f == NULL) ? ___pdr_luaL_fileresult(L, 0, filename) : 1;
}


/*
** function to close 'popen' files
*/
static int io_pclose (___pdr_lua_State *L) {
  LStream *p = ___pdr_tolstream(L);
  return ___pdr_luaL_execresult(L, ___pdr_l_pclose(L, p->f));
}


static int io_popen (___pdr_lua_State *L) {
  const char *filename = ___pdr_luaL_checkstring(L, 1);
  const char *mode = ___pdr_luaL_optstring(L, 2, "r");
  LStream *p = newprefile(L);
  p->f = ___pdr_l_popen(L, filename, mode);
  p->closef = &io_pclose;
  return (p->f == NULL) ? ___pdr_luaL_fileresult(L, 0, filename) : 1;
}


static int io_tmpfile (___pdr_lua_State *L) {
  LStream *p = newfile(L);
  p->f = tmpfile();
  return (p->f == NULL) ? ___pdr_luaL_fileresult(L, 0, NULL) : 1;
}


static FILE *getiofile (___pdr_lua_State *L, const char *findex) {
  LStream *p;
  ___pdr_lua_getfield(L, ___PDR_LUA_REGISTRYINDEX, findex);
  p = (LStream *)___pdr_lua_touserdata(L, -1);
  if (___pdr_isclosed(p))
    ___pdr_luaL_error(L, "standard %s file is closed", findex + ___PDR_IOPREF_LEN);
  return p->f;
}


static int g_iofile (___pdr_lua_State *L, const char *f, const char *mode) {
  if (!___pdr_lua_isnoneornil(L, 1)) {
    const char *filename = ___pdr_lua_tostring(L, 1);
    if (filename)
      opencheck(L, filename, mode);
    else {
      tofile(L);  /* check that it's a valid file handle */
      ___pdr_lua_pushvalue(L, 1);
    }
    ___pdr_lua_setfield(L, ___PDR_LUA_REGISTRYINDEX, f);
  }
  /* return current value */
  ___pdr_lua_getfield(L, ___PDR_LUA_REGISTRYINDEX, f);
  return 1;
}


static int io_input (___pdr_lua_State *L) {
  return g_iofile(L, ___PDR_IO_INPUT, "r");
}


static int io_output (___pdr_lua_State *L) {
  return g_iofile(L, ___PDR_IO_OUTPUT, "w");
}


static int io_readline (___pdr_lua_State *L);


/*
** maximum number of arguments to 'f:lines'/'io.lines' (it + 3 must fit
** in the limit for upvalues of a closure)
*/
#define ___PDR_MAXARGLINE	250

static void aux_lines (___pdr_lua_State *L, int toclose) {
  int n = ___pdr_lua_gettop(L) - 1;  /* number of arguments to read */
  ___pdr_luaL_argcheck(L, n <= ___PDR_MAXARGLINE, ___PDR_MAXARGLINE + 2, "too many arguments");
  ___pdr_lua_pushinteger(L, n);  /* number of arguments to read */
  ___pdr_lua_pushboolean(L, toclose);  /* close/not close file when finished */
  ___pdr_lua_rotate(L, 2, 2);  /* move 'n' and 'toclose' to their positions */
  ___pdr_lua_pushcclosure(L, io_readline, 3 + n);
}


static int f_lines (___pdr_lua_State *L) {
  tofile(L);  /* check that it's a valid file handle */
  aux_lines(L, 0);
  return 1;
}


static int io_lines (___pdr_lua_State *L) {
  int toclose;
  if (___pdr_lua_isnone(L, 1)) ___pdr_lua_pushnil(L);  /* at least one argument */
  if (___pdr_lua_isnil(L, 1)) {  /* no file name? */
    ___pdr_lua_getfield(L, ___PDR_LUA_REGISTRYINDEX, ___PDR_IO_INPUT);  /* get default input */
    ___pdr_lua_replace(L, 1);  /* put it at index 1 */
    tofile(L);  /* check that it's a valid file handle */
    toclose = 0;  /* do not close it after iteration */
  }
  else {  /* open a new file */
    const char *filename = ___pdr_luaL_checkstring(L, 1);
    opencheck(L, filename, "r");
    ___pdr_lua_replace(L, 1);  /* put file at index 1 */
    toclose = 1;  /* close it after iteration */
  }
  aux_lines(L, toclose);
  return 1;
}


/*
** {======================================================
** READ
** =======================================================
*/


/* maximum length of a numeral */
#if !defined (___PDR_L_MAXLENNUM)
#define ___PDR_L_MAXLENNUM     200
#endif


/* auxiliary structure used by 'read_number' */
typedef struct {
  FILE *f;  /* file being read */
  int c;  /* current character (look ahead) */
  int n;  /* number of elements in buffer 'buff' */
  char buff[___PDR_L_MAXLENNUM + 1];  /* +1 for ending '\0' */
} RN;


/*
** Add current char to buffer (if not out of space) and read next one
*/
static int nextc (RN *rn) {
  if (rn->n >= ___PDR_L_MAXLENNUM) {  /* buffer overflow? */
    rn->buff[0] = '\0';  /* invalidate result */
    return 0;  /* fail */
  }
  else {
    rn->buff[rn->n++] = rn->c;  /* save current char */
    rn->c = ___pdr_l_getc(rn->f);  /* read next one */
    return 1;
  }
}


/*
** Accept current char if it is in 'set' (of size 2)
*/
static int test2 (RN *rn, const char *set) {
  if (rn->c == set[0] || rn->c == set[1])
    return nextc(rn);
  else return 0;
}


/*
** Read a sequence of (hex)digits
*/
static int readdigits (RN *rn, int hex) {
  int count = 0;
  while ((hex ? isxdigit(rn->c) : isdigit(rn->c)) && nextc(rn))
    count++;
  return count;
}


/*
** Read a number: first reads a valid prefix of a numeral into a buffer.
** Then it calls 'lua_stringtonumber' to check whether the format is
** correct and to convert it to a Lua number
*/
static int read_number (___pdr_lua_State *L, FILE *f) {
  RN rn;
  int count = 0;
  int hex = 0;
  char decp[2];
  rn.f = f; rn.n = 0;
  decp[0] = ___pdr_lua_getlocaledecpoint();  /* get decimal point from locale */
  decp[1] = '.';  /* always accept a dot */
  ___pdr_l_lockfile(rn.f);
  do { rn.c = ___pdr_l_getc(rn.f); } while (isspace(rn.c));  /* skip spaces */
  test2(&rn, "-+");  /* optional signal */
  if (test2(&rn, "00")) {
    if (test2(&rn, "xX")) hex = 1;  /* numeral is hexadecimal */
    else count = 1;  /* count initial '0' as a valid digit */
  }
  count += readdigits(&rn, hex);  /* integral part */
  if (test2(&rn, decp))  /* decimal point? */
    count += readdigits(&rn, hex);  /* fractional part */
  if (count > 0 && test2(&rn, (hex ? "pP" : "eE"))) {  /* exponent mark? */
    test2(&rn, "-+");  /* exponent signal */
    readdigits(&rn, 0);  /* exponent digits */
  }
  ungetc(rn.c, rn.f);  /* unread look-ahead char */
  ___pdr_l_unlockfile(rn.f);
  rn.buff[rn.n] = '\0';  /* finish string */
  if (___pdr_lua_stringtonumber(L, rn.buff))  /* is this a valid number? */
    return 1;  /* ok */
  else {  /* invalid format */
   ___pdr_lua_pushnil(L);  /* "result" to be removed */
   return 0;  /* read fails */
  }
}


static int test_eof (___pdr_lua_State *L, FILE *f) {
  int c = getc(f);
  ungetc(c, f);  /* no-op when c == EOF */
  ___pdr_lua_pushliteral(L, "");
  return (c != EOF);
}


static int read_line (___pdr_lua_State *L, FILE *f, int chop) {
  ___pdr_luaL_Buffer b;
  int c = '\0';
  ___pdr_luaL_buffinit(L, &b);
  while (c != EOF && c != '\n') {  /* repeat until end of line */
    char *buff = ___pdr_luaL_prepbuffer(&b);  /* preallocate buffer */
    int i = 0;
    ___pdr_l_lockfile(f);  /* no memory errors can happen inside the lock */
    while (i < ___PDR_LUAL_BUFFERSIZE && (c = ___pdr_l_getc(f)) != EOF && c != '\n')
      buff[i++] = c;
    ___pdr_l_unlockfile(f);
    ___pdr_luaL_addsize(&b, i);
  }
  if (!chop && c == '\n')  /* want a newline and have one? */
    ___pdr_luaL_addchar(&b, c);  /* add ending newline to result */
  ___pdr_luaL_pushresult(&b);  /* close buffer */
  /* return ok if read something (either a newline or something else) */
  return (c == '\n' || ___pdr_lua_rawlen(L, -1) > 0);
}


static void read_all (___pdr_lua_State *L, FILE *f) {
  size_t nr;
  ___pdr_luaL_Buffer b;
  ___pdr_luaL_buffinit(L, &b);
  do {  /* read file in chunks of LUAL_BUFFERSIZE bytes */
    char *p = ___pdr_luaL_prepbuffer(&b);
    nr = fread(p, sizeof(char), ___PDR_LUAL_BUFFERSIZE, f);
    ___pdr_luaL_addsize(&b, nr);
  } while (nr == ___PDR_LUAL_BUFFERSIZE);
  ___pdr_luaL_pushresult(&b);  /* close buffer */
}


static int read_chars (___pdr_lua_State *L, FILE *f, size_t n) {
  size_t nr;  /* number of chars actually read */
  char *p;
  ___pdr_luaL_Buffer b;
  ___pdr_luaL_buffinit(L, &b);
  p = ___pdr_luaL_prepbuffsize(&b, n);  /* prepare buffer to read whole block */
  nr = fread(p, sizeof(char), n, f);  /* try to read 'n' chars */
  ___pdr_luaL_addsize(&b, nr);
  ___pdr_luaL_pushresult(&b);  /* close buffer */
  return (nr > 0);  /* true iff read something */
}


static int g_read (___pdr_lua_State *L, FILE *f, int first) {
  int nargs = ___pdr_lua_gettop(L) - 1;
  int success;
  int n;
  clearerr(f);
  if (nargs == 0) {  /* no arguments? */
    success = read_line(L, f, 1);
    n = first+1;  /* to return 1 result */
  }
  else {  /* ensure stack space for all results and for auxlib's buffer */
    ___pdr_luaL_checkstack(L, nargs+___PDR_LUA_MINSTACK, "too many arguments");
    success = 1;
    for (n = first; nargs-- && success; n++) {
      if (___pdr_lua_type(L, n) == ___PDR_LUA_TNUMBER) {
        size_t l = (size_t)___pdr_luaL_checkinteger(L, n);
        success = (l == 0) ? test_eof(L, f) : read_chars(L, f, l);
      }
      else {
        const char *p = ___pdr_luaL_checkstring(L, n);
        if (*p == '*') p++;  /* skip optional '*' (for compatibility) */
        switch (*p) {
          case 'n':  /* number */
            success = read_number(L, f);
            break;
          case 'l':  /* line */
            success = read_line(L, f, 1);
            break;
          case 'L':  /* line with end-of-line */
            success = read_line(L, f, 0);
            break;
          case 'a':  /* file */
            read_all(L, f);  /* read entire file */
            success = 1; /* always success */
            break;
          default:
            return ___pdr_luaL_argerror(L, n, "invalid format");
        }
      }
    }
  }
  if (ferror(f))
    return ___pdr_luaL_fileresult(L, 0, NULL);
  if (!success) {
    ___pdr_lua_pop(L, 1);  /* remove last result */
    ___pdr_lua_pushnil(L);  /* push nil instead */
  }
  return n - first;
}


static int io_read (___pdr_lua_State *L) {
  return g_read(L, getiofile(L, ___PDR_IO_INPUT), 1);
}


static int f_read (___pdr_lua_State *L) {
  return g_read(L, tofile(L), 2);
}


static int io_readline (___pdr_lua_State *L) {
  LStream *p = (LStream *)___pdr_lua_touserdata(L, ___pdr_lua_upvalueindex(1));
  int i;
  int n = (int)___pdr_lua_tointeger(L, ___pdr_lua_upvalueindex(2));
  if (___pdr_isclosed(p))  /* file is already closed? */
    return ___pdr_luaL_error(L, "file is already closed");
  ___pdr_lua_settop(L , 1);
  ___pdr_luaL_checkstack(L, n, "too many arguments");
  for (i = 1; i <= n; i++)  /* push arguments to 'g_read' */
    ___pdr_lua_pushvalue(L, ___pdr_lua_upvalueindex(3 + i));
  n = g_read(L, p->f, 2);  /* 'n' is number of results */
  ___pdr_lua_assert(n > 0);  /* should return at least a nil */
  if (___pdr_lua_toboolean(L, -n))  /* read at least one value? */
    return n;  /* return them */
  else {  /* first result is nil: EOF or error */
    if (n > 1) {  /* is there error information? */
      /* 2nd result is error message */
      return ___pdr_luaL_error(L, "%s", ___pdr_lua_tostring(L, -n + 1));
    }
    if (___pdr_lua_toboolean(L, ___pdr_lua_upvalueindex(3))) {  /* generator created file? */
      ___pdr_lua_settop(L, 0);
      ___pdr_lua_pushvalue(L, ___pdr_lua_upvalueindex(1));
      aux_close(L);  /* close it */
    }
    return 0;
  }
}

/* }====================================================== */


static int g_write (___pdr_lua_State *L, FILE *f, int arg) {
  int nargs = ___pdr_lua_gettop(L) - arg;
  int status = 1;
  for (; nargs--; arg++) {
    if (___pdr_lua_type(L, arg) == ___PDR_LUA_TNUMBER) {
      /* optimization: could be done exactly as for strings */
      int len = ___pdr_lua_isinteger(L, arg)
                ? fprintf(f, ___PDR_LUA_INTEGER_FMT,
                             (___PDR_LUAI_UACINT)___pdr_lua_tointeger(L, arg))
                : fprintf(f, ___PDR_LUA_NUMBER_FMT,
                             (___PDR_LUAI_UACNUMBER)___pdr_lua_tonumber(L, arg));
      status = status && (len > 0);
    }
    else {
      size_t l;
      const char *s = ___pdr_luaL_checklstring(L, arg, &l);
      status = status && (fwrite(s, sizeof(char), l, f) == l);
    }
  }
  if (status) return 1;  /* file handle already on stack top */
  else return ___pdr_luaL_fileresult(L, status, NULL);
}


static int io_write (___pdr_lua_State *L) {
  return g_write(L, getiofile(L, ___PDR_IO_OUTPUT), 1);
}


static int f_write (___pdr_lua_State *L) {
  FILE *f = tofile(L);
  ___pdr_lua_pushvalue(L, 1);  /* push file at the stack top (to be returned) */
  return g_write(L, f, 2);
}


static int f_seek (___pdr_lua_State *L) {
  static const int mode[] = {SEEK_SET, SEEK_CUR, SEEK_END};
  static const char *const modenames[] = {"set", "cur", "end", NULL};
  FILE *f = tofile(L);
  int op = ___pdr_luaL_checkoption(L, 2, "cur", modenames);
  ___pdr_lua_Integer p3 = ___pdr_luaL_optinteger(L, 3, 0);
  ___pdr_l_seeknum offset = (___pdr_l_seeknum)p3;
  ___pdr_luaL_argcheck(L, (___pdr_lua_Integer)offset == p3, 3,
                  "not an integer in proper range");
  op = ___pdr_l_fseek(f, offset, mode[op]);
  if (op)
    return ___pdr_luaL_fileresult(L, 0, NULL);  /* error */
  else {
    ___pdr_lua_pushinteger(L, (___pdr_lua_Integer)___pdr_l_ftell(f));
    return 1;
  }
}


static int f_setvbuf (___pdr_lua_State *L) {
  static const int mode[] = {_IONBF, _IOFBF, _IOLBF};
  static const char *const modenames[] = {"no", "full", "line", NULL};
  FILE *f = tofile(L);
  int op = ___pdr_luaL_checkoption(L, 2, NULL, modenames);
  ___pdr_lua_Integer sz = ___pdr_luaL_optinteger(L, 3, ___PDR_LUAL_BUFFERSIZE);
  int res = setvbuf(f, NULL, mode[op], (size_t)sz);
  return ___pdr_luaL_fileresult(L, res == 0, NULL);
}



static int io_flush (___pdr_lua_State *L) {
  return ___pdr_luaL_fileresult(L, fflush(getiofile(L, ___PDR_IO_OUTPUT)) == 0, NULL);
}


static int f_flush (___pdr_lua_State *L) {
  return ___pdr_luaL_fileresult(L, fflush(tofile(L)) == 0, NULL);
}


/*
** functions for 'io' library
*/
static const ___pdr_luaL_Reg iolib[] = {
  {"close", io_close},
  {"flush", io_flush},
  {"input", io_input},
  {"lines", io_lines},
  {"open", io_open},
  {"output", io_output},
  {"popen", io_popen},
  {"read", io_read},
  {"tmpfile", io_tmpfile},
  {"type", io_type},
  {"write", io_write},
  {NULL, NULL}
};


/*
** methods for file handles
*/
static const ___pdr_luaL_Reg flib[] = {
  {"close", io_close},
  {"flush", f_flush},
  {"lines", f_lines},
  {"read", f_read},
  {"seek", f_seek},
  {"setvbuf", f_setvbuf},
  {"write", f_write},
  {"__gc", f_gc},
  {"__tostring", f_tostring},
  {NULL, NULL}
};


static void createmeta (___pdr_lua_State *L) {
  ___pdr_luaL_newmetatable(L, ___PDR_LUA_FILEHANDLE);  /* create metatable for file handles */
  ___pdr_lua_pushvalue(L, -1);  /* push metatable */
  ___pdr_lua_setfield(L, -2, "__index");  /* metatable.__index = metatable */
  ___pdr_luaL_setfuncs(L, flib, 0);  /* add file methods to new metatable */
  ___pdr_lua_pop(L, 1);  /* pop new metatable */
}


/*
** function to (not) close the standard files stdin, stdout, and stderr
*/
static int io_noclose (___pdr_lua_State *L) {
  LStream *p = ___pdr_tolstream(L);
  p->closef = &io_noclose;  /* keep file opened */
  ___pdr_lua_pushnil(L);
  ___pdr_lua_pushliteral(L, "cannot close standard file");
  return 2;
}


static void createstdfile (___pdr_lua_State *L, FILE *f, const char *k,
                           const char *fname) {
  LStream *p = newprefile(L);
  p->f = f;
  p->closef = &io_noclose;
  if (k != NULL) {
    ___pdr_lua_pushvalue(L, -1);
    ___pdr_lua_setfield(L, ___PDR_LUA_REGISTRYINDEX, k);  /* add file to registry */
  }
  ___pdr_lua_setfield(L, -2, fname);  /* add file to module */
}


___PDR_LUAMOD_API int ___pdr_luaopen_io (___pdr_lua_State *L) {
  ___pdr_luaL_newlib(L, iolib);  /* new module */
  createmeta(L);
  /* create (and set) default files */
  createstdfile(L, stdin, ___PDR_IO_INPUT, "stdin");
  createstdfile(L, stdout, ___PDR_IO_OUTPUT, "stdout");
  createstdfile(L, stderr, NULL, "stderr");
  return 1;
}

} // end NS_PDR_SLUA