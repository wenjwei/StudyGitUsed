/*
** $Id: lstate.h,v 2.133 2016/12/22 13:08:50 roberto Exp $
** Global State
** See Copyright Notice in lua.h
*/

#ifndef ___pdr_lstate_h___
#define ___pdr_lstate_h___

#include "lua.h"
#include "lobject.h"
#include "ltm.h"
#include "lzio.h"

/*

** Some notes about garbage-collected objects: All objects in Lua must
** be kept somehow accessible until being freed, so all objects always
** belong to one (and only one) of these lists, using field 'next' of
** the 'CommonHeader' for the link:
**
** 'allgc': all objects not marked for finalization;
** 'finobj': all objects marked for finalization;
** 'tobefnz': all objects ready to be finalized;
** 'fixedgc': all objects that are not to be collected (currently
** only small strings, such as reserved words).

*/


/*
** Atomic type (relative to signals) to better ensure that 'lua_sethook'
** is thread safe
*/
#if !defined(___pdr_l_signalT)
#include <signal.h>
#define ___pdr_l_signalT	sig_atomic_t
#endif

namespace NS_PDR_SLUA {

struct ___pdr_lua_longjmp; /* defined in ldo.c */

/* extra stack space to handle TM calls and some other extras */
#define ___PDR_EXTRA_STACK   5


#define ___PDR_BASIC_STACK_SIZE        (2*___PDR_LUA_MINSTACK)


/* kinds of Garbage Collection */
#define ___PDR_KGC_NORMAL	0
#define ___PDR_KGC_EMERGENCY	1	/* gc was forced by an allocation failure */

typedef struct ___pdr_stringtable {
  ___pdr_TString **hash;
  int nuse;  /* number of elements */
  int size;
} ___pdr_stringtable;


/*
** Information about a call.
** When a thread yields, 'func' is adjusted to pretend that the
** top function has only the yielded values in its stack; in that
** case, the actual 'func' value is saved in field 'extra'.
** When a function calls another with a continuation, 'extra' keeps
** the function index so that, in case of errors, the continuation
** function can be called with the correct top.
*/
typedef struct ___pdr_CallInfo {
  ___pdr_StkId func;  /* function index in the stack */
  ___pdr_StkId	top;  /* top for this function */
  struct ___pdr_CallInfo *previous, *next;  /* dynamic call link */
  union {
    struct {  /* only for Lua functions */
      ___pdr_StkId base;  /* base for this function */
      const ___pdr_Instruction *savedpc;
    } l;
    struct {  /* only for C functions */
      ___pdr_lua_KFunction k;  /* continuation in case of yields */
      ptrdiff_t old_errfunc;
      ___pdr_lua_KContext ctx;  /* context info. in case of yields */
    } c;
  } u;
  ptrdiff_t extra;
  short nresults;  /* expected number of results from this function */
  unsigned short callstatus;
} ___pdr_CallInfo;


/*
** Bits in CallInfo status
*/
#define ___PDR_CIST_OAH	(1<<0)	/* original value of 'allowhook' */
#define ___PDR_CIST_LUA	(1<<1)	/* call is running a Lua function */
#define ___PDR_CIST_HOOKED	(1<<2)	/* call is running a debug hook */
#define ___PDR_CIST_FRESH	(1<<3)	/* call is running on a fresh invocation
                                   of luaV_execute */
#define ___PDR_CIST_YPCALL	(1<<4)	/* call is a yieldable protected call */
#define ___PDR_CIST_TAIL	(1<<5)	/* call was tail called */
#define ___PDR_CIST_HOOKYIELD	(1<<6)	/* last hook called yielded */
#define ___PDR_CIST_LEQ	(1<<7)  /* using __lt for __le */
#define ___PDR_CIST_FIN	(1<<8)  /* call is running a finalizer */

#define ___pdr_isLua(ci)	((ci)->callstatus & ___PDR_CIST_LUA)

/* assume that CIST_OAH has offset 0 and that 'v' is strictly 0/1 */
#define ___pdr_setoah(st,v)	((st) = ((st) & ~___PDR_CIST_OAH) | (v))
#define ___pdr_getoah(st)	((st) & ___PDR_CIST_OAH)


/*
** 'global state', shared by all threads of this state
*/
typedef struct ___pdr_global_State {
  ___pdr_lua_Alloc frealloc;  /* function to reallocate memory */
  void *ud;         /* auxiliary data to 'frealloc' */
  ___pdr_l_mem totalbytes;  /* number of bytes currently allocated - GCdebt */
  ___pdr_l_mem GCdebt;  /* bytes allocated not yet compensated by the collector */
  ___pdr_lu_mem GCmemtrav;  /* memory traversed by the GC */
  ___pdr_lu_mem GCestimate;  /* an estimate of the non-garbage memory in use */
  ___pdr_stringtable strt;  /* hash table for strings */
  ___pdr_TValue l_registry;
  unsigned int seed;  /* randomized seed for hashes */
  ___pdr_lu_byte currentwhite;
  ___pdr_lu_byte gcstate;  /* state of garbage collector */
  ___pdr_lu_byte gckind;  /* kind of GC running */
  ___pdr_lu_byte gcrunning;  /* true if GC is running */
  ___pdr_GCObject *allgc;  /* list of all collectable objects */
  ___pdr_GCObject **sweepgc;  /* current position of sweep in list */
  ___pdr_GCObject *finobj;  /* list of collectable objects with finalizers */
  ___pdr_GCObject *gray;  /* list of gray objects */
  ___pdr_GCObject *grayagain;  /* list of objects to be traversed atomically */
  ___pdr_GCObject *weak;  /* list of tables with weak values */
  ___pdr_GCObject *ephemeron;  /* list of ephemeron tables (weak keys) */
  ___pdr_GCObject *allweak;  /* list of all-weak tables */
  ___pdr_GCObject *tobefnz;  /* list of userdata to be GC */
  ___pdr_GCObject *fixedgc;  /* list of objects not to be collected */
  struct ___pdr_lua_State *twups;  /* list of threads with open upvalues */
  unsigned int gcfinnum;  /* number of finalizers to call in each GC step */
  int gcpause;  /* size of pause between successive GCs */
  int gcstepmul;  /* GC 'granularity' */
  ___pdr_lua_CFunction panic;  /* to be called in unprotected errors */
  struct ___pdr_lua_State *mainthread;
  const ___pdr_lua_Number *version;  /* pointer to version number */
  ___pdr_TString *memerrmsg;  /* memory-error message */
  ___pdr_TString *tmname[PDR_TM_N];  /* array with tag-method names */
  struct ___pdr_Table *mt[___PDR_LUA_NUMTAGS];  /* metatables for basic types */
  ___pdr_TString *strcache[___PDR_STRCACHE_N][___PDR_STRCACHE_M];  /* cache for strings in API */
} ___pdr_global_State;


/*
** 'per thread' state
*/
struct ___pdr_lua_State {
  ___pdr_CommonHeader;
  unsigned short nci;  /* number of items in 'ci' list */
  ___pdr_lu_byte status;
  ___pdr_StkId top;  /* first free slot in the stack */
  ___pdr_global_State *l_G;
  ___pdr_CallInfo *ci;  /* call info for current function */
  const ___pdr_Instruction *oldpc;  /* last pc traced */
  ___pdr_StkId stack_last;  /* last free slot in the stack */
  ___pdr_StkId stack;  /* stack base */
  ___pdr_UpVal *openupval;  /* list of open upvalues in this stack */
  ___pdr_GCObject *gclist;
  struct ___pdr_lua_State *twups;  /* list of threads with open upvalues */
  struct ___pdr_lua_longjmp *errorJmp;  /* current error recover point */
  ___pdr_CallInfo base_ci;  /* CallInfo for first level (C calling Lua) */
  volatile ___pdr_lua_Hook hook;
  ptrdiff_t errfunc;  /* current error handling function (stack index) */
  int stacksize;
  int basehookcount;
  int hookcount;
  unsigned short nny;  /* number of non-yieldable calls in stack */
  unsigned short nCcalls;  /* number of nested C calls */
  ___pdr_l_signalT hookmask;
  ___pdr_lu_byte allowhook;
  int onlyluac;
};

// #define G(L)	(L->l_G)
inline ___pdr_global_State* ___pdr_G(___pdr_lua_State* L) {
  return L->l_G;
}

