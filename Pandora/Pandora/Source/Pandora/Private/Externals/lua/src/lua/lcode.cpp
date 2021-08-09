/*
** $Id: lcode.c,v 2.112 2016/12/22 13:08:50 roberto Exp $
** Code generator for Lua
** See Copyright Notice in lua.h
*/

#define ___pdr_lcode_c
#define ___PDR_LUA_CORE

#include "lcode.h"
#include "lprefix.h"

#include <math.h>
#include <stdlib.h>

#include "lua.h"
#include "ldebug.h"
#include "ldo.h"
#include "lgc.h"
#include "llex.h"
#include "lmem.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lparser.h"
#include "lstring.h"
#include "ltable.h"
#include "lvm.h"

namespace NS_PDR_SLUA {

/* Maximum number of registers in a Lua function (must fit in 8 bits) */
#define ___PDR_MAXREGS		255


#define ___pdr_hasjumps(e)	((e)->t != (e)->f)

/*
** If expression is a numeric constant, fills 'v' with its value
** and returns 1. Otherwise, returns 0.
*/
static int tonumeral(const ___pdr_expdesc *e, ___pdr_TValue *v) {
  if (___pdr_hasjumps(e))
    return 0;  /* not a numeral */
  switch (e->k) {
    case PDR_VKINT:
      if (v) ___pdr_setivalue(v, e->u.ival);
      return 1;
    case PDR_VKFLT:
      if (v) ___pdr_setfltvalue(v, e->u.nval);
      return 1;
    default: return 0;
  }
}


/*
** Create a OP_LOADNIL instruction, but try to optimize: if the previous
** instruction is also OP_LOADNIL and ranges are compatible, adjust
** range of previous instruction instead of emitting a new one. (For
** instance, 'local a; local b' will generate a single opcode.)
*/
void ___pdr_luaK_nil (___pdr_FuncState *fs, int from, int n) {
  ___pdr_Instruction *previous;
  int l = from + n - 1;  /* last register to set nil */
  if (fs->pc > fs->lasttarget) {  /* no jumps to current position? */
    previous = &fs->f->code[fs->pc-1];
    if (___PDR_GET_OPCODE(*previous) == PDR_OP_LOADNIL) {  /* previous is LOADNIL? */
      int pfrom = ___PDR_GETARG_A(*previous);  /* get previous range */
      int pl = pfrom + ___PDR_GETARG_B(*previous);
      if ((pfrom <= from && from <= pl + 1) ||
          (from <= pfrom && pfrom <= l + 1)) {  /* can connect both? */
        if (pfrom < from) from = pfrom;  /* from = min(from, pfrom) */
        if (pl > l) l = pl;  /* l = max(l, pl) */
        ___PDR_SETARG_A(*previous, from);
        ___PDR_SETARG_B(*previous, l - from);
        return;
      }
    }  /* else go through */
  }
  ___pdr_luaK_codeABC(fs, PDR_OP_LOADNIL, from, n - 1, 0);  /* else no optimization */
}


/*
** Gets the destination address of a jump instruction. Used to traverse
** a list of jumps.
*/
static int getjump (___pdr_FuncState *fs, int pc) {
  int offset = ___PDR_GETARG_sBx(fs->f->code[pc]);
  if (offset == ___PDR_NO_JUMP)  /* point to itself represents end of list */
    return ___PDR_NO_JUMP;  /* end of list */
  else
    return (pc+1)+offset;  /* turn offset into absolute position */
}


/*
** Fix jump instruction at position 'pc' to jump to 'dest'.
** (Jump addresses are relative in Lua)
*/
static void fixjump (___pdr_FuncState *fs, int pc, int dest) {
  ___pdr_Instruction *jmp = &fs->f->code[pc];
  int offset = dest - (pc + 1);
  ___pdr_lua_assert(dest != ___PDR_NO_JUMP);
  if (abs(offset) > ___PDR_MAXARG_sBx)
    ___pdr_luaX_syntaxerror(fs->ls, "control structure too long");
  ___PDR_SETARG_sBx(*jmp, offset);
}


/*
** Concatenate jump-list 'l2' into jump-list 'l1'
*/
void ___pdr_luaK_concat (___pdr_FuncState *fs, int *l1, int l2) {
  if (l2 == ___PDR_NO_JUMP) return;  /* nothing to concatenate? */
  else if (*l1 == ___PDR_NO_JUMP)  /* no original list? */
    *l1 = l2;  /* 'l1' points to 'l2' */
  else {
    int list = *l1;
    int next;
    while ((next = getjump(fs, list)) != ___PDR_NO_JUMP)  /* find last element */
      list = next;
    fixjump(fs, list, l2);  /* last element links to 'l2' */
  }
}


/*
** Create a jump instruction and return its position, so its destination
** can be fixed later (with 'fixjump'). If there are jumps to
** this position (kept in 'jpc'), link them all together so that
** 'patchlistaux' will fix all them directly to the final destination.
*/
int ___pdr_luaK_jump (___pdr_FuncState *fs) {
  int jpc = fs->jpc;  /* save list of jumps to here */
  int j;
  fs->jpc = ___PDR_NO_JUMP;  /* no more jumps to here */
  j = ___pdr_luaK_codeAsBx(fs, PDR_OP_JMP, 0, ___PDR_NO_JUMP);
  ___pdr_luaK_concat(fs, &j, jpc);  /* keep them on hold */
  return j;
}


/*
** Code a 'return' instruction
*/
void ___pdr_luaK_ret (___pdr_FuncState *fs, int first, int nret) {
  ___pdr_luaK_codeABC(fs, PDR_OP_RETURN, first, nret+1, 0);
}


/*
** Code a "conditional jump", that is, a test or comparison opcode
** followed by a jump. Return jump position.
*/
static int condjump (___pdr_FuncState *fs, ___pdr_OpCode op, int A, int B, int C) {
  ___pdr_luaK_codeABC(fs, op, A, B, C);
  return ___pdr_luaK_jump(fs);
}


/*
** returns current 'pc' and marks it as a jump target (to avoid wrong
** optimizations with consecutive instructions not in the same basic block).
*/
int ___pdr_luaK_getlabel (___pdr_FuncState *fs) {
  fs->lasttarget = fs->pc;
  return fs->pc;
}


/*
** Returns the position of the instruction "controlling" a given
** jump (that is, its condition), or the jump itself if it is
** unconditional.
*/
static ___pdr_Instruction *getjumpcontrol (___pdr_FuncState *fs, int pc) {
  ___pdr_Instruction *pi = &fs->f->code[pc];
  if (pc >= 1 && ___pdr_testTMode(___PDR_GET_OPCODE(*(pi-1))))
    return pi-1;
  else
    return pi;
}


/*
** Patch destination register for a TESTSET instruction.
** If instruction in position 'node' is not a TESTSET, return 0 ("fails").
** Otherwise, if 'reg' is not 'NO_REG', set it as the destination
** register. Otherwise, change instruction to a simple 'TEST' (produces
** no register value)
*/
static int patchtestreg (___pdr_FuncState *fs, int node, int reg) {
  ___pdr_Instruction *i = getjumpcontrol(fs, node);
  if (___PDR_GET_OPCODE(*i) != PDR_OP_TESTSET)
    return 0;  /* cannot patch other instructions */
  if (reg != ___PDR_NO_REG && reg != ___PDR_GETARG_B(*i))
    ___PDR_SETARG_A(*i, reg);
  else {
     /* no register to put value or register already has the value;
        change instruction to simple test */
    *i = ___PDR_CREATE_ABC(PDR_OP_TEST, ___PDR_GETARG_B(*i), 0, ___PDR_GETARG_C(*i));
  }
  return 1;
}


/*
** Traverse a list of tests ensuring no one produces a value
*/
static void removevalues (___pdr_FuncState *fs, int list) {
  for (; list != ___PDR_NO_JUMP; list = getjump(fs, list))
      patchtestreg(fs, list, ___PDR_NO_REG);
}


/*
** Traverse a list of tests, patching their destination address and
** registers: tests producing values jump to 'vtarget' (and put their
** values in 'reg'), other tests jump to 'dtarget'.
*/
static void patchlistaux (___pdr_FuncState *fs, int list, int vtarget, int reg,
                          int dtarget) {
  while (list != ___PDR_NO_JUMP) {
    int next = getjump(fs, list);
    if (patchtestreg(fs, list, reg))
      fixjump(fs, list, vtarget);
    else
      fixjump(fs, list, dtarget);  /* jump to default target */
    list = next;
  }
}


/*
** Ensure all pending jumps to current position are fixed (jumping
** to current position with no values) and reset list of pending
** jumps
*/
static void dischargejpc (___pdr_FuncState *fs) {
  patchlistaux(fs, fs->jpc, fs->pc, ___PDR_NO_REG, fs->pc);
  fs->jpc = ___PDR_NO_JUMP;
}


/*
** Add elements in 'list' to list of pending jumps to "here"
** (current position)
*/
void ___pdr_luaK_patchtohere (___pdr_FuncState *fs, int list) {
  ___pdr_luaK_getlabel(fs);  /* mark "here" as a jump target */
  ___pdr_luaK_concat(fs, &fs->jpc, list);
}


/*
** Path all jumps in 'list' to jump to 'target'.
** (The assert means that we cannot fix a jump to a forward address
** because we only know addresses once code is generated.)
*/
void ___pdr_luaK_patchlist (___pdr_FuncState *fs, int list, int target) {
  if (target == fs->pc)  /* 'target' is current position? */
    ___pdr_luaK_patchtohere(fs, list);  /* add list to pending jumps */
  else {
    ___pdr_lua_assert(target < fs->pc);
    patchlistaux(fs, list, target, ___PDR_NO_REG, target);
  }
}


/*
** Path all jumps in 'list' to close upvalues up to given 'level'
** (The assertion checks that jumps either were closing nothing
** or were closing higher levels, from inner blocks.)
*/
void ___pdr_luaK_patchclose (___pdr_FuncState *fs, int list, int level) {
  level++;  /* argument is +1 to reserve 0 as non-op */
  for (; list != ___PDR_NO_JUMP; list = getjump(fs, list)) {
    ___pdr_lua_assert(___PDR_GET_OPCODE(fs->f->code[list]) == PDR_OP_JMP &&
                (___PDR_GETARG_A(fs->f->code[list]) == 0 ||
                 ___PDR_GETARG_A(fs->f->code[list]) >= level));
    ___PDR_SETARG_A(fs->f->code[list], level);
  }
}


/*
** Emit instruction 'i', checking for array sizes and saving also its
** line information. Return 'i' position.
*/
static int luaK_code (___pdr_FuncState *fs, ___pdr_Instruction i) {
  ___pdr_Proto *f = fs->f;
  dischargejpc(fs);  /* 'pc' will change */
  /* put new instruction in code array */
  ___pdr_luaM_growvector(fs->ls->L, f->code, fs->pc, f->sizecode, ___pdr_Instruction,
                  ___PDR_MAX_INT, "opcodes");
  f->code[fs->pc] = i;
  /* save corresponding line information */
  ___pdr_luaM_growvector(fs->ls->L, f->lineinfo, fs->pc, f->sizelineinfo, int,
                  ___PDR_MAX_INT, "opcodes");
  f->lineinfo[fs->pc] = fs->ls->lastline;
  return fs->pc++;
}


/*
** Format and emit an 'iABC' instruction. (Assertions check consistency
** of parameters versus opcode.)
*/
int ___pdr_luaK_codeABC (___pdr_FuncState *fs, ___pdr_OpCode o, int a, int b, int c) {
  ___pdr_lua_assert(___pdr_getOpMode(o) == PDR_iABC);
  ___pdr_lua_assert(___pdr_getBMode(o) != PDR_OpArgN || b == 0);
  ___pdr_lua_assert(___pdr_getCMode(o) != PDR_OpArgN || c == 0);
  ___pdr_lua_assert(a <= ___PDR_MAXARG_A && b <= ___PDR_MAXARG_B && c <= ___PDR_MAXARG_C);
  return luaK_code(fs, ___PDR_CREATE_ABC(o, a, b, c));
}


/*
** Format and emit an 'iABx' instruction.
*/
int ___pdr_luaK_codeABx (___pdr_FuncState *fs, ___pdr_OpCode o, int a, unsigned int bc) {
  ___pdr_lua_assert(___pdr_getOpMode(o) == PDR_iABx || ___pdr_getOpMode(o) == PDR_iAsBx);
  ___pdr_lua_assert(___pdr_getCMode(o) == PDR_OpArgN);
  ___pdr_lua_assert(a <= ___PDR_MAXARG_A && bc <= ___PDR_MAXARG_Bx);
  return luaK_code(fs, ___PDR_CREATE_ABx(o, a, bc));
}


/*
** Emit an "extra argument" instruction (format 'iAx')
*/
static int codeextraarg (___pdr_FuncState *fs, int a) {
  ___pdr_lua_assert(a <= ___PDR_MAXARG_Ax);
  return luaK_code(fs, ___PDR_CREATE_Ax(PDR_OP_EXTRAARG, a));
}


/*
** Emit a "load constant" instruction, using either 'OP_LOADK'
** (if constant index 'k' fits in 18 bits) or an 'OP_LOADKX'
** instruction with "extra argument".
*/
int ___pdr_luaK_codek (___pdr_FuncState *fs, int reg, int k) {
  if (k <= ___PDR_MAXARG_Bx)
    return ___pdr_luaK_codeABx(fs, PDR_OP_LOADK, reg, k);
  else {
    int p = ___pdr_luaK_codeABx(fs, PDR_OP_LOADKX, reg, 0);
    codeextraarg(fs, k);
    return p;
  }
}


/*
** Check register-stack level, keeping track of its maximum size
** in field 'maxstacksize'
*/
void ___pdr_luaK_checkstack (___pdr_FuncState *fs, int n) {
  int newstack = fs->freereg + n;
  if (newstack > fs->f->maxstacksize) {
    if (newstack >= ___PDR_MAXREGS)
      ___pdr_luaX_syntaxerror(fs->ls,
        "function or expression needs too many registers");
    fs->f->maxstacksize = ___pdr_cast_byte(newstack);
  }
}


/*
** Reserve 'n' registers in register stack
*/
void ___pdr_luaK_reserveregs (___pdr_FuncState *fs, int n) {
  ___pdr_luaK_checkstack(fs, n);
  fs->freereg += n;
}


/*
** Free register 'reg', if it is neither a constant index nor
** a local variable.
)
*/
static void freereg (___pdr_FuncState *fs, int reg) {
  if (!___PDR_ISK(reg) && reg >= fs->nactvar) {
    fs->freereg--;
    ___pdr_lua_assert(reg == fs->freereg);
  }
}


/*
** Free register used by expression 'e' (if any)
*/
static void freeexp (___pdr_FuncState *fs, ___pdr_expdesc *e) {
  if (e->k == PDR_VNONRELOC)
    freereg(fs, e->u.info);
}


/*
** Free registers used by expressions 'e1' and 'e2' (if any) in proper
** order.
*/
static void freeexps (___pdr_FuncState *fs, ___pdr_expdesc *e1, ___pdr_expdesc *e2) {
  int r1 = (e1->k == PDR_VNONRELOC) ? e1->u.info : -1;
  int r2 = (e2->k == PDR_VNONRELOC) ? e2->u.info : -1;
  if (r1 > r2) {
    freereg(fs, r1);
    freereg(fs, r2);
  }
  else {
    freereg(fs, r2);
    freereg(fs, r1);
  }
}


/*
** Add constant 'v' to prototype's list of constants (field 'k').
** Use scanner's table to cache position of constants in constant list
** and try to reuse constants. Because some values should not be used
** as keys (nil cannot be a key, integer keys can collapse with float
** keys), the caller must provide a useful 'key' for indexing the cache.
*/
static int addk (___pdr_FuncState *fs, ___pdr_TValue *key, ___pdr_TValue *v) {
  ___pdr_lua_State *L = fs->ls->L;
  ___pdr_Proto *f = fs->f;
  ___pdr_TValue *idx = ___pdr_luaH_set(L, fs->ls->h, key);  /* index scanner table */
  int k, oldsize;
  if (___pdr_ttisinteger(idx)) {  /* is there an index there? */
    k = ___pdr_cast_int(___pdr_ivalue(idx));
    /* correct value? (warning: must distinguish floats from integers!) */
    if (k < fs->nk && ___pdr_ttype(&f->k[k]) == ___pdr_ttype(v) &&
                      ___pdr_luaV_rawequalobj(&f->k[k], v))
      return k;  /* reuse index */
  }
  /* constant not found; create a new entry */
  oldsize = f->sizek;
  k = fs->nk;
  /* numerical value does not need GC barrier;
     table has no metatable, so it does not need to invalidate cache */
  ___pdr_setivalue(idx, k);
  ___pdr_luaM_growvector(L, f->k, k, f->sizek, ___pdr_TValue, ___PDR_MAXARG_Ax, "constants");
  while (oldsize < f->sizek) ___pdr_setnilvalue(&f->k[oldsize++]);
  ___pdr_setobj(L, &f->k[k], v);
  fs->nk++;
  ___pdr_luaC_barrier(L, f, v);
  return k;
}


/*
** Add a string to list of constants and return its index.
*/
int ___pdr_luaK_stringK (___pdr_FuncState *fs, ___pdr_TString *s) {
  ___pdr_TValue o;
  ___pdr_setsvalue(fs->ls->L, &o, s);
  return addk(fs, &o, &o);  /* use string itself as key */
}


/*
** Add an integer to list of constants and return its index.
** Integers use userdata as keys to avoid collision with floats with
** same value; conversion to 'void*' is used only for hashing, so there
** are no "precision" problems.
*/
int ___pdr_luaK_intK (___pdr_FuncState *fs, ___pdr_lua_Integer n) {
  ___pdr_TValue k, o;
  ___pdr_setpvalue(&k, ___pdr_cast(void*, ___pdr_cast(size_t, n)));
  ___pdr_setivalue(&o, n);
  return addk(fs, &k, &o);
}

/*
** Add a float to list of constants and return its index.
*/
static int luaK_numberK (___pdr_FuncState *fs, ___pdr_lua_Number r) {
  ___pdr_TValue o;
  ___pdr_setfltvalue(&o, r);
  return addk(fs, &o, &o);  /* use number itself as key */
}


/*
** Add a boolean to list of constants and return its index.
*/
static int boolK (___pdr_FuncState *fs, int b) {
  ___pdr_TValue o;
  ___pdr_setbvalue(&o, b);
  return addk(fs, &o, &o);  /* use boolean itself as key */
}


/*
** Add nil to list of constants and return its index.
*/
static int nilK (___pdr_FuncState *fs) {
  ___pdr_TValue k, v;
  ___pdr_setnilvalue(&v);
  /* cannot use nil as key; instead use table itself to represent nil */
  ___pdr_sethvalue(fs->ls->L, &k, fs->ls->h);
  return addk(fs, &k, &v);
}


/*
** Fix an expression to return the number of results 'nresults'.
** Either 'e' is a multi-ret expression (function call or vararg)
** or 'nresults' is ___PDR_LUA_MULTRET (as any expression can satisfy that).
*/
void ___pdr_luaK_setreturns (___pdr_FuncState *fs, ___pdr_expdesc *e, int nresults) {
  if (e->k == PDR_VCALL) {  /* expression is an open function call? */
    ___PDR_SETARG_C(___pdr_getinstruction(fs, e), nresults + 1);
  }
  else if (e->k == PDR_VVARARG) {
    ___pdr_Instruction *pc = &___pdr_getinstruction(fs, e);
    ___PDR_SETARG_B(*pc, nresults + 1);
    ___PDR_SETARG_A(*pc, fs->freereg);
    ___pdr_luaK_reserveregs(fs, 1);
  }
  else ___pdr_lua_assert(nresults == ___PDR_LUA_MULTRET);
}


/*
** Fix an expression to return one result.
** If expression is not a multi-ret expression (function call or
** vararg), it already returns one result, so nothing needs to be done.
** Function calls become VNONRELOC expressions (as its result comes
** fixed in the base register of the call), while vararg expressions
** become VRELOCABLE (as OP_VARARG puts its results where it wants).
** (Calls are created returning one result, so that does not need
** to be fixed.)
*/
void ___pdr_luaK_setoneret (___pdr_FuncState *fs, ___pdr_expdesc *e) {
  if (e->k == PDR_VCALL) {  /* expression is an open function call? */
    /* already returns 1 value */
    ___pdr_lua_assert(___PDR_GETARG_C(___pdr_getinstruction(fs, e)) == 2);
    e->k = PDR_VNONRELOC;  /* result has fixed position */
    e->u.info = ___PDR_GETARG_A(___pdr_getinstruction(fs, e));
  }
  else if (e->k == PDR_VVARARG) {
    ___PDR_SETARG_B(___pdr_getinstruction(fs, e), 2);
    e->k = PDR_VRELOCABLE;  /* can relocate its simple result */
  }
}


/*
** Ensure that expression 'e' is not a variable.
*/
void ___pdr_luaK_dischargevars (___pdr_FuncState *fs, ___pdr_expdesc *e) {
  switch (e->k) {
    case PDR_VLOCAL: {  /* already in a register */
      e->k = PDR_VNONRELOC;  /* becomes a non-relocatable value */
      break;
    }
    case PDR_VUPVAL: {  /* move value to some (pending) register */
      e->u.info = ___pdr_luaK_codeABC(fs, PDR_OP_GETUPVAL, 0, e->u.info, 0);
      e->k = PDR_VRELOCABLE;
      break;
    }
    case PDR_VINDEXED: {
      ___pdr_OpCode op;
      freereg(fs, e->u.ind.idx);
      if (e->u.ind.vt == PDR_VLOCAL) {  /* is 't' in a register? */
        freereg(fs, e->u.ind.t);
        op = PDR_OP_GETTABLE;
      }
      else {
        ___pdr_lua_assert(e->u.ind.vt == PDR_VUPVAL);
        op = PDR_OP_GETTABUP;  /* 't' is in an upvalue */
      }
      e->u.info = ___pdr_luaK_codeABC(fs, op, 0, e->u.ind.t, e->u.ind.idx);
      e->k = PDR_VRELOCABLE;
      break;
    }
    case PDR_VVARARG: case PDR_VCALL: {
      ___pdr_luaK_setoneret(fs, e);
      break;
    }
    default: break;  /* there is one value available (somewhere) */
  }
}


/*
** Ensures expression value is in register 'reg' (and therefore
** 'e' will become a non-relocatable expression).
*/
static void discharge2reg (___pdr_FuncState *fs, ___pdr_expdesc *e, int reg) {
  ___pdr_luaK_dischargevars(fs, e);
  switch (e->k) {
    case PDR_VNIL: {
      ___pdr_luaK_nil(fs, reg, 1);
      break;
    }
    case PDR_VFALSE: case PDR_VTRUE: {
      ___pdr_luaK_codeABC(fs, PDR_OP_LOADBOOL, reg, e->k == PDR_VTRUE, 0);
      break;
    }
    case PDR_VK: {
      ___pdr_luaK_codek(fs, reg, e->u.info);
      break;
    }
    case PDR_VKFLT: {
      ___pdr_luaK_codek(fs, reg, luaK_numberK(fs, e->u.nval));
      break;
    }
    case PDR_VKINT: {
      ___pdr_luaK_codek(fs, reg, ___pdr_luaK_intK(fs, e->u.ival));
      break;
    }
    case PDR_VRELOCABLE: {
      ___pdr_Instruction *pc = &___pdr_getinstruction(fs, e);
      ___PDR_SETARG_A(*pc, reg);  /* instruction will put result in 'reg' */
      break;
    }
    case PDR_VNONRELOC: {
      if (reg != e->u.info)
        ___pdr_luaK_codeABC(fs, PDR_OP_MOVE, reg, e->u.info, 0);
      break;
    }
    default: {
      ___pdr_lua_assert(e->k == PDR_VJMP);
      return;  /* nothing to do... */
    }
  }
  e->u.info = reg;
  e->k = PDR_VNONRELOC;
}


/*
** Ensures expression value is in any register.
*/
static void discharge2anyreg (___pdr_FuncState *fs, ___pdr_expdesc *e) {
  if (e->k != PDR_VNONRELOC) {  /* no fixed register yet? */
    ___pdr_luaK_reserveregs(fs, 1);  /* get a register */
    discharge2reg(fs, e, fs->freereg-1);  /* put value there */
  }
}


static int code_loadbool (___pdr_FuncState *fs, int A, int b, int jump) {
  ___pdr_luaK_getlabel(fs);  /* those instructions may be jump targets */
  return ___pdr_luaK_codeABC(fs, PDR_OP_LOADBOOL, A, b, jump);
}


/*
** check whether list has any jump that do not produce a value
** or produce an inverted value
*/
static int need_value (___pdr_FuncState *fs, int list) {
  for (; list != ___PDR_NO_JUMP; list = getjump(fs, list)) {
    ___pdr_Instruction i = *getjumpcontrol(fs, list);
    if (___PDR_GET_OPCODE(i) != PDR_OP_TESTSET) return 1;
  }
  return 0;  /* not found */
}


/*
** Ensures final expression result (including results from its jump
** lists) is in register 'reg'.
** If expression has jumps, need to patch these jumps either to
** its final position or to "load" instructions (for those tests
** that do not produce values).
*/
static void exp2reg (___pdr_FuncState *fs, ___pdr_expdesc *e, int reg) {
  discharge2reg(fs, e, reg);
  if (e->k == PDR_VJMP)  /* expression itself is a test? */
    ___pdr_luaK_concat(fs, &e->t, e->u.info);  /* put this jump in 't' list */
  if (___pdr_hasjumps(e)) {
    int final;  /* position after whole expression */
    int p_f = ___PDR_NO_JUMP;  /* position of an eventual LOAD false */
    int p_t = ___PDR_NO_JUMP;  /* position of an eventual LOAD true */
    if (need_value(fs, e->t) || need_value(fs, e->f)) {
      int fj = (e->k == PDR_VJMP) ? ___PDR_NO_JUMP : ___pdr_luaK_jump(fs);
      p_f = code_loadbool(fs, reg, 0, 1);
      p_t = code_loadbool(fs, reg, 1, 0);
      ___pdr_luaK_patchtohere(fs, fj);
    }
    final = ___pdr_luaK_getlabel(fs);
    patchlistaux(fs, e->f, final, reg, p_f);
    patchlistaux(fs, e->t, final, reg, p_t);
  }
  e->f = e->t = ___PDR_NO_JUMP;
  e->u.info = reg;
  e->k = PDR_VNONRELOC;
}


/*
** Ensures final expression result (including results from its jump
** lists) is in next available register.
*/
void ___pdr_luaK_exp2nextreg (___pdr_FuncState *fs, ___pdr_expdesc *e) {
  ___pdr_luaK_dischargevars(fs, e);
  freeexp(fs, e);
  ___pdr_luaK_reserveregs(fs, 1);
  exp2reg(fs, e, fs->freereg - 1);
}


/*
** Ensures final expression result (including results from its jump
** lists) is in some (any) register and return that register.
*/
int ___pdr_luaK_exp2anyreg (___pdr_FuncState *fs, ___pdr_expdesc *e) {
  ___pdr_luaK_dischargevars(fs, e);
  if (e->k == PDR_VNONRELOC) {  /* expression already has a register? */
    if (!___pdr_hasjumps(e))  /* no jumps? */
      return e->u.info;  /* result is already in a register */
    if (e->u.info >= fs->nactvar) {  /* reg. is not a local? */
      exp2reg(fs, e, e->u.info);  /* put final result in it */
      return e->u.info;
    }
  }
  ___pdr_luaK_exp2nextreg(fs, e);  /* otherwise, use next available register */
  return e->u.info;
}


/*
** Ensures final expression result is either in a register or in an
** upvalue.
*/
void ___pdr_luaK_exp2anyregup (___pdr_FuncState *fs, ___pdr_expdesc *e) {
  if (e->k != PDR_VUPVAL || ___pdr_hasjumps(e))
    ___pdr_luaK_exp2anyreg(fs, e);
}


/*
** Ensures final expression result is either in a register or it is
** a constant.
*/
void ___pdr_luaK_exp2val (___pdr_FuncState *fs, ___pdr_expdesc *e) {
  if (___pdr_hasjumps(e))
    ___pdr_luaK_exp2anyreg(fs, e);
  else
    ___pdr_luaK_dischargevars(fs, e);
}


/*
** Ensures final expression result is in a valid R/K index
** (that is, it is either in a register or in 'k' with an index
** in the range of R/K indices).
** Returns R/K index.
*/
int ___pdr_luaK_exp2RK (___pdr_FuncState *fs, ___pdr_expdesc *e) {
  ___pdr_luaK_exp2val(fs, e);
  switch (e->k) {  /* move constants to 'k' */
    case PDR_VTRUE: e->u.info = boolK(fs, 1); goto vk;
    case PDR_VFALSE: e->u.info = boolK(fs, 0); goto vk;
    case PDR_VNIL: e->u.info = nilK(fs); goto vk;
    case PDR_VKINT: e->u.info = ___pdr_luaK_intK(fs, e->u.ival); goto vk;
    case PDR_VKFLT: e->u.info = luaK_numberK(fs, e->u.nval); goto vk;
    case PDR_VK:
     vk:
      e->k = PDR_VK;
      if (e->u.info <= ___PDR_MAXINDEXRK)  /* constant fits in 'argC'? */
        return ___PDR_RKASK(e->u.info);
      else break;
    default: break;
  }
  /* not a constant in the right range: put it in a register */
  return ___pdr_luaK_exp2anyreg(fs, e);
}


/*
** Generate code to store result of expression 'ex' into variable 'var'.
*/
void ___pdr_luaK_storevar (___pdr_FuncState *fs, ___pdr_expdesc *var, ___pdr_expdesc *ex) {
  switch (var->k) {
    case PDR_VLOCAL: {
      freeexp(fs, ex);
      exp2reg(fs, ex, var->u.info);  /* compute 'ex' into proper place */
      return;
    }
    case PDR_VUPVAL: {
      int e = ___pdr_luaK_exp2anyreg(fs, ex);
      ___pdr_luaK_codeABC(fs, PDR_OP_SETUPVAL, e, var->u.info, 0);
      break;
    }
    case PDR_VINDEXED: {
      ___pdr_OpCode op = (var->u.ind.vt == PDR_VLOCAL) ? PDR_OP_SETTABLE : PDR_OP_SETTABUP;
      int e = ___pdr_luaK_exp2RK(fs, ex);
      ___pdr_luaK_codeABC(fs, op, var->u.ind.t, var->u.ind.idx, e);
      break;
    }
    default: ___pdr_lua_assert(0);  /* invalid var kind to store */
  }
  freeexp(fs, ex);
}


/*
** Emit SELF instruction (convert expression 'e' into 'e:key(e,').
*/
void ___pdr_luaK_self (___pdr_FuncState *fs, ___pdr_expdesc *e, ___pdr_expdesc *key) {
  int ereg;
  ___pdr_luaK_exp2anyreg(fs, e);
  ereg = e->u.info;  /* register where 'e' was placed */
  freeexp(fs, e);
  e->u.info = fs->freereg;  /* base register for op_self */
  e->k = PDR_VNONRELOC;  /* self expression has a fixed register */
  ___pdr_luaK_reserveregs(fs, 2);  /* function and 'self' produced by op_self */
  ___pdr_luaK_codeABC(fs, PDR_OP_SELF, e->u.info, ereg, ___pdr_luaK_exp2RK(fs, key));
  freeexp(fs, key);
}


/*
** Negate condition 'e' (where 'e' is a comparison).
*/
static void negatecondition (___pdr_FuncState *fs, ___pdr_expdesc *e) {
  ___pdr_Instruction *pc = getjumpcontrol(fs, e->u.info);
  ___pdr_lua_assert(___pdr_testTMode(___PDR_GET_OPCODE(*pc)) && ___PDR_GET_OPCODE(*pc) != PDR_OP_TESTSET &&
                                           ___PDR_GET_OPCODE(*pc) != PDR_OP_TEST);
  ___PDR_SETARG_A(*pc, !(___PDR_GETARG_A(*pc)));
}


/*
** Emit instruction to jump if 'e' is 'cond' (that is, if 'cond'
** is true, code will jump if 'e' is true.) Return jump position.
** Optimize when 'e' is 'not' something, inverting the condition
** and removing the 'not'.
*/
static int jumponcond (___pdr_FuncState *fs, ___pdr_expdesc *e, int cond) {
  if (e->k == PDR_VRELOCABLE) {
    ___pdr_Instruction ie = ___pdr_getinstruction(fs, e);
    if (___PDR_GET_OPCODE(ie) == PDR_OP_NOT) {
      fs->pc--;  /* remove previous OP_NOT */
      return condjump(fs, PDR_OP_TEST, ___PDR_GETARG_B(ie), 0, !cond);
    }
    /* else go through */
  }
  discharge2anyreg(fs, e);
  freeexp(fs, e);
  return condjump(fs, PDR_OP_TESTSET, ___PDR_NO_REG, e->u.info, cond);
}


/*
** Emit code to go through if 'e' is true, jump otherwise.
*/
void ___pdr_luaK_goiftrue (___pdr_FuncState *fs, ___pdr_expdesc *e) {
  int pc;  /* pc of new jump */
  ___pdr_luaK_dischargevars(fs, e);
  switch (e->k) {
    case PDR_VJMP: {  /* condition? */
      negatecondition(fs, e);  /* jump when it is false */
      pc = e->u.info;  /* save jump position */
      break;
    }
    case PDR_VK: case PDR_VKFLT: case PDR_VKINT: case PDR_VTRUE: {
      pc = ___PDR_NO_JUMP;  /* always true; do nothing */
      break;
    }
    default: {
      pc = jumponcond(fs, e, 0);  /* jump when false */
      break;
    }
  }
  ___pdr_luaK_concat(fs, &e->f, pc);  /* insert new jump in false list */
  ___pdr_luaK_patchtohere(fs, e->t);  /* true list jumps to here (to go through) */
  e->t = ___PDR_NO_JUMP;
}


/*
** Emit code to go through if 'e' is false, jump otherwise.
*/
void ___pdr_luaK_goiffalse (___pdr_FuncState *fs, ___pdr_expdesc *e) {
  int pc;  /* pc of new jump */
  ___pdr_luaK_dischargevars(fs, e);
  switch (e->k) {
    case PDR_VJMP: {
      pc = e->u.info;  /* already jump if true */
      break;
    }
    case PDR_VNIL: case PDR_VFALSE: {
      pc = ___PDR_NO_JUMP;  /* always false; do nothing */
      break;
    }
    default: {
      pc = jumponcond(fs, e, 1);  /* jump if true */
      break;
    }
  }
  ___pdr_luaK_concat(fs, &e->t, pc);  /* insert new jump in 't' list */
  ___pdr_luaK_patchtohere(fs, e->f);  /* false list jumps to here (to go through) */
  e->f = ___PDR_NO_JUMP;
}


/*
** Code 'not e', doing constant folding.
*/
static void codenot (___pdr_FuncState *fs, ___pdr_expdesc *e) {
  ___pdr_luaK_dischargevars(fs, e);
  switch (e->k) {
    case PDR_VNIL: case PDR_VFALSE: {
      e->k = PDR_VTRUE;  /* true == not nil == not false */
      break;
    }
    case PDR_VK: case PDR_VKFLT: case PDR_VKINT: case PDR_VTRUE: {
      e->k = PDR_VFALSE;  /* false == not "x" == not 0.5 == not 1 == not true */
      break;
    }
    case PDR_VJMP: {
      negatecondition(fs, e);
      break;
    }
    case PDR_VRELOCABLE:
    case PDR_VNONRELOC: {
      discharge2anyreg(fs, e);
      freeexp(fs, e);
      e->u.info = ___pdr_luaK_codeABC(fs, PDR_OP_NOT, 0, e->u.info, 0);
      e->k = PDR_VRELOCABLE;
      break;
    }
    default: ___pdr_lua_assert(0);  /* cannot happen */
  }
  /* interchange true and false lists */
  { int temp = e->f; e->f = e->t; e->t = temp; }
  removevalues(fs, e->f);  /* values are useless when negated */
  removevalues(fs, e->t);
}


/*
** Create expression 't[k]'. 't' must have its final result already in a
** register or upvalue.
*/
void ___pdr_luaK_indexed (___pdr_FuncState *fs, ___pdr_expdesc *t, ___pdr_expdesc *k) {
  ___pdr_lua_assert(!___pdr_hasjumps(t) && (___pdr_vkisinreg(t->k) || t->k == PDR_VUPVAL));
  t->u.ind.t = t->u.info;  /* register or upvalue index */
  t->u.ind.idx = ___pdr_luaK_exp2RK(fs, k);  /* R/K index for key */
  t->u.ind.vt = (t->k == PDR_VUPVAL) ? PDR_VUPVAL : PDR_VLOCAL;
  t->k = PDR_VINDEXED;
}


/*
** Return false if folding can raise an error.
** Bitwise operations need operands convertible to integers; division
** operations cannot have 0 as divisor.
*/
static int validop (int op, ___pdr_TValue *v1, ___pdr_TValue *v2) {
  switch (op) {
    case ___PDR_LUA_OPBAND: case ___PDR_LUA_OPBOR: case ___PDR_LUA_OPBXOR:
    case ___PDR_LUA_OPSHL: case ___PDR_LUA_OPSHR: case ___PDR_LUA_OPBNOT: {  /* conversion errors */
      ___pdr_lua_Integer i;
      return (___pdr_tointeger(v1, &i) && ___pdr_tointeger(v2, &i));
    }
    case ___PDR_LUA_OPDIV: case ___PDR_LUA_OPIDIV: case ___PDR_LUA_OPMOD:  /* division by 0 */
      return (___pdr_nvalue(v2) != 0);
    default: return 1;  /* everything else is valid */
  }
}


/*
** Try to "constant-fold" an operation; return 1 iff successful.
** (In this case, 'e1' has the final result.)
*/
static int constfolding (___pdr_FuncState *fs, int op, ___pdr_expdesc *e1,
                                                const ___pdr_expdesc *e2) {
  ___pdr_TValue v1, v2, res;
  if (!tonumeral(e1, &v1) || !tonumeral(e2, &v2) || !validop(op, &v1, &v2))
    return 0;  /* non-numeric operands or not safe to fold */
  ___pdr_luaO_arith(fs->ls->L, op, &v1, &v2, &res);  /* does operation */
  if (___pdr_ttisinteger(&res)) {
    e1->k = PDR_VKINT;
    e1->u.ival = ___pdr_ivalue(&res);
  }
  else {  /* folds neither NaN nor 0.0 (to avoid problems with -0.0) */
    ___pdr_lua_Number n = ___pdr_fltvalue(&res);
    if (___pdr_luai_numisnan(n) || n == 0)
      return 0;
    e1->k = PDR_VKFLT;
    e1->u.nval = n;
  }
  return 1;
}


/*
** Emit code for unary expressions that "produce values"
** (everything but 'not').
** Expression to produce final result will be encoded in 'e'.
*/
static void codeunexpval (___pdr_FuncState *fs, ___pdr_OpCode op, ___pdr_expdesc *e, int line) {
  int r = ___pdr_luaK_exp2anyreg(fs, e);  /* opcodes operate only on registers */
  freeexp(fs, e);
  e->u.info = ___pdr_luaK_codeABC(fs, op, 0, r, 0);  /* generate opcode */
  e->k = PDR_VRELOCABLE;  /* all those operations are relocatable */
  ___pdr_luaK_fixline(fs, line);
}


/*
** Emit code for binary expressions that "produce values"
** (everything but logical operators 'and'/'or' and comparison
** operators).
** Expression to produce final result will be encoded in 'e1'.
** Because 'luaK_exp2RK' can free registers, its calls must be
** in "stack order" (that is, first on 'e2', which may have more
** recent registers to be released).
*/
static void codebinexpval (___pdr_FuncState *fs, ___pdr_OpCode op,
                           ___pdr_expdesc *e1, ___pdr_expdesc *e2, int line) {
  int rk2 = ___pdr_luaK_exp2RK(fs, e2);  /* both operands are "RK" */
  int rk1 = ___pdr_luaK_exp2RK(fs, e1);
  freeexps(fs, e1, e2);
  e1->u.info = ___pdr_luaK_codeABC(fs, op, 0, rk1, rk2);  /* generate opcode */
  e1->k = PDR_VRELOCABLE;  /* all those operations are relocatable */
  ___pdr_luaK_fixline(fs, line);
}


/*
** Emit code for comparisons.
** 'e1' was already put in R/K form by 'luaK_infix'.
*/
static void codecomp (___pdr_FuncState *fs, ___pdr_BinOpr opr, ___pdr_expdesc *e1, ___pdr_expdesc *e2) {
  int rk1 = (e1->k == PDR_VK) ? ___PDR_RKASK(e1->u.info)
                          : ___pdr_check_exp(e1->k == PDR_VNONRELOC, e1->u.info);
  int rk2 = ___pdr_luaK_exp2RK(fs, e2);
  freeexps(fs, e1, e2);
  switch (opr) {
    case PDR_OPR_NE: {  /* '(a ~= b)' ==> 'not (a == b)' */
      e1->u.info = condjump(fs, PDR_OP_EQ, 0, rk1, rk2);
      break;
    }
    case PDR_OPR_GT: case PDR_OPR_GE: {
      /* '(a > b)' ==> '(b < a)';  '(a >= b)' ==> '(b <= a)' */
      ___pdr_OpCode op = ___pdr_cast(___pdr_OpCode, (opr - PDR_OPR_NE) + PDR_OP_EQ);
      e1->u.info = condjump(fs, op, 1, rk2, rk1);  /* invert operands */
      break;
    }
    default: {  /* '==', '<', '<=' use their own opcodes */
      ___pdr_OpCode op = ___pdr_cast(___pdr_OpCode, (opr - PDR_OPR_EQ) + PDR_OP_EQ);
      e1->u.info = condjump(fs, op, 1, rk1, rk2);
      break;
    }
  }
  e1->k = PDR_VJMP;
}


/*
** Aplly prefix operation 'op' to expression 'e'.
*/
void ___pdr_luaK_prefix (___pdr_FuncState *fs, ___pdr_UnOpr op, ___pdr_expdesc *e, int line) {
  static const ___pdr_expdesc ef = {PDR_VKINT, {0}, ___PDR_NO_JUMP, ___PDR_NO_JUMP};
  switch (op) {
    case PDR_OPR_MINUS: case PDR_OPR_BNOT:  /* use 'ef' as fake 2nd operand */
      if (constfolding(fs, op + ___PDR_LUA_OPUNM, e, &ef))
        break;
      /* FALLTHROUGH */
    case PDR_OPR_LEN:
      codeunexpval(fs, ___pdr_cast(___pdr_OpCode, op + PDR_OP_UNM), e, line);
      break;
    case PDR_OPR_NOT: codenot(fs, e); break;
    default: ___pdr_lua_assert(0);
  }
}


/*
** Process 1st operand 'v' of binary operation 'op' before reading
** 2nd operand.
*/
void ___pdr_luaK_infix (___pdr_FuncState *fs, ___pdr_BinOpr op, ___pdr_expdesc *v) {
  switch (op) {
    case PDR_OPR_AND: {
      ___pdr_luaK_goiftrue(fs, v);  /* go ahead only if 'v' is true */
      break;
    }
    case PDR_OPR_OR: {
      ___pdr_luaK_goiffalse(fs, v);  /* go ahead only if 'v' is false */
      break;
    }
    case PDR_OPR_CONCAT: {
      ___pdr_luaK_exp2nextreg(fs, v);  /* operand must be on the 'stack' */
      break;
    }
    case PDR_OPR_ADD: case PDR_OPR_SUB:
    case PDR_OPR_MUL: case PDR_OPR_DIV: case PDR_OPR_IDIV:
    case PDR_OPR_MOD: case PDR_OPR_POW:
    case PDR_OPR_BAND: case PDR_OPR_BOR: case PDR_OPR_BXOR:
    case PDR_OPR_SHL: case PDR_OPR_SHR: {
      if (!tonumeral(v, NULL))
        ___pdr_luaK_exp2RK(fs, v);
      /* else keep numeral, which may be folded with 2nd operand */
      break;
    }
    default: {
      ___pdr_luaK_exp2RK(fs, v);
      break;
    }
  }
}


/*
** Finalize code for binary operation, after reading 2nd operand.
** For '(a .. b .. c)' (which is '(a .. (b .. c))', because
** concatenation is right associative), merge second CONCAT into first
** one.
*/
void ___pdr_luaK_posfix (___pdr_FuncState *fs, ___pdr_BinOpr op,
                  ___pdr_expdesc *e1, ___pdr_expdesc *e2, int line) {
  switch (op) {
    case PDR_OPR_AND: {
      ___pdr_lua_assert(e1->t == ___PDR_NO_JUMP);  /* list closed by 'luK_infix' */
      ___pdr_luaK_dischargevars(fs, e2);
      ___pdr_luaK_concat(fs, &e2->f, e1->f);
      *e1 = *e2;
      break;
    }
    case PDR_OPR_OR: {
      ___pdr_lua_assert(e1->f == ___PDR_NO_JUMP);  /* list closed by 'luK_infix' */
      ___pdr_luaK_dischargevars(fs, e2);
      ___pdr_luaK_concat(fs, &e2->t, e1->t);
      *e1 = *e2;
      break;
    }
    case PDR_OPR_CONCAT: {
      ___pdr_luaK_exp2val(fs, e2);
      if (e2->k == PDR_VRELOCABLE &&
          ___PDR_GET_OPCODE(___pdr_getinstruction(fs, e2)) == PDR_OP_CONCAT) {
        ___pdr_lua_assert(e1->u.info == ___PDR_GETARG_B(___pdr_getinstruction(fs, e2))-1);
        freeexp(fs, e1);
        ___PDR_SETARG_B(___pdr_getinstruction(fs, e2), e1->u.info);
        e1->k = PDR_VRELOCABLE; e1->u.info = e2->u.info;
      }
      else {
        ___pdr_luaK_exp2nextreg(fs, e2);  /* operand must be on the 'stack' */
        codebinexpval(fs, PDR_OP_CONCAT, e1, e2, line);
      }
      break;
    }
    case PDR_OPR_ADD: case PDR_OPR_SUB: case PDR_OPR_MUL: case PDR_OPR_DIV:
    case PDR_OPR_IDIV: case PDR_OPR_MOD: case PDR_OPR_POW:
    case PDR_OPR_BAND: case PDR_OPR_BOR: case PDR_OPR_BXOR:
    case PDR_OPR_SHL: case PDR_OPR_SHR: {
      if (!constfolding(fs, op + ___PDR_LUA_OPADD, e1, e2))
        codebinexpval(fs, ___pdr_cast(___pdr_OpCode, op + PDR_OP_ADD), e1, e2, line);
      break;
    }
    case PDR_OPR_EQ: case PDR_OPR_LT: case PDR_OPR_LE:
    case PDR_OPR_NE: case PDR_OPR_GT: case PDR_OPR_GE: {
      codecomp(fs, op, e1, e2);
      break;
    }
    default: ___pdr_lua_assert(0);
  }
}


/*
** Change line information associated with current position.
*/
void ___pdr_luaK_fixline (___pdr_FuncState *fs, int line) {
  fs->f->lineinfo[fs->pc - 1] = line;
}


/*
** Emit a SETLIST instruction.
** 'base' is register that keeps table;
** 'nelems' is #table plus those to be stored now;
** 'tostore' is number of values (in registers 'base + 1',...) to add to
** table (or ___PDR_LUA_MULTRET to add up to stack top).
*/
void ___pdr_luaK_setlist (___pdr_FuncState *fs, int base, int nelems, int tostore) {
  int c =  (nelems - 1)/___PDR_LFIELDS_PER_FLUSH + 1;
  int b = (tostore == ___PDR_LUA_MULTRET) ? 0 : tostore;
  ___pdr_lua_assert(tostore != 0 && tostore <= ___PDR_LFIELDS_PER_FLUSH);
  if (c <= ___PDR_MAXARG_C)
    ___pdr_luaK_codeABC(fs, PDR_OP_SETLIST, base, b, c);
  else if (c <= ___PDR_MAXARG_Ax) {
    ___pdr_luaK_codeABC(fs, PDR_OP_SETLIST, base, b, 0);
    codeextraarg(fs, c);
  }
  else
    ___pdr_luaX_syntaxerror(fs->ls, "constructor too long");
  fs->freereg = base + 1;  /* free registers with list values */
}

} // end NS_PDR_SLUA
