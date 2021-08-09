/*
** $Id: lopcodes.h,v 1.149 2016/07/19 17:12:21 roberto Exp $
** Opcodes for Lua virtual machine
** See Copyright Notice in lua.h
*/

#ifndef ___pdr_lopcodes_h___
#define ___pdr_lopcodes_h___

#include "llimits.h"

namespace NS_PDR_SLUA {

/*===========================================================================
  We assume that instructions are unsigned numbers.
  All instructions have an opcode in the first 6 bits.
  Instructions can have the following fields:
	'A' : 8 bits
	'B' : 9 bits
	'C' : 9 bits
	'Ax' : 26 bits ('A', 'B', and 'C' together)
	'Bx' : 18 bits ('B' and 'C' together)
	'sBx' : signed Bx

  A signed argument is represented in excess K; that is, the number
  value is the unsigned value minus K. K is exactly the maximum value
  for that argument (so that -max is represented by 0, and +max is
  represented by 2*max), which is half the maximum for the corresponding
  unsigned argument.
===========================================================================*/


enum ___pdr_OpMode {PDR_iABC, PDR_iABx, PDR_iAsBx, PDR_iAx};  /* basic instruction format */


/*
** size and position of opcode arguments.
*/
#define ___PDR_SIZE_C		9
#define ___PDR_SIZE_B		9
#define ___PDR_SIZE_Bx		(___PDR_SIZE_C + ___PDR_SIZE_B)
#define ___PDR_SIZE_A		8
#define ___PDR_SIZE_Ax		(___PDR_SIZE_C + ___PDR_SIZE_B + ___PDR_SIZE_A)

#define ___PDR_SIZE_OP		6

#define ___PDR_POS_OP		0
#define ___PDR_POS_A		(___PDR_POS_OP + ___PDR_SIZE_OP)
#define ___PDR_POS_C		(___PDR_POS_A + ___PDR_SIZE_A)
#define ___PDR_POS_B		(___PDR_POS_C + ___PDR_SIZE_C)
#define ___PDR_POS_Bx		___PDR_POS_C
#define ___PDR_POS_Ax		___PDR_POS_A


/*
** limits for opcode arguments.
** we use (signed) int to manipulate most arguments,
** so they must fit in LUAI_BITSINT-1 bits (-1 for sign)
*/
#if ___PDR_SIZE_Bx < ___PDR_LUAI_BITSINT-1
#define ___PDR_MAXARG_Bx        ((1<<___PDR_SIZE_Bx)-1)
#define ___PDR_MAXARG_sBx        (___PDR_MAXARG_Bx>>1)         /* 'sBx' is signed */
#else
#define ___PDR_MAXARG_Bx        ___PDR_MAX_INT
#define ___PDR_MAXARG_sBx        ___PDR_MAX_INT
#endif

#if ___PDR_SIZE_Ax < ___PDR_LUAI_BITSINT-1
#define ___PDR_MAXARG_Ax	((1<<___PDR_SIZE_Ax)-1)
#else
#define ___PDR_MAXARG_Ax	___PDR_MAX_INT
#endif


#define ___PDR_MAXARG_A        ((1<<___PDR_SIZE_A)-1)
#define ___PDR_MAXARG_B        ((1<<___PDR_SIZE_B)-1)
#define ___PDR_MAXARG_C        ((1<<___PDR_SIZE_C)-1)


/* creates a mask with 'n' 1 bits at position 'p' */
#define ___PDR_MASK1(n,p)	((~((~(___pdr_Instruction)0)<<(n)))<<(p))

/* creates a mask with 'n' 0 bits at position 'p' */
#define ___PDR_MASK0(n,p)	(~___PDR_MASK1(n,p))

/*
** the following macros help to manipulate instructions
*/

#define ___PDR_GET_OPCODE(i)	(___pdr_cast(___pdr_OpCode, ((i)>>___PDR_POS_OP) & ___PDR_MASK1(___PDR_SIZE_OP,0)))
#define ___PDR_SET_OPCODE(i,o)	((i) = (((i)&___PDR_MASK0(___PDR_SIZE_OP,___PDR_POS_OP)) | \
		((___pdr_cast(___pdr_Instruction, o)<<___PDR_POS_OP)&___PDR_MASK1(___PDR_SIZE_OP,___PDR_POS_OP))))

