/*
** $Id: llex.c,v 2.96 2016/05/02 14:02:12 roberto Exp $
** Lexical Analyzer
** See Copyright Notice in lua.h
*/

#define ___pdr_llex_c
#define ___PDR_LUA_CORE

#include "llex.h"
#include "lprefix.h"

#include <locale.h>
#include <string.h>

#include "lua.h"
#include "lctype.h"
#include "ldebug.h"
#include "ldo.h"
#include "lgc.h"
#include "lobject.h"
#include "lparser.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "lzio.h"

namespace NS_PDR_SLUA {

#define ___pdr_next_tk(ls) (ls->current = ___pdr_zgetc(ls->z))


#define ___pdr_currIsNewline(ls)	(ls->current == '\n' || ls->current == '\r')


/* ORDER RESERVED */
static const char *const luaX_tokens [] = {
    "and", "break", "do", "else", "elseif",
    "end", "false", "for", "function", "goto", "if",
    "in", "local", "nil", "not", "or", "repeat",
    "return", "then", "true", "until", "while",
    "//", "..", "...", "==", ">=", "<=", "~=",
    "<<", ">>", "::", "<eof>",
    "<number>", "<integer>", "<name>", "<string>"
};


#define ___pdr_save_and_next(ls) (save(ls, ls->current), ___pdr_next_tk(ls))


static ___pdr_l_noret lexerror (___pdr_LexState *ls, const char *msg, int token);


static void save (___pdr_LexState *ls, int c) {
  ___pdr_Mbuffer *b = ls->buff;
  if (___pdr_luaZ_bufflen(b) + 1 > ___pdr_luaZ_sizebuffer(b)) {
    size_t newsize;
    if (___pdr_luaZ_sizebuffer(b) >= ___PDR_MAX_SIZE/2)
      lexerror(ls, "lexical element too long", 0);
    newsize = ___pdr_luaZ_sizebuffer(b) * 2;
    ___pdr_luaZ_resizebuffer(ls->L, b, newsize);
  }
  b->buffer[___pdr_luaZ_bufflen(b)++] = ___pdr_cast(char, c);
}


void ___pdr_luaX_init (___pdr_lua_State *L) {
  int i;
  ___pdr_TString *e = ___pdr_luaS_newliteral(L, ___PDR_LUA_ENV);  /* create env name */
  ___pdr_luaC_fix(L, ___pdr_obj2gco(e));  /* never collect this name */
  for (i=0; i<___PDR_NUM_RESERVED; i++) {
    ___pdr_TString *ts = ___pdr_luaS_new(L, luaX_tokens[i]);
    ___pdr_luaC_fix(L, ___pdr_obj2gco(ts));  /* reserved words are never collected */
    ts->extra = ___pdr_cast_byte(i+1);  /* reserved word */
  }
}


const char *___pdr_luaX_token2str (___pdr_LexState *ls, int token) {
  if (token < ___PDR_FIRST_RESERVED) {  /* single-byte symbols? */
    ___pdr_lua_assert(token == ___pdr_cast_uchar(token));
    return ___pdr_luaO_pushfstring(ls->L, "'%c'", token);
  }
  else {
    const char *s = luaX_tokens[token - ___PDR_FIRST_RESERVED];
    if (token < PDR_TK_EOS)  /* fixed format (symbols and reserved words)? */
      return ___pdr_luaO_pushfstring(ls->L, "'%s'", s);
    else  /* names, strings, and numerals */
      return s;
  }
}


static const char *txtToken (___pdr_LexState *ls, int token) {
  switch (token) {
    case PDR_TK_NAME: case PDR_TK_STRING:
    case PDR_TK_FLT: case PDR_TK_INT:
      save(ls, '\0');
      return ___pdr_luaO_pushfstring(ls->L, "'%s'", ___pdr_luaZ_buffer(ls->buff));
    default:
      return ___pdr_luaX_token2str(ls, token);
  }
}


static ___pdr_l_noret lexerror (___pdr_LexState *ls, const char *msg, int token) {
  msg = ___pdr_luaG_addinfo(ls->L, msg, ls->source, ls->linenumber);
  if (token)
    ___pdr_luaO_pushfstring(ls->L, "%s near %s", msg, txtToken(ls, token));
  ___pdr_luaD_throw(ls->L, ___PDR_LUA_ERRSYNTAX);
}


___pdr_l_noret ___pdr_luaX_syntaxerror (___pdr_LexState *ls, const char *msg) {
  lexerror(ls, msg, ls->t.token);
}


/*
** creates a new string and anchors it in scanner's table so that
** it will not be collected until the end of the compilation
** (by that time it should be anchored somewhere)
*/
___pdr_TString *___pdr_luaX_newstring (___pdr_LexState *ls, const char *str, size_t l) {
  ___pdr_lua_State *L = ls->L;
  ___pdr_TValue *o;  /* entry for 'str' */
  ___pdr_TString *ts = ___pdr_luaS_newlstr(L, str, l);  /* create new string */
  ___pdr_setsvalue2s(L, L->top++, ts);  /* temporarily anchor it in stack */
  o = ___pdr_luaH_set(L, ls->h, L->top - 1);
  if (___pdr_ttisnil(o)) {  /* not in use yet? */
    /* boolean value does not need GC barrier;
       table has no metatable, so it does not need to invalidate cache */
    ___pdr_setbvalue(o, 1);  /* t[string] = true */
    ___pdr_luaC_checkGC(L);
  }
  else {  /* string already present */
    ts = ___pdr_tsvalue(___pdr_keyfromval(o));  /* re-use value previously stored */
  }
  L->top--;  /* remove string from stack */
  return ts;
}


/*
** increment line number and skips newline sequence (any of
** \n, \r, \n\r, or \r\n)
*/
static void inclinenumber (___pdr_LexState *ls) {
  int old = ls->current;
  ___pdr_lua_assert(___pdr_currIsNewline(ls));
  ___pdr_next_tk(ls);  /* skip '\n' or '\r' */
  if (___pdr_currIsNewline(ls) && ls->current != old)
    ___pdr_next_tk(ls);  /* skip '\n\r' or '\r\n' */
  if (++ls->linenumber >= ___PDR_MAX_INT)
    lexerror(ls, "chunk has too many lines", 0);
}


void ___pdr_luaX_setinput (___pdr_lua_State *L, ___pdr_LexState *ls, ___pdr_ZIO *z, ___pdr_TString *source,
                    int firstchar) {
  ls->t.token = 0;
  ls->L = L;
  ls->current = firstchar;
  ls->lookahead.token = PDR_TK_EOS;  /* no look-ahead token */
  ls->z = z;
  ls->fs = NULL;
  ls->linenumber = 1;
  ls->lastline = 1;
  ls->source = source;
  ls->envn = ___pdr_luaS_newliteral(L, ___PDR_LUA_ENV);  /* get env name */
  ___pdr_luaZ_resizebuffer(ls->L, ls->buff, ___PDR_LUA_MINBUFFER);  /* initialize buffer */
}



/*
** =======================================================
** LEXICAL ANALYZER
** =======================================================
*/


static int check_next1 (___pdr_LexState *ls, int c) {
  if (ls->current == c) {
    ___pdr_next_tk(ls);
    return 1;
  }
  else return 0;
}


/*
** Check whether current char is in set 'set' (with two chars) and
** saves it
*/
static int check_next2 (___pdr_LexState *ls, const char *set) {
  ___pdr_lua_assert(set[2] == '\0');
  if (ls->current == set[0] || ls->current == set[1]) {
    ___pdr_save_and_next(ls);
    return 1;
  }
  else return 0;
}


/* LUA_NUMBER */
/*
** this function is quite liberal in what it accepts, as 'luaO_str2num'
** will reject ill-formed numerals.
*/
static int read_numeral (___pdr_LexState *ls, ___pdr_SemInfo *seminfo) {
  ___pdr_TValue obj;
  const char *expo = "Ee";
  int first = ls->current;
  ___pdr_lua_assert(___pdr_lisdigit(ls->current));
  ___pdr_save_and_next(ls);
  if (first == '0' && check_next2(ls, "xX"))  /* hexadecimal? */
    expo = "Pp";
  for (;;) {
    if (check_next2(ls, expo))  /* exponent part? */
      check_next2(ls, "-+");  /* optional exponent sign */
    if (___pdr_lisxdigit(ls->current))
      ___pdr_save_and_next(ls);
    else if (ls->current == '.')
      ___pdr_save_and_next(ls);
    else break;
  }
  save(ls, '\0');
  if (___pdr_luaO_str2num(___pdr_luaZ_buffer(ls->buff), &obj) == 0)  /* format error? */
    lexerror(ls, "malformed number", PDR_TK_FLT);
  if (___pdr_ttisinteger(&obj)) {
    seminfo->i = ___pdr_ivalue(&obj);
    return PDR_TK_INT;
  }
  else {
    ___pdr_lua_assert(___pdr_ttisfloat(&obj));
    seminfo->r = ___pdr_fltvalue(&obj);
    return PDR_TK_FLT;
  }
}


/*
** skip a sequence '[=*[' or ']=*]'; if sequence is well formed, return
** its number of '='s; otherwise, return a negative number (-1 iff there
** are no '='s after initial bracket)
*/
static int skip_sep (___pdr_LexState *ls) {
  int count = 0;
  int s = ls->current;
  ___pdr_lua_assert(s == '[' || s == ']');
  ___pdr_save_and_next(ls);
  while (ls->current == '=') {
    ___pdr_save_and_next(ls);
    count++;
  }
  return (ls->current == s) ? count : (-count) - 1;
}


static void read_long_string (___pdr_LexState *ls, ___pdr_SemInfo *seminfo, int sep) {
  int line = ls->linenumber;  /* initial line (for error message) */
  ___pdr_save_and_next(ls);  /* skip 2nd '[' */
  if (___pdr_currIsNewline(ls))  /* string starts with a newline? */
    inclinenumber(ls);  /* skip it */
  for (;;) {
    switch (ls->current) {
      case ___PDR_EOZ: {  /* error */
        const char *what = (seminfo ? "string" : "comment");
        const char *msg = ___pdr_luaO_pushfstring(ls->L,
                     "unfinished long %s (starting at line %d)", what, line);
        lexerror(ls, msg, PDR_TK_EOS);
        break;  /* to avoid warnings */
      }
      case ']': {
        if (skip_sep(ls) == sep) {
          ___pdr_save_and_next(ls);  /* skip 2nd ']' */
          goto endloop;
        }
        break;
      }
      case '\n': case '\r': {
        save(ls, '\n');
        inclinenumber(ls);
        if (!seminfo) ___pdr_luaZ_resetbuffer(ls->buff);  /* avoid wasting space */
        break;
      }
      default: {
        if (seminfo) ___pdr_save_and_next(ls);
        else ___pdr_next_tk(ls);
      }
    }
  } endloop:
  if (seminfo)
    seminfo->ts = ___pdr_luaX_newstring(ls, ___pdr_luaZ_buffer(ls->buff) + (2 + sep),
                                     ___pdr_luaZ_bufflen(ls->buff) - 2*(2 + sep));
}


static void esccheck (___pdr_LexState *ls, int c, const char *msg) {
  if (!c) {
    if (ls->current != ___PDR_EOZ)
      ___pdr_save_and_next(ls);  /* add current to buffer for error message */
    lexerror(ls, msg, PDR_TK_STRING);
  }
}


static int gethexa (___pdr_LexState *ls) {
  ___pdr_save_and_next(ls);
  esccheck (ls, ___pdr_lisxdigit(ls->current), "hexadecimal digit expected");
  return ___pdr_luaO_hexavalue(ls->current);
}


static int readhexaesc (___pdr_LexState *ls) {
  int r = gethexa(ls);
  r = (r << 4) + gethexa(ls);
  ___pdr_luaZ_buffremove(ls->buff, 2);  /* remove saved chars from buffer */
  return r;
}


static unsigned long readutf8esc (___pdr_LexState *ls) {
  unsigned long r;
  int i = 4;  /* chars to be removed: '\', 'u', '{', and first digit */
  ___pdr_save_and_next(ls);  /* skip 'u' */
  esccheck(ls, ls->current == '{', "missing '{'");
  r = gethexa(ls);  /* must have at least one digit */
  while ((___pdr_save_and_next(ls), ___pdr_lisxdigit(ls->current))) {
    i++;
    r = (r << 4) + ___pdr_luaO_hexavalue(ls->current);
    esccheck(ls, r <= 0x10FFFF, "UTF-8 value too large");
  }
  esccheck(ls, ls->current == '}', "missing '}'");
  ___pdr_next_tk(ls);  /* skip '}' */
  ___pdr_luaZ_buffremove(ls->buff, i);  /* remove saved chars from buffer */
  return r;
}


static void utf8esc (___pdr_LexState *ls) {
  char buff[___PDR_UTF8BUFFSZ];
  int n = ___pdr_luaO_utf8esc(buff, readutf8esc(ls));
  for (; n > 0; n--)  /* add 'buff' to string */
    save(ls, buff[___PDR_UTF8BUFFSZ - n]);
}


static int readdecesc (___pdr_LexState *ls) {
  int i;
  int r = 0;  /* result accumulator */
  for (i = 0; i < 3 && ___pdr_lisdigit(ls->current); i++) {  /* read up to 3 digits */
    r = 10*r + ls->current - '0';
    ___pdr_save_and_next(ls);
  }
  esccheck(ls, r <= UCHAR_MAX, "decimal escape too large");
  ___pdr_luaZ_buffremove(ls->buff, i);  /* remove read digits from buffer */
  return r;
}


static void read_string (___pdr_LexState *ls, int del, ___pdr_SemInfo *seminfo) {
  ___pdr_save_and_next(ls);  /* keep delimiter (for error messages) */
  while (ls->current != del) {
    switch (ls->current) {
      case ___PDR_EOZ:
        lexerror(ls, "unfinished string", PDR_TK_EOS);
        break;  /* to avoid warnings */
      case '\n':
      case '\r':
        lexerror(ls, "unfinished string", PDR_TK_STRING);
        break;  /* to avoid warnings */
      case '\\': {  /* escape sequences */
        int c;  /* final character to be saved */
        ___pdr_save_and_next(ls);  /* keep '\\' for error messages */
        switch (ls->current) {
          case 'a': c = '\a'; goto read_save;
          case 'b': c = '\b'; goto read_save;
          case 'f': c = '\f'; goto read_save;
          case 'n': c = '\n'; goto read_save;
          case 'r': c = '\r'; goto read_save;
          case 't': c = '\t'; goto read_save;
          case 'v': c = '\v'; goto read_save;
          case 'x': c = readhexaesc(ls); goto read_save;
          case 'u': utf8esc(ls);  goto no_save;
          case '\n': case '\r':
            inclinenumber(ls); c = '\n'; goto only_save;
          case '\\': case '\"': case '\'':
            c = ls->current; goto read_save;
          case ___PDR_EOZ: goto no_save;  /* will raise an error next loop */
          case 'z': {  /* zap following span of spaces */
            ___pdr_luaZ_buffremove(ls->buff, 1);  /* remove '\\' */
            ___pdr_next_tk(ls);  /* skip the 'z' */
            while (___pdr_lisspace(ls->current)) {
              if (___pdr_currIsNewline(ls)) inclinenumber(ls);
              else ___pdr_next_tk(ls);
            }
            goto no_save;
          }
          default: {
            esccheck(ls, ___pdr_lisdigit(ls->current), "invalid escape sequence");
            c = readdecesc(ls);  /* digital escape '\ddd' */
            goto only_save;
          }
        }
       read_save:
         ___pdr_next_tk(ls);
         /* go through */
       only_save:
         ___pdr_luaZ_buffremove(ls->buff, 1);  /* remove '\\' */
         save(ls, c);
         /* go through */
       no_save: break;
      }
      default:
        ___pdr_save_and_next(ls);
    }
  }
  ___pdr_save_and_next(ls);  /* skip delimiter */
  seminfo->ts = ___pdr_luaX_newstring(ls, ___pdr_luaZ_buffer(ls->buff) + 1,
                                   ___pdr_luaZ_bufflen(ls->buff) - 2);
}


static int llex (___pdr_LexState *ls, ___pdr_SemInfo *seminfo) {
  ___pdr_luaZ_resetbuffer(ls->buff);
  for (;;) {
    switch (ls->current) {
      case '\n': case '\r': {  /* line breaks */
        inclinenumber(ls);
        break;
      }
      case ' ': case '\f': case '\t': case '\v': {  /* spaces */
        ___pdr_next_tk(ls);
        break;
      }
      case '-': {  /* '-' or '--' (comment) */
        ___pdr_next_tk(ls);
        if (ls->current != '-') return '-';
        /* else is a comment */
        ___pdr_next_tk(ls);
        if (ls->current == '[') {  /* long comment? */
          int sep = skip_sep(ls);
          ___pdr_luaZ_resetbuffer(ls->buff);  /* 'skip_sep' may dirty the buffer */
          if (sep >= 0) {
            read_long_string(ls, NULL, sep);  /* skip long comment */
            ___pdr_luaZ_resetbuffer(ls->buff);  /* previous call may dirty the buff. */
            break;
          }
        }
        /* else short comment */
        while (!___pdr_currIsNewline(ls) && ls->current != ___PDR_EOZ)
          ___pdr_next_tk(ls);  /* skip until end of line (or end of file) */
        break;
      }
      case '[': {  /* long string or simply '[' */
        int sep = skip_sep(ls);
        if (sep >= 0) {
          read_long_string(ls, seminfo, sep);
          return PDR_TK_STRING;
        }
        else if (sep != -1)  /* '[=...' missing second bracket */
          lexerror(ls, "invalid long string delimiter", PDR_TK_STRING);
        return '[';
      }
      case '=': {
        ___pdr_next_tk(ls);
        if (check_next1(ls, '=')) return PDR_TK_EQ;
        else return '=';
      }
      case '<': {
        ___pdr_next_tk(ls);
        if (check_next1(ls, '=')) return PDR_TK_LE;
        else if (check_next1(ls, '<')) return PDR_TK_SHL;
        else return '<';
      }
      case '>': {
        ___pdr_next_tk(ls);
        if (check_next1(ls, '=')) return PDR_TK_GE;
        else if (check_next1(ls, '>')) return PDR_TK_SHR;
        else return '>';
      }
      case '/': {
        ___pdr_next_tk(ls);
        if (check_next1(ls, '/')) return PDR_TK_IDIV;
        else return '/';
      }
      case '~': {
        ___pdr_next_tk(ls);
        if (check_next1(ls, '=')) return PDR_TK_NE;
        else return '~';
      }
      case ':': {
        ___pdr_next_tk(ls);
        if (check_next1(ls, ':')) return PDR_TK_DBCOLON;
        else return ':';
      }
      case '"': case '\'': {  /* short literal strings */
        read_string(ls, ls->current, seminfo);
        return PDR_TK_STRING;
      }
      case '.': {  /* '.', '..', '...', or number */
        ___pdr_save_and_next(ls);
        if (check_next1(ls, '.')) {
          if (check_next1(ls, '.'))
            return PDR_TK_DOTS;   /* '...' */
          else return PDR_TK_CONCAT;   /* '..' */
        }
        else if (!___pdr_lisdigit(ls->current)) return '.';
        else return read_numeral(ls, seminfo);
      }
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9': {
        return read_numeral(ls, seminfo);
      }
      case ___PDR_EOZ: {
        return PDR_TK_EOS;
      }
      default: {
        if (___pdr_lislalpha(ls->current)) {  /* identifier or reserved word? */
          ___pdr_TString *ts;
          do {
            ___pdr_save_and_next(ls);
          } while (___pdr_lislalnum(ls->current));
          ts = ___pdr_luaX_newstring(ls, ___pdr_luaZ_buffer(ls->buff),
                                  ___pdr_luaZ_bufflen(ls->buff));
          seminfo->ts = ts;
          if (___pdr_isreserved(ts))  /* reserved word? */
            return ts->extra - 1 + ___PDR_FIRST_RESERVED;
          else {
            return PDR_TK_NAME;
          }
        }
        else {  /* single-char tokens (+ - / ...) */
          int c = ls->current;
          ___pdr_next_tk(ls);
          return c;
        }
      }
    }
  }
}


void ___pdr_luaX_next (___pdr_LexState *ls) {
  ls->lastline = ls->linenumber;
  if (ls->lookahead.token != PDR_TK_EOS) {  /* is there a look-ahead token? */
    ls->t = ls->lookahead;  /* use this one */
    ls->lookahead.token = PDR_TK_EOS;  /* and discharge it */
  }
  else
    ls->t.token = llex(ls, &ls->t.seminfo);  /* read next token */
}


int ___pdr_luaX_lookahead (___pdr_LexState *ls) {
  ___pdr_lua_assert(ls->lookahead.token == PDR_TK_EOS);
  ls->lookahead.token = llex(ls, &ls->lookahead.seminfo);
  return ls->lookahead.token;
}

} // end NS_PDR_SLUA