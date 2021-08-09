/*
** $Id: llimits.h,v 1.141 2015/11/19 19:16:22 roberto Exp $
** Limits, basic types, and some other 'installation-dependent' definitions
** See Copyright Notice in lua.h
*/

#ifndef ___pdr_llimits_h___
#define ___pdr_llimits_h___

#include <limits.h>
#include <stddef.h>

#include "lua.h"

namespace NS_PDR_SLUA {

/*
** 'lu_mem' and 'l_mem' are unsigned/signed integers big enough to count
** the total memory used by Lua (in bytes). Usually, 'size_t' and
** 'ptrdiff_t' should work, but we use 'long' for 16-bit machines.
*/
#if defined(___PDR_LUAI_MEM)		/* { external definitions? */
typedef ___PDR_LUAI_UMEM ___pdr_lu_mem;
typedef ___PDR_LUAI_MEM ___pdr_l_mem;
#elif ___PDR_LUAI_BITSINT >= 32	/* }{ */
typedef size_t ___pdr_lu_mem;
typedef ptrdiff_t ___pdr_l_mem;
#else  /* 16-bit ints */	/* }{ */
typedef unsigned long ___pdr_lu_mem;
typedef long ___pdr_l_mem;
#endif				/* } */


/* chars used as small naturals (so that 'char' is reserved for characters) */
typedef unsigned char ___pdr_lu_byte;


/* maximum value for size_t */
#define ___PDR_MAX_SIZET	((size_t)(~(size_t)0))

/* maximum size visible for Lua (must be representable in a ___pdr_lua_Integer */
#define ___PDR_MAX_SIZE	(sizeof(size_t) < sizeof(___pdr_lua_Integer) ? ___PDR_MAX_SIZET \
                          : (size_t)(___PDR_LUA_MAXINTEGER))


#define ___PDR_MAX_LUMEM	((___pdr_lu_mem)(~(___pdr_lu_mem)0))

#define ___PDR_MAX_LMEM	((___pdr_l_mem)(___PDR_MAX_LUMEM >> 1))


#define ___PDR_MAX_INT		INT_MAX  /* maximum value of an int */


/*
** conversion of pointer to unsigned integer:
** this is for hashing only; there is no problem if the integer
** cannot hold the whole pointer value
*/
#define ___pdr_point2uint(p)	((unsigned int)((size_t)(p) & UINT_MAX))



/* type to ensure maximum alignment */
#if defined(___PDR_LUAI_USER_ALIGNMENT_T)
typedef ___PDR_LUAI_USER_ALIGNMENT_T ___pdr_L_Umaxalign;
#else
typedef union {
  ___pdr_lua_Number n;
  double u;
  void *s;
  ___pdr_lua_Integer i;
  long l;
} ___pdr_L_Umaxalign;
#endif



/* types of 'usual argument conversions' for lua_Number and ___pdr_lua_Integer */
typedef ___PDR_LUAI_UACNUMBER ___pdr_l_uacNumber;
typedef ___PDR_LUAI_UACINT ___pdr_l_uacInt;


/* internal assertions for in-house debugging */
#if defined(___pdr_lua_assert)
#define ___pdr_check_exp(c,e)		(___pdr_lua_assert(c), (e))
/* to avoid problems with conditions too long */
#define ___pdr_lua_longassert(c)	((c) ? (void)0 : ___pdr_lua_assert(0))
#else
#define ___pdr_lua_assert(c)		((void)0)
#define ___pdr_check_exp(c,e)		(e)
#define ___pdr_lua_longassert(c)	((void)0)
#endif

/*
** assertion for checking API calls
*/
#if !defined(___pdr_luai_apicheck)
#define ___pdr_luai_apicheck(l,e)	___pdr_lua_assert(e)
#endif

#define ___pdr_api_check(l,e,msg)	___pdr_luai_apicheck(l,(e) && msg)


/* macro to avoid warnings about unused variables */
#if !defined(___PDR_UNUSED)
#define ___PDR_UNUSED(x)	((void)(x))
#endif


