/*
** $Id: lparser.h,v 1.76 2015/12/30 18:16:13 roberto Exp $
** Lua Parser
** See Copyright Notice in lua.h
*/

#ifndef ___pdr_lparser_h___
#define ___pdr_lparser_h___

#include "llimits.h"
#include "lobject.h"
#include "lzio.h"

namespace NS_PDR_SLUA {

/*
** Expression and variable descriptor.
** Code generation for variables and expressions can be delayed to allow
** optimizations; An 'expdesc' structure describes a potentially-delayed
** variable/expression. It has a description of its "main" value plus a
** list of conditional jumps that can also produce its value (generated
** by short-circuit operators 'and'/'or').
*/

/* kinds of variables/expressions */
typedef enum {
  PDR_VVOID,  /* when 'expdesc' describes the last expression a list,
             this kind means an empty list (so, no expression) */
  PDR_VNIL,  /* constant nil */
  PDR_VTRUE,  /* constant true */
  PDR_VFALSE,  /* constant false */
  PDR_VK,  /* constant in 'k'; info = index of constant in 'k' */
  PDR_VKFLT,  /* floating constant; nval = numerical float value */
  PDR_VKINT,  /* integer constant; nval = numerical integer value */
  PDR_VNONRELOC,  /* expression has its value in a fixed register;
                 info = result register */
  PDR_VLOCAL,  /* local variable; info = local register */
  PDR_VUPVAL,  /* upvalue variable; info = index of upvalue in 'upvalues' */
  PDR_VINDEXED,  /* indexed variable;
                ind.vt = whether 't' is register or upvalue;
                ind.t = table register or upvalue;
                ind.idx = key's R/K index */
  PDR_VJMP,  /* expression is a test/comparison;
            info = pc of corresponding jump instruction */
  PDR_VRELOCABLE,  /* expression can put result in any register;
                  info = instruction pc */
  PDR_VCALL,  /* expression is a function call; info = instruction pc */
  PDR_VVARARG  /* vararg expression; info = instruction pc */
} ___pdr_expkind;


#define ___pdr_vkisvar(k)	(PDR_VLOCAL <= (k) && (k) <= PDR_VINDEXED)
#define ___pdr_vkisinreg(k)	((k) == PDR_VNONRELOC || (k) == PDR_VLOCAL)

typedef struct ___pdr_expdesc {
  ___pdr_expkind k;
  union {
    ___pdr_lua_Integer ival;    /* for VKINT */
    ___pdr_lua_Number nval;  /* for VKFLT */
    int info;  /* for generic use */
    struct {  /* for indexed variables (VINDEXED) */
      short idx;  /* index (R/K) */
      ___pdr_lu_byte t;  /* table (register or upvalue) */
      ___pdr_lu_byte vt;  /* whether 't' is register (VLOCAL) or upvalue (VUPVAL) */
    } ind;
  } u;
  int t;  /* patch list of 'exit when true' */
  int f;  /* patch list of 'exit when false' */
} ___pdr_expdesc;


/* description of active local variable */
typedef struct ___pdr_Vardesc {
  short idx;  /* variable index in stack */
} ___pdr_Vardesc;


/* description of pending goto statements and label statements */
typedef struct ___pdr_Labeldesc {
  ___pdr_TString *name;  /* label identifier */
  int pc;  /* position in code */
  int line;  /* line where it appeared */
  ___pdr_lu_byte nactvar;  /* local level where it appears in current block */
} ___pdr_Labeldesc;


/* list of labels or gotos */
typedef struct ___pdr_Labellist {
  ___pdr_Labeldesc *arr;  /* array */
  int n;  /* number of entries in use */
  int size;  /* array size */
} ___pdr_Labellist;


/* dynamic structures used by the parser */
typedef struct ___pdr_Dyndata {
  struct {  /* list of active local variables */
    ___pdr_Vardesc *arr;
    int n;
    int size;
  } actvar;
  ___pdr_Labellist gt;  /* list of pending gotos */
  ___pdr_Labellist label;   /* list of active labels */
} ___pdr_Dyndata;


/* control of blocks */
struct ___pdr_BlockCnt;  /* defined in lparser.c */


/* state needed to generate code for a given function */
typedef struct ___pdr_FuncState {
  ___pdr_Proto *f;  /* current function header */
  struct ___pdr_FuncState *prev;  /* enclosing function */
  struct ___pdr_LexState *ls;  /* lexical state */
  struct ___pdr_BlockCnt *bl;  /* chain of current blocks */
  int pc;  /* next position to code (equivalent to 'ncode') */
  int lasttarget;   /* 'label' of last 'jump label' */
  int jpc;  /* list of pending jumps to 'pc' */
  int nk;  /* number of elements in 'k' */
  int np;  /* number of elements in 'p' */
  int firstlocal;  /* index of first local var (in Dyndata array) */
  short nlocvars;  /* number of elements in 'f->locvars' */
  ___pdr_lu_byte nactvar;  /* number of active local variables */
  ___pdr_lu_byte nups;  /* number of upvalues */
  ___pdr_lu_byte freereg;  /* first free register */
} ___pdr_FuncState;


___PDR_LUAI_FUNC ___pdr_LClosure *___pdr_luaY_parser (___pdr_lua_State *L, ___pdr_ZIO *z, ___pdr_Mbuffer *buff,
                                 ___pdr_Dyndata *dyd, const char *name, int firstchar);

} // end NS_PDR_SLUA

#endif
