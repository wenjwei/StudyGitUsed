/*
** $Id: lopcodes.c,v 1.55 2015/01/05 13:48:33 roberto Exp $
** Opcodes for Lua virtual machine
** See Copyright Notice in lua.h
*/

#define ___pdr_lopcodes_c
#define ___PDR_LUA_CORE

#include "lopcodes.h"
#include "lprefix.h"

#include <stddef.h>

namespace NS_PDR_SLUA {

#ifdef ___PDR_LUAC

  /* ORDER OP */

___PDR_LUAI_DDEF const char *const ___pdr_luaP_opnames[___PDR_NUM_OPCODES+1] = {
  "MOVE",
  "LOADK",
  "LOADKX",
  "LOADBOOL",
  "LOADNIL",
  "GETUPVAL",
  "GETTABUP",
  "GETTABLE",
  "SETTABUP",
  "SETUPVAL",
  "SETTABLE",
  "NEWTABLE",
  "SELF",
  "ADD",
  "SUB",
  "MUL",
  "MOD",
  "POW",
  "DIV",
  "IDIV",
  "BAND",
  "BOR",
  "BXOR",
  "SHL",
  "SHR",
  "UNM",
  "BNOT",
  "NOT",
  "LEN",
  "CONCAT",
  "JMP",
  "EQ",
  "LT",
  "LE",
  "TEST",
  "TESTSET",
  "CALL",
  "TAILCALL",
  "RETURN",
  "FORLOOP",
  "FORPREP",
  "TFORCALL",
  "TFORLOOP",
  "SETLIST",
  "CLOSURE",
  "VARARG",
  "EXTRAARG",
  NULL
};
#endif



/* ORDER OP */
#define ___pdr_opmode(t,a,b,c,m) (((t)<<7) | ((a)<<6) | ((b)<<4) | ((c)<<2) | (m))

___PDR_LUAI_DDEF const ___pdr_lu_byte ___pdr_luaP_opmodes[___PDR_NUM_OPCODES] = {
/*       T  A    B       C     mode		   opcode	*/
  ___pdr_opmode(0, 1, PDR_OpArgR, PDR_OpArgN, PDR_iABC)		/* OP_MOVE */
 ,___pdr_opmode(0, 1, PDR_OpArgR, PDR_OpArgK, PDR_iABC)		/* OP_SELF */
 ,___pdr_opmode(0, 1, PDR_OpArgK, PDR_OpArgK, PDR_iABC)		/* OP_ADD */
 ,___pdr_opmode(0, 1, PDR_OpArgK, PDR_OpArgK, PDR_iABC)		/* OP_SUB */
 ,___pdr_opmode(0, 1, PDR_OpArgK, PDR_OpArgK, PDR_iABC)		/* OP_MUL */
 ,___pdr_opmode(0, 1, PDR_OpArgK, PDR_OpArgK, PDR_iABC)		/* OP_MOD */
 ,___pdr_opmode(0, 1, PDR_OpArgK, PDR_OpArgK, PDR_iABC)		/* OP_POW */
 ,___pdr_opmode(0, 1, PDR_OpArgK, PDR_OpArgK, PDR_iABC)		/* OP_DIV */
 ,___pdr_opmode(0, 1, PDR_OpArgK, PDR_OpArgK, PDR_iABC)		/* OP_IDIV */
 ,___pdr_opmode(0, 1, PDR_OpArgK, PDR_OpArgK, PDR_iABC)		/* OP_BAND */
 ,___pdr_opmode(0, 1, PDR_OpArgK, PDR_OpArgK, PDR_iABC)		/* OP_BOR */
 ,___pdr_opmode(0, 1, PDR_OpArgK, PDR_OpArgK, PDR_iABC)		/* OP_BXOR */
 ,___pdr_opmode(0, 1, PDR_OpArgK, PDR_OpArgK, PDR_iABC)		/* OP_SHL */
 ,___pdr_opmode(0, 1, PDR_OpArgK, PDR_OpArgK, PDR_iABC)		/* OP_SHR */
 ,___pdr_opmode(0, 1, PDR_OpArgR, PDR_OpArgN, PDR_iABC)		/* OP_UNM */
 ,___pdr_opmode(0, 1, PDR_OpArgR, PDR_OpArgN, PDR_iABC)		/* OP_BNOT */
 ,___pdr_opmode(0, 1, PDR_OpArgR, PDR_OpArgN, PDR_iABC)		/* OP_NOT */
 ,___pdr_opmode(0, 1, PDR_OpArgR, PDR_OpArgN, PDR_iABC)		/* OP_LEN */
 ,___pdr_opmode(0, 1, PDR_OpArgR, PDR_OpArgR, PDR_iABC)		/* OP_CONCAT */
 ,___pdr_opmode(0, 0, PDR_OpArgR, PDR_OpArgN, PDR_iAsBx)		/* OP_JMP */
 ,___pdr_opmode(1, 0, PDR_OpArgK, PDR_OpArgK, PDR_iABC)		/* OP_EQ */
 ,___pdr_opmode(1, 0, PDR_OpArgK, PDR_OpArgK, PDR_iABC)		/* OP_LT */
 ,___pdr_opmode(1, 0, PDR_OpArgK, PDR_OpArgK, PDR_iABC)		/* OP_LE */
 ,___pdr_opmode(1, 0, PDR_OpArgN, PDR_OpArgU, PDR_iABC)		/* OP_TEST */
 ,___pdr_opmode(1, 1, PDR_OpArgR, PDR_OpArgU, PDR_iABC)		/* OP_TESTSET */
 ,___pdr_opmode(0, 1, PDR_OpArgU, PDR_OpArgU, PDR_iABC)		/* OP_CALL */
 ,___pdr_opmode(0, 1, PDR_OpArgU, PDR_OpArgU, PDR_iABC)		/* OP_TAILCALL */
 ,___pdr_opmode(0, 0, PDR_OpArgU, PDR_OpArgN, PDR_iABC)		/* OP_RETURN */
 ,___pdr_opmode(0, 1, PDR_OpArgR, PDR_OpArgN, PDR_iAsBx)		/* OP_FORLOOP */
 ,___pdr_opmode(0, 1, PDR_OpArgR, PDR_OpArgN, PDR_iAsBx)		/* OP_FORPREP */
 ,___pdr_opmode(0, 0, PDR_OpArgN, PDR_OpArgU, PDR_iABC)		/* OP_TFORCALL */
 ,___pdr_opmode(0, 1, PDR_OpArgR, PDR_OpArgN, PDR_iAsBx)		/* OP_TFORLOOP */
 ,___pdr_opmode(0, 0, PDR_OpArgU, PDR_OpArgU, PDR_iABC)		/* OP_SETLIST */
 ,___pdr_opmode(0, 1, PDR_OpArgU, PDR_OpArgN, PDR_iABx)		/* OP_CLOSURE */
 ,___pdr_opmode(0, 1, PDR_OpArgU, PDR_OpArgN, PDR_iABC)		/* OP_VARARG */
 ,___pdr_opmode(0, 1, PDR_OpArgK, PDR_OpArgN, PDR_iABx)		/* OP_LOADK */
 ,___pdr_opmode(0, 1, PDR_OpArgN, PDR_OpArgN, PDR_iABx)		/* OP_LOADKX */
 ,___pdr_opmode(0, 1, PDR_OpArgU, PDR_OpArgU, PDR_iABC)		/* OP_LOADBOOL */
 ,___pdr_opmode(0, 1, PDR_OpArgU, PDR_OpArgN, PDR_iABC)		/* OP_LOADNIL */
 ,___pdr_opmode(0, 1, PDR_OpArgU, PDR_OpArgN, PDR_iABC)		/* OP_GETUPVAL */
 ,___pdr_opmode(0, 1, PDR_OpArgU, PDR_OpArgK, PDR_iABC)		/* OP_GETTABUP */
 ,___pdr_opmode(0, 1, PDR_OpArgR, PDR_OpArgK, PDR_iABC)		/* OP_GETTABLE */
 ,___pdr_opmode(0, 0, PDR_OpArgK, PDR_OpArgK, PDR_iABC)		/* OP_SETTABUP */
 ,___pdr_opmode(0, 0, PDR_OpArgU, PDR_OpArgN, PDR_iABC)		/* OP_SETUPVAL */
 ,___pdr_opmode(0, 0, PDR_OpArgK, PDR_OpArgK, PDR_iABC)		/* OP_SETTABLE */
 ,___pdr_opmode(0, 1, PDR_OpArgU, PDR_OpArgU, PDR_iABC)		/* OP_NEWTABLE */
 ,___pdr_opmode(0, 0, PDR_OpArgU, PDR_OpArgU, PDR_iAx)		/* OP_EXTRAARG */
};

} // end NS_PDR_SLUA