/*
** $Id: lobject.c,v 2.113 2016/12/22 13:08:50 roberto Exp $
** Some generic functions over Lua objects
** See Copyright Notice in lua.h
*/

#define ___pdr_lobject_c
#define ___PDR_LUA_CORE

#include "lobject.h"
#include "lprefix.h"

#include <locale.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lctype.h"
#include "ldebug.h"
#include "ldo.h"
#include "lmem.h"
#include "lstate.h"
#include "lstring.h"
#include "lvm.h"

namespace NS_PDR_SLUA {

___PDR_LUAI_DDEF const ___pdr_TValue ___pdr_luaO_nilobject_ = {___PDR_NILCONSTANT};


/*
** converts an integer to a "floating point byte", represented as
** (eeeeexxx), where the real value is (1xxx) * 2^(eeeee - 1) if
** eeeee != 0 and (xxx) otherwise.
*/
int ___pdr_luaO_int2fb (unsigned int x) {
  int e = 0;  /* exponent */
  if (x < 8) return x;
  while (x >= (8 << 4)) {  /* coarse steps */
    x = (x + 0xf) >> 4;  /* x = ceil(x / 16) */
    e += 4;
  }
  while (x >= (8 << 1)) {  /* fine steps */
    x = (x + 1) >> 1;  /* x = ceil(x / 2) */
    e++;
  }
  return ((e+1) << 3) | (___pdr_cast_int(x) - 8);
}


/* converts back */
int ___pdr_luaO_fb2int (int x) {
  return (x < 8) ? x : ((x & 7) + 8) << ((x >> 3) - 1);
}


/*
** Computes ceil(log2(x))
*/
int ___pdr_luaO_ceillog2 (unsigned int x) {
  static const ___pdr_lu_byte log_2[256] = {  /* log_2[i] = ceil(log2(i - 1)) */
    0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
  };
  int l = 0;
  x--;
  while (x >= 256) { l += 8; x >>= 8; }
  return l + log_2[x];
}


static ___pdr_lua_Integer intarith (___pdr_lua_State *L, int op, ___pdr_lua_Integer v1,
                                                   ___pdr_lua_Integer v2) {
  switch (op) {
    case ___PDR_LUA_OPADD: return ___pdr_intop(+, v1, v2);
    case ___PDR_LUA_OPSUB:return ___pdr_intop(-, v1, v2);
    case ___PDR_LUA_OPMUL:return ___pdr_intop(*, v1, v2);
    case ___PDR_LUA_OPMOD: return ___pdr_luaV_mod(L, v1, v2);
    case ___PDR_LUA_OPIDIV: return ___pdr_luaV_div(L, v1, v2);
    case ___PDR_LUA_OPBAND: return ___pdr_intop(&, v1, v2);
    case ___PDR_LUA_OPBOR: return ___pdr_intop(|, v1, v2);
    case ___PDR_LUA_OPBXOR: return ___pdr_intop(^, v1, v2);
    case ___PDR_LUA_OPSHL: return ___pdr_luaV_shiftl(v1, v2);
    case ___PDR_LUA_OPSHR: return ___pdr_luaV_shiftl(v1, -v2);
    case ___PDR_LUA_OPUNM: return ___pdr_intop(-, 0, v1);
    case ___PDR_LUA_OPBNOT: return ___pdr_intop(^, ~___pdr_l_castS2U(0), v1);
    default: ___pdr_lua_assert(0); return 0;
  }
}


static ___pdr_lua_Number numarith (___pdr_lua_State *L, int op, ___pdr_lua_Number v1,
                                                  ___pdr_lua_Number v2) {
  switch (op) {
    case ___PDR_LUA_OPADD: return ___pdr_luai_numadd(L, v1, v2);
    case ___PDR_LUA_OPSUB: return ___pdr_luai_numsub(L, v1, v2);
    case ___PDR_LUA_OPMUL: return ___pdr_luai_nummul(L, v1, v2);
    case ___PDR_LUA_OPDIV: return ___pdr_luai_numdiv(L, v1, v2);
    case ___PDR_LUA_OPPOW: return ___pdr_luai_numpow(L, v1, v2);
    case ___PDR_LUA_OPIDIV: return ___pdr_luai_numidiv(L, v1, v2);
    case ___PDR_LUA_OPUNM: return ___pdr_luai_numunm(L, v1);
    case ___PDR_LUA_OPMOD: {
      ___pdr_lua_Number m;
      ___pdr_luai_nummod(L, v1, v2, m);
      return m;
    }
    default: ___pdr_lua_assert(0); return 0;
  }
}


void ___pdr_luaO_arith (___pdr_lua_State *L, int op, const ___pdr_TValue *p1, const ___pdr_TValue *p2,
                 ___pdr_TValue *res) {
  switch (op) {
    case ___PDR_LUA_OPBAND: case ___PDR_LUA_OPBOR: case ___PDR_LUA_OPBXOR:
    case ___PDR_LUA_OPSHL: case ___PDR_LUA_OPSHR:
    case ___PDR_LUA_OPBNOT: {  /* operate only on integers */
      ___pdr_lua_Integer i1; ___pdr_lua_Integer i2;
      if (___pdr_tointeger(p1, &i1) && ___pdr_tointeger(p2, &i2)) {
        ___pdr_setivalue(res, intarith(L, op, i1, i2));
        return;
      }
      else break;  /* go to the end */
    }
    case ___PDR_LUA_OPDIV: case ___PDR_LUA_OPPOW: {  /* operate only on floats */
      ___pdr_lua_Number n1; ___pdr_lua_Number n2;
      if (___pdr_tonumber(p1, &n1) && ___pdr_tonumber(p2, &n2)) {
        ___pdr_setfltvalue(res, numarith(L, op, n1, n2));
        return;
      }
      else break;  /* go to the end */
    }
    default: {  /* other operations */
      ___pdr_lua_Number n1; ___pdr_lua_Number n2;
      if (___pdr_ttisinteger(p1) && ___pdr_ttisinteger(p2)) {
        ___pdr_setivalue(res, intarith(L, op, ___pdr_ivalue(p1), ___pdr_ivalue(p2)));
        return;
      }
      else if (___pdr_tonumber(p1, &n1) && ___pdr_tonumber(p2, &n2)) {
        ___pdr_setfltvalue(res, numarith(L, op, n1, n2));
        return;
      }
      else break;  /* go to the end */
    }
  }
  /* could not perform raw operation; try metamethod */
  ___pdr_lua_assert(L != NULL);  /* should not fail when folding (compile time) */
  ___pdr_luaT_trybinTM(L, p1, p2, res, ___pdr_cast(___pdr_TMS, (op - ___PDR_LUA_OPADD) + PDR_TM_ADD));
}


int ___pdr_luaO_hexavalue (int c) {
  if (___pdr_lisdigit(c)) return c - '0';
  else return (___pdr_ltolower(c) - 'a') + 10;
}


static int isneg (const char **s) {
  if (**s == '-') { (*s)++; return 1; }
  else if (**s == '+') (*s)++;
  return 0;
}



/*
** {==================================================================
** Lua's implementation for 'lua_strx2number'
** ===================================================================
*/

#if !defined(___pdr_lua_strx2number)

/* maximum number of significant digits to read (to avoid overflows
   even with single floats) */
#define ___PDR_MAXSIGDIG	30

/*
** convert an hexadecimal numeric string to a number, following
** C99 specification for 'strtod'
*/
static ___pdr_lua_Number ___pdr_lua_strx2number (const char *s, char **endptr) {
  int dot = ___pdr_lua_getlocaledecpoint();
  ___pdr_lua_Number r = 0.0;  /* result (accumulator) */
  int sigdig = 0;  /* number of significant digits */
  int nosigdig = 0;  /* number of non-significant digits */
  int e = 0;  /* exponent correction */
  int neg;  /* 1 if number is negative */
  int hasdot = 0;  /* true after seen a dot */
  *endptr = ___pdr_cast(char *, s);  /* nothing is valid yet */
  while (___pdr_lisspace(___pdr_cast_uchar(*s))) s++;  /* skip initial spaces */
  neg = isneg(&s);  /* check signal */
  if (!(*s == '0' && (*(s + 1) == 'x' || *(s + 1) == 'X')))  /* check '0x' */
    return 0.0;  /* invalid format (no '0x') */
  for (s += 2; ; s++) {  /* skip '0x' and read numeral */
    if (*s == dot) {
      if (hasdot) break;  /* second dot? stop loop */
      else hasdot = 1;
    }
    else if (___pdr_lisxdigit(___pdr_cast_uchar(*s))) {
      if (sigdig == 0 && *s == '0')  /* non-significant digit (zero)? */
        nosigdig++;
      else if (++sigdig <= ___PDR_MAXSIGDIG)  /* can read it without overflow? */
          r = (r * ___pdr_cast_num(16.0)) + ___pdr_luaO_hexavalue(*s);
      else e++; /* too many digits; ignore, but still count for exponent */
      if (hasdot) e--;  /* decimal digit? correct exponent */
    }
    else break;  /* neither a dot nor a digit */
  }
  if (nosigdig + sigdig == 0)  /* no digits? */
    return 0.0;  /* invalid format */
  *endptr = ___pdr_cast(char *, s);  /* valid up to here */
  e *= 4;  /* each digit multiplies/divides value by 2^4 */
  if (*s == 'p' || *s == 'P') {  /* exponent part? */
    int exp1 = 0;  /* exponent value */
    int neg1;  /* exponent signal */
    s++;  /* skip 'p' */
    neg1 = isneg(&s);  /* signal */
    if (!___pdr_lisdigit(___pdr_cast_uchar(*s)))
      return 0.0;  /* invalid; must have at least one digit */
    while (___pdr_lisdigit(___pdr_cast_uchar(*s)))  /* read exponent */
      exp1 = exp1 * 10 + *(s++) - '0';
    if (neg1) exp1 = -exp1;
    e += exp1;
    *endptr = ___pdr_cast(char *, s);  /* valid up to here */
  }
  if (neg) r = -r;
  return ___pdr_l_mathop(ldexp)(r, e);
}

#endif
/* }====================================================== */


/* maximum length of a numeral */
#if !defined (___PDR_L_MAXLENNUM)
#define ___PDR_L_MAXLENNUM	200
#endif

static const char *l_str2dloc (const char *s, ___pdr_lua_Number *result, int mode) {
  char *endptr;
  *result = (mode == 'x') ? ___pdr_lua_strx2number(s, &endptr)  /* try to convert */
                          : ___pdr_lua_str2number(s, &endptr);
  if (endptr == s) return NULL;  /* nothing recognized? */
  while (___pdr_lisspace(___pdr_cast_uchar(*endptr))) endptr++;  /* skip trailing spaces */
  return (*endptr == '\0') ? endptr : NULL;  /* OK if no trailing characters */
}


/*
** Convert string 's' to a Lua number (put in 'result'). Return NULL
** on fail or the address of the ending '\0' on success.
** 'pmode' points to (and 'mode' contains) special things in the string:
** - 'x'/'X' means an hexadecimal numeral
** - 'n'/'N' means 'inf' or 'nan' (which should be rejected)
** - '.' just optimizes the search for the common case (nothing special)
** This function accepts both the current locale or a dot as the radix
** mark. If the convertion fails, it may mean number has a dot but
** locale accepts something else. In that case, the code copies 's'
** to a buffer (because 's' is read-only), changes the dot to the
** current locale radix mark, and tries to convert again.
*/
static const char *l_str2d (const char *s, ___pdr_lua_Number *result) {
  const char *endptr;
  const char *pmode = strpbrk(s, ".xXnN");
  int mode = pmode ? ___pdr_ltolower(___pdr_cast_uchar(*pmode)) : 0;
  if (mode == 'n')  /* reject 'inf' and 'nan' */
    return NULL;
  endptr = l_str2dloc(s, result, mode);  /* try to convert */
  if (endptr == NULL) {  /* failed? may be a different locale */
    char buff[___PDR_L_MAXLENNUM + 1];
    const char *pdot = strchr(s, '.');
    if (strlen(s) > ___PDR_L_MAXLENNUM || pdot == NULL)
      return NULL;  /* string too long or no dot; fail */
    strcpy(buff, s);  /* copy string to buffer */
    buff[pdot - s] = ___pdr_lua_getlocaledecpoint();  /* correct decimal point */
    endptr = l_str2dloc(buff, result, mode);  /* try again */
    if (endptr != NULL)
      endptr = s + (endptr - buff);  /* make relative to 's' */
  }
  return endptr;
}


#define ___PDR_MAXBY10      ___pdr_cast(___pdr_lua_Unsigned, ___PDR_LUA_MAXINTEGER / 10)
#define ___PDR_MAXLASTD     ___pdr_cast_int(___PDR_LUA_MAXINTEGER % 10)

static const char *l_str2int (const char *s, ___pdr_lua_Integer *result) {
  ___pdr_lua_Unsigned a = 0;
  int empty = 1;
  int neg;
  while (___pdr_lisspace(___pdr_cast_uchar(*s))) s++;  /* skip initial spaces */
  neg = isneg(&s);
  if (s[0] == '0' &&
      (s[1] == 'x' || s[1] == 'X')) {  /* hex? */
    s += 2;  /* skip '0x' */
    for (; ___pdr_lisxdigit(___pdr_cast_uchar(*s)); s++) {
      a = a * 16 + ___pdr_luaO_hexavalue(*s);
      empty = 0;
    }
  }
  else {  /* decimal */
    for (; ___pdr_lisdigit(___pdr_cast_uchar(*s)); s++) {
      int d = *s - '0';
      if (a >= ___PDR_MAXBY10 && (a > ___PDR_MAXBY10 || d > ___PDR_MAXLASTD + neg))  /* overflow? */
        return NULL;  /* do not accept it (as integer) */
      a = a * 10 + d;
      empty = 0;
    }
  }
  while (___pdr_lisspace(___pdr_cast_uchar(*s))) s++;  /* skip trailing spaces */
  if (empty || *s != '\0') return NULL;  /* something wrong in the numeral */
  else {
    *result = ___pdr_l_castU2S((neg) ? 0u - a : a);
    return s;
  }
}


size_t ___pdr_luaO_str2num (const char *s, ___pdr_TValue *o) {
  ___pdr_lua_Integer i; ___pdr_lua_Number n;
  const char *e;
  if ((e = l_str2int(s, &i)) != NULL) {  /* try as an integer */
    ___pdr_setivalue(o, i);
  }
  else if ((e = l_str2d(s, &n)) != NULL) {  /* else try as a float */
    ___pdr_setfltvalue(o, n);
  }
  else
    return 0;  /* conversion failed */
  return (e - s) + 1;  /* success; return string size */
}


int ___pdr_luaO_utf8esc (char *buff, unsigned long x) {
  int n = 1;  /* number of bytes put in buffer (backwards) */
  ___pdr_lua_assert(x <= 0x10FFFF);
  if (x < 0x80)  /* ascii? */
    buff[___PDR_UTF8BUFFSZ - 1] = ___pdr_cast(char, x);
  else {  /* need continuation bytes */
    unsigned int mfb = 0x3f;  /* maximum that fits in first byte */
    do {  /* add continuation bytes */
      buff[___PDR_UTF8BUFFSZ - (n++)] = ___pdr_cast(char, 0x80 | (x & 0x3f));
      x >>= 6;  /* remove added bits */
      mfb >>= 1;  /* now there is one less bit available in first byte */
    } while (x > mfb);  /* still needs continuation byte? */
    buff[___PDR_UTF8BUFFSZ - n] = ___pdr_cast(char, (~mfb << 1) | x);  /* add first byte */
  }
  return n;
}


/* maximum length of the conversion of a number to a string */
#define ___PDR_MAXNUMBER2STR	50


/*
** Convert a number object to a string
*/
void ___pdr_luaO_tostring (___pdr_lua_State *L, ___pdr_StkId obj) {
  char buff[___PDR_MAXNUMBER2STR];
  size_t len;
  ___pdr_lua_assert(___pdr_ttisnumber(obj));
  if (___pdr_ttisinteger(obj))
    len = ___pdr_lua_integer2str(buff, sizeof(buff), ___pdr_ivalue(obj));
  else {
    len = ___pdr_lua_number2str(buff, sizeof(buff), ___pdr_fltvalue(obj));
#if !defined(LUA_COMPAT_FLOATSTRING)
    if (buff[strspn(buff, "-0123456789")] == '\0') {  /* looks like an int? */
      buff[len++] = ___pdr_lua_getlocaledecpoint();
      buff[len++] = '0';  /* adds '.0' to result */
    }
#endif
  }
  ___pdr_setsvalue2s(L, obj, ___pdr_luaS_newlstr(L, buff, len));
}


static void pushstr (___pdr_lua_State *L, const char *str, size_t l) {
  ___pdr_setsvalue2s(L, L->top, ___pdr_luaS_newlstr(L, str, l));
  ___pdr_luaD_inctop(L);
}


/*
** this function handles only '%d', '%c', '%f', '%p', and '%s'
   conventional formats, plus Lua-specific '%I' and '%U'
*/
const char *___pdr_luaO_pushvfstring (___pdr_lua_State *L, const char *fmt, va_list argp) {
  int n = 0;
  for (;;) {
    const char *e = strchr(fmt, '%');
    if (e == NULL) break;
    pushstr(L, fmt, e - fmt);
    switch (*(e+1)) {
      case 's': {  /* zero-terminated string */
        const char *s = va_arg(argp, char *);
        if (s == NULL) s = "(null)";
        pushstr(L, s, strlen(s));
        break;
      }
      case 'c': {  /* an 'int' as a character */
        char buff = ___pdr_cast(char, va_arg(argp, int));
        if (___pdr_lisprint(___pdr_cast_uchar(buff)))
          pushstr(L, &buff, 1);
        else  /* non-printable character; print its code */
          ___pdr_luaO_pushfstring(L, "<\\%d>", ___pdr_cast_uchar(buff));
        break;
      }
      case 'd': {  /* an 'int' */
        ___pdr_setivalue(L->top, va_arg(argp, int));
        goto top2str;
      }
      case 'I': {  /* a '___pdr_lua_Integer' */
        ___pdr_setivalue(L->top, ___pdr_cast(___pdr_lua_Integer, va_arg(argp, ___pdr_l_uacInt)));
        goto top2str;
      }
      case 'f': {  /* a 'lua_Number' */
        ___pdr_setfltvalue(L->top, ___pdr_cast_num(va_arg(argp, ___pdr_l_uacNumber)));
      top2str:  /* convert the top element to a string */
        ___pdr_luaD_inctop(L);
        ___pdr_luaO_tostring(L, L->top - 1);
        break;
      }
      case 'p': {  /* a pointer */
        char buff[4*sizeof(void *) + 8]; /* should be enough space for a '%p' */
        int l = ___pdr_l_sprintf(buff, sizeof(buff), "%p", va_arg(argp, void *));
        pushstr(L, buff, l);
        break;
      }
      case 'U': {  /* an 'int' as a UTF-8 sequence */
        char buff[___PDR_UTF8BUFFSZ];
        int l = ___pdr_luaO_utf8esc(buff, ___pdr_cast(long, va_arg(argp, long)));
        pushstr(L, buff + ___PDR_UTF8BUFFSZ - l, l);
        break;
      }
      case '%': {
        pushstr(L, "%", 1);
        break;
      }
      default: {
        ___pdr_luaG_runerror(L, "invalid option '%%%c' to 'lua_pushfstring'",
                         *(e + 1));
      }
    }
    n += 2;
    fmt = e+2;
  }
  ___pdr_luaD_checkstack(L, 1);
  pushstr(L, fmt, strlen(fmt));
  if (n > 0) ___pdr_luaV_concat(L, n + 1);
  return ___pdr_svalue(L->top - 1);
}


const char *___pdr_luaO_pushfstring (___pdr_lua_State *L, const char *fmt, ...) {
  const char *msg;
  va_list argp;
  va_start(argp, fmt);
  msg = ___pdr_luaO_pushvfstring(L, fmt, argp);
  va_end(argp);
  return msg;
}


/* number of chars of a literal string without the ending \0 */
#define ___PDR_LL(x)	(sizeof(x)/sizeof(char) - 1)

#define ___PDR_RETS	"..."
#define ___PDR_PRE	"[string \""
#define ___PDR_POS	"\"]"

#define ___pdr_addstr(a,b,l)	( memcpy(a,b,(l) * sizeof(char)), a += (l) )

void ___pdr_luaO_chunkid (char *out, const char *source, size_t bufflen) {
  size_t l = strlen(source);
  if (*source == '=') {  /* 'literal' source */
    if (l <= bufflen)  /* small enough? */
      memcpy(out, source + 1, l * sizeof(char));
    else {  /* truncate it */
      ___pdr_addstr(out, source + 1, bufflen - 1);
      *out = '\0';
    }
  }
  else if (*source == '@') {  /* file name */
    if (l <= bufflen)  /* small enough? */
      memcpy(out, source + 1, l * sizeof(char));
    else {  /* add '...' before rest of name */
      ___pdr_addstr(out, ___PDR_RETS, ___PDR_LL(___PDR_RETS));
      bufflen -= ___PDR_LL(___PDR_RETS);
      memcpy(out, source + 1 + l - bufflen, bufflen * sizeof(char));
    }
  }
  else {  /* string; format as [string "source"] */
    const char *nl = strchr(source, '\n');  /* find first new line (if any) */
    ___pdr_addstr(out, ___PDR_PRE, ___PDR_LL(___PDR_PRE));  /* add prefix */
    bufflen -= ___PDR_LL(___PDR_PRE ___PDR_RETS ___PDR_POS) + 1;  /* save space for prefix+suffix+'\0' */
    if (l < bufflen && nl == NULL) {  /* small one-line source? */
      ___pdr_addstr(out, source, l);  /* keep it */
    }
    else {
      if (nl != NULL) l = nl - source;  /* stop at first newline */
      if (l > bufflen) l = bufflen;
      ___pdr_addstr(out, source, l);
      ___pdr_addstr(out, ___PDR_RETS, ___PDR_LL(___PDR_RETS));
    }
    memcpy(out, ___PDR_POS, (___PDR_LL(___PDR_POS) + 1) * sizeof(char));
  }
}

} // end NS_PDR_SLUA