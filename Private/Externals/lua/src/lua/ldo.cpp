/*
** $Id: ldo.c,v 2.157 2016/12/13 15:52:21 roberto Exp $
** Stack and Call structure of Lua
** See Copyright Notice in lua.h
*/

#define ___pdr_ldo_c
#define ___PDR_LUA_CORE

#include "ldo.h"
#include "lprefix.h"

#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lapi.h"
#include "ldebug.h"
#include "lfunc.h"
#include "lgc.h"
#include "lmem.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lparser.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "ltm.h"
#include "lundump.h"
#include "lvm.h"
#include "lzio.h"

#define ___pdr_errorstatus(s)	((s) > ___PDR_LUA_YIELD)
/*
** {======================================================
** Error-recovery functions
** =======================================================
*/

/*
** LUAI_THROW/LUAI_TRY define how Lua does exception handling. By
** default, Lua handles errors with exceptions when compiling as
** C++ code, with _longjmp/_setjmp when asked to use them, and with
** longjmp/setjmp otherwise.
*/
#if !defined(___PDR_LUAI_THROW)				/* { */

#if defined(__cplusplus) && !defined(___PDR_LUA_USE_LONGJMP)	/* { */

/* C++ exceptions */
#define ___PDR_LUAI_THROW(L,c)		throw(c)
#define ___PDR_LUAI_TRY(L,c,a) \
	try { a } catch(...) { if ((c)->status == 0) (c)->status = -1; }
#define ___pdr_luai_jmpbuf		int  /* dummy variable */

#elif defined(___PDR_LUA_USE_POSIX)				/* }{ */

/* in POSIX, try _longjmp/_setjmp (more efficient) */
#define ___PDR_LUAI_THROW(L,c)		_longjmp((c)->b, 1)
#define ___PDR_LUAI_TRY(L,c,a)		if (_setjmp((c)->b) == 0) { a }
#define ___pdr_luai_jmpbuf		jmp_buf

#else							/* }{ */

/* ISO C handling with long jumps */
#define ___PDR_LUAI_THROW(L,c)		longjmp((c)->b, 1)
#define ___PDR_LUAI_TRY(L,c,a)		if (setjmp((c)->b) == 0) { a }
#define ___pdr_luai_jmpbuf		jmp_buf

#endif							/* } */

#endif							/* } */