/* type casts (a macro highlights casts in the code) */
#define ___pdr_cast(t, exp)	((t)(exp))

#define ___pdr_cast_void(i)	___pdr_cast(void, (i))
#define ___pdr_cast_byte(i)	___pdr_cast(___pdr_lu_byte, (i))
#define ___pdr_cast_num(i)	___pdr_cast(___pdr_lua_Number, (i))
#define ___pdr_cast_int(i)	___pdr_cast(int, (i))
#define ___pdr_cast_uchar(i)	___pdr_cast(unsigned char, (i))


/* cast a signed ___pdr_lua_Integer to lua_Unsigned */
#if !defined(___pdr_l_castS2U)
#define ___pdr_l_castS2U(i)	((___pdr_lua_Unsigned)(i))
#endif

/*
** cast a lua_Unsigned to a signed ___pdr_lua_Integer; this cast is
** not strict ISO C, but two-complement architectures should
** work fine.
*/
#if !defined(___pdr_l_castU2S)
#define ___pdr_l_castU2S(i)	((___pdr_lua_Integer)(i))
#endif


// difference between void and noreturn
// function without noreturn mark will return to the caller and continue what is left in the caller
// function with noreturn will no return the caller
// this will cause compile error(or warning, depends on the compiler) on android when LUA_USE_LONGJUMP is defined
// the longjump definition on Linux doesn't come with noreturn mark, which cause the error we mention above.
//#if PLATFORM_ANDROID
#define ___pdr_l_noret void
//#else 

/*
** non-return type
*/
//#if defined(__GNUC__)
//#define l_noret                void __attribute__((noreturn))
//#elif defined(_MSC_VER) && _MSC_VER >= 1200
//#define l_noret                void __declspec(noreturn)
//#else
//#define l_noret                void
//#endif

//#endif

/*
** maximum depth for nested C calls and syntactical nested non-terminals
** in a program. (Value must fit in an unsigned short int.)
*/
#if !defined(___PDR_LUAI_MAXCCALLS)
#define ___PDR_LUAI_MAXCCALLS		200
#endif



/*
** type for virtual-machine instructions;
** must be an unsigned with (at least) 4 bytes (see details in lopcodes.h)
*/
#if ___PDR_LUAI_BITSINT >= 32
typedef unsigned int ___pdr_Instruction;
#else
typedef unsigned long ___pdr_Instruction;
#endif



/*
** Maximum length for short strings, that is, strings that are
** internalized. (Cannot be smaller than reserved words or tags for
** metamethods, as these strings must be internalized;
** #("function") = 8, #("__newindex") = 10.)
*/
#if !defined(___PDR_LUAI_MAXSHORTLEN)
#define ___PDR_LUAI_MAXSHORTLEN	40
#endif


/*
** Initial size for the string table (must be power of 2).
** The Lua core alone registers ~50 strings (reserved words +
** metaevent keys + a few others). Libraries would typically add
** a few dozens more.
*/
#if !defined(___PDR_MINSTRTABSIZE)
#define ___PDR_MINSTRTABSIZE	128
#endif


/*
** Size of cache for strings in the API. 'N' is the number of
** sets (better be a prime) and "M" is the size of each set (M == 1
** makes a direct cache.)
*/
#if !defined(___PDR_STRCACHE_N)
#define ___PDR_STRCACHE_N		53
#define ___PDR_STRCACHE_M		2
#endif


/* minimum size for string buffer */
#if !defined(___PDR_LUA_MINBUFFER)
#define ___PDR_LUA_MINBUFFER	32
#endif


/*
** macros that are executed whenever program enters the Lua core
** ('lua_lock') and leaves the core ('lua_unlock')
*/
#if !defined(___pdr_lua_lock)
#define ___pdr_lua_lock(L)	((void) 0)
#define ___pdr_lua_unlock(L)	((void) 0)
#endif

