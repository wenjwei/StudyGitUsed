/*
** $Id: lcode.h,v 1.64 2016/01/05 16:22:37 roberto Exp $
** Code generator for Lua
** See Copyright Notice in lua.h
*/

#ifndef ___pdr_lcode_h___
#define ___pdr_lcode_h___

#include "llex.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lparser.h"

/*
** Marks the end of a patch list. It is an invalid value both as an absolute
** address, and as a list link (would link an element to itself).
*/
#define ___PDR_NO_JUMP (-1)

namespace NS_PDR_SLUA {

/*
** grep "ORDER OPR" if you change these enums  (ORDER OP)
*/
typedef enum ___pdr_BinOpr {
  PDR_OPR_ADD, PDR_OPR_SUB, PDR_OPR_MUL, PDR_OPR_MOD, PDR_OPR_POW,
  PDR_OPR_DIV,
  PDR_OPR_IDIV,
  PDR_OPR_BAND, PDR_OPR_BOR, PDR_OPR_BXOR,
  PDR_OPR_SHL, PDR_OPR_SHR,
  PDR_OPR_CONCAT,
  PDR_OPR_EQ, PDR_OPR_LT, PDR_OPR_LE,
  PDR_OPR_NE, PDR_OPR_GT, PDR_OPR_GE,
  PDR_OPR_AND, PDR_OPR_OR,
  PDR_OPR_NOBINOPR
} ___pdr_BinOpr;


typedef enum ___pdr_UnOpr { PDR_OPR_MINUS, PDR_OPR_BNOT, PDR_OPR_NOT, PDR_OPR_LEN, PDR_OPR_NOUNOPR } ___pdr_UnOpr;


/* get (pointer to) instruction of given 'expdesc' */
#define ___pdr_getinstruction(fs,e)	((fs)->f->code[(e)->u.info])

#define ___pdr_luaK_codeAsBx(fs,o,A,sBx)	___pdr_luaK_codeABx(fs,o,A,(sBx)+___PDR_MAXARG_sBx)

#define ___pdr_luaK_setmultret(fs,e)	___pdr_luaK_setreturns(fs, e, ___PDR_LUA_MULTRET)

#define ___pdr_luaK_jumpto(fs,t)	___pdr_luaK_patchlist(fs, ___pdr_luaK_jump(fs), t)

___PDR_LUAI_FUNC int ___pdr_luaK_codeABx (___pdr_FuncState *fs, ___pdr_OpCode o, int A, unsigned int Bx);
___PDR_LUAI_FUNC int ___pdr_luaK_codeABC (___pdr_FuncState *fs, ___pdr_OpCode o, int A, int B, int C);
___PDR_LUAI_FUNC int ___pdr_luaK_codek (___pdr_FuncState *fs, int reg, int k);
___PDR_LUAI_FUNC void ___pdr_luaK_fixline (___pdr_FuncState *fs, int line);
___PDR_LUAI_FUNC void ___pdr_luaK_nil (___pdr_FuncState *fs, int from, int n);
___PDR_LUAI_FUNC void ___pdr_luaK_reserveregs (___pdr_FuncState *fs, int n);
___PDR_LUAI_FUNC void ___pdr_luaK_checkstack (___pdr_FuncState *fs, int n);
___PDR_LUAI_FUNC int ___pdr_luaK_stringK (___pdr_FuncState *fs, ___pdr_TString *s);
___PDR_LUAI_FUNC int ___pdr_luaK_intK (___pdr_FuncState *fs, ___pdr_lua_Integer n);
___PDR_LUAI_FUNC void ___pdr_luaK_dischargevars (___pdr_FuncState *fs, ___pdr_expdesc *e);
___PDR_LUAI_FUNC int ___pdr_luaK_exp2anyreg (___pdr_FuncState *fs, ___pdr_expdesc *e);
___PDR_LUAI_FUNC void ___pdr_luaK_exp2anyregup (___pdr_FuncState *fs, ___pdr_expdesc *e);
___PDR_LUAI_FUNC void ___pdr_luaK_exp2nextreg (___pdr_FuncState *fs, ___pdr_expdesc *e);
___PDR_LUAI_FUNC void ___pdr_luaK_exp2val (___pdr_FuncState *fs, ___pdr_expdesc *e);
___PDR_LUAI_FUNC int ___pdr_luaK_exp2RK (___pdr_FuncState *fs, ___pdr_expdesc *e);
___PDR_LUAI_FUNC void ___pdr_luaK_self (___pdr_FuncState *fs, ___pdr_expdesc *e, ___pdr_expdesc *key);
___PDR_LUAI_FUNC void ___pdr_luaK_indexed (___pdr_FuncState *fs, ___pdr_expdesc *t, ___pdr_expdesc *k);
___PDR_LUAI_FUNC void ___pdr_luaK_goiftrue (___pdr_FuncState *fs, ___pdr_expdesc *e);
___PDR_LUAI_FUNC void ___pdr_luaK_goiffalse (___pdr_FuncState *fs, ___pdr_expdesc *e);
___PDR_LUAI_FUNC void ___pdr_luaK_storevar (___pdr_FuncState *fs, ___pdr_expdesc *var, ___pdr_expdesc *e);
___PDR_LUAI_FUNC void ___pdr_luaK_setreturns (___pdr_FuncState *fs, ___pdr_expdesc *e, int nresults);
___PDR_LUAI_FUNC void ___pdr_luaK_setoneret (___pdr_FuncState *fs, ___pdr_expdesc *e);
___PDR_LUAI_FUNC int ___pdr_luaK_jump (___pdr_FuncState *fs);
___PDR_LUAI_FUNC void ___pdr_luaK_ret (___pdr_FuncState *fs, int first, int nret);
___PDR_LUAI_FUNC void ___pdr_luaK_patchlist (___pdr_FuncState *fs, int list, int target);
___PDR_LUAI_FUNC void ___pdr_luaK_patchtohere (___pdr_FuncState *fs, int list);
___PDR_LUAI_FUNC void ___pdr_luaK_patchclose (___pdr_FuncState *fs, int list, int level);
___PDR_LUAI_FUNC void ___pdr_luaK_concat (___pdr_FuncState *fs, int *l1, int l2);
___PDR_LUAI_FUNC int ___pdr_luaK_getlabel (___pdr_FuncState *fs);
___PDR_LUAI_FUNC void ___pdr_luaK_prefix (___pdr_FuncState *fs, ___pdr_UnOpr op, ___pdr_expdesc *v, int line);
___PDR_LUAI_FUNC void ___pdr_luaK_infix (___pdr_FuncState *fs, ___pdr_BinOpr op, ___pdr_expdesc *v);
___PDR_LUAI_FUNC void ___pdr_luaK_posfix (___pdr_FuncState *fs, ___pdr_BinOpr op, ___pdr_expdesc *v1,
                            ___pdr_expdesc *v2, int line);
___PDR_LUAI_FUNC void ___pdr_luaK_setlist (___pdr_FuncState *fs, int base, int nelems, int tostore);

} // end NS_PDR_SLUA

#endif