namespace NS_PDR_SLUA {

/* chain list of long jump buffers */
struct ___pdr_lua_longjmp {
  struct ___pdr_lua_longjmp *previous;
  ___pdr_luai_jmpbuf b;
  volatile int status;  /* error code */
};


static void seterrorobj (___pdr_lua_State *L, int errcode, ___pdr_StkId oldtop) {
  switch (errcode) {
    case ___PDR_LUA_ERRMEM: {  /* memory error? */
      ___pdr_setsvalue2s(L, oldtop, ___pdr_G(L)->memerrmsg); /* reuse preregistered msg. */
      break;
    }
    case ___PDRLUA_ERRERR: {
      ___pdr_setsvalue2s(L, oldtop, ___pdr_luaS_newliteral(L, "error in error handling"));
      break;
    }
    default: {
      ___pdr_setobjs2s(L, oldtop, L->top - 1);  /* error message on current top */
      break;
    }
  }
  L->top = oldtop + 1;
}


___pdr_l_noret ___pdr_luaD_throw (___pdr_lua_State *L, int errcode) {
  if (L->errorJmp) {  /* thread has an error handler? */
    L->errorJmp->status = errcode;  /* set status */
    ___PDR_LUAI_THROW(L, L->errorJmp);  /* jump to it */
  }
  else {  /* thread has no error handler */
    ___pdr_global_State *g = ___pdr_G(L);
    L->status = ___pdr_cast_byte(errcode);  /* mark it as dead */
    if (g->mainthread->errorJmp) {  /* main thread has a handler? */
      ___pdr_setobjs2s(L, g->mainthread->top++, L->top - 1);  /* copy error obj. */
      ___pdr_luaD_throw(g->mainthread, errcode);  /* re-throw in main thread */
    }
    else {  /* no handler at all; abort */
      if (g->panic) {  /* panic function? */
        seterrorobj(L, errcode, L->top);  /* assume EXTRA_STACK */
        if (L->ci->top < L->top)
          L->ci->top = L->top;  /* pushing msg. can break this invariant */
        ___pdr_lua_unlock(L);
        g->panic(L);  /* call panic function (last chance to jump out) */
      }
      abort();
    }
  }
}


int ___pdr_luaD_rawrunprotected (___pdr_lua_State *L, ___pdr_Pfunc f, void *ud) {
  unsigned short oldnCcalls = L->nCcalls;
  struct ___pdr_lua_longjmp lj;
  lj.status = ___PDR_LUA_OK;
  lj.previous = L->errorJmp;  /* chain new error handler */
  L->errorJmp = &lj;
  ___PDR_LUAI_TRY(L, &lj,
    (*f)(L, ud);
  );
  L->errorJmp = lj.previous;  /* restore old error handler */
  L->nCcalls = oldnCcalls;
  return lj.status;
}

/* }====================================================== */


/*
** {==================================================================
** Stack reallocation
** ===================================================================
*/
static void correctstack (___pdr_lua_State *L, ___pdr_TValue *oldstack) {
  ___pdr_CallInfo *ci;
  ___pdr_UpVal *up;
  L->top = (L->top - oldstack) + L->stack;
  for (up = L->openupval; up != NULL; up = up->u.open.next)
    up->v = (up->v - oldstack) + L->stack;
  for (ci = L->ci; ci != NULL; ci = ci->previous) {
    ci->top = (ci->top - oldstack) + L->stack;
    ci->func = (ci->func - oldstack) + L->stack;
    if (___pdr_isLua(ci))
      ci->u.l.base = (ci->u.l.base - oldstack) + L->stack;
  }
}


/* some space for error handling */
#define ___PDR_ERRORSTACKSIZE	(___PDR_LUAI_MAXSTACK + 200)


void ___pdr_luaD_reallocstack (___pdr_lua_State *L, int newsize) {
  ___pdr_TValue *oldstack = L->stack;
  int lim = L->stacksize;
  ___pdr_lua_assert(newsize <= ___PDR_LUAI_MAXSTACK || newsize == ___PDR_ERRORSTACKSIZE);
  ___pdr_lua_assert(L->stack_last - L->stack == L->stacksize - ___PDR_EXTRA_STACK);
  ___pdr_luaM_reallocvector(L, L->stack, L->stacksize, newsize, ___pdr_TValue);
  for (; lim < newsize; lim++)
    ___pdr_setnilvalue(L->stack + lim); /* erase new segment */
  L->stacksize = newsize;
  L->stack_last = L->stack + newsize - ___PDR_EXTRA_STACK;
  correctstack(L, oldstack);
}


void ___pdr_luaD_growstack (___pdr_lua_State *L, int n) {
  int size = L->stacksize;
  if (size > ___PDR_LUAI_MAXSTACK)  /* error after extra size? */
    ___pdr_luaD_throw(L, ___PDRLUA_ERRERR);
  else {
    int needed = ___pdr_cast_int(L->top - L->stack) + n + ___PDR_EXTRA_STACK;
    int newsize = 2 * size;
    if (newsize > ___PDR_LUAI_MAXSTACK) newsize = ___PDR_LUAI_MAXSTACK;
    if (newsize < needed) newsize = needed;
    if (newsize > ___PDR_LUAI_MAXSTACK) {  /* stack overflow? */
      ___pdr_luaD_reallocstack(L, ___PDR_ERRORSTACKSIZE);
      ___pdr_luaG_runerror(L, "stack overflow");
    }
    else
      ___pdr_luaD_reallocstack(L, newsize);
  }
}


static int stackinuse (___pdr_lua_State *L) {
  ___pdr_CallInfo *ci;
  ___pdr_StkId lim = L->top;
  for (ci = L->ci; ci != NULL; ci = ci->previous) {
    if (lim < ci->top) lim = ci->top;
  }
  ___pdr_lua_assert(lim <= L->stack_last);
  return ___pdr_cast_int(lim - L->stack) + 1;  /* part of stack in use */
}


void ___pdr_luaD_shrinkstack (___pdr_lua_State *L) {
  int inuse = stackinuse(L);
  int goodsize = inuse + (inuse / 8) + 2*___PDR_EXTRA_STACK;
  if (goodsize > ___PDR_LUAI_MAXSTACK)
    goodsize = ___PDR_LUAI_MAXSTACK;  /* respect stack limit */
  if (L->stacksize > ___PDR_LUAI_MAXSTACK)  /* had been handling stack overflow? */
    ___pdr_luaE_freeCI(L);  /* free all CIs (list grew because of an error) */
  else
    ___pdr_luaE_shrinkCI(L);  /* shrink list */
  /* if thread is currently not handling a stack overflow and its
     good size is smaller than current size, shrink its stack */
  if (inuse <= (___PDR_LUAI_MAXSTACK - ___PDR_EXTRA_STACK) &&
      goodsize < L->stacksize)
    ___pdr_luaD_reallocstack(L, goodsize);
  else  /* don't change stack */
    ___pdr_condmovestack(L,{},{});  /* (change only for debugging) */
}


void ___pdr_luaD_inctop (___pdr_lua_State *L) {
  ___pdr_luaD_checkstack(L, 1);
  L->top++;
}

/* }================================================================== */


/*
** Call a hook for the given event. Make sure there is a hook to be
** called. (Both 'L->hook' and 'L->hookmask', which triggers this
** function, can be changed asynchronously by signals.)
*/
void ___pdr_luaD_hook (___pdr_lua_State *L, int event, int line) {
  ___pdr_lua_Hook hook = L->hook;
  if (hook && L->allowhook) {  /* make sure there is a hook */
    ___pdr_CallInfo *ci = L->ci;
    ptrdiff_t top = ___pdr_savestack(L, L->top);
    ptrdiff_t ci_top = ___pdr_savestack(L, ci->top);
    ___pdr_lua_Debug ar;
    ar.event = event;
    ar.currentline = line;
    ar.i_ci = ci;
    ___pdr_luaD_checkstack(L, ___PDR_LUA_MINSTACK);  /* ensure minimum stack size */
    ci->top = L->top + ___PDR_LUA_MINSTACK;
    ___pdr_lua_assert(ci->top <= L->stack_last);
    L->allowhook = 0;  /* cannot call hooks inside a hook */
    ci->callstatus |= ___PDR_CIST_HOOKED;
    ___pdr_lua_unlock(L);
    (*hook)(L, &ar);
    ___pdr_lua_lock(L);
    ___pdr_lua_assert(!L->allowhook);
    L->allowhook = 1;
    ci->top = ___pdr_restorestack(L, ci_top);
    L->top = ___pdr_restorestack(L, top);
    ci->callstatus &= ~___PDR_CIST_HOOKED;
  }
}


static void callhook (___pdr_lua_State *L, ___pdr_CallInfo *ci) {
  int hook = ___PDR_LUA_HOOKCALL;
  ci->u.l.savedpc++;  /* hooks assume 'pc' is already incremented */
  if (___pdr_isLua(ci->previous) &&
      ___PDR_GET_OPCODE(*(ci->previous->u.l.savedpc - 1)) == PDR_OP_TAILCALL) {
    ci->callstatus |= ___PDR_CIST_TAIL;
    hook = ___PDR_LUA_HOOKTAILCALL;
  }
  ___pdr_luaD_hook(L, hook, -1);
  ci->u.l.savedpc--;  /* correct 'pc' */
}


static ___pdr_StkId adjust_varargs (___pdr_lua_State *L, ___pdr_Proto *p, int actual) {
  int i;
  int nfixargs = p->numparams;
  ___pdr_StkId base, fixed;
  /* move fixed parameters to final position */
  fixed = L->top - actual;  /* first fixed argument */
  base = L->top;  /* final position of first argument */
  for (i = 0; i < nfixargs && i < actual; i++) {
    ___pdr_setobjs2s(L, L->top++, fixed + i);
    ___pdr_setnilvalue(fixed + i);  /* erase original copy (for GC) */
  }
  for (; i < nfixargs; i++)
    ___pdr_setnilvalue(L->top++);  /* complete missing arguments */
  return base;
}


/*
** Check whether __call metafield of 'func' is a function. If so, put
** it in stack below original 'func' so that 'luaD_precall' can call
** it. Raise an error if __call metafield is not a function.
*/
static void tryfuncTM (___pdr_lua_State *L, ___pdr_StkId func) {
  const ___pdr_TValue *tm = ___pdr_luaT_gettmbyobj(L, func, PDR_TM_CALL);
  ___pdr_StkId p;
  if (!___pdr_ttisfunction(tm))
    ___pdr_luaG_typeerror(L, func, "call");
  /* Open a hole inside the stack at 'func' */
  for (p = L->top; p > func; p--)
    ___pdr_setobjs2s(L, p, p-1);
  L->top++;  /* slot ensured by caller */
  ___pdr_setobj2s(L, func, tm);  /* tag method is the new function to be called */
}


/*
** Given 'nres' results at 'firstResult', move 'wanted' of them to 'res'.
** Handle most typical cases (zero results for commands, one result for
** expressions, multiple results for tail calls/single parameters)
** separated.
*/
static int moveresults (___pdr_lua_State *L, const ___pdr_TValue *firstResult, ___pdr_StkId res,
                                      int nres, int wanted) {
  switch (wanted) {  /* handle typical cases separately */
    case 0: break;  /* nothing to move */
    case 1: {  /* one result needed */
      if (nres == 0)   /* no results? */
        firstResult = ___pdr_luaO_nilobject;  /* adjust with nil */
      ___pdr_setobjs2s(L, res, firstResult);  /* move it to proper place */
      break;
    }
    case ___PDR_LUA_MULTRET: {
      int i;
      for (i = 0; i < nres; i++)  /* move all results to correct place */
        ___pdr_setobjs2s(L, res + i, firstResult + i);
      L->top = res + nres;
      return 0;  /* wanted == ___PDR_LUA_MULTRET */
    }
    default: {
      int i;
      if (wanted <= nres) {  /* enough results? */
        for (i = 0; i < wanted; i++)  /* move wanted results to correct place */
          ___pdr_setobjs2s(L, res + i, firstResult + i);
      }
      else {  /* not enough results; use all of them plus nils */
        for (i = 0; i < nres; i++)  /* move all results to correct place */
          ___pdr_setobjs2s(L, res + i, firstResult + i);
        for (; i < wanted; i++)  /* complete wanted number of results */
          ___pdr_setnilvalue(res + i);
      }
      break;
    }
  }
  L->top = res + wanted;  /* top points after the last result */
  return 1;
}


/*
** Finishes a function call: calls hook if necessary, removes CallInfo,
** moves current number of results to proper place; returns 0 iff call
** wanted multiple (variable number of) results.
*/
int ___pdr_luaD_poscall (___pdr_lua_State *L, ___pdr_CallInfo *ci, ___pdr_StkId firstResult, int nres) {
  ___pdr_StkId res;
  int wanted = ci->nresults;
  if (L->hookmask & (___PDR_LUA_MASKRET | ___PDR_LUA_MASKLINE)) {
    if (L->hookmask & ___PDR_LUA_MASKRET) {
      ptrdiff_t fr = ___pdr_savestack(L, firstResult);  /* hook may change stack */
      ___pdr_luaD_hook(L, ___PDR_LUA_HOOKRET, -1);
      firstResult = ___pdr_restorestack(L, fr);
    }
    L->oldpc = ci->previous->u.l.savedpc;  /* 'oldpc' for caller function */
  }
  res = ci->func;  /* res == final position of 1st result */
  L->ci = ci->previous;  /* back to caller */
  /* move results to proper place */
  return moveresults(L, firstResult, res, nres, wanted);
}



#define ___pdr_next_ci(L) (L->ci = (L->ci->next ? L->ci->next : ___pdr_luaE_extendCI(L)))


/* macro to check stack size, preserving 'p' */
#define ___pdr_checkstackp(L,n,p)  \
  ___pdr_luaD_checkstackaux(L, n, \
    ptrdiff_t t__ = ___pdr_savestack(L, p);  /* save 'p' */ \
    ___pdr_luaC_checkGC(L),  /* stack grow uses memory */ \
    p = ___pdr_restorestack(L, t__))  /* 'pos' part: restore 'p' */


/*
** Prepares a function call: checks the stack, creates a new CallInfo
** entry, fills in the relevant information, calls hook if needed.
** If function is a C function, does the call, too. (Otherwise, leave
** the execution ('luaV_execute') to the caller, to allow stackless
** calls.) Returns true iff function has been executed (C function).
*/
int ___pdr_luaD_precall (___pdr_lua_State *L, ___pdr_StkId func, int nresults) {
  ___pdr_lua_CFunction f;
  ___pdr_CallInfo *ci;
  switch (___pdr_ttype(func)) {
    case ___PDR_LUA_TCCL:  /* C closure */
      f = ___pdr_clCvalue(func)->f;
      goto Cfunc;
    case ___PDR_LUA_TLCF:  /* light C function */
      f = ___pdr_fvalue(func);
     Cfunc: {
      int n;  /* number of returns */
      ___pdr_checkstackp(L, ___PDR_LUA_MINSTACK, func);  /* ensure minimum stack size */
      ci = ___pdr_next_ci(L);  /* now 'enter' new function */
      ci->nresults = nresults;
      ci->func = func;
      ci->top = L->top + ___PDR_LUA_MINSTACK;
      ___pdr_lua_assert(ci->top <= L->stack_last);
      ci->callstatus = 0;
      if (L->hookmask & ___PDR_LUA_MASKCALL)
        ___pdr_luaD_hook(L, ___PDR_LUA_HOOKCALL, -1);
      ___pdr_lua_unlock(L);
      n = (*f)(L);  /* do the actual call */
      ___pdr_lua_lock(L);
      ___pdr_api_checknelems(L, n);
      ___pdr_luaD_poscall(L, ci, L->top - n, n);
      return 1;
    }
    case ___PDR_LUA_TLCL: {  /* Lua function: prepare its call */
      ___pdr_StkId base;
      ___pdr_Proto *p = ___pdr_clLvalue(func)->p;
      int n = ___pdr_cast_int(L->top - func) - 1;  /* number of real arguments */
      int fsize = p->maxstacksize;  /* frame size */
      ___pdr_checkstackp(L, fsize, func);
      if (p->is_vararg)
        base = adjust_varargs(L, p, n);
      else {  /* non vararg function */
        for (; n < p->numparams; n++)
          ___pdr_setnilvalue(L->top++);  /* complete missing arguments */
        base = func + 1;
      }
      ci = ___pdr_next_ci(L);  /* now 'enter' new function */
      ci->nresults = nresults;
      ci->func = func;
      ci->u.l.base = base;
      L->top = ci->top = base + fsize;
      ___pdr_lua_assert(ci->top <= L->stack_last);
      ci->u.l.savedpc = p->code;  /* starting point */
      ci->callstatus = ___PDR_CIST_LUA;
      if (L->hookmask & ___PDR_LUA_MASKCALL)
        callhook(L, ci);
      return 0;
    }
    default: {  /* not a function */
      ___pdr_checkstackp(L, 1, func);  /* ensure space for metamethod */
      tryfuncTM(L, func);  /* try to get '__call' metamethod */
      return ___pdr_luaD_precall(L, func, nresults);  /* now it must be a function */
    }
  }
}


/*
** Check appropriate error for stack overflow ("regular" overflow or
** overflow while handling stack overflow). If 'nCalls' is larger than
** LUAI_MAXCCALLS (which means it is handling a "regular" overflow) but
** smaller than 9/8 of LUAI_MAXCCALLS, does not report an error (to
** allow overflow handling to work)
*/
static void stackerror (___pdr_lua_State *L) {
  if (L->nCcalls == ___PDR_LUAI_MAXCCALLS)
    ___pdr_luaG_runerror(L, "C stack overflow");
  else if (L->nCcalls >= (___PDR_LUAI_MAXCCALLS + (___PDR_LUAI_MAXCCALLS>>3)))
    ___pdr_luaD_throw(L, ___PDRLUA_ERRERR);  /* error while handing stack error */
}


/*
** Call a function (C or Lua). The function to be called is at *func.
** The arguments are on the stack, right after the function.
** When returns, all the results are on the stack, starting at the original
** function position.
*/
void ___pdr_luaD_call (___pdr_lua_State *L, ___pdr_StkId func, int nResults) {
  if (++L->nCcalls >= ___PDR_LUAI_MAXCCALLS)
    stackerror(L);
  if (!___pdr_luaD_precall(L, func, nResults))  /* is a Lua function? */
    ___pdr_luaV_execute(L);  /* call it */
  L->nCcalls--;
}


/*
** Similar to 'luaD_call', but does not allow yields during the call
*/
void ___pdr_luaD_callnoyield (___pdr_lua_State *L, ___pdr_StkId func, int nResults) {
  L->nny++;
  ___pdr_luaD_call(L, func, nResults);
  L->nny--;
}


/*
** Completes the execution of an interrupted C function, calling its
** continuation function.
*/
static void finishCcall (___pdr_lua_State *L, int status) {
  ___pdr_CallInfo *ci = L->ci;
  int n;
  /* must have a continuation and must be able to call it */
  ___pdr_lua_assert(ci->u.c.k != NULL && L->nny == 0);
  /* error status can only happen in a protected call */
  ___pdr_lua_assert((ci->callstatus & ___PDR_CIST_YPCALL) || status == ___PDR_LUA_YIELD);
  if (ci->callstatus & ___PDR_CIST_YPCALL) {  /* was inside a pcall? */
    ci->callstatus &= ~___PDR_CIST_YPCALL;  /* continuation is also inside it */
    L->errfunc = ci->u.c.old_errfunc;  /* with the same error function */
  }
  /* finish 'lua_callk'/'lua_pcall'; CIST_YPCALL and 'errfunc' already
     handled */
  ___pdr_adjustresults(L, ci->nresults);
  ___pdr_lua_unlock(L);
  n = (*ci->u.c.k)(L, status, ci->u.c.ctx);  /* call continuation function */
  ___pdr_lua_lock(L);
  ___pdr_api_checknelems(L, n);
  ___pdr_luaD_poscall(L, ci, L->top - n, n);  /* finish 'luaD_precall' */
}


/*
** Executes "full continuation" (everything in the stack) of a
** previously interrupted coroutine until the stack is empty (or another
** interruption long-jumps out of the loop). If the coroutine is
** recovering from an error, 'ud' points to the error status, which must
** be passed to the first continuation function (otherwise the default
** status is LUA_YIELD).
*/
static void unroll (___pdr_lua_State *L, void *ud) {
  if (ud != NULL)  /* error status? */
    finishCcall(L, *(int *)ud);  /* finish 'lua_pcallk' callee */
  while (L->ci != &L->base_ci) {  /* something in the stack */
    if (!___pdr_isLua(L->ci))  /* C function? */
      finishCcall(L, ___PDR_LUA_YIELD);  /* complete its execution */
    else {  /* Lua function */
      ___pdr_luaV_finishOp(L);  /* finish interrupted instruction */
      ___pdr_luaV_execute(L);  /* execute down to higher C 'boundary' */
    }
  }
}


/*
** Try to find a suspended protected call (a "recover point") for the
** given thread.
*/
static ___pdr_CallInfo *findpcall (___pdr_lua_State *L) {
  ___pdr_CallInfo *ci;
  for (ci = L->ci; ci != NULL; ci = ci->previous) {  /* search for a pcall */
    if (ci->callstatus & ___PDR_CIST_YPCALL)
      return ci;
  }
  return NULL;  /* no pending pcall */
}


/*
** Recovers from an error in a coroutine. Finds a recover point (if
** there is one) and completes the execution of the interrupted
** 'luaD_pcall'. If there is no recover point, returns zero.
*/
static int recover (___pdr_lua_State *L, int status) {
  ___pdr_StkId oldtop;
  ___pdr_CallInfo *ci = findpcall(L);
  if (ci == NULL) return 0;  /* no recovery point */
  /* "finish" luaD_pcall */
  oldtop = ___pdr_restorestack(L, ci->extra);
  ___pdr_luaF_close(L, oldtop);
  seterrorobj(L, status, oldtop);
  L->ci = ci;
  L->allowhook = ___pdr_getoah(ci->callstatus);  /* restore original 'allowhook' */
  L->nny = 0;  /* should be zero to be yieldable */
  ___pdr_luaD_shrinkstack(L);
  L->errfunc = ci->u.c.old_errfunc;
  return 1;  /* continue running the coroutine */
}


/*
** Signal an error in the call to 'lua_resume', not in the execution
** of the coroutine itself. (Such errors should not be handled by any
** coroutine error handler and should not kill the coroutine.)
*/
static int resume_error (___pdr_lua_State *L, const char *msg, int narg) {
  L->top -= narg;  /* remove args from the stack */
  ___pdr_setsvalue2s(L, L->top, ___pdr_luaS_new(L, msg));  /* push error message */
  ___pdr_api_incr_top(L);
  ___pdr_lua_unlock(L);
  return ___PDR_LUA_ERRRUN;
}


/*
** Do the work for 'lua_resume' in protected mode. Most of the work
** depends on the status of the coroutine: initial state, suspended
** inside a hook, or regularly suspended (optionally with a continuation
** function), plus erroneous cases: non-suspended coroutine or dead
** coroutine.
*/
static void resume (___pdr_lua_State *L, void *ud) {
  int n = *(___pdr_cast(int*, ud));  /* number of arguments */
  ___pdr_StkId firstArg = L->top - n;  /* first argument */
  ___pdr_CallInfo *ci = L->ci;
  if (L->status == ___PDR_LUA_OK) {  /* starting a coroutine? */
    if (!___pdr_luaD_precall(L, firstArg - 1, ___PDR_LUA_MULTRET))  /* Lua function? */
      ___pdr_luaV_execute(L);  /* call it */
  }
  else {  /* resuming from previous yield */
    ___pdr_lua_assert(L->status == ___PDR_LUA_YIELD);
    L->status = ___PDR_LUA_OK;  /* mark that it is running (again) */
    ci->func = ___pdr_restorestack(L, ci->extra);
    if (___pdr_isLua(ci))  /* yielded inside a hook? */
      ___pdr_luaV_execute(L);  /* just continue running Lua code */
    else {  /* 'common' yield */
      if (ci->u.c.k != NULL) {  /* does it have a continuation function? */
        ___pdr_lua_unlock(L);
        n = (*ci->u.c.k)(L, ___PDR_LUA_YIELD, ci->u.c.ctx); /* call continuation */
        ___pdr_lua_lock(L);
        ___pdr_api_checknelems(L, n);
        firstArg = L->top - n;  /* yield results come from continuation */
      }
      ___pdr_luaD_poscall(L, ci, firstArg, n);  /* finish 'luaD_precall' */
    }
    unroll(L, NULL);  /* run continuation */
  }
}


___PDR_LUA_API int ___pdr_lua_resume (___pdr_lua_State *L, ___pdr_lua_State *from, int nargs) {
  int status;
  unsigned short oldnny = L->nny;  /* save "number of non-yieldable" calls */
  ___pdr_lua_lock(L);
  if (L->status == ___PDR_LUA_OK) {  /* may be starting a coroutine */
    if (L->ci != &L->base_ci)  /* not in base level? */
      return resume_error(L, "cannot resume non-suspended coroutine", nargs);
  }
  else if (L->status != ___PDR_LUA_YIELD)
    return resume_error(L, "cannot resume dead coroutine", nargs);
  L->nCcalls = (from) ? from->nCcalls + 1 : 1;
  if (L->nCcalls >= ___PDR_LUAI_MAXCCALLS)
    return resume_error(L, "C stack overflow", nargs);
  ___pdr_luai_userstateresume(L, nargs);
  L->nny = 0;  /* allow yields */
  ___pdr_api_checknelems(L, (L->status == ___PDR_LUA_OK) ? nargs + 1 : nargs);
  status = ___pdr_luaD_rawrunprotected(L, resume, &nargs);
  if (status == -1)  /* error calling 'lua_resume'? */
    status = ___PDR_LUA_ERRRUN;
  else {  /* continue running after recoverable errors */
    while (___pdr_errorstatus(status) && recover(L, status)) {
      /* unroll continuation */
      status = ___pdr_luaD_rawrunprotected(L, unroll, &status);
    }
    if (___pdr_errorstatus(status)) {  /* unrecoverable error? */
      L->status = ___pdr_cast_byte(status);  /* mark thread as 'dead' */
      seterrorobj(L, status, L->top);  /* push error message */
      L->ci->top = L->top;
    }
    else ___pdr_lua_assert(status == L->status);  /* normal end or yield */
  }
  L->nny = oldnny;  /* restore 'nny' */
  L->nCcalls--;
  ___pdr_lua_assert(L->nCcalls == ((from) ? from->nCcalls : 0));
  ___pdr_lua_unlock(L);
  return status;
}


___PDR_LUA_API int ___pdr_lua_isyieldable (___pdr_lua_State *L) {
  return (L->nny == 0);
}


___PDR_LUA_API int ___pdr_lua_yieldk (___pdr_lua_State *L, int nresults, ___pdr_lua_KContext ctx,
                        ___pdr_lua_KFunction k) {
  ___pdr_CallInfo *ci = L->ci;
  ___pdr_luai_userstateyield(L, nresults);
  ___pdr_lua_lock(L);
  ___pdr_api_checknelems(L, nresults);
  if (L->nny > 0) {
    if (L != ___pdr_G(L)->mainthread)
      ___pdr_luaG_runerror(L, "attempt to yield across a C-call boundary");
    else
      ___pdr_luaG_runerror(L, "attempt to yield from outside a coroutine");
  }
  L->status = ___PDR_LUA_YIELD;
  ci->extra = ___pdr_savestack(L, ci->func);  /* save current 'func' */
  if (___pdr_isLua(ci)) {  /* inside a hook? */
    ___pdr_api_check(L, k == NULL, "hooks cannot continue after yielding");
  }
  else {
    if ((ci->u.c.k = k) != NULL)  /* is there a continuation? */
      ci->u.c.ctx = ctx;  /* save context */
    ci->func = L->top - nresults - 1;  /* protect stack below results */
    ___pdr_luaD_throw(L, ___PDR_LUA_YIELD);
  }
  ___pdr_lua_assert(ci->callstatus & ___PDR_CIST_HOOKED);  /* must be inside a hook */
  ___pdr_lua_unlock(L);
  return 0;  /* return to 'luaD_hook' */
}


int ___pdr_luaD_pcall (___pdr_lua_State *L, ___pdr_Pfunc func, void *u,
                ptrdiff_t old_top, ptrdiff_t ef) {
  int status;
  ___pdr_CallInfo *old_ci = L->ci;
  ___pdr_lu_byte old_allowhooks = L->allowhook;
  unsigned short old_nny = L->nny;
  ptrdiff_t old_errfunc = L->errfunc;
  L->errfunc = ef;
  status = ___pdr_luaD_rawrunprotected(L, func, u);
  if (status != ___PDR_LUA_OK) {  /* an error occurred? */
    ___pdr_StkId oldtop = ___pdr_restorestack(L, old_top);
    ___pdr_luaF_close(L, oldtop);  /* close possible pending closures */
    seterrorobj(L, status, oldtop);
    L->ci = old_ci;
    L->allowhook = old_allowhooks;
    L->nny = old_nny;
    ___pdr_luaD_shrinkstack(L);
  }
  L->errfunc = old_errfunc;
  return status;
}



/*
** Execute a protected parser.
*/
struct SParser {  /* data to 'f_parser' */
  ___pdr_ZIO *z;
  ___pdr_Mbuffer buff;  /* dynamic structure used by the scanner */
  ___pdr_Dyndata dyd;  /* dynamic structures used by the parser */
  const char *mode;
  const char *name;
};


static void checkmode (___pdr_lua_State *L, const char *mode, const char *x) {
  if (mode && strchr(mode, x[0]) == NULL) {
    ___pdr_luaO_pushfstring(L,
       "attempt to load a %s chunk (mode is '%s')", x, mode);
    ___pdr_luaD_throw(L, ___PDR_LUA_ERRSYNTAX);
  }
}


static void f_parser (___pdr_lua_State *L, void *ud) {
  ___pdr_LClosure *cl;
  struct SParser *p = ___pdr_cast(struct SParser *, ud);
  if (L->onlyluac == 0) {
    int c = ___pdr_zgetc(p->z); /* read first character */
    if (c == ___PDR_LUA_SIGNATURE[0]) {
      checkmode(L, p->mode, "binary");
      cl = ___pdr_luaU_undump(L, p->z, p->name);
    } else {
      checkmode(L, p->mode, "text");
      cl = ___pdr_luaY_parser(L, p->z, &p->buff, &p->dyd, p->name, c);
    }
  } else {
    checkmode(L, p->mode, "binary");
    cl = ___pdr_luaU_undump(L, p->z, p->name);
  }
  ___pdr_lua_assert(cl->nupvalues == cl->p->sizeupvalues);
  ___pdr_luaF_initupvals(L, cl);
}


int ___pdr_luaD_protectedparser (___pdr_lua_State *L, ___pdr_ZIO *z, const char *name,
                                        const char *mode) {
  struct SParser p;
  int status;
  L->nny++;  /* cannot yield during parsing */
  p.z = z; p.name = name; p.mode = mode;
  p.dyd.actvar.arr = NULL; p.dyd.actvar.size = 0;
  p.dyd.gt.arr = NULL; p.dyd.gt.size = 0;
  p.dyd.label.arr = NULL; p.dyd.label.size = 0;
  ___pdr_luaZ_initbuffer(L, &p.buff);
  status = ___pdr_luaD_pcall(L, f_parser, &p, ___pdr_savestack(L, L->top), L->errfunc);
  ___pdr_luaZ_freebuffer(L, &p.buff);
  ___pdr_luaM_freearray(L, p.dyd.actvar.arr, p.dyd.actvar.size);
  ___pdr_luaM_freearray(L, p.dyd.gt.arr, p.dyd.gt.size);
  ___pdr_luaM_freearray(L, p.dyd.label.arr, p.dyd.label.size);
  L->nny--;
  return status;
}

} // end NS_PDR_SLUA