#define ___pdr_getarg(i,pos,size)	(___pdr_cast(int, ((i)>>pos) & ___PDR_MASK1(size,0)))
#define ___pdr_setarg(i,v,pos,size)	((i) = (((i)&___PDR_MASK0(size,pos)) | \
                ((___pdr_cast(___pdr_Instruction, v)<<pos)&___PDR_MASK1(size,pos))))

#define ___PDR_GETARG_A(i)	___pdr_getarg(i, ___PDR_POS_A, ___PDR_SIZE_A)
#define ___PDR_SETARG_A(i,v)	___pdr_setarg(i, v, ___PDR_POS_A, ___PDR_SIZE_A)

#define ___PDR_GETARG_B(i)	___pdr_getarg(i, ___PDR_POS_B, ___PDR_SIZE_B)
#define ___PDR_SETARG_B(i,v)	___pdr_setarg(i, v, ___PDR_POS_B, ___PDR_SIZE_B)

#define ___PDR_GETARG_C(i)	___pdr_getarg(i, ___PDR_POS_C, ___PDR_SIZE_C)
#define ___PDR_SETARG_C(i,v)	___pdr_setarg(i, v, ___PDR_POS_C, ___PDR_SIZE_C)

#define ___PDR_GETARG_Bx(i)	___pdr_getarg(i, ___PDR_POS_Bx, ___PDR_SIZE_Bx)
#define ___PDR_SETARG_Bx(i,v)	___pdr_setarg(i, v, ___PDR_POS_Bx, ___PDR_SIZE_Bx)

#define ___PDR_GETARG_Ax(i)	___pdr_getarg(i, ___PDR_POS_Ax, ___PDR_SIZE_Ax)
#define ___PDR_SETARG_Ax(i,v)	___pdr_setarg(i, v, ___PDR_POS_Ax, ___PDR_SIZE_Ax)

#define ___PDR_GETARG_sBx(i)	(___PDR_GETARG_Bx(i)-___PDR_MAXARG_sBx)
#define ___PDR_SETARG_sBx(i,b)	___PDR_SETARG_Bx((i),___pdr_cast(unsigned int, (b)+___PDR_MAXARG_sBx))


#define ___PDR_CREATE_ABC(o,a,b,c)	((___pdr_cast(___pdr_Instruction, o)<<___PDR_POS_OP) \
			| (___pdr_cast(___pdr_Instruction, a)<<___PDR_POS_A) \
			| (___pdr_cast(___pdr_Instruction, b)<<___PDR_POS_B) \
			| (___pdr_cast(___pdr_Instruction, c)<<___PDR_POS_C))

#define ___PDR_CREATE_ABx(o,a,bc)	((___pdr_cast(___pdr_Instruction, o)<<___PDR_POS_OP) \
			| (___pdr_cast(___pdr_Instruction, a)<<___PDR_POS_A) \
			| (___pdr_cast(___pdr_Instruction, bc)<<___PDR_POS_Bx))

#define ___PDR_CREATE_Ax(o,a)		((___pdr_cast(___pdr_Instruction, o)<<___PDR_POS_OP) \
			| (___pdr_cast(___pdr_Instruction, a)<<___PDR_POS_Ax))


/*
** Macros to operate RK indices
*/

/* this bit 1 means constant (0 means register) */
#define ___PDR_BITRK		(1 << (___PDR_SIZE_B - 1))

/* test whether value is a constant */
#define ___PDR_ISK(x)		((x) & ___PDR_BITRK)

/* gets the index of the constant */
#define ___PDR_INDEXK(r)	((int)(r) & ~___PDR_BITRK)

#if !defined(___PDR_MAXINDEXRK)  /* (for debugging only) */
#define ___PDR_MAXINDEXRK	(___PDR_BITRK - 1)
#endif

/* code a constant index as a RK value */
#define ___PDR_RKASK(x)	((x) | ___PDR_BITRK)


