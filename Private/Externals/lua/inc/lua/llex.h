/*
** $Id: llex.h,v 1.79 2016/05/02 14:02:12 roberto Exp $
** Lexical Analyzer
** See Copyright Notice in lua.h
*/

#ifndef ___pdr_llex_h___
#define ___pdr_llex_h___

#include "lobject.h"
#include "lzio.h"

namespace NS_PDR_SLUA {

#define ___PDR_FIRST_RESERVED	257


#if !defined(___PDR_LUA_ENV)
#define ___PDR_LUA_ENV		"_ENV"
#endif


/*
* WARNING: if you change the order of this enumeration,
* grep "ORDER RESERVED"
*/
enum ___PDR_RESERVED {
  /* terminal symbols denoted by reserved words */
  PDR_TK_AND = ___PDR_FIRST_RESERVED, PDR_TK_BREAK,
  PDR_TK_DO, PDR_TK_ELSE, PDR_TK_ELSEIF, PDR_TK_END, PDR_TK_FALSE, PDR_TK_FOR, PDR_TK_FUNCTION,
  PDR_TK_GOTO, PDR_TK_IF, PDR_TK_IN, PDR_TK_LOCAL, PDR_TK_NIL, PDR_TK_NOT, PDR_TK_OR, PDR_TK_REPEAT,
  PDR_TK_RETURN, PDR_TK_THEN, PDR_TK_TRUE, PDR_TK_UNTIL, PDR_TK_WHILE,
  /* other terminal symbols */
  PDR_TK_IDIV, PDR_TK_CONCAT, PDR_TK_DOTS, PDR_TK_EQ, PDR_TK_GE, PDR_TK_LE, PDR_TK_NE,
  PDR_TK_SHL, PDR_TK_SHR,
  PDR_TK_DBCOLON, PDR_TK_EOS,
  PDR_TK_FLT, PDR_TK_INT, PDR_TK_NAME, PDR_TK_STRING
};

/* number of reserved words */
#define ___PDR_NUM_RESERVED	(___pdr_cast(int, PDR_TK_WHILE-___PDR_FIRST_RESERVED+1))


typedef union {
  ___pdr_lua_Number r;
  ___pdr_lua_Integer i;
  ___pdr_TString *ts;
} ___pdr_SemInfo;  /* semantics information */


typedef struct ___pdr_Token {
  int token;
  ___pdr_SemInfo seminfo;
} ___pdr_Token;


/* state of the lexer plus state of the parser when shared by all
   functions */
typedef struct ___pdr_LexState {
  int current;  /* current character (charint) */
  int linenumber;  /* input line counter */
  int lastline;  /* line of last token 'consumed' */
  ___pdr_Token t;  /* current token */
  ___pdr_Token lookahead;  /* look ahead token */
  struct ___pdr_FuncState *fs;  /* current function (parser) */
  struct ___pdr_lua_State *L;
  ___pdr_ZIO *z;  /* input stream */
  ___pdr_Mbuffer *buff;  /* buffer for tokens */
  ___pdr_Table *h;  /* to avoid collection/reuse strings */
  struct ___pdr_Dyndata *dyd;  /* dynamic structures used by the parser */
  ___pdr_TString *source;  /* current source name */
  ___pdr_TString *envn;  /* environment variable name */
} ___pdr_LexState;


___PDR_LUAI_FUNC void ___pdr_luaX_init (___pdr_lua_State *L);
___PDR_LUAI_FUNC void ___pdr_luaX_setinput (___pdr_lua_State *L, ___pdr_LexState *ls, ___pdr_ZIO *z,
                              ___pdr_TString *source, int firstchar);
___PDR_LUAI_FUNC ___pdr_TString *___pdr_luaX_newstring (___pdr_LexState *ls, const char *str, size_t l);
___PDR_LUAI_FUNC void ___pdr_luaX_next (___pdr_LexState *ls);
___PDR_LUAI_FUNC int ___pdr_luaX_lookahead (___pdr_LexState *ls);
___PDR_LUAI_FUNC ___pdr_l_noret ___pdr_luaX_syntaxerror (___pdr_LexState *ls, const char *s);
___PDR_LUAI_FUNC const char *___pdr_luaX_token2str (___pdr_LexState *ls, int token);

} // end NS_PDR_SLUA

#endif
