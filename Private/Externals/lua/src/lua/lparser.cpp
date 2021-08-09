/*
** $Id: lparser.c,v 2.155 2016/08/01 19:51:24 roberto Exp $
** Lua Parser
** See Copyright Notice in lua.h
*/

#define ___pdr_lparser_c
#define ___PDR_LUA_CORE

#include "lparser.h"
#include "lprefix.h"

#include <string.h>

#include "lua.h"
#include "lcode.h"
#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "llex.h"
#include "lmem.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"

namespace NS_PDR_SLUA {

/* maximum number of local variables per function (must be smaller
   than 250, due to the bytecode format) */
#define ___PDR_MAXVARS		200


#define ___pdr_hasmultret(k)		((k) == PDR_VCALL || (k) == PDR_VVARARG)


/* because all strings are unified by the scanner, the parser
   can use pointer equality for string equality */
#define ___pdr_eqstr(a,b)	((a) == (b))


/*
** nodes for block list (list of active blocks)
*/
typedef struct ___pdr_BlockCnt {
  struct ___pdr_BlockCnt *previous;  /* chain */
  int firstlabel;  /* index of first label in this block */
  int firstgoto;  /* index of first pending goto in this block */
  ___pdr_lu_byte nactvar;  /* # active locals outside the block */
  ___pdr_lu_byte upval;  /* true if some variable in the block is an upvalue */
  ___pdr_lu_byte isloop;  /* true if 'block' is a loop */
} ___pdr_BlockCnt;



/*
** prototypes for recursive non-terminal functions
*/
static void statement (___pdr_LexState *ls);
static void expr (___pdr_LexState *ls, ___pdr_expdesc *v);


/* semantic error */
static ___pdr_l_noret semerror (___pdr_LexState *ls, const char *msg) {
  ls->t.token = 0;  /* remove "near <token>" from final message */
  ___pdr_luaX_syntaxerror(ls, msg);
}


static ___pdr_l_noret error_expected (___pdr_LexState *ls, int token) {
  ___pdr_luaX_syntaxerror(ls,
      ___pdr_luaO_pushfstring(ls->L, "%s expected", ___pdr_luaX_token2str(ls, token)));
}


static ___pdr_l_noret errorlimit (___pdr_FuncState *fs, int limit, const char *what) {
  ___pdr_lua_State *L = fs->ls->L;
  const char *msg;
  int line = fs->f->linedefined;
  const char *where = (line == 0)
                      ? "main function"
                      : ___pdr_luaO_pushfstring(L, "function at line %d", line);
  msg = ___pdr_luaO_pushfstring(L, "too many %s (limit is %d) in %s",
                             what, limit, where);
  ___pdr_luaX_syntaxerror(fs->ls, msg);
}


static void checklimit (___pdr_FuncState *fs, int v, int l, const char *what) {
  if (v > l) errorlimit(fs, l, what);
}


static int testnext (___pdr_LexState *ls, int c) {
  if (ls->t.token == c) {
    ___pdr_luaX_next(ls);
    return 1;
  }
  else return 0;
}


static void check_tk (___pdr_LexState *ls, int c) {
  if (ls->t.token != c)
    error_expected(ls, c);
}


static void checknext (___pdr_LexState *ls, int c) {
  check_tk(ls, c);
  ___pdr_luaX_next(ls);
}


#define ___pdr_check_condition(ls,c,msg)	{ if (!(c)) ___pdr_luaX_syntaxerror(ls, msg); }



static void check_match (___pdr_LexState *ls, int what, int who, int where) {
  if (!testnext(ls, what)) {
    if (where == ls->linenumber)
      error_expected(ls, what);
    else {
      ___pdr_luaX_syntaxerror(ls, ___pdr_luaO_pushfstring(ls->L,
             "%s expected (to close %s at line %d)",
              ___pdr_luaX_token2str(ls, what), ___pdr_luaX_token2str(ls, who), where));
    }
  }
}


static ___pdr_TString *str_checkname (___pdr_LexState *ls) {
  ___pdr_TString *ts;
  check_tk(ls, PDR_TK_NAME);
  ts = ls->t.seminfo.ts;
  ___pdr_luaX_next(ls);
  return ts;
}


static void init_exp (___pdr_expdesc *e, ___pdr_expkind k, int i) {
  e->f = e->t = ___PDR_NO_JUMP;
  e->k = k;
  e->u.info = i;
}


static void codestring (___pdr_LexState *ls, ___pdr_expdesc *e, ___pdr_TString *s) {
  init_exp(e, PDR_VK, ___pdr_luaK_stringK(ls->fs, s));
}


static void checkname (___pdr_LexState *ls, ___pdr_expdesc *e) {
  codestring(ls, e, str_checkname(ls));
}


static int registerlocalvar (___pdr_LexState *ls, ___pdr_TString *varname) {
  ___pdr_FuncState *fs = ls->fs;
  ___pdr_Proto *f = fs->f;
  int oldsize = f->sizelocvars;
  ___pdr_luaM_growvector(ls->L, f->locvars, fs->nlocvars, f->sizelocvars,
                  ___pdr_LocVar, SHRT_MAX, "local variables");
  while (oldsize < f->sizelocvars)
    f->locvars[oldsize++].varname = NULL;
  f->locvars[fs->nlocvars].varname = varname;
  ___pdr_luaC_objbarrier(ls->L, f, varname);
  return fs->nlocvars++;
}


static void new_localvar (___pdr_LexState *ls, ___pdr_TString *name) {
  ___pdr_FuncState *fs = ls->fs;
  ___pdr_Dyndata *dyd = ls->dyd;
  int reg = registerlocalvar(ls, name);
  checklimit(fs, dyd->actvar.n + 1 - fs->firstlocal,
                  ___PDR_MAXVARS, "local variables");
  ___pdr_luaM_growvector(ls->L, dyd->actvar.arr, dyd->actvar.n + 1,
                  dyd->actvar.size, ___pdr_Vardesc, ___PDR_MAX_INT, "local variables");
  dyd->actvar.arr[dyd->actvar.n++].idx = ___pdr_cast(short, reg);
}


static void new_localvarliteral_ (___pdr_LexState *ls, const char *name, size_t sz) {
  new_localvar(ls, ___pdr_luaX_newstring(ls, name, sz));
}

#define ___pdr_new_localvarliteral(ls,v) \
	new_localvarliteral_(ls, "" v, (sizeof(v)/sizeof(char))-1)


static ___pdr_LocVar *getlocvar (___pdr_FuncState *fs, int i) {
  int idx = fs->ls->dyd->actvar.arr[fs->firstlocal + i].idx;
  ___pdr_lua_assert(idx < fs->nlocvars);
  return &fs->f->locvars[idx];
}


static void adjustlocalvars (___pdr_LexState *ls, int nvars) {
  ___pdr_FuncState *fs = ls->fs;
  fs->nactvar = ___pdr_cast_byte(fs->nactvar + nvars);
  for (; nvars; nvars--) {
    getlocvar(fs, fs->nactvar - nvars)->startpc = fs->pc;
  }
}


static void removevars (___pdr_FuncState *fs, int tolevel) {
  fs->ls->dyd->actvar.n -= (fs->nactvar - tolevel);
  while (fs->nactvar > tolevel)
    getlocvar(fs, --fs->nactvar)->endpc = fs->pc;
}


static int searchupvalue (___pdr_FuncState *fs, ___pdr_TString *name) {
  int i;
  ___pdr_Upvaldesc *up = fs->f->upvalues;
  for (i = 0; i < fs->nups; i++) {
    if (___pdr_eqstr(up[i].name, name)) return i;
  }
  return -1;  /* not found */
}


static int newupvalue (___pdr_FuncState *fs, ___pdr_TString *name, ___pdr_expdesc *v) {
  ___pdr_Proto *f = fs->f;
  int oldsize = f->sizeupvalues;
  checklimit(fs, fs->nups + 1, ___PDR_MAXUPVAL, "upvalues");
  ___pdr_luaM_growvector(fs->ls->L, f->upvalues, fs->nups, f->sizeupvalues,
                  ___pdr_Upvaldesc, ___PDR_MAXUPVAL, "upvalues");
  while (oldsize < f->sizeupvalues)
    f->upvalues[oldsize++].name = NULL;
  f->upvalues[fs->nups].instack = (v->k == PDR_VLOCAL);
  f->upvalues[fs->nups].idx = ___pdr_cast_byte(v->u.info);
  f->upvalues[fs->nups].name = name;
  ___pdr_luaC_objbarrier(fs->ls->L, f, name);
  return fs->nups++;
}


static int searchvar (___pdr_FuncState *fs, ___pdr_TString *n) {
  int i;
  for (i = ___pdr_cast_int(fs->nactvar) - 1; i >= 0; i--) {
    if (___pdr_eqstr(n, getlocvar(fs, i)->varname))
      return i;
  }
  return -1;  /* not found */
}


/*
  Mark block where variable at given level was defined
  (to emit close instructions later).
*/
static void markupval (___pdr_FuncState *fs, int level) {
  ___pdr_BlockCnt *bl = fs->bl;
  while (bl->nactvar > level)
    bl = bl->previous;
  bl->upval = 1;
}


/*
  Find variable with given name 'n'. If it is an upvalue, add this
  upvalue into all intermediate functions.
*/
static void singlevaraux (___pdr_FuncState *fs, ___pdr_TString *n, ___pdr_expdesc *var, int base) {
  if (fs == NULL)  /* no more levels? */
    init_exp(var, PDR_VVOID, 0);  /* default is global */
  else {
    int v = searchvar(fs, n);  /* look up locals at current level */
    if (v >= 0) {  /* found? */
      init_exp(var, PDR_VLOCAL, v);  /* variable is local */
      if (!base)
        markupval(fs, v);  /* local will be used as an upval */
    }
    else {  /* not found as local at current level; try upvalues */
      int idx = searchupvalue(fs, n);  /* try existing upvalues */
      if (idx < 0) {  /* not found? */
        singlevaraux(fs->prev, n, var, 0);  /* try upper levels */
        if (var->k == PDR_VVOID)  /* not found? */
          return;  /* it is a global */
        /* else was LOCAL or UPVAL */
        idx  = newupvalue(fs, n, var);  /* will be a new upvalue */
      }
      init_exp(var, PDR_VUPVAL, idx);  /* new or old upvalue */
    }
  }
}


static void singlevar (___pdr_LexState *ls, ___pdr_expdesc *var) {
  ___pdr_TString *varname = str_checkname(ls);
  ___pdr_FuncState *fs = ls->fs;
  singlevaraux(fs, varname, var, 1);
  if (var->k == PDR_VVOID) {  /* global name? */
    ___pdr_expdesc key;
    singlevaraux(fs, ls->envn, var, 1);  /* get environment variable */
    ___pdr_lua_assert(var->k != PDR_VVOID);  /* this one must exist */
    codestring(ls, &key, varname);  /* key is variable name */
    ___pdr_luaK_indexed(fs, var, &key);  /* env[varname] */
  }
}


static void adjust_assign (___pdr_LexState *ls, int nvars, int nexps, ___pdr_expdesc *e) {
  ___pdr_FuncState *fs = ls->fs;
  int extra = nvars - nexps;
  if (___pdr_hasmultret(e->k)) {
    extra++;  /* includes call itself */
    if (extra < 0) extra = 0;
    ___pdr_luaK_setreturns(fs, e, extra);  /* last exp. provides the difference */
    if (extra > 1) ___pdr_luaK_reserveregs(fs, extra-1);
  }
  else {
    if (e->k != PDR_VVOID) ___pdr_luaK_exp2nextreg(fs, e);  /* close last expression */
    if (extra > 0) {
      int reg = fs->freereg;
      ___pdr_luaK_reserveregs(fs, extra);
      ___pdr_luaK_nil(fs, reg, extra);
    }
  }
  if (nexps > nvars)
    ls->fs->freereg -= nexps - nvars;  /* remove extra values */
}


static void enterlevel (___pdr_LexState *ls) {
  ___pdr_lua_State *L = ls->L;
  ++L->nCcalls;
  checklimit(ls->fs, L->nCcalls, ___PDR_LUAI_MAXCCALLS, "C levels");
}


#define ___pdr_leavelevel(ls)	((ls)->L->nCcalls--)


static void closegoto (___pdr_LexState *ls, int g, ___pdr_Labeldesc *label) {
  int i;
  ___pdr_FuncState *fs = ls->fs;
  ___pdr_Labellist *gl = &ls->dyd->gt;
  ___pdr_Labeldesc *gt = &gl->arr[g];
  ___pdr_lua_assert(___pdr_eqstr(gt->name, label->name));
  if (gt->nactvar < label->nactvar) {
    ___pdr_TString *vname = getlocvar(fs, gt->nactvar)->varname;
    const char *msg = ___pdr_luaO_pushfstring(ls->L,
      "<goto %s> at line %d jumps into the scope of local '%s'",
      ___pdr_getstr(gt->name), gt->line, ___pdr_getstr(vname));
    semerror(ls, msg);
  }
  ___pdr_luaK_patchlist(fs, gt->pc, label->pc);
  /* remove goto from pending list */
  for (i = g; i < gl->n - 1; i++)
    gl->arr[i] = gl->arr[i + 1];
  gl->n--;
}


/*
** try to close a goto with existing labels; this solves backward jumps
*/
static int findlabel (___pdr_LexState *ls, int g) {
  int i;
  ___pdr_BlockCnt *bl = ls->fs->bl;
  ___pdr_Dyndata *dyd = ls->dyd;
  ___pdr_Labeldesc *gt = &dyd->gt.arr[g];
  /* check labels in current block for a match */
  for (i = bl->firstlabel; i < dyd->label.n; i++) {
    ___pdr_Labeldesc *lb = &dyd->label.arr[i];
    if (___pdr_eqstr(lb->name, gt->name)) {  /* correct label? */
      if (gt->nactvar > lb->nactvar &&
          (bl->upval || dyd->label.n > bl->firstlabel))
        ___pdr_luaK_patchclose(ls->fs, gt->pc, lb->nactvar);
      closegoto(ls, g, lb);  /* close it */
      return 1;
    }
  }
  return 0;  /* label not found; cannot close goto */
}


static int newlabelentry (___pdr_LexState *ls, ___pdr_Labellist *l, ___pdr_TString *name,
                          int line, int pc) {
  int n = l->n;
  ___pdr_luaM_growvector(ls->L, l->arr, n, l->size,
                  ___pdr_Labeldesc, SHRT_MAX, "labels/gotos");
  l->arr[n].name = name;
  l->arr[n].line = line;
  l->arr[n].nactvar = ls->fs->nactvar;
  l->arr[n].pc = pc;
  l->n = n + 1;
  return n;
}


/*
** check whether new label 'lb' matches any pending gotos in current
** block; solves forward jumps
*/
static void findgotos (___pdr_LexState *ls, ___pdr_Labeldesc *lb) {
  ___pdr_Labellist *gl = &ls->dyd->gt;
  int i = ls->fs->bl->firstgoto;
  while (i < gl->n) {
    if (___pdr_eqstr(gl->arr[i].name, lb->name))
      closegoto(ls, i, lb);
    else
      i++;
  }
}


/*
** export pending gotos to outer level, to check them against
** outer labels; if the block being exited has upvalues, and
** the goto exits the scope of any variable (which can be the
** upvalue), close those variables being exited.
*/
static void movegotosout (___pdr_FuncState *fs, ___pdr_BlockCnt *bl) {
  int i = bl->firstgoto;
  ___pdr_Labellist *gl = &fs->ls->dyd->gt;
  /* correct pending gotos to current block and try to close it
     with visible labels */
  while (i < gl->n) {
    ___pdr_Labeldesc *gt = &gl->arr[i];
    if (gt->nactvar > bl->nactvar) {
      if (bl->upval)
        ___pdr_luaK_patchclose(fs, gt->pc, bl->nactvar);
      gt->nactvar = bl->nactvar;
    }
    if (!findlabel(fs->ls, i))
      i++;  /* move to next one */
  }
}


static void enterblock (___pdr_FuncState *fs, ___pdr_BlockCnt *bl, ___pdr_lu_byte isloop) {
  bl->isloop = isloop;
  bl->nactvar = fs->nactvar;
  bl->firstlabel = fs->ls->dyd->label.n;
  bl->firstgoto = fs->ls->dyd->gt.n;
  bl->upval = 0;
  bl->previous = fs->bl;
  fs->bl = bl;
  ___pdr_lua_assert(fs->freereg == fs->nactvar);
}


/*
** create a label named 'break' to resolve break statements
*/
static void breaklabel (___pdr_LexState *ls) {
  ___pdr_TString *n = ___pdr_luaS_new(ls->L, "break");
  int l = newlabelentry(ls, &ls->dyd->label, n, 0, ls->fs->pc);
  findgotos(ls, &ls->dyd->label.arr[l]);
}

/*
** generates an error for an undefined 'goto'; choose appropriate
** message when label name is a reserved word (which can only be 'break')
*/
static ___pdr_l_noret undefgoto (___pdr_LexState *ls, ___pdr_Labeldesc *gt) {
  const char *msg = ___pdr_isreserved(gt->name)
                    ? "<%s> at line %d not inside a loop"
                    : "no visible label '%s' for <goto> at line %d";
  msg = ___pdr_luaO_pushfstring(ls->L, msg, ___pdr_getstr(gt->name), gt->line);
  semerror(ls, msg);
}


static void leaveblock (___pdr_FuncState *fs) {
  ___pdr_BlockCnt *bl = fs->bl;
  ___pdr_LexState *ls = fs->ls;
  if (bl->previous && bl->upval) {
    /* create a 'jump to here' to close upvalues */
    int j = ___pdr_luaK_jump(fs);
    ___pdr_luaK_patchclose(fs, j, bl->nactvar);
    ___pdr_luaK_patchtohere(fs, j);
  }
  if (bl->isloop)
    breaklabel(ls);  /* close pending breaks */
  fs->bl = bl->previous;
  removevars(fs, bl->nactvar);
  ___pdr_lua_assert(bl->nactvar == fs->nactvar);
  fs->freereg = fs->nactvar;  /* free registers */
  ls->dyd->label.n = bl->firstlabel;  /* remove local labels */
  if (bl->previous)  /* inner block? */
    movegotosout(fs, bl);  /* update pending gotos to outer block */
  else if (bl->firstgoto < ls->dyd->gt.n)  /* pending gotos in outer block? */
    undefgoto(ls, &ls->dyd->gt.arr[bl->firstgoto]);  /* error */
}


/*
** adds a new prototype into list of prototypes
*/
static ___pdr_Proto *addprototype (___pdr_LexState *ls) {
  ___pdr_Proto *clp;
  ___pdr_lua_State *L = ls->L;
  ___pdr_FuncState *fs = ls->fs;
  ___pdr_Proto *f = fs->f;  /* prototype of current function */
  if (fs->np >= f->sizep) {
    int oldsize = f->sizep;
    ___pdr_luaM_growvector(L, f->p, fs->np, f->sizep, ___pdr_Proto *, ___PDR_MAXARG_Bx, "functions");
    while (oldsize < f->sizep)
      f->p[oldsize++] = NULL;
  }
  f->p[fs->np++] = clp = ___pdr_luaF_newproto(L);
  ___pdr_luaC_objbarrier(L, f, clp);
  return clp;
}


/*
** codes instruction to create new closure in parent function.
** The OP_CLOSURE instruction must use the last available register,
** so that, if it invokes the GC, the GC knows which registers
** are in use at that time.
*/
static void codeclosure (___pdr_LexState *ls, ___pdr_expdesc *v) {
  ___pdr_FuncState *fs = ls->fs->prev;
  init_exp(v, PDR_VRELOCABLE, ___pdr_luaK_codeABx(fs, PDR_OP_CLOSURE, 0, fs->np - 1));
  ___pdr_luaK_exp2nextreg(fs, v);  /* fix it at the last register */
}


static void open_func (___pdr_LexState *ls, ___pdr_FuncState *fs, ___pdr_BlockCnt *bl) {
  ___pdr_Proto *f;
  fs->prev = ls->fs;  /* linked list of funcstates */
  fs->ls = ls;
  ls->fs = fs;
  fs->pc = 0;
  fs->lasttarget = 0;
  fs->jpc = ___PDR_NO_JUMP;
  fs->freereg = 0;
  fs->nk = 0;
  fs->np = 0;
  fs->nups = 0;
  fs->nlocvars = 0;
  fs->nactvar = 0;
  fs->firstlocal = ls->dyd->actvar.n;
  fs->bl = NULL;
  f = fs->f;
  f->source = ls->source;
  f->maxstacksize = 2;  /* registers 0/1 are always valid */
  enterblock(fs, bl, 0);
}


static void close_func (___pdr_LexState *ls) {
  ___pdr_lua_State *L = ls->L;
  ___pdr_FuncState *fs = ls->fs;
  ___pdr_Proto *f = fs->f;
  ___pdr_luaK_ret(fs, 0, 0);  /* final return */
  leaveblock(fs);
  ___pdr_luaM_reallocvector(L, f->code, f->sizecode, fs->pc, ___pdr_Instruction);
  f->sizecode = fs->pc;
  ___pdr_luaM_reallocvector(L, f->lineinfo, f->sizelineinfo, fs->pc, int);
  f->sizelineinfo = fs->pc;
  ___pdr_luaM_reallocvector(L, f->k, f->sizek, fs->nk, ___pdr_TValue);
  f->sizek = fs->nk;
  ___pdr_luaM_reallocvector(L, f->p, f->sizep, fs->np, ___pdr_Proto *);
  f->sizep = fs->np;
  ___pdr_luaM_reallocvector(L, f->locvars, f->sizelocvars, fs->nlocvars, ___pdr_LocVar);
  f->sizelocvars = fs->nlocvars;
  ___pdr_luaM_reallocvector(L, f->upvalues, f->sizeupvalues, fs->nups, ___pdr_Upvaldesc);
  f->sizeupvalues = fs->nups;
  ___pdr_lua_assert(fs->bl == NULL);
  ls->fs = fs->prev;
  ___pdr_luaC_checkGC(L);
}



/*============================================================*/
/* GRAMMAR RULES */
/*============================================================*/


/*
** check whether current token is in the follow set of a block.
** 'until' closes syntactical blocks, but do not close scope,
** so it is handled in separate.
*/
static int block_follow (___pdr_LexState *ls, int withuntil) {
  switch (ls->t.token) {
    case PDR_TK_ELSE: case PDR_TK_ELSEIF:
    case PDR_TK_END: case PDR_TK_EOS:
      return 1;
    case PDR_TK_UNTIL: return withuntil;
    default: return 0;
  }
}


static void statlist (___pdr_LexState *ls) {
  /* statlist -> { stat [';'] } */
  while (!block_follow(ls, 1)) {
    if (ls->t.token == PDR_TK_RETURN) {
      statement(ls);
      return;  /* 'return' must be last statement */
    }
    statement(ls);
  }
}


static void fieldsel (___pdr_LexState *ls, ___pdr_expdesc *v) {
  /* fieldsel -> ['.' | ':'] NAME */
  ___pdr_FuncState *fs = ls->fs;
  ___pdr_expdesc key;
  ___pdr_luaK_exp2anyregup(fs, v);
  ___pdr_luaX_next(ls);  /* skip the dot or colon */
  checkname(ls, &key);
  ___pdr_luaK_indexed(fs, v, &key);
}


static void yindex (___pdr_LexState *ls, ___pdr_expdesc *v) {
  /* index -> '[' expr ']' */
  ___pdr_luaX_next(ls);  /* skip the '[' */
  expr(ls, v);
  ___pdr_luaK_exp2val(ls->fs, v);
  checknext(ls, ']');
}


/*
** {======================================================================
** Rules for Constructors
** =======================================================================
*/


struct ConsControl {
  ___pdr_expdesc v;  /* last list item read */
  ___pdr_expdesc *t;  /* table descriptor */
  int nh;  /* total number of 'record' elements */
  int na;  /* total number of array elements */
  int tostore;  /* number of array elements pending to be stored */
};


static void recfield (___pdr_LexState *ls, struct ConsControl *cc) {
  /* recfield -> (NAME | '['exp1']') = exp1 */
  ___pdr_FuncState *fs = ls->fs;
  int reg = ls->fs->freereg;
  ___pdr_expdesc key, val;
  int rkkey;
  if (ls->t.token == PDR_TK_NAME) {
    checklimit(fs, cc->nh, ___PDR_MAX_INT, "items in a constructor");
    checkname(ls, &key);
  }
  else  /* ls->t.token == '[' */
    yindex(ls, &key);
  cc->nh++;
  checknext(ls, '=');
  rkkey = ___pdr_luaK_exp2RK(fs, &key);
  expr(ls, &val);
  ___pdr_luaK_codeABC(fs, PDR_OP_SETTABLE, cc->t->u.info, rkkey, ___pdr_luaK_exp2RK(fs, &val));
  fs->freereg = reg;  /* free registers */
}


static void closelistfield (___pdr_FuncState *fs, struct ConsControl *cc) {
  if (cc->v.k == PDR_VVOID) return;  /* there is no list item */
  ___pdr_luaK_exp2nextreg(fs, &cc->v);
  cc->v.k = PDR_VVOID;
  if (cc->tostore == ___PDR_LFIELDS_PER_FLUSH) {
    ___pdr_luaK_setlist(fs, cc->t->u.info, cc->na, cc->tostore);  /* flush */
    cc->tostore = 0;  /* no more items pending */
  }
}


static void lastlistfield (___pdr_FuncState *fs, struct ConsControl *cc) {
  if (cc->tostore == 0) return;
  if (___pdr_hasmultret(cc->v.k)) {
    ___pdr_luaK_setmultret(fs, &cc->v);
    ___pdr_luaK_setlist(fs, cc->t->u.info, cc->na, ___PDR_LUA_MULTRET);
    cc->na--;  /* do not count last expression (unknown number of elements) */
  }
  else {
    if (cc->v.k != PDR_VVOID)
      ___pdr_luaK_exp2nextreg(fs, &cc->v);
    ___pdr_luaK_setlist(fs, cc->t->u.info, cc->na, cc->tostore);
  }
}


static void listfield (___pdr_LexState *ls, struct ConsControl *cc) {
  /* listfield -> exp */
  expr(ls, &cc->v);
  checklimit(ls->fs, cc->na, ___PDR_MAX_INT, "items in a constructor");
  cc->na++;
  cc->tostore++;
}


static void field (___pdr_LexState *ls, struct ConsControl *cc) {
  /* field -> listfield | recfield */
  switch(ls->t.token) {
    case PDR_TK_NAME: {  /* may be 'listfield' or 'recfield' */
      if (___pdr_luaX_lookahead(ls) != '=')  /* expression? */
        listfield(ls, cc);
      else
        recfield(ls, cc);
      break;
    }
    case '[': {
      recfield(ls, cc);
      break;
    }
    default: {
      listfield(ls, cc);
      break;
    }
  }
}


static void constructor (___pdr_LexState *ls, ___pdr_expdesc *t) {
  /* constructor -> '{' [ field { sep field } [sep] ] '}'
     sep -> ',' | ';' */
  ___pdr_FuncState *fs = ls->fs;
  int line = ls->linenumber;
  int pc = ___pdr_luaK_codeABC(fs, PDR_OP_NEWTABLE, 0, 0, 0);
  struct ConsControl cc;
  cc.na = cc.nh = cc.tostore = 0;
  cc.t = t;
  init_exp(t, PDR_VRELOCABLE, pc);
  init_exp(&cc.v, PDR_VVOID, 0);  /* no value (yet) */
  ___pdr_luaK_exp2nextreg(ls->fs, t);  /* fix it at stack top */
  checknext(ls, '{');
  do {
    ___pdr_lua_assert(cc.v.k == PDR_VVOID || cc.tostore > 0);
    if (ls->t.token == '}') break;
    closelistfield(fs, &cc);
    field(ls, &cc);
  } while (testnext(ls, ',') || testnext(ls, ';'));
  check_match(ls, '}', '{', line);
  lastlistfield(fs, &cc);
  ___PDR_SETARG_B(fs->f->code[pc], ___pdr_luaO_int2fb(cc.na)); /* set initial array size */
  ___PDR_SETARG_C(fs->f->code[pc], ___pdr_luaO_int2fb(cc.nh));  /* set initial table size */
}

/* }====================================================================== */



static void parlist (___pdr_LexState *ls) {
  /* parlist -> [ param { ',' param } ] */
  ___pdr_FuncState *fs = ls->fs;
  ___pdr_Proto *f = fs->f;
  int nparams = 0;
  f->is_vararg = 0;
  if (ls->t.token != ')') {  /* is 'parlist' not empty? */
    do {
      switch (ls->t.token) {
        case PDR_TK_NAME: {  /* param -> NAME */
          new_localvar(ls, str_checkname(ls));
          nparams++;
          break;
        }
        case PDR_TK_DOTS: {  /* param -> '...' */
          ___pdr_luaX_next(ls);
          f->is_vararg = 1;  /* declared vararg */
          break;
        }
        default: ___pdr_luaX_syntaxerror(ls, "<name> or '...' expected");
      }
    } while (!f->is_vararg && testnext(ls, ','));
  }
  adjustlocalvars(ls, nparams);
  f->numparams = ___pdr_cast_byte(fs->nactvar);
  ___pdr_luaK_reserveregs(fs, fs->nactvar);  /* reserve register for parameters */
}


static void body (___pdr_LexState *ls, ___pdr_expdesc *e, int ismethod, int line) {
  /* body ->  '(' parlist ')' block END */
  ___pdr_FuncState new_fs;
  ___pdr_BlockCnt bl;
  new_fs.f = addprototype(ls);
  new_fs.f->linedefined = line;
  open_func(ls, &new_fs, &bl);
  checknext(ls, '(');
  if (ismethod) {
    ___pdr_new_localvarliteral(ls, "self");  /* create 'self' parameter */
    adjustlocalvars(ls, 1);
  }
  parlist(ls);
  checknext(ls, ')');
  statlist(ls);
  new_fs.f->lastlinedefined = ls->linenumber;
  check_match(ls, PDR_TK_END, PDR_TK_FUNCTION, line);
  codeclosure(ls, e);
  close_func(ls);
}


static int explist (___pdr_LexState *ls, ___pdr_expdesc *v) {
  /* explist -> expr { ',' expr } */
  int n = 1;  /* at least one expression */
  expr(ls, v);
  while (testnext(ls, ',')) {
    ___pdr_luaK_exp2nextreg(ls->fs, v);
    expr(ls, v);
    n++;
  }
  return n;
}


static void funcargs (___pdr_LexState *ls, ___pdr_expdesc *f, int line) {
  ___pdr_FuncState *fs = ls->fs;
  ___pdr_expdesc args;
  int base, nparams;
  switch (ls->t.token) {
    case '(': {  /* funcargs -> '(' [ explist ] ')' */
      ___pdr_luaX_next(ls);
      if (ls->t.token == ')')  /* arg list is empty? */
        args.k = PDR_VVOID;
      else {
        explist(ls, &args);
        ___pdr_luaK_setmultret(fs, &args);
      }
      check_match(ls, ')', '(', line);
      break;
    }
    case '{': {  /* funcargs -> constructor */
      constructor(ls, &args);
      break;
    }
    case PDR_TK_STRING: {  /* funcargs -> STRING */
      codestring(ls, &args, ls->t.seminfo.ts);
      ___pdr_luaX_next(ls);  /* must use 'seminfo' before 'next' */
      break;
    }
    default: {
      ___pdr_luaX_syntaxerror(ls, "function arguments expected");
    }
  }
  ___pdr_lua_assert(f->k == PDR_VNONRELOC);
  base = f->u.info;  /* base register for call */
  if (___pdr_hasmultret(args.k))
    nparams = ___PDR_LUA_MULTRET;  /* open call */
  else {
    if (args.k != PDR_VVOID)
      ___pdr_luaK_exp2nextreg(fs, &args);  /* close last argument */
    nparams = fs->freereg - (base+1);
  }
  init_exp(f, PDR_VCALL, ___pdr_luaK_codeABC(fs, PDR_OP_CALL, base, nparams+1, 2));
  ___pdr_luaK_fixline(fs, line);
  fs->freereg = base+1;  /* call remove function and arguments and leaves
                            (unless changed) one result */
}




/*
** {======================================================================
** Expression parsing
** =======================================================================
*/


static void primaryexp (___pdr_LexState *ls, ___pdr_expdesc *v) {
  /* primaryexp -> NAME | '(' expr ')' */
  switch (ls->t.token) {
    case '(': {
      int line = ls->linenumber;
      ___pdr_luaX_next(ls);
      expr(ls, v);
      check_match(ls, ')', '(', line);
      ___pdr_luaK_dischargevars(ls->fs, v);
      return;
    }
    case PDR_TK_NAME: {
      singlevar(ls, v);
      return;
    }
    default: {
      ___pdr_luaX_syntaxerror(ls, "unexpected symbol");
    }
  }
}


static void suffixedexp (___pdr_LexState *ls, ___pdr_expdesc *v) {
  /* suffixedexp ->
       primaryexp { '.' NAME | '[' exp ']' | ':' NAME funcargs | funcargs } */
  ___pdr_FuncState *fs = ls->fs;
  int line = ls->linenumber;
  primaryexp(ls, v);
  for (;;) {
    switch (ls->t.token) {
      case '.': {  /* fieldsel */
        fieldsel(ls, v);
        break;
      }
      case '[': {  /* '[' exp1 ']' */
        ___pdr_expdesc key;
        ___pdr_luaK_exp2anyregup(fs, v);
        yindex(ls, &key);
        ___pdr_luaK_indexed(fs, v, &key);
        break;
      }
      case ':': {  /* ':' NAME funcargs */
        ___pdr_expdesc key;
        ___pdr_luaX_next(ls);
        checkname(ls, &key);
        ___pdr_luaK_self(fs, v, &key);
        funcargs(ls, v, line);
        break;
      }
      case '(': case PDR_TK_STRING: case '{': {  /* funcargs */
        ___pdr_luaK_exp2nextreg(fs, v);
        funcargs(ls, v, line);
        break;
      }
      default: return;
    }
  }
}


static void simpleexp (___pdr_LexState *ls, ___pdr_expdesc *v) {
  /* simpleexp -> FLT | INT | STRING | NIL | TRUE | FALSE | ... |
                  constructor | FUNCTION body | suffixedexp */
  switch (ls->t.token) {
    case PDR_TK_FLT: {
      init_exp(v, PDR_VKFLT, 0);
      v->u.nval = ls->t.seminfo.r;
      break;
    }
    case PDR_TK_INT: {
      init_exp(v, PDR_VKINT, 0);
      v->u.ival = ls->t.seminfo.i;
      break;
    }
    case PDR_TK_STRING: {
      codestring(ls, v, ls->t.seminfo.ts);
      break;
    }
    case PDR_TK_NIL: {
      init_exp(v, PDR_VNIL, 0);
      break;
    }
    case PDR_TK_TRUE: {
      init_exp(v, PDR_VTRUE, 0);
      break;
    }
    case PDR_TK_FALSE: {
      init_exp(v, PDR_VFALSE, 0);
      break;
    }
    case PDR_TK_DOTS: {  /* vararg */
      ___pdr_FuncState *fs = ls->fs;
      ___pdr_check_condition(ls, fs->f->is_vararg,
                      "cannot use '...' outside a vararg function");
      init_exp(v, PDR_VVARARG, ___pdr_luaK_codeABC(fs, PDR_OP_VARARG, 0, 1, 0));
      break;
    }
    case '{': {  /* constructor */
      constructor(ls, v);
      return;
    }
    case PDR_TK_FUNCTION: {
      ___pdr_luaX_next(ls);
      body(ls, v, 0, ls->linenumber);
      return;
    }
    default: {
      suffixedexp(ls, v);
      return;
    }
  }
  ___pdr_luaX_next(ls);
}


static ___pdr_UnOpr getunopr (int op) {
  switch (op) {
    case PDR_TK_NOT: return PDR_OPR_NOT;
    case '-': return PDR_OPR_MINUS;
    case '~': return PDR_OPR_BNOT;
    case '#': return PDR_OPR_LEN;
    default: return PDR_OPR_NOUNOPR;
  }
}


static ___pdr_BinOpr getbinopr (int op) {
  switch (op) {
    case '+': return PDR_OPR_ADD;
    case '-': return PDR_OPR_SUB;
    case '*': return PDR_OPR_MUL;
    case '%': return PDR_OPR_MOD;
    case '^': return PDR_OPR_POW;
    case '/': return PDR_OPR_DIV;
    case PDR_TK_IDIV: return PDR_OPR_IDIV;
    case '&': return PDR_OPR_BAND;
    case '|': return PDR_OPR_BOR;
    case '~': return PDR_OPR_BXOR;
    case PDR_TK_SHL: return PDR_OPR_SHL;
    case PDR_TK_SHR: return PDR_OPR_SHR;
    case PDR_TK_CONCAT: return PDR_OPR_CONCAT;
    case PDR_TK_NE: return PDR_OPR_NE;
    case PDR_TK_EQ: return PDR_OPR_EQ;
    case '<': return PDR_OPR_LT;
    case PDR_TK_LE: return PDR_OPR_LE;
    case '>': return PDR_OPR_GT;
    case PDR_TK_GE: return PDR_OPR_GE;
    case PDR_TK_AND: return PDR_OPR_AND;
    case PDR_TK_OR: return PDR_OPR_OR;
    default: return PDR_OPR_NOBINOPR;
  }
}


static const struct {
  ___pdr_lu_byte left;  /* left priority for each binary operator */
  ___pdr_lu_byte right; /* right priority */
} priority[] = {  /* ORDER OPR */
   {10, 10}, {10, 10},           /* '+' '-' */
   {11, 11}, {11, 11},           /* '*' '%' */
   {14, 13},                  /* '^' (right associative) */
   {11, 11}, {11, 11},           /* '/' '//' */
   {6, 6}, {4, 4}, {5, 5},   /* '&' '|' '~' */
   {7, 7}, {7, 7},           /* '<<' '>>' */
   {9, 8},                   /* '..' (right associative) */
   {3, 3}, {3, 3}, {3, 3},   /* ==, <, <= */
   {3, 3}, {3, 3}, {3, 3},   /* ~=, >, >= */
   {2, 2}, {1, 1}            /* and, or */
};

#define ___PDR_UNARY_PRIORITY	12  /* priority for unary operators */


/*
** subexpr -> (simpleexp | unop subexpr) { binop subexpr }
** where 'binop' is any binary operator with a priority higher than 'limit'
*/
static ___pdr_BinOpr subexpr (___pdr_LexState *ls, ___pdr_expdesc *v, int limit) {
  ___pdr_BinOpr op;
  ___pdr_UnOpr uop;
  enterlevel(ls);
  uop = getunopr(ls->t.token);
  if (uop != PDR_OPR_NOUNOPR) {
    int line = ls->linenumber;
    ___pdr_luaX_next(ls);
    subexpr(ls, v, ___PDR_UNARY_PRIORITY);
    ___pdr_luaK_prefix(ls->fs, uop, v, line);
  }
  else simpleexp(ls, v);
  /* expand while operators have priorities higher than 'limit' */
  op = getbinopr(ls->t.token);
  while (op != PDR_OPR_NOBINOPR && priority[op].left > limit) {
    ___pdr_expdesc v2;
    ___pdr_BinOpr nextop;
    int line = ls->linenumber;
    ___pdr_luaX_next(ls);
    ___pdr_luaK_infix(ls->fs, op, v);
    /* read sub-expression with higher priority */
    nextop = subexpr(ls, &v2, priority[op].right);
    ___pdr_luaK_posfix(ls->fs, op, v, &v2, line);
    op = nextop;
  }
  ___pdr_leavelevel(ls);
  return op;  /* return first untreated operator */
}


static void expr (___pdr_LexState *ls, ___pdr_expdesc *v) {
  subexpr(ls, v, 0);
}

/* }==================================================================== */



/*
** {======================================================================
** Rules for Statements
** =======================================================================
*/


static void block (___pdr_LexState *ls) {
  /* block -> statlist */
  ___pdr_FuncState *fs = ls->fs;
  ___pdr_BlockCnt bl;
  enterblock(fs, &bl, 0);
  statlist(ls);
  leaveblock(fs);
}


/*
** structure to chain all variables in the left-hand side of an
** assignment
*/
struct LHS_assign {
  struct LHS_assign *prev;
  ___pdr_expdesc v;  /* variable (global, local, upvalue, or indexed) */
};


/*
** check whether, in an assignment to an upvalue/local variable, the
** upvalue/local variable is begin used in a previous assignment to a
** table. If so, save original upvalue/local value in a safe place and
** use this safe copy in the previous assignment.
*/
static void check_conflict (___pdr_LexState *ls, struct LHS_assign *lh, ___pdr_expdesc *v) {
  ___pdr_FuncState *fs = ls->fs;
  int extra = fs->freereg;  /* eventual position to save local variable */
  int conflict = 0;
  for (; lh; lh = lh->prev) {  /* check all previous assignments */
    if (lh->v.k == PDR_VINDEXED) {  /* assigning to a table? */
      /* table is the upvalue/local being assigned now? */
      if (lh->v.u.ind.vt == v->k && lh->v.u.ind.t == v->u.info) {
        conflict = 1;
        lh->v.u.ind.vt = PDR_VLOCAL;
        lh->v.u.ind.t = extra;  /* previous assignment will use safe copy */
      }
      /* index is the local being assigned? (index cannot be upvalue) */
      if (v->k == PDR_VLOCAL && lh->v.u.ind.idx == v->u.info) {
        conflict = 1;
        lh->v.u.ind.idx = extra;  /* previous assignment will use safe copy */
      }
    }
  }
  if (conflict) {
    /* copy upvalue/local value to a temporary (in position 'extra') */
    ___pdr_OpCode op = (v->k == PDR_VLOCAL) ? PDR_OP_MOVE : PDR_OP_GETUPVAL;
    ___pdr_luaK_codeABC(fs, op, extra, v->u.info, 0);
    ___pdr_luaK_reserveregs(fs, 1);
  }
}


static void assignment (___pdr_LexState *ls, struct LHS_assign *lh, int nvars) {
  ___pdr_expdesc e;
  ___pdr_check_condition(ls, ___pdr_vkisvar(lh->v.k), "syntax error");
  if (testnext(ls, ',')) {  /* assignment -> ',' suffixedexp assignment */
    struct LHS_assign nv;
    nv.prev = lh;
    suffixedexp(ls, &nv.v);
    if (nv.v.k != PDR_VINDEXED)
      check_conflict(ls, lh, &nv.v);
    checklimit(ls->fs, nvars + ls->L->nCcalls, ___PDR_LUAI_MAXCCALLS,
                    "C levels");
    assignment(ls, &nv, nvars+1);
  }
  else {  /* assignment -> '=' explist */
    int nexps;
    checknext(ls, '=');
    nexps = explist(ls, &e);
    if (nexps != nvars)
      adjust_assign(ls, nvars, nexps, &e);
    else {
      ___pdr_luaK_setoneret(ls->fs, &e);  /* close last expression */
      ___pdr_luaK_storevar(ls->fs, &lh->v, &e);
      return;  /* avoid default */
    }
  }
  init_exp(&e, PDR_VNONRELOC, ls->fs->freereg-1);  /* default assignment */
  ___pdr_luaK_storevar(ls->fs, &lh->v, &e);
}


static int cond (___pdr_LexState *ls) {
  /* cond -> exp */
  ___pdr_expdesc v;
  expr(ls, &v);  /* read condition */
  if (v.k == PDR_VNIL) v.k = PDR_VFALSE;  /* 'falses' are all equal here */
  ___pdr_luaK_goiftrue(ls->fs, &v);
  return v.f;
}


static void gotostat (___pdr_LexState *ls, int pc) {
  int line = ls->linenumber;
  ___pdr_TString *label;
  int g;
  if (testnext(ls, PDR_TK_GOTO))
    label = str_checkname(ls);
  else {
    ___pdr_luaX_next(ls);  /* skip break */
    label = ___pdr_luaS_new(ls->L, "break");
  }
  g = newlabelentry(ls, &ls->dyd->gt, label, line, pc);
  findlabel(ls, g);  /* close it if label already defined */
}


/* check for repeated labels on the same block */
static void checkrepeated (___pdr_FuncState *fs, ___pdr_Labellist *ll, ___pdr_TString *label) {
  int i;
  for (i = fs->bl->firstlabel; i < ll->n; i++) {
    if (___pdr_eqstr(label, ll->arr[i].name)) {
      const char *msg = ___pdr_luaO_pushfstring(fs->ls->L,
                          "label '%s' already defined on line %d",
                          ___pdr_getstr(label), ll->arr[i].line);
      semerror(fs->ls, msg);
    }
  }
}


/* skip no-op statements */
static void skipnoopstat (___pdr_LexState *ls) {
  while (ls->t.token == ';' || ls->t.token == PDR_TK_DBCOLON)
    statement(ls);
}


static void labelstat (___pdr_LexState *ls, ___pdr_TString *label, int line) {
  /* label -> '::' NAME '::' */
  ___pdr_FuncState *fs = ls->fs;
  ___pdr_Labellist *ll = &ls->dyd->label;
  int l;  /* index of new label being created */
  checkrepeated(fs, ll, label);  /* check for repeated labels */
  checknext(ls, PDR_TK_DBCOLON);  /* skip double colon */
  /* create new entry for this label */
  l = newlabelentry(ls, ll, label, line, ___pdr_luaK_getlabel(fs));
  skipnoopstat(ls);  /* skip other no-op statements */
  if (block_follow(ls, 0)) {  /* label is last no-op statement in the block? */
    /* assume that locals are already out of scope */
    ll->arr[l].nactvar = fs->bl->nactvar;
  }
  findgotos(ls, &ll->arr[l]);
}


static void whilestat (___pdr_LexState *ls, int line) {
  /* whilestat -> WHILE cond DO block END */
  ___pdr_FuncState *fs = ls->fs;
  int whileinit;
  int condexit;
  ___pdr_BlockCnt bl;
  ___pdr_luaX_next(ls);  /* skip WHILE */
  whileinit = ___pdr_luaK_getlabel(fs);
  condexit = cond(ls);
  enterblock(fs, &bl, 1);
  checknext(ls, PDR_TK_DO);
  block(ls);
  ___pdr_luaK_jumpto(fs, whileinit);
  check_match(ls, PDR_TK_END, PDR_TK_WHILE, line);
  leaveblock(fs);
  ___pdr_luaK_patchtohere(fs, condexit);  /* false conditions finish the loop */
}


static void repeatstat (___pdr_LexState *ls, int line) {
  /* repeatstat -> REPEAT block UNTIL cond */
  int condexit;
  ___pdr_FuncState *fs = ls->fs;
  int repeat_init = ___pdr_luaK_getlabel(fs);
  ___pdr_BlockCnt bl1, bl2;
  enterblock(fs, &bl1, 1);  /* loop block */
  enterblock(fs, &bl2, 0);  /* scope block */
  ___pdr_luaX_next(ls);  /* skip REPEAT */
  statlist(ls);
  check_match(ls, PDR_TK_UNTIL, PDR_TK_REPEAT, line);
  condexit = cond(ls);  /* read condition (inside scope block) */
  if (bl2.upval)  /* upvalues? */
    ___pdr_luaK_patchclose(fs, condexit, bl2.nactvar);
  leaveblock(fs);  /* finish scope */
  ___pdr_luaK_patchlist(fs, condexit, repeat_init);  /* close the loop */
  leaveblock(fs);  /* finish loop */
}


static int exp1 (___pdr_LexState *ls) {
  ___pdr_expdesc e;
  int reg;
  expr(ls, &e);
  ___pdr_luaK_exp2nextreg(ls->fs, &e);
  ___pdr_lua_assert(e.k == PDR_VNONRELOC);
  reg = e.u.info;
  return reg;
}


static void forbody (___pdr_LexState *ls, int base, int line, int nvars, int isnum) {
  /* forbody -> DO block */
  ___pdr_BlockCnt bl;
  ___pdr_FuncState *fs = ls->fs;
  int prep, endfor;
  adjustlocalvars(ls, 3);  /* control variables */
  checknext(ls, PDR_TK_DO);
  prep = isnum ? ___pdr_luaK_codeAsBx(fs, PDR_OP_FORPREP, base, ___PDR_NO_JUMP) : ___pdr_luaK_jump(fs);
  enterblock(fs, &bl, 0);  /* scope for declared variables */
  adjustlocalvars(ls, nvars);
  ___pdr_luaK_reserveregs(fs, nvars);
  block(ls);
  leaveblock(fs);  /* end of scope for declared variables */
  ___pdr_luaK_patchtohere(fs, prep);
  if (isnum)  /* numeric for? */
    endfor = ___pdr_luaK_codeAsBx(fs, PDR_OP_FORLOOP, base, ___PDR_NO_JUMP);
  else {  /* generic for */
    ___pdr_luaK_codeABC(fs, PDR_OP_TFORCALL, base, 0, nvars);
    ___pdr_luaK_fixline(fs, line);
    endfor = ___pdr_luaK_codeAsBx(fs, PDR_OP_TFORLOOP, base + 2, ___PDR_NO_JUMP);
  }
  ___pdr_luaK_patchlist(fs, endfor, prep + 1);
  ___pdr_luaK_fixline(fs, line);
}


static void fornum (___pdr_LexState *ls, ___pdr_TString *varname, int line) {
  /* fornum -> NAME = exp1,exp1[,exp1] forbody */
  ___pdr_FuncState *fs = ls->fs;
  int base = fs->freereg;
  ___pdr_new_localvarliteral(ls, "(for index)");
  ___pdr_new_localvarliteral(ls, "(for limit)");
  ___pdr_new_localvarliteral(ls, "(for step)");
  new_localvar(ls, varname);
  checknext(ls, '=');
  exp1(ls);  /* initial value */
  checknext(ls, ',');
  exp1(ls);  /* limit */
  if (testnext(ls, ','))
    exp1(ls);  /* optional step */
  else {  /* default step = 1 */
    ___pdr_luaK_codek(fs, fs->freereg, ___pdr_luaK_intK(fs, 1));
    ___pdr_luaK_reserveregs(fs, 1);
  }
  forbody(ls, base, line, 1, 1);
}


static void forlist (___pdr_LexState *ls, ___pdr_TString *indexname) {
  /* forlist -> NAME {,NAME} IN explist forbody */
  ___pdr_FuncState *fs = ls->fs;
  ___pdr_expdesc e;
  int nvars = 4;  /* gen, state, control, plus at least one declared var */
  int line;
  int base = fs->freereg;
  /* create control variables */
  ___pdr_new_localvarliteral(ls, "(for generator)");
  ___pdr_new_localvarliteral(ls, "(for state)");
  ___pdr_new_localvarliteral(ls, "(for control)");
  /* create declared variables */
  new_localvar(ls, indexname);
  while (testnext(ls, ',')) {
    new_localvar(ls, str_checkname(ls));
    nvars++;
  }
  checknext(ls, PDR_TK_IN);
  line = ls->linenumber;
  adjust_assign(ls, 3, explist(ls, &e), &e);
  ___pdr_luaK_checkstack(fs, 3);  /* extra space to call generator */
  forbody(ls, base, line, nvars - 3, 0);
}


static void forstat (___pdr_LexState *ls, int line) {
  /* forstat -> FOR (fornum | forlist) END */
  ___pdr_FuncState *fs = ls->fs;
  ___pdr_TString *varname;
  ___pdr_BlockCnt bl;
  enterblock(fs, &bl, 1);  /* scope for loop and control variables */
  ___pdr_luaX_next(ls);  /* skip 'for' */
  varname = str_checkname(ls);  /* first variable name */
  switch (ls->t.token) {
    case '=': fornum(ls, varname, line); break;
    case ',': case PDR_TK_IN: forlist(ls, varname); break;
    default: ___pdr_luaX_syntaxerror(ls, "'=' or 'in' expected");
  }
  check_match(ls, PDR_TK_END, PDR_TK_FOR, line);
  leaveblock(fs);  /* loop scope ('break' jumps to this point) */
}


static void test_then_block (___pdr_LexState *ls, int *escapelist) {
  /* test_then_block -> [IF | ELSEIF] cond THEN block */
  ___pdr_BlockCnt bl;
  ___pdr_FuncState *fs = ls->fs;
  ___pdr_expdesc v;
  int jf;  /* instruction to skip 'then' code (if condition is false) */
  ___pdr_luaX_next(ls);  /* skip IF or ELSEIF */
  expr(ls, &v);  /* read condition */
  checknext(ls, PDR_TK_THEN);
  if (ls->t.token == PDR_TK_GOTO || ls->t.token == PDR_TK_BREAK) {
    ___pdr_luaK_goiffalse(ls->fs, &v);  /* will jump to label if condition is true */
    enterblock(fs, &bl, 0);  /* must enter block before 'goto' */
    gotostat(ls, v.t);  /* handle goto/break */
    skipnoopstat(ls);  /* skip other no-op statements */
    if (block_follow(ls, 0)) {  /* 'goto' is the entire block? */
      leaveblock(fs);
      return;  /* and that is it */
    }
    else  /* must skip over 'then' part if condition is false */
      jf = ___pdr_luaK_jump(fs);
  }
  else {  /* regular case (not goto/break) */
    ___pdr_luaK_goiftrue(ls->fs, &v);  /* skip over block if condition is false */
    enterblock(fs, &bl, 0);
    jf = v.f;
  }
  statlist(ls);  /* 'then' part */
  leaveblock(fs);
  if (ls->t.token == PDR_TK_ELSE ||
      ls->t.token == PDR_TK_ELSEIF)  /* followed by 'else'/'elseif'? */
    ___pdr_luaK_concat(fs, escapelist, ___pdr_luaK_jump(fs));  /* must jump over it */
  ___pdr_luaK_patchtohere(fs, jf);
}


static void ifstat (___pdr_LexState *ls, int line) {
  /* ifstat -> IF cond THEN block {ELSEIF cond THEN block} [ELSE block] END */
  ___pdr_FuncState *fs = ls->fs;
  int escapelist = ___PDR_NO_JUMP;  /* exit list for finished parts */
  test_then_block(ls, &escapelist);  /* IF cond THEN block */
  while (ls->t.token == PDR_TK_ELSEIF)
    test_then_block(ls, &escapelist);  /* ELSEIF cond THEN block */
  if (testnext(ls, PDR_TK_ELSE))
    block(ls);  /* 'else' part */
  check_match(ls, PDR_TK_END, PDR_TK_IF, line);
  ___pdr_luaK_patchtohere(fs, escapelist);  /* patch escape list to 'if' end */
}


static void localfunc (___pdr_LexState *ls) {
  ___pdr_expdesc b;
  ___pdr_FuncState *fs = ls->fs;
  new_localvar(ls, str_checkname(ls));  /* new local variable */
  adjustlocalvars(ls, 1);  /* enter its scope */
  body(ls, &b, 0, ls->linenumber);  /* function created in next register */
  /* debug information will only see the variable after this point! */
  getlocvar(fs, b.u.info)->startpc = fs->pc;
}


static void localstat (___pdr_LexState *ls) {
  /* stat -> LOCAL NAME {',' NAME} ['=' explist] */
  int nvars = 0;
  int nexps;
  ___pdr_expdesc e;
  do {
    new_localvar(ls, str_checkname(ls));
    nvars++;
  } while (testnext(ls, ','));
  if (testnext(ls, '='))
    nexps = explist(ls, &e);
  else {
    e.k = PDR_VVOID;
    nexps = 0;
  }
  adjust_assign(ls, nvars, nexps, &e);
  adjustlocalvars(ls, nvars);
}


static int funcname (___pdr_LexState *ls, ___pdr_expdesc *v) {
  /* funcname -> NAME {fieldsel} [':' NAME] */
  int ismethod = 0;
  singlevar(ls, v);
  while (ls->t.token == '.')
    fieldsel(ls, v);
  if (ls->t.token == ':') {
    ismethod = 1;
    fieldsel(ls, v);
  }
  return ismethod;
}


static void funcstat (___pdr_LexState *ls, int line) {
  /* funcstat -> FUNCTION funcname body */
  int ismethod;
  ___pdr_expdesc v, b;
  ___pdr_luaX_next(ls);  /* skip FUNCTION */
  ismethod = funcname(ls, &v);
  body(ls, &b, ismethod, line);
  ___pdr_luaK_storevar(ls->fs, &v, &b);
  ___pdr_luaK_fixline(ls->fs, line);  /* definition "happens" in the first line */
}


static void exprstat (___pdr_LexState *ls) {
  /* stat -> func | assignment */
  ___pdr_FuncState *fs = ls->fs;
  struct LHS_assign v;
  suffixedexp(ls, &v.v);
  if (ls->t.token == '=' || ls->t.token == ',') { /* stat -> assignment ? */
    v.prev = NULL;
    assignment(ls, &v, 1);
  }
  else {  /* stat -> func */
    ___pdr_check_condition(ls, v.v.k == PDR_VCALL, "syntax error");
    ___PDR_SETARG_C(___pdr_getinstruction(fs, &v.v), 1);  /* call statement uses no results */
  }
}


static void retstat (___pdr_LexState *ls) {
  /* stat -> RETURN [explist] [';'] */
  ___pdr_FuncState *fs = ls->fs;
  ___pdr_expdesc e;
  int first, nret;  /* registers with returned values */
  if (block_follow(ls, 1) || ls->t.token == ';')
    first = nret = 0;  /* return no values */
  else {
    nret = explist(ls, &e);  /* optional return values */
    if (___pdr_hasmultret(e.k)) {
      ___pdr_luaK_setmultret(fs, &e);
      if (e.k == PDR_VCALL && nret == 1) {  /* tail call? */
        ___PDR_SET_OPCODE(___pdr_getinstruction(fs,&e), PDR_OP_TAILCALL);
        ___pdr_lua_assert(___PDR_GETARG_A(___pdr_getinstruction(fs,&e)) == fs->nactvar);
      }
      first = fs->nactvar;
      nret = ___PDR_LUA_MULTRET;  /* return all values */
    }
    else {
      if (nret == 1)  /* only one single value? */
        first = ___pdr_luaK_exp2anyreg(fs, &e);
      else {
        ___pdr_luaK_exp2nextreg(fs, &e);  /* values must go to the stack */
        first = fs->nactvar;  /* return all active values */
        ___pdr_lua_assert(nret == fs->freereg - first);
      }
    }
  }
  ___pdr_luaK_ret(fs, first, nret);
  testnext(ls, ';');  /* skip optional semicolon */
}


static void statement (___pdr_LexState *ls) {
  int line = ls->linenumber;  /* may be needed for error messages */
  enterlevel(ls);
  switch (ls->t.token) {
    case ';': {  /* stat -> ';' (empty statement) */
      ___pdr_luaX_next(ls);  /* skip ';' */
      break;
    }
    case PDR_TK_IF: {  /* stat -> ifstat */
      ifstat(ls, line);
      break;
    }
    case PDR_TK_WHILE: {  /* stat -> whilestat */
      whilestat(ls, line);
      break;
    }
    case PDR_TK_DO: {  /* stat -> DO block END */
      ___pdr_luaX_next(ls);  /* skip DO */
      block(ls);
      check_match(ls, PDR_TK_END, PDR_TK_DO, line);
      break;
    }
    case PDR_TK_FOR: {  /* stat -> forstat */
      forstat(ls, line);
      break;
    }
    case PDR_TK_REPEAT: {  /* stat -> repeatstat */
      repeatstat(ls, line);
      break;
    }
    case PDR_TK_FUNCTION: {  /* stat -> funcstat */
      funcstat(ls, line);
      break;
    }
    case PDR_TK_LOCAL: {  /* stat -> localstat */
      ___pdr_luaX_next(ls);  /* skip LOCAL */
      if (testnext(ls, PDR_TK_FUNCTION))  /* local function? */
        localfunc(ls);
      else
        localstat(ls);
      break;
    }
    case PDR_TK_DBCOLON: {  /* stat -> label */
      ___pdr_luaX_next(ls);  /* skip double colon */
      labelstat(ls, str_checkname(ls), line);
      break;
    }
    case PDR_TK_RETURN: {  /* stat -> retstat */
      ___pdr_luaX_next(ls);  /* skip RETURN */
      retstat(ls);
      break;
    }
    case PDR_TK_BREAK:   /* stat -> breakstat */
    case PDR_TK_GOTO: {  /* stat -> 'goto' NAME */
      gotostat(ls, ___pdr_luaK_jump(ls->fs));
      break;
    }
    default: {  /* stat -> func | assignment */
      exprstat(ls);
      break;
    }
  }
  ___pdr_lua_assert(ls->fs->f->maxstacksize >= ls->fs->freereg &&
             ls->fs->freereg >= ls->fs->nactvar);
  ls->fs->freereg = ls->fs->nactvar;  /* free registers */
  ___pdr_leavelevel(ls);
}

/* }====================================================================== */


/*
** compiles the main function, which is a regular vararg function with an
** upvalue named LUA_ENV
*/
static void mainfunc (___pdr_LexState *ls, ___pdr_FuncState *fs) {
  ___pdr_BlockCnt bl;
  ___pdr_expdesc v;
  open_func(ls, fs, &bl);
  fs->f->is_vararg = 1;  /* main function is always declared vararg */
  init_exp(&v, PDR_VLOCAL, 0);  /* create and... */
  newupvalue(fs, ls->envn, &v);  /* ...set environment upvalue */
  ___pdr_luaX_next(ls);  /* read first token */
  statlist(ls);  /* parse main body */
  check_tk(ls, PDR_TK_EOS);
  close_func(ls);
}


___pdr_LClosure *___pdr_luaY_parser (___pdr_lua_State *L, ___pdr_ZIO *z, ___pdr_Mbuffer *buff,
                       ___pdr_Dyndata *dyd, const char *name, int firstchar) {
  ___pdr_LexState lexstate;
  ___pdr_FuncState funcstate;
  ___pdr_LClosure *cl = ___pdr_luaF_newLclosure(L, 1);  /* create main closure */
  ___pdr_setclLvalue(L, L->top, cl);  /* anchor it (to avoid being collected) */
  ___pdr_luaD_inctop(L);
  lexstate.h = ___pdr_luaH_new(L);  /* create table for scanner */
  ___pdr_sethvalue(L, L->top, lexstate.h);  /* anchor it */
  ___pdr_luaD_inctop(L);
  funcstate.f = cl->p = ___pdr_luaF_newproto(L);
  funcstate.f->source = ___pdr_luaS_new(L, name);  /* create and anchor TString */
  ___pdr_lua_assert(___pdr_iswhite(funcstate.f));  /* do not need barrier here */
  lexstate.buff = buff;
  lexstate.dyd = dyd;
  dyd->actvar.n = dyd->gt.n = dyd->label.n = 0;
  ___pdr_luaX_setinput(L, &lexstate, z, funcstate.f->source, firstchar);
  mainfunc(&lexstate, &funcstate);
  ___pdr_lua_assert(!funcstate.prev && funcstate.nups == 1 && !lexstate.fs);
  /* all scopes should be correctly finished */
  ___pdr_lua_assert(dyd->actvar.n == 0 && dyd->gt.n == 0 && dyd->label.n == 0);
  L->top--;  /* remove scanner's table */
  return cl;  /* closure is on the stack, too */
}

} // end NS_PDR_SLUA