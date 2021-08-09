/*
** $Id: lutf8lib.c,v 1.16 2016/12/22 13:08:50 roberto Exp $
** Standard library for UTF-8 manipulation
** See Copyright Notice in lua.h
*/

#define ___pdr_lutf8lib_c
#define ___PDR_LUA_LIB

#include "lprefix.h"


#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#define ___PDR_MAXUNICODE	0x10FFFF

#define ___pdr_iscont(p)	((*(p) & 0xC0) == 0x80)

namespace NS_PDR_SLUA {

/* from strlib */
/* translate a relative string position: negative means back from end */
static ___pdr_lua_Integer u_posrelat (___pdr_lua_Integer pos, size_t len) {
  if (pos >= 0) return pos;
  else if (0u - (size_t)pos > len) return 0;
  else return (___pdr_lua_Integer)len + pos + 1;
}


/*
** Decode one UTF-8 sequence, returning NULL if byte sequence is invalid.
*/
static const char *utf8_decode (const char *o, int *val) {
  static const unsigned int limits[] = {0xFF, 0x7F, 0x7FF, 0xFFFF};
  const unsigned char *s = (const unsigned char *)o;
  unsigned int c = s[0];
  unsigned int res = 0;  /* final result */
  if (c < 0x80)  /* ascii? */
    res = c;
  else {
    int count = 0;  /* to count number of continuation bytes */
    while (c & 0x40) {  /* still have continuation bytes? */
      int cc = s[++count];  /* read next byte */
      if ((cc & 0xC0) != 0x80)  /* not a continuation byte? */
        return NULL;  /* invalid byte sequence */
      res = (res << 6) | (cc & 0x3F);  /* add lower 6 bits from cont. byte */
      c <<= 1;  /* to test next bit */
    }
    res |= ((c & 0x7F) << (count * 5));  /* add first byte */
    if (count > 3 || res > ___PDR_MAXUNICODE || res <= limits[count])
      return NULL;  /* invalid byte sequence */
    s += count;  /* skip continuation bytes read */
  }
  if (val) *val = res;
  return (const char *)s + 1;  /* +1 to include first byte */
}


/*
** utf8len(s [, i [, j]]) --> number of characters that start in the
** range [i,j], or nil + current position if 's' is not well formed in
** that interval
*/
static int utflen (___pdr_lua_State *L) {
  int n = 0;
  size_t len;
  const char *s = ___pdr_luaL_checklstring(L, 1, &len);
  ___pdr_lua_Integer posi = u_posrelat(___pdr_luaL_optinteger(L, 2, 1), len);
  ___pdr_lua_Integer posj = u_posrelat(___pdr_luaL_optinteger(L, 3, -1), len);
  ___pdr_luaL_argcheck(L, 1 <= posi && --posi <= (___pdr_lua_Integer)len, 2,
                   "initial position out of string");
  ___pdr_luaL_argcheck(L, --posj < (___pdr_lua_Integer)len, 3,
                   "final position out of string");
  while (posi <= posj) {
    const char *s1 = utf8_decode(s + posi, NULL);
    if (s1 == NULL) {  /* conversion error? */
      ___pdr_lua_pushnil(L);  /* return nil ... */
      ___pdr_lua_pushinteger(L, posi + 1);  /* ... and current position */
      return 2;
    }
    posi = s1 - s;
    n++;
  }
  ___pdr_lua_pushinteger(L, n);
  return 1;
}


/*
** codepoint(s, [i, [j]])  -> returns codepoints for all characters
** that start in the range [i,j]
*/
static int codepoint (___pdr_lua_State *L) {
  size_t len;
  const char *s = ___pdr_luaL_checklstring(L, 1, &len);
  ___pdr_lua_Integer posi = u_posrelat(___pdr_luaL_optinteger(L, 2, 1), len);
  ___pdr_lua_Integer pose = u_posrelat(___pdr_luaL_optinteger(L, 3, posi), len);
  int n;
  const char *se;
  ___pdr_luaL_argcheck(L, posi >= 1, 2, "out of range");
  ___pdr_luaL_argcheck(L, pose <= (___pdr_lua_Integer)len, 3, "out of range");
  if (posi > pose) return 0;  /* empty interval; return no values */
  if (pose - posi >= INT_MAX)  /* (___pdr_lua_Integer -> int) overflow? */
    return ___pdr_luaL_error(L, "string slice too long");
  n = (int)(pose -  posi) + 1;
  ___pdr_luaL_checkstack(L, n, "string slice too long");
  n = 0;
  se = s + pose;
  for (s += posi - 1; s < se;) {
    int code;
    s = utf8_decode(s, &code);
    if (s == NULL)
      return ___pdr_luaL_error(L, "invalid UTF-8 code");
    ___pdr_lua_pushinteger(L, code);
    n++;
  }
  return n;
}


static void pushutfchar (___pdr_lua_State *L, int arg) {
  ___pdr_lua_Integer code = ___pdr_luaL_checkinteger(L, arg);
  ___pdr_luaL_argcheck(L, 0 <= code && code <= ___PDR_MAXUNICODE, arg, "value out of range");
  ___pdr_lua_pushfstring(L, "%U", (long)code);
}


/*
** utfchar(n1, n2, ...)  -> char(n1)..char(n2)...
*/
static int utfchar (___pdr_lua_State *L) {
  int n = ___pdr_lua_gettop(L);  /* number of arguments */
  if (n == 1)  /* optimize common case of single char */
    pushutfchar(L, 1);
  else {
    int i;
    ___pdr_luaL_Buffer b;
    ___pdr_luaL_buffinit(L, &b);
    for (i = 1; i <= n; i++) {
      pushutfchar(L, i);
      ___pdr_luaL_addvalue(&b);
    }
    ___pdr_luaL_pushresult(&b);
  }
  return 1;
}


/*
** offset(s, n, [i])  -> index where n-th character counting from
**   position 'i' starts; 0 means character at 'i'.
*/
static int byteoffset (___pdr_lua_State *L) {
  size_t len;
  const char *s = ___pdr_luaL_checklstring(L, 1, &len);
  ___pdr_lua_Integer n  = ___pdr_luaL_checkinteger(L, 2);
  ___pdr_lua_Integer posi = (n >= 0) ? 1 : len + 1;
  posi = u_posrelat(___pdr_luaL_optinteger(L, 3, posi), len);
  ___pdr_luaL_argcheck(L, 1 <= posi && --posi <= (___pdr_lua_Integer)len, 3,
                   "position out of range");
  if (n == 0) {
    /* find beginning of current byte sequence */
    while (posi > 0 && ___pdr_iscont(s + posi)) posi--;
  }
  else {
    if (___pdr_iscont(s + posi))
      ___pdr_luaL_error(L, "initial position is a continuation byte");
    if (n < 0) {
       while (n < 0 && posi > 0) {  /* move back */
         do {  /* find beginning of previous character */
           posi--;
         } while (posi > 0 && ___pdr_iscont(s + posi));
         n++;
       }
     }
     else {
       n--;  /* do not move for 1st character */
       while (n > 0 && posi < (___pdr_lua_Integer)len) {
         do {  /* find beginning of next character */
           posi++;
         } while (___pdr_iscont(s + posi));  /* (cannot pass final '\0') */
         n--;
       }
     }
  }
  if (n == 0)  /* did it find given character? */
    ___pdr_lua_pushinteger(L, posi + 1);
  else  /* no such character */
    ___pdr_lua_pushnil(L);
  return 1;
}


static int iter_aux (___pdr_lua_State *L) {
  size_t len;
  const char *s = ___pdr_luaL_checklstring(L, 1, &len);
  ___pdr_lua_Integer n = ___pdr_lua_tointeger(L, 2) - 1;
  if (n < 0)  /* first iteration? */
    n = 0;  /* start from here */
  else if (n < (___pdr_lua_Integer)len) {
    n++;  /* skip current byte */
    while (___pdr_iscont(s + n)) n++;  /* and its continuations */
  }
  if (n >= (___pdr_lua_Integer)len)
    return 0;  /* no more codepoints */
  else {
    int code;
    const char *next = utf8_decode(s + n, &code);
    if (next == NULL || ___pdr_iscont(next))
      return ___pdr_luaL_error(L, "invalid UTF-8 code");
    ___pdr_lua_pushinteger(L, n + 1);
    ___pdr_lua_pushinteger(L, code);
    return 2;
  }
}


static int iter_codes (___pdr_lua_State *L) {
  ___pdr_luaL_checkstring(L, 1);
  ___pdr_lua_pushcfunction(L, iter_aux);
  ___pdr_lua_pushvalue(L, 1);
  ___pdr_lua_pushinteger(L, 0);
  return 3;
}


/* pattern to match a single UTF-8 character */
#define ___PDR_UTF8PATT	"[\0-\x7F\xC2-\xF4][\x80-\xBF]*"


static const ___pdr_luaL_Reg funcs[] = {
  {"offset", byteoffset},
  {"codepoint", codepoint},
  {"char", utfchar},
  {"len", utflen},
  {"codes", iter_codes},
  /* placeholders */
  {"charpattern", NULL},
  {NULL, NULL}
};


___PDR_LUAMOD_API int ___pdr_luaopen_utf8 (___pdr_lua_State *L) {
  ___pdr_luaL_newlib(L, funcs);
  ___pdr_lua_pushlstring(L, ___PDR_UTF8PATT, sizeof(___PDR_UTF8PATT)/sizeof(char) - 1);
  ___pdr_lua_setfield(L, -2, "charpattern");
  return 1;
}

} // end NS_PDR_SLUA