    /*
** Union of all collectable objects (only for conversions)
*/
union ___pdr_GCUnion {
  ___pdr_GCObject gc;  /* common header */
  struct ___pdr_TString ts;
  struct ___pdr_Udata u;
  union ___pdr_Closure cl;
  struct ___pdr_Table h;
  struct ___pdr_Proto p;
  struct ___pdr_lua_State th;  /* thread */
};


#define ___pdr_cast_u(o)	___pdr_cast(union ___pdr_GCUnion *, (o))

/* macros to convert a GCObject into a specific value */
#define ___pdr_gco2ts(o)  \
	___pdr_check_exp(___pdr_novariant((o)->tt) == ___PDR_LUA_TSTRING, &((___pdr_cast_u(o))->ts))
#define ___pdr_gco2u(o)  ___pdr_check_exp((o)->tt == ___PDR_LUA_TUSERDATA, &((___pdr_cast_u(o))->u))
#define ___pdr_gco2lcl(o)  ___pdr_check_exp((o)->tt == ___PDR_LUA_TLCL, &((___pdr_cast_u(o))->cl.l))
#define ___pdr_gco2ccl(o)  ___pdr_check_exp((o)->tt == ___PDR_LUA_TCCL, &((___pdr_cast_u(o))->cl.c))
#define ___pdr_gco2cl(o)  \
	___pdr_check_exp(___pdr_novariant((o)->tt) == ___PDR_LUA_TFUNCTION, &((___pdr_cast_u(o))->cl))
#define ___pdr_gco2t(o)  ___pdr_check_exp((o)->tt == ___PDR_LUA_TTABLE, &((___pdr_cast_u(o))->h))
#define ___pdr_gco2p(o)  ___pdr_check_exp((o)->tt == ___PDR_LUA_TPROTO, &((___pdr_cast_u(o))->p))
#define ___pdr_gco2th(o)  ___pdr_check_exp((o)->tt == ___PDR_LUA_TTHREAD, &((___pdr_cast_u(o))->th))


/* macro to convert a Lua object into a GCObject */
#define ___pdr_obj2gco(v) \
	___pdr_check_exp(___pdr_novariant((v)->tt) < ___PDR_LUA_TDEADKEY, (&(___pdr_cast_u(v)->gc)))


/* actual number of total bytes allocated */
#define ___pdr_gettotalbytes(g)	___pdr_cast(___pdr_lu_mem, (g)->totalbytes + (g)->GCdebt)

___PDR_LUAI_FUNC void ___pdr_luaE_setdebt (___pdr_global_State *g, ___pdr_l_mem debt);
___PDR_LUAI_FUNC void ___pdr_luaE_freethread (___pdr_lua_State *L, ___pdr_lua_State *L1);
___PDR_LUAI_FUNC ___pdr_CallInfo *___pdr_luaE_extendCI (___pdr_lua_State *L);
___PDR_LUAI_FUNC void ___pdr_luaE_freeCI (___pdr_lua_State *L);
___PDR_LUAI_FUNC void ___pdr_luaE_shrinkCI (___pdr_lua_State *L);

} // end NS_PDR_SLUA

#endif