/*
** macro executed during Lua functions at points where the
** function can yield.
*/
#if !defined(___pdr_luai_threadyield)
#define ___pdr_luai_threadyield(L)	{___pdr_lua_unlock(L); ___pdr_lua_lock(L);}
#endif


/*
** these macros allow user-specific actions on threads when you defined
** LUAI_EXTRASPACE and need to do something extra when a thread is
** created/deleted/resumed/yielded.
*/
#if !defined(___pdr_luai_userstateopen)
#define ___pdr_luai_userstateopen(L)		((void)L)
#endif

#if !defined(___pdr_luai_userstateclose)
#define ___pdr_luai_userstateclose(L)		((void)L)
#endif

#if !defined(___pdr_luai_userstatethread)
#define ___pdr_luai_userstatethread(L,L1)	((void)L)
#endif

#if !defined(___pdr_luai_userstatefree)
#define ___pdr_luai_userstatefree(L,L1)	((void)L)
#endif

#if !defined(___pdr_luai_userstateresume)
#define ___pdr_luai_userstateresume(L,n)	((void)L)
#endif

#if !defined(___pdr_luai_userstateyield)
#define ___pdr_luai_userstateyield(L,n)	((void)L)
#endif



/*
** The luai_num* macros define the primitive operations over numbers.
*/

/* floor division (defined as 'floor(a/b)') */
#if !defined(___pdr_luai_numidiv)
#define ___pdr_luai_numidiv(L,a,b)     ((void)L, ___pdr_l_floor(___pdr_luai_numdiv(L,a,b)))
#endif

/* float division */
#if !defined(___pdr_luai_numdiv)
#define ___pdr_luai_numdiv(L,a,b)      ((a)/(b))
#endif

/*
** modulo: defined as 'a - floor(a/b)*b'; this definition gives NaN when
** 'b' is huge, but the result should be 'a'. 'fmod' gives the result of
** 'a - trunc(a/b)*b', and therefore must be corrected when 'trunc(a/b)
** ~= floor(a/b)'. That happens when the division has a non-integer
** negative result, which is equivalent to the test below.
*/
#if !defined(___pdr_luai_nummod)
#define ___pdr_luai_nummod(L,a,b,m)  \
  { (m) = ___pdr_l_mathop(fmod)(a,b); if ((m)*(b) < 0) (m) += (b); }
#endif

/* exponentiation */
#if !defined(___pdr_luai_numpow)
#define ___pdr_luai_numpow(L,a,b)      ((void)L, ___pdr_l_mathop(pow)(a,b))
#endif

/* the others are quite standard operations */
#if !defined(___pdr_luai_numadd)
#define ___pdr_luai_numadd(L,a,b)      ((a)+(b))
#define ___pdr_luai_numsub(L,a,b)      ((a)-(b))
#define ___pdr_luai_nummul(L,a,b)      ((a)*(b))
#define ___pdr_luai_numunm(L,a)        (-(a))
#define ___pdr_luai_numeq(a,b)         ((a)==(b))
#define ___pdr_luai_numlt(a,b)         ((a)<(b))
#define ___pdr_luai_numle(a,b)         ((a)<=(b))
#define ___pdr_luai_numisnan(a)        (!___pdr_luai_numeq((a), (a)))
#endif





/*
** macro to control inclusion of some hard tests on stack reallocation
*/
#if !defined(___PDR_HARDSTACKTESTS)
#define ___pdr_condmovestack(L,pre,pos)	((void)0)
#else
/* realloc stack keeping its size */
#define ___pdr_condmovestack(L,pre,pos)  \
	{ int sz_ = (L)->stacksize; pre; ___pdr_luaD_reallocstack((L), sz_); pos; }
#endif

#if !defined(___PDR_HARDMEMTESTS)
#define ___pdr_condchangemem(L,pre,pos)	((void)0)
#else
#define ___pdr_condchangemem(L,pre,pos)  \
	{ if (___pdr_G(L)->gcrunning) { pre; ___pdr_luaC_fullgc(L, 0); pos; } }
#endif

} // end NS_PDR_SLUA

#endif
