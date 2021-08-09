/*
** $Id: ldebug.c,v 2.121 2016/10/19 12:32:10 roberto Exp $
** Debug Interface
** See Copyright Notice in lua.h
*/

#define ___pdr_ldebug_c
#define ___PDR_LUA_CORE

#include "ldebug.h"
#include "lprefix.h"

#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "lua.h"
#include "lapi.h"
#include "lcode.h"
#include "ldo.h"
#include "lfunc.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "ltm.h"
#include "lvm.h"

namespace NS_PDR_SLUA {

#define ___pdr_noLuaClosure(f)		((f) == NULL || (f)->c.tt == ___PDR_LUA_TCCL)


/* Active Lua function (given call info) */
#define ___pdr_ci_func(ci)		(___pdr_clLvalue((ci)->func))


static const char *funcnamefromcode (___pdr_lua_State *L, ___pdr_CallInfo *ci,
                                    const char **name);


static int currentpc (___pdr_CallInfo *ci) {
  ___pdr_lua_assert(___pdr_isLua(ci));
  return ___pdr_pcRel(ci->u.l.savedpc, ___pdr_ci_func(ci)->p);
}


static int currentline (___pdr_CallInfo *ci) {
  return ___pdr_getfuncline(___pdr_ci_func(ci)->p, currentpc(ci));
}


/*
** If function yielded, its 'func' can be in the 'extra' field. The
** next function restores 'func' to its correct value for debugging
** purposes. (It exchanges 'func' and 'extra'; so, when called again,
** after debugging, it also "re-restores" ** 'func' to its altered value.
*/
static void swapextra (___pdr_lua_State *L) {
  if (L->status == ___PDR_LUA_YIELD) {
    ___pdr_CallInfo *ci = L->ci;  /* get function that yielded */
    ___pdr_StkId temp = ci->func;  /* exchange its 'func' and 'extra' values */
    ci->func = ___pdr_restorestack(L, ci->extra);
    ci->extra = ___pdr_savestack(L, temp);
  }
}


/*
** This function can be called asynchronously (e.g. during a signal).
** Fields 'oldpc', 'basehookcount', and 'hookcount' (set by
** 'resethookcount') are for debug only, and it is no problem if they
** get arbitrary values (causes at most one wrong hook call). 'hookmask'
** is an atomic value. We assume that pointers are atomic too (e.g., gcc
** ensures that for all platforms where it runs). Moreover, 'hook' is
** always checked before being called (see 'luaD_hook').
*/
___PDR_LUA_API void ___pdr_lua_sethook (___pdr_lua_State *L, ___pdr_lua_Hook func, int mask, int count) {
  if (func == NULL || mask == 0) {  /* turn off hooks? */
    mask = 0;
    func = NULL;
  }
  if (___pdr_isLua(L->ci))
    L->oldpc = L->ci->u.l.savedpc;
  L->hook = func;
  L->basehookcount = count;
  ___pdr_resethookcount(L);
  L->hookmask = ___pdr_cast_byte(mask);
}


___PDR_LUA_API ___pdr_lua_Hook ___pdr_lua_gethook (___pdr_lua_State *L) {
  return L->hook;
}


___PDR_LUA_API int ___pdr_lua_gethookmask (___pdr_lua_State *L) {
  return L->hookmask;
}


___PDR_LUA_API int ___pdr_lua_gethookcount (___pdr_lua_State *L) {
  return L->basehookcount;
}


___PDR_LUA_API int ___pdr_lua_getstack (___pdr_lua_State *L, int level, ___pdr_lua_Debug *ar) {
  int status;
  ___pdr_CallInfo *ci;
  if (level < 0) return 0;  /* invalid (negative) level */
  ___pdr_lua_lock(L);
  for (ci = L->ci; level > 0 && ci != &L->base_ci; ci = ci->previous)
    level--;
  if (level == 0 && ci != &L->base_ci) {  /* level found? */
    status = 1;
    ar->i_ci = ci;
  }
  else status = 0;  /* no such level */
  ___pdr_lua_unlock(L);
  return status;
}


static const char *upvalname (___pdr_Proto *p, int uv) {
  ___pdr_TString *s = ___pdr_check_exp(uv < p->sizeupvalues, p->upvalues[uv].name);
  if (s == NULL) return "?";
  else return ___pdr_getstr(s);
}


static const char *findvararg (___pdr_CallInfo *ci, int n, ___pdr_StkId *pos) {
  int nparams = ___pdr_clLvalue(ci->func)->p->numparams;
  if (n >= ___pdr_cast_int(ci->u.l.base - ci->func) - nparams)
    return NULL;  /* no such vararg */
  else {
    *pos = ci->func + nparams + n;
    return "(*vararg)";  /* generic name for any vararg */
  }
}


static const char *findlocal (___pdr_lua_State *L, ___pdr_CallInfo *ci, int n,
                              ___pdr_StkId *pos) {
  const char *name = NULL;
  ___pdr_StkId base;
  if (___pdr_isLua(ci)) {
    if (n < 0)  /* access to vararg values? */
      return findvararg(ci, -n, pos);
    else {
      base = ci->u.l.base;
      name = ___pdr_luaF_getlocalname(___pdr_ci_func(ci)->p, n, currentpc(ci));
    }
  }
  else
    base = ci->func + 1;
  if (name == NULL) {  /* no 'standard' name? */
    ___pdr_StkId limit = (ci == L->ci) ? L->top : ci->next->func;
    if (limit - base >= n && n > 0)  /* is 'n' inside 'ci' stack? */
      name = "(*temporary)";  /* generic name for any valid slot */
    else
      return NULL;  /* no name */
  }
  *pos = base + (n - 1);
  return name;
}


___PDR_LUA_API const char *___pdr_lua_getlocal (___pdr_lua_State *L, const ___pdr_lua_Debug *ar, int n) {
  const char *name;
  ___pdr_lua_lock(L);
  swapextra(L);
  if (ar == NULL) {  /* information about non-active function? */
    if (!___pdr_isLfunction(L->top - 1))  /* not a Lua function? */
      name = NULL;
    else  /* consider live variables at function start (parameters) */
      name = ___pdr_luaF_getlocalname(___pdr_clLvalue(L->top - 1)->p, n, 0);
  }
  else {  /* active function; get information through 'ar' */
    ___pdr_StkId pos = NULL;  /* to avoid warnings */
    name = findlocal(L, ar->i_ci, n, &pos);
    if (name) {
      ___pdr_setobj2s(L, L->top, pos);
      ___pdr_api_incr_top(L);
    }
  }
  swapextra(L);
  ___pdr_lua_unlock(L);
  return name;
}


___PDR_LUA_API const char *___pdr_lua_setlocal (___pdr_lua_State *L, const ___pdr_lua_Debug *ar, int n) {
  ___pdr_StkId pos = NULL;  /* to avoid warnings */
  const char *name;
  ___pdr_lua_lock(L);
  swapextra(L);
  name = findlocal(L, ar->i_ci, n, &pos);
  if (name) {
    ___pdr_setobjs2s(L, pos, L->top - 1);
    L->top--;  /* pop value */
  }
  swapextra(L);
  ___pdr_lua_unlock(L);
  return name;
}


static void funcinfo (___pdr_lua_Debug *ar, ___pdr_Closure *cl) {
  if (___pdr_noLuaClosure(cl)) {
    ar->source = "=[C]";
    ar->linedefined = -1;
    ar->lastlinedefined = -1;
    ar->what = "C";
  }
  else {
    ___pdr_Proto *p = cl->l.p;
    ar->source = p->source ? ___pdr_getstr(p->source) : "=?";
    ar->linedefined = p->linedefined;
    ar->lastlinedefined = p->lastlinedefined;
    ar->what = (ar->linedefined == 0) ? "main" : "Lua";
  }
  ___pdr_luaO_chunkid(ar->short_src, ar->source, ___PDR_LUA_IDSIZE);
}


static void collectvalidlines (___pdr_lua_State *L, ___pdr_Closure *f) {
  if (___pdr_noLuaClosure(f)) {
    ___pdr_setnilvalue(L->top);
    ___pdr_api_incr_top(L);
  }
  else {
    int i;
    ___pdr_TValue v;
    int *lineinfo = f->l.p->lineinfo;
    ___pdr_Table *t = ___pdr_luaH_new(L);  /* new table to store active lines */
    ___pdr_sethvalue(L, L->top, t);  /* push it on stack */
    ___pdr_api_incr_top(L);
    ___pdr_setbvalue(&v, 1);  /* boolean 'true' to be the value of all indices */
    for (i = 0; i < f->l.p->sizelineinfo; i++)  /* for all lines with code */
      ___pdr_luaH_setint(L, t, lineinfo[i], &v);  /* table[line] = true */
  }
}


static const char *getfuncname (___pdr_lua_State *L, ___pdr_CallInfo *ci, const char **name) {
  if (ci == NULL)  /* no 'ci'? */
    return NULL;  /* no info */
  else if (ci->callstatus & ___PDR_CIST_FIN) {  /* is this a finalizer? */
    *name = "__gc";
    return "metamethod";  /* report it as such */
  }
  /* calling function is a known Lua function? */
  else if (!(ci->callstatus & ___PDR_CIST_TAIL) && ___pdr_isLua(ci->previous))
    return funcnamefromcode(L, ci->previous, name);
  else return NULL;  /* no way to find a name */
}


static int auxgetinfo (___pdr_lua_State *L, const char *what, ___pdr_lua_Debug *ar,
                       ___pdr_Closure *f, ___pdr_CallInfo *ci) {
  int status = 1;
  for (; *what; what++) {
    switch (*what) {
      case 'S': {
        funcinfo(ar, f);
        break;
      }
      case 'l': {
        ar->currentline = (ci && ___pdr_isLua(ci)) ? currentline(ci) : -1;
        break;
      }
      case 'u': {
        ar->nups = (f == NULL) ? 0 : f->c.nupvalues;
        if (___pdr_noLuaClosure(f)) {
          ar->isvararg = 1;
          ar->nparams = 0;
        }
        else {
          ar->isvararg = f->l.p->is_vararg;
          ar->nparams = f->l.p->numparams;
        }
        break;
      }
      case 't': {
        ar->istailcall = (ci) ? ci->callstatus & ___PDR_CIST_TAIL : 0;
        break;
      }
      case 'n': {
        ar->namewhat = getfuncname(L, ci, &ar->name);
        if (ar->namewhat == NULL) {
          ar->namewhat = "";  /* not found */
          ar->name = NULL;
        }
        break;
      }
      case 'L':
      case 'f':  /* handled by lua_getinfo */
        break;
      default: status = 0;  /* invalid option */
    }
  }
  return status;
}


___PDR_LUA_API int ___pdr_lua_getinfo (___pdr_lua_State *L, const char *what, ___pdr_lua_Debug *ar) {
  int status;
  ___pdr_Closure *cl;
  ___pdr_CallInfo *ci;
  ___pdr_StkId func;
  ___pdr_lua_lock(L);
  swapextra(L);
  if (*what == '>') {
    ci = NULL;
    func = L->top - 1;
    ___pdr_api_check(L, ___pdr_ttisfunction(func), "function expected");
    what++;  /* skip the '>' */
    L->top--;  /* pop function */
  }
  else {
    ci = ar->i_ci;
    func = ci->func;
    ___pdr_lua_assert(___pdr_ttisfunction(ci->func));
  }
  cl = ___pdr_ttisclosure(func) ? ___pdr_clvalue(func) : NULL;
  status = auxgetinfo(L, what, ar, cl, ci);
  if (strchr(what, 'f')) {
    ___pdr_setobjs2s(L, L->top, func);
    ___pdr_api_incr_top(L);
  }
  swapextra(L);  /* correct before option 'L', which can raise a mem. error */
  if (strchr(what, 'L'))
    collectvalidlines(L, cl);
  ___pdr_lua_unlock(L);
  return status;
}


/*
** {======================================================
** Symbolic Execution
** =======================================================
*/

static const char *getobjname (___pdr_Proto *p, int lastpc, int reg,
                               const char **name);


/*
** find a "name" for the RK value 'c'
*/
static void kname (___pdr_Proto *p, int pc, int c, const char **name) {
  if (___PDR_ISK(c)) {  /* is 'c' a constant? */
    ___pdr_TValue *kvalue = &p->k[___PDR_INDEXK(c)];
    if (___pdr_ttisstring(kvalue)) {  /* literal constant? */
      *name = ___pdr_svalue(kvalue);  /* it is its own name */
      return;
    }
    /* else no reasonable name found */
  }
  else {  /* 'c' is a register */
    const char *what = getobjname(p, pc, c, name); /* search for 'c' */
    if (what && *what == 'c') {  /* found a constant name? */
      return;  /* 'name' already filled */
    }
    /* else no reasonable name found */
  }
  *name = "?";  /* no reasonable name found */
}


static int filterpc (int pc, int jmptarget) {
  if (pc < jmptarget)  /* is code conditional (inside a jump)? */
    return -1;  /* cannot know who sets that register */
  else return pc;  /* current position sets that register */
}


/*
** try to find last instruction before 'lastpc' that modified register 'reg'
*/
static int findsetreg (___pdr_Proto *p, int lastpc, int reg) {
  int pc;
  int setreg = -1;  /* keep last instruction that changed 'reg' */
  int jmptarget = 0;  /* any code before this address is conditional */
  for (pc = 0; pc < lastpc; pc++) {
    ___pdr_Instruction i = p->code[pc];
    ___pdr_OpCode op = ___PDR_GET_OPCODE(i);
    int a = ___PDR_GETARG_A(i);
    switch (op) {
      case PDR_OP_LOADNIL: {
        int b = ___PDR_GETARG_B(i);
        if (a <= reg && reg <= a + b)  /* set registers from 'a' to 'a+b' */
          setreg = filterpc(pc, jmptarget);
        break;
      }
      case PDR_OP_TFORCALL: {
        if (reg >= a + 2)  /* affect all regs above its base */
          setreg = filterpc(pc, jmptarget);
        break;
      }
      case PDR_OP_CALL:
      case PDR_OP_TAILCALL: {
        if (reg >= a)  /* affect all registers above base */
          setreg = filterpc(pc, jmptarget);
        break;
      }
      case PDR_OP_JMP: {
        int b = ___PDR_GETARG_sBx(i);
        int dest = pc + 1 + b;
        /* jump is forward and do not skip 'lastpc'? */
        if (pc < dest && dest <= lastpc) {
          if (dest > jmptarget)
            jmptarget = dest;  /* update 'jmptarget' */
        }
        break;
      }
      default:
        if (___pdr_testAMode(op) && reg == a)  /* any instruction that set A */
          setreg = filterpc(pc, jmptarget);
        break;
    }
  }
  return setreg;
}


static const char *getobjname (___pdr_Proto *p, int lastpc, int reg,
                               const char **name) {
  int pc;
  *name = ___pdr_luaF_getlocalname(p, reg + 1, lastpc);
  if (*name)  /* is a local? */
    return "local";
  /* else try symbolic execution */
  pc = findsetreg(p, lastpc, reg);
  if (pc != -1) {  /* could find instruction? */
    ___pdr_Instruction i = p->code[pc];
    ___pdr_OpCode op = ___PDR_GET_OPCODE(i);
    switch (op) {
      case PDR_OP_MOVE: {
        int b = ___PDR_GETARG_B(i);  /* move from 'b' to 'a' */
        if (b < ___PDR_GETARG_A(i))
          return getobjname(p, pc, b, name);  /* get name for 'b' */
        break;
      }
      case PDR_OP_GETTABUP:
      case PDR_OP_GETTABLE: {
        int k = ___PDR_GETARG_C(i);  /* key index */
        int t = ___PDR_GETARG_B(i);  /* table index */
        const char *vn = (op == PDR_OP_GETTABLE)  /* name of indexed variable */
                         ? ___pdr_luaF_getlocalname(p, t + 1, pc)
                         : upvalname(p, t);
        kname(p, pc, k, name);
        return (vn && strcmp(vn, ___PDR_LUA_ENV) == 0) ? "global" : "field";
      }
      case PDR_OP_GETUPVAL: {
        *name = upvalname(p, ___PDR_GETARG_B(i));
        return "upvalue";
      }
      case PDR_OP_LOADK:
      case PDR_OP_LOADKX: {
        int b = (op == PDR_OP_LOADK) ? ___PDR_GETARG_Bx(i)
                                 : ___PDR_GETARG_Ax(p->code[pc + 1]);
        if (___pdr_ttisstring(&p->k[b])) {
          *name = ___pdr_svalue(&p->k[b]);
          return "constant";
        }
        break;
      }
      case PDR_OP_SELF: {
        int k = ___PDR_GETARG_C(i);  /* key index */
        kname(p, pc, k, name);
        return "method";
      }
      default: break;  /* go through to return NULL */
    }
  }
  return NULL;  /* could not find reasonable name */
}


/*
** Try to find a name for a function based on the code that called it.
** (Only works when function was called by a Lua function.)
** Returns what the name is (e.g., "for iterator", "method",
** "metamethod") and sets '*name' to point to the name.
*/
static const char *funcnamefromcode (___pdr_lua_State *L, ___pdr_CallInfo *ci,
                                     const char **name) {
  ___pdr_TMS tm = (___pdr_TMS)0;  /* (initial value avoids warnings) */
  ___pdr_Proto *p = ___pdr_ci_func(ci)->p;  /* calling function */
  int pc = currentpc(ci);  /* calling instruction index */
  ___pdr_Instruction i = p->code[pc];  /* calling instruction */
  if (ci->callstatus & ___PDR_CIST_HOOKED) {  /* was it called inside a hook? */
    *name = "?";
    return "hook";
  }
  switch (___PDR_GET_OPCODE(i)) {
    case PDR_OP_CALL:
    case PDR_OP_TAILCALL:
      return getobjname(p, pc, ___PDR_GETARG_A(i), name);  /* get function name */
    case PDR_OP_TFORCALL: {  /* for iterator */
      *name = "for iterator";
       return "for iterator";
    }
    /* other instructions can do calls through metamethods */
    case PDR_OP_SELF: case PDR_OP_GETTABUP: case PDR_OP_GETTABLE:
      tm = PDR_TM_INDEX;
      break;
    case PDR_OP_SETTABUP: case PDR_OP_SETTABLE:
      tm = PDR_TM_NEWINDEX;
      break;
    case PDR_OP_ADD: case PDR_OP_SUB: case PDR_OP_MUL: case PDR_OP_MOD:
    case PDR_OP_POW: case PDR_OP_DIV: case PDR_OP_IDIV: case PDR_OP_BAND:
    case PDR_OP_BOR: case PDR_OP_BXOR: case PDR_OP_SHL: case PDR_OP_SHR: {
      int offset = ___pdr_cast_int(___PDR_GET_OPCODE(i)) - ___pdr_cast_int(PDR_OP_ADD);  /* ORDER OP */
      tm = ___pdr_cast(___pdr_TMS, offset + ___pdr_cast_int(PDR_TM_ADD));  /* ORDER TM */
      break;
    }
    case PDR_OP_UNM: tm = PDR_TM_UNM; break;
    case PDR_OP_BNOT: tm = PDR_TM_BNOT; break;
    case PDR_OP_LEN: tm = PDR_TM_LEN; break;
    case PDR_OP_CONCAT: tm = PDR_TM_CONCAT; break;
    case PDR_OP_EQ: tm = PDR_TM_EQ; break;
    case PDR_OP_LT: tm = PDR_TM_LT; break;
    case PDR_OP_LE: tm = PDR_TM_LE; break;
    default:
      return NULL;  /* cannot find a reasonable name */
  }
  *name = ___pdr_getstr(___pdr_G(L)->tmname[tm]);
  return "metamethod";
}

/* }====================================================== */



/*
** The subtraction of two potentially unrelated pointers is
** not ISO C, but it should not crash a program; the subsequent
** checks are ISO C and ensure a correct result.
*/
static int isinstack (___pdr_CallInfo *ci, const ___pdr_TValue *o) {
  ptrdiff_t i = o - ci->u.l.base;
  return (0 <= i && i < (ci->top - ci->u.l.base) && ci->u.l.base + i == o);
}


/*
** Checks whether value 'o' came from an upvalue. (That can only happen
** with instructions OP_GETTABUP/OP_SETTABUP, which operate directly on
** upvalues.)
*/
static const char *getupvalname (___pdr_CallInfo *ci, const ___pdr_TValue *o,
                                 const char **name) {
  ___pdr_LClosure *c = ___pdr_ci_func(ci);
  int i;
  for (i = 0; i < c->nupvalues; i++) {
    if (c->upvals[i]->v == o) {
      *name = upvalname(c->p, i);
      return "upvalue";
    }
  }
  return NULL;
}


static const char *varinfo (___pdr_lua_State *L, const ___pdr_TValue *o) {
  const char *name = NULL;  /* to avoid warnings */
  ___pdr_CallInfo *ci = L->ci;
  const char *kind = NULL;
  if (___pdr_isLua(ci)) {
    kind = getupvalname(ci, o, &name);  /* check whether 'o' is an upvalue */
    if (!kind && isinstack(ci, o))  /* no? try a register */
      kind = getobjname(___pdr_ci_func(ci)->p, currentpc(ci),
                        ___pdr_cast_int(o - ci->u.l.base), &name);
  }
  return (kind) ? ___pdr_luaO_pushfstring(L, " (%s '%s')", kind, name) : "";
}


___pdr_l_noret ___pdr_luaG_typeerror (___pdr_lua_State *L, const ___pdr_TValue *o, const char *op) {
  const char *t = ___pdr_luaT_objtypename(L, o);
  ___pdr_luaG_runerror(L, "attempt to %s a %s value%s", op, t, varinfo(L, o));
}


___pdr_l_noret ___pdr_luaG_concaterror (___pdr_lua_State *L, const ___pdr_TValue *p1, const ___pdr_TValue *p2) {
  if (___pdr_ttisstring(p1) || ___pdr_cvt2str(p1)) p1 = p2;
  ___pdr_luaG_typeerror(L, p1, "concatenate");
}


___pdr_l_noret ___pdr_luaG_opinterror (___pdr_lua_State *L, const ___pdr_TValue *p1,
                         const ___pdr_TValue *p2, const char *msg) {
  ___pdr_lua_Number temp;
  if (!___pdr_tonumber(p1, &temp))  /* first operand is wrong? */
    p2 = p1;  /* now second is wrong */
  ___pdr_luaG_typeerror(L, p2, msg);
}


/*
** Error when both values are convertible to numbers, but not to integers
*/
___pdr_l_noret ___pdr_luaG_tointerror (___pdr_lua_State *L, const ___pdr_TValue *p1, const ___pdr_TValue *p2) {
  ___pdr_lua_Integer temp;
  if (!___pdr_tointeger(p1, &temp))
    p2 = p1;
  ___pdr_luaG_runerror(L, "number%s has no integer representation", varinfo(L, p2));
}


___pdr_l_noret ___pdr_luaG_ordererror (___pdr_lua_State *L, const ___pdr_TValue *p1, const ___pdr_TValue *p2) {
  const char *t1 = ___pdr_luaT_objtypename(L, p1);
  const char *t2 = ___pdr_luaT_objtypename(L, p2);
  if (strcmp(t1, t2) == 0)
    ___pdr_luaG_runerror(L, "attempt to compare two %s values", t1);
  else
    ___pdr_luaG_runerror(L, "attempt to compare %s with %s", t1, t2);
}


/* add src:line information to 'msg' */
const char *___pdr_luaG_addinfo (___pdr_lua_State *L, const char *msg, ___pdr_TString *src,
                                        int line) {
  char buff[___PDR_LUA_IDSIZE];
  if (src)
    ___pdr_luaO_chunkid(buff, ___pdr_getstr(src), ___PDR_LUA_IDSIZE);
  else {  /* no source available; use "?" instead */
    buff[0] = '?'; buff[1] = '\0';
  }
  return ___pdr_luaO_pushfstring(L, "%s:%d: %s", buff, line, msg);
}


___pdr_l_noret ___pdr_luaG_errormsg (___pdr_lua_State *L) {
  if (L->errfunc != 0) {  /* is there an error handling function? */
    ___pdr_StkId errfunc = ___pdr_restorestack(L, L->errfunc);
    ___pdr_setobjs2s(L, L->top, L->top - 1);  /* move argument */
    ___pdr_setobjs2s(L, L->top - 1, errfunc);  /* push function */
    L->top++;  /* assume EXTRA_STACK */
    ___pdr_luaD_callnoyield(L, L->top - 2, 1);  /* call it */
  }
  ___pdr_luaD_throw(L, ___PDR_LUA_ERRRUN);
}


___pdr_l_noret ___pdr_luaG_runerror (___pdr_lua_State *L, const char *fmt, ...) {
  ___pdr_CallInfo *ci = L->ci;
  const char *msg;
  va_list argp;
  va_start(argp, fmt);
  msg = ___pdr_luaO_pushvfstring(L, fmt, argp);  /* format message */
  va_end(argp);
  if (___pdr_isLua(ci))  /* if Lua function, add source:line information */
    ___pdr_luaG_addinfo(L, msg, ___pdr_ci_func(ci)->p->source, currentline(ci));
  ___pdr_luaG_errormsg(L);
}


void ___pdr_luaG_traceexec (___pdr_lua_State *L) {
  ___pdr_CallInfo *ci = L->ci;
  ___pdr_lu_byte mask = L->hookmask;
  int counthook = (--L->hookcount == 0 && (mask & ___PDR_LUA_MASKCOUNT));
  if (counthook)
    ___pdr_resethookcount(L);  /* reset count */
  else if (!(mask & ___PDR_LUA_MASKLINE))
    return;  /* no line hook and count != 0; nothing to be done */
  if (ci->callstatus & ___PDR_CIST_HOOKYIELD) {  /* called hook last time? */
    ci->callstatus &= ~___PDR_CIST_HOOKYIELD;  /* erase mark */
    return;  /* do not call hook again (VM yielded, so it did not move) */
  }
  if (counthook)
    ___pdr_luaD_hook(L, ___PDR_LUA_HOOKCOUNT, -1);  /* call count hook */
  if (mask & ___PDR_LUA_MASKLINE) {
    ___pdr_Proto *p = ___pdr_ci_func(ci)->p;
    int npc = ___pdr_pcRel(ci->u.l.savedpc, p);
    int newline = ___pdr_getfuncline(p, npc);
    if (npc == 0 ||  /* call linehook when enter a new function, */
        ci->u.l.savedpc <= L->oldpc ||  /* when jump back (loop), or when */
        newline != ___pdr_getfuncline(p, ___pdr_pcRel(L->oldpc, p)))  /* enter a new line */
      ___pdr_luaD_hook(L, ___PDR_LUA_HOOKLINE, newline);  /* call line hook */
  }
  L->oldpc = ci->u.l.savedpc;
  if (L->status == ___PDR_LUA_YIELD) {  /* did hook yield? */
    if (counthook)
      L->hookcount = 1;  /* undo decrement to zero */
    ci->u.l.savedpc--;  /* undo increment (resume will increment it again) */
    ci->callstatus |= ___PDR_CIST_HOOKYIELD;  /* mark that it yielded */
    ci->func = L->top - 1;  /* protect stack below results */
    ___pdr_luaD_throw(L, ___PDR_LUA_YIELD);
  }
}

} // end NS_PDR_SLUA