/*
** invalid register that fits in 8 bits
*/
#define ___PDR_NO_REG		___PDR_MAXARG_A


/*
** R(x) - register
** Kst(x) - constant (in constant table)
** RK(x) == if ISK(x) then Kst(INDEXK(x)) else R(x)
*/


/*
** grep "ORDER OP" if you change these enums
*/

typedef enum {
/*----------------------------------------------------------------------
name		args	description
------------------------------------------------------------------------*/
PDR_OP_MOVE,/*	A B	R(A) := R(B)					*/

PDR_OP_SELF,/*	A B C	R(A+1) := R(B); R(A) := R(B)[RK(C)]		*/

PDR_OP_ADD,/*	A B C	R(A) := RK(B) + RK(C)				*/
PDR_OP_SUB,/*	A B C	R(A) := RK(B) - RK(C)				*/
PDR_OP_MUL,/*	A B C	R(A) := RK(B) * RK(C)				*/
PDR_OP_MOD,/*	A B C	R(A) := RK(B) % RK(C)				*/
PDR_OP_POW,/*	A B C	R(A) := RK(B) ^ RK(C)				*/
PDR_OP_DIV,/*	A B C	R(A) := RK(B) / RK(C)				*/
PDR_OP_IDIV,/*	A B C	R(A) := RK(B) // RK(C)				*/
PDR_OP_BAND,/*	A B C	R(A) := RK(B) & RK(C)				*/
PDR_OP_BOR,/*	A B C	R(A) := RK(B) | RK(C)				*/
PDR_OP_BXOR,/*	A B C	R(A) := RK(B) ~ RK(C)				*/
PDR_OP_SHL,/*	A B C	R(A) := RK(B) << RK(C)				*/
PDR_OP_SHR,/*	A B C	R(A) := RK(B) >> RK(C)				*/
PDR_OP_UNM,/*	A B	R(A) := -R(B)					*/
PDR_OP_BNOT,/*	A B	R(A) := ~R(B)					*/
PDR_OP_NOT,/*	A B	R(A) := not R(B)				*/
PDR_OP_LEN,/*	A B	R(A) := length of R(B)				*/

PDR_OP_CONCAT,/*	A B C	R(A) := R(B).. ... ..R(C)			*/

PDR_OP_JMP,/*	A sBx	pc+=sBx; if (A) close all upvalues >= R(A - 1)	*/
PDR_OP_EQ,/*	A B C	if ((RK(B) == RK(C)) ~= A) then pc++		*/
PDR_OP_LT,/*	A B C	if ((RK(B) <  RK(C)) ~= A) then pc++		*/
PDR_OP_LE,/*	A B C	if ((RK(B) <= RK(C)) ~= A) then pc++		*/

PDR_OP_TEST,/*	A C	if not (R(A) <=> C) then pc++			*/
PDR_OP_TESTSET,/*	A B C	if (R(B) <=> C) then R(A) := R(B) else pc++	*/

PDR_OP_CALL,/*	A B C	R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1)) */
PDR_OP_TAILCALL,/*	A B C	return R(A)(R(A+1), ... ,R(A+B-1))		*/
PDR_OP_RETURN,/*	A B	return R(A), ... ,R(A+B-2)	(see note)	*/

PDR_OP_FORLOOP,/*	A sBx	R(A)+=R(A+2);
			if R(A) <?= R(A+1) then { pc+=sBx; R(A+3)=R(A) }*/
PDR_OP_FORPREP,/*	A sBx	R(A)-=R(A+2); pc+=sBx				*/

PDR_OP_TFORCALL,/*	A C	R(A+3), ... ,R(A+2+C) := R(A)(R(A+1), R(A+2));	*/
PDR_OP_TFORLOOP,/*	A sBx	if R(A+1) ~= nil then { R(A)=R(A+1); pc += sBx }*/

PDR_OP_SETLIST,/*	A B C	R(A)[(C-1)*FPF+i] := R(A+i), 1 <= i <= B	*/

PDR_OP_CLOSURE,/*	A Bx	R(A) := closure(KPROTO[Bx])			*/

PDR_OP_VARARG,/*	A B	R(A), R(A+1), ..., R(A+B-2) = vararg		*/
PDR_OP_LOADK,/*	A Bx	R(A) := Kst(Bx)					*/
PDR_OP_LOADKX,/*	A 	R(A) := Kst(extra arg)				*/
PDR_OP_LOADBOOL,/*	A B C	R(A) := (Bool)B; if (C) pc++			*/
PDR_OP_LOADNIL,/*	A B	R(A), R(A+1), ..., R(A+B) := nil		*/
PDR_OP_GETUPVAL,/*	A B	R(A) := UpValue[B]				*/

PDR_OP_GETTABUP,/*	A B C	R(A) := UpValue[B][RK(C)]			*/
PDR_OP_GETTABLE,/*	A B C	R(A) := R(B)[RK(C)]				*/

PDR_OP_SETTABUP,/*	A B C	UpValue[A][RK(B)] := RK(C)			*/
PDR_OP_SETUPVAL,/*	A B	UpValue[B] := R(A)				*/
PDR_OP_SETTABLE,/*	A B C	R(A)[RK(B)] := RK(C)				*/

PDR_OP_NEWTABLE,/*	A B C	R(A) := {} (size = B,C)				*/

PDR_OP_EXTRAARG/*	Ax	extra (larger) argument for previous opcode	*/
} ___pdr_OpCode;


#define ___PDR_NUM_OPCODES	(___pdr_cast(int, PDR_OP_EXTRAARG) + 1)



/*===========================================================================
  Notes:
  (*) In OP_CALL, if (B == 0) then B = top. If (C == 0), then 'top' is
  set to last_result+1, so next open instruction (OP_CALL, OP_RETURN,
  OP_SETLIST) may use 'top'.

  (*) In OP_VARARG, if (B == 0) then use actual number of varargs and
  set top (like in OP_CALL with C == 0).

  (*) In OP_RETURN, if (B == 0) then return up to 'top'.

  (*) In OP_SETLIST, if (B == 0) then B = 'top'; if (C == 0) then next
  'instruction' is EXTRAARG(real C).

  (*) In OP_LOADKX, the next 'instruction' is always EXTRAARG.

  (*) For comparisons, A specifies what condition the test should accept
  (true or false).

  (*) All 'skips' (pc++) assume that next instruction is a jump.

===========================================================================*/


/*
** masks for instruction properties. The format is:
** bits 0-1: op mode
** bits 2-3: C arg mode
** bits 4-5: B arg mode
** bit 6: instruction set register A
** bit 7: operator is a test (next instruction must be a jump)
*/

enum ___pdr_OpArgMask {
  PDR_OpArgN,  /* argument is not used */
  PDR_OpArgU,  /* argument is used */
  PDR_OpArgR,  /* argument is a register or a jump offset */
  PDR_OpArgK   /* argument is a constant or register/constant */
};

___PDR_LUAI_DDEC const ___pdr_lu_byte ___pdr_luaP_opmodes[___PDR_NUM_OPCODES];

#define ___pdr_getOpMode(m)	(___pdr_cast(enum ___pdr_OpMode, ___pdr_luaP_opmodes[m] & 3))
#define ___pdr_getBMode(m)	(___pdr_cast(enum ___pdr_OpArgMask, (___pdr_luaP_opmodes[m] >> 4) & 3))
#define ___pdr_getCMode(m)	(___pdr_cast(enum ___pdr_OpArgMask, (___pdr_luaP_opmodes[m] >> 2) & 3))
#define ___pdr_testAMode(m)	(___pdr_luaP_opmodes[m] & (1 << 6))
#define ___pdr_testTMode(m)	(___pdr_luaP_opmodes[m] & (1 << 7))

#ifdef ___PDR_LUAC
___PDR_LUAI_DDEC const char *const ___pdr_luaP_opnames[___PDR_NUM_OPCODES+1];  /* opcode names */
#endif

/* number of list items to accumulate before a SETLIST instruction */
#define ___PDR_LFIELDS_PER_FLUSH	50

} // end NS_PDR_SLUA

#endif
