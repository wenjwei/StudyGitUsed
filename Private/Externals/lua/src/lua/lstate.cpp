/*
** $Id: lstate.c,v 2.133 2015/11/13 12:16:51 roberto Exp $
** Global State
** See Copyright Notice in lua.h
*/

#define ___pdr_lstate_c
#define ___PDR_LUA_CORE

#include "lstate.h"
#include "lprefix.h"

#include <stddef.h>
#include <string.h>

#include "lua.h"
#include "lapi.h"
#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "llex.h"
#include "lmem.h"
#include "lstring.h"
#include "ltable.h"
#include "ltm.h"

#if !defined(___PDR_LUAI_GCPAUSE)
#define ___PDR_LUAI_GCPAUSE	200  /* 200% */
#endif

#if !defined(___PDR_LUAI_GCMUL)
#define ___PDR_LUAI_GCMUL	200 /* GC runs 'twice the speed' of memory allocation */
#endif


/*
** a macro to help the creation of a unique random seed when a state is
** created; the seed is used to randomize hashes.
*/
#if !defined(___pdr_luai_makeseed)
#include <time.h>
#define ___pdr_luai_makeseed()		___pdr_cast(unsigned int, time(NULL))
#endif

namespace NS_PDR_SLUA {

/*
** thread state + extra space
*/
typedef struct LX {
  ___pdr_lu_byte extra_[___PDR_LUA_EXTRASPACE];
  ___pdr_lua_State l;
} LX;


/*
** Main thread combines a thread state and the global state
*/
typedef struct LG {
  LX l;
  ___pdr_global_State g;
} LG;



#define ___pdr_fromstate(L)	(___pdr_cast(LX *, ___pdr_cast(___pdr_lu_byte *, (L)) - offsetof(LX, l)))


/*
** Compute an initial seed as random as possible. Rely on Address Space
** Layout Randomization (if present) to increase randomness..
*/
#define ___pdr_addbuff(b,p,e) \
  { size_t t = ___pdr_cast(size_t, e); \
    memcpy(b + p, &t, sizeof(t)); p += sizeof(t); }

static unsigned int makeseed (___pdr_lua_State *L) {
  char buff[4 * sizeof(size_t)];
  unsigned int h = ___pdr_luai_makeseed();
  int p = 0;
  ___pdr_addbuff(buff, p, L);  /* heap variable */
  ___pdr_addbuff(buff, p, &h);  /* local variable */
  ___pdr_addbuff(buff, p, ___pdr_luaO_nilobject);  /* global variable */
  ___pdr_addbuff(buff, p, &___pdr_lua_newstate);  /* public function */
  ___pdr_lua_assert(p == sizeof(buff));
  return ___pdr_luaS_hash(buff, p, h);
}


/*
** set GCdebt to a new value keeping the value (totalbytes + GCdebt)
** invariant (and avoiding underflows in 'totalbytes')
*/
void ___pdr_luaE_setdebt (___pdr_global_State *g, ___pdr_l_mem debt) {
  ___pdr_l_mem tb = ___pdr_gettotalbytes(g);
  ___pdr_lua_assert(tb > 0);
  if (debt < tb - ___PDR_MAX_LMEM)
    debt = tb - ___PDR_MAX_LMEM;  /* will make 'totalbytes == MAX_LMEM' */
  g->totalbytes = tb - debt;
  g->GCdebt = debt;
}


___pdr_CallInfo *___pdr_luaE_extendCI (___pdr_lua_State *L) {
  ___pdr_CallInfo *ci = ___pdr_luaM_new(L, ___pdr_CallInfo);
  ___pdr_lua_assert(L->ci->next == NULL);
  L->ci->next = ci;
  ci->previous = L->ci;
  ci->next = NULL;
  L->nci++;
  return ci;
}


/*
** free all CallInfo structures not in use by a thread
*/
void ___pdr_luaE_freeCI (___pdr_lua_State *L) {
  ___pdr_CallInfo *ci = L->ci;
  ___pdr_CallInfo *next = ci->next;
  ci->next = NULL;
  while ((ci = next) != NULL) {
    next = ci->next;
    ___pdr_luaM_free(L, ci);
    L->nci--;
  }
}


/*
** free half of the CallInfo structures not in use by a thread
*/
void ___pdr_luaE_shrinkCI (___pdr_lua_State *L) {
  ___pdr_CallInfo *ci = L->ci;
  ___pdr_CallInfo *next2;  /* next's next */
  /* while there are two nexts */
  while (ci->next != NULL && (next2 = ci->next->next) != NULL) {
    ___pdr_luaM_free(L, ci->next);  /* free next */
    L->nci--;
    ci->next = next2;  /* remove 'next' from the list */
    next2->previous = ci;
    ci = next2;  /* keep next's next */
  }
}


static void stack_init (___pdr_lua_State *L1, ___pdr_lua_State *L) {
  int i; ___pdr_CallInfo *ci;
  /* initialize stack array */
  L1->stack = ___pdr_luaM_newvector(L, ___PDR_BASIC_STACK_SIZE, ___pdr_TValue);
  L1->stacksize = ___PDR_BASIC_STACK_SIZE;
  for (i = 0; i < ___PDR_BASIC_STACK_SIZE; i++)
    ___pdr_setnilvalue(L1->stack + i);  /* erase new stack */
  L1->top = L1->stack;
  L1->stack_last = L1->stack + L1->stacksize - ___PDR_EXTRA_STACK;
  /* initialize first ci */
  ci = &L1->base_ci;
  ci->next = ci->previous = NULL;
  ci->callstatus = 0;
  ci->func = L1->top;
  ___pdr_setnilvalue(L1->top++);  /* 'function' entry for this 'ci' */
  ci->top = L1->top + ___PDR_LUA_MINSTACK;
  L1->ci = ci;
}


static void freestack (___pdr_lua_State *L) {
  if (L->stack == NULL)
    return;  /* stack not completely built yet */
  L->ci = &L->base_ci;  /* free the entire 'ci' list */
  ___pdr_luaE_freeCI(L);
  ___pdr_lua_assert(L->nci == 0);
  ___pdr_luaM_freearray(L, L->stack, L->stacksize);  /* free stack array */
}


/*
** Create registry table and its predefined values
*/
static void init_registry (___pdr_lua_State *L, ___pdr_global_State *g) {
  ___pdr_TValue temp;
  /* create registry */
  ___pdr_Table *registry = ___pdr_luaH_new(L);
  ___pdr_sethvalue(L, &g->l_registry, registry);
  ___pdr_luaH_resize(L, registry, ___PDR_LUA_RIDX_LAST, 0);
  /* registry[LUA_RIDX_MAINTHREAD] = L */
  ___pdr_setthvalue(L, &temp, L);  /* temp = L */
  ___pdr_luaH_setint(L, registry, ___PDR_LUA_RIDX_MAINTHREAD, &temp);
  /* registry[LUA_RIDX_GLOBALS] = table of globals */
  ___pdr_sethvalue(L, &temp, ___pdr_luaH_new(L));  /* temp = new table (global table) */
  ___pdr_luaH_setint(L, registry, ___PDR_LUA_RIDX_GLOBALS, &temp);
}


/*
** open parts of the state that may cause memory-allocation errors.
** ('g->version' != NULL flags that the state was completely build)
*/
static void f_luaopen (___pdr_lua_State *L, void *ud) {
  ___pdr_global_State *g = ___pdr_G(L);
  ___PDR_UNUSED(ud);
  stack_init(L, L);  /* init stack */
  init_registry(L, g);
  ___pdr_luaS_init(L);
  ___pdr_luaT_init(L);
  ___pdr_luaX_init(L);
  g->gcrunning = 1;  /* allow gc */
  g->version = ___pdr_lua_version(NULL);
  ___pdr_luai_userstateopen(L);
}


/*
** preinitialize a thread with consistent values without allocating
** any memory (to avoid errors)
*/
static void preinit_thread (___pdr_lua_State *L, ___pdr_global_State *g) {
  L->l_G = g;
  L->stack = NULL;
  L->ci = NULL;
  L->nci = 0;
  L->stacksize = 0;
  L->twups = L;  /* thread has no upvalues */
  L->errorJmp = NULL;
  L->nCcalls = 0;
  L->hook = NULL;
  L->hookmask = 0;
  L->basehookcount = 0;
  L->allowhook = 1;
  ___pdr_resethookcount(L);
  L->openupval = NULL;
  L->nny = 1;
  L->status = ___PDR_LUA_OK;
  L->errfunc = 0;
  L->onlyluac = 0;
}

___PDR_LUA_API void ___pdr_lua_setonlyluac(___pdr_lua_State *L, int v) {
	L->onlyluac = v;
}


static void close_state (___pdr_lua_State *L) {
  ___pdr_global_State *g = ___pdr_G(L);
  ___pdr_luaF_close(L, L->stack);  /* close all upvalues for this thread */
  ___pdr_luaC_freeallobjects(L);  /* collect all objects */
  if (g->version)  /* closing a fully built state? */
    ___pdr_luai_userstateclose(L);
  ___pdr_luaM_freearray(L, ___pdr_G(L)->strt.hash, ___pdr_G(L)->strt.size);
  freestack(L);
  ___pdr_lua_assert(___pdr_gettotalbytes(g) == sizeof(LG));
  (*g->frealloc)(g->ud, ___pdr_fromstate(L), sizeof(LG), 0);  /* free main block */
}


___PDR_LUA_API ___pdr_lua_State *___pdr_lua_newthread (___pdr_lua_State *L) {
  ___pdr_global_State *g = ___pdr_G(L);
  ___pdr_lua_State *L1;
  ___pdr_lua_lock(L);
  ___pdr_luaC_checkGC(L);
  /* create new thread */
  L1 = &___pdr_cast(LX *, ___pdr_luaM_newobject(L, ___PDR_LUA_TTHREAD, sizeof(LX)))->l;
  L1->marked = ___pdr_luaC_white(g);
  L1->tt = ___PDR_LUA_TTHREAD;
  /* link it on list 'allgc' */
  L1->next = g->allgc;
  g->allgc = ___pdr_obj2gco(L1);
  /* anchor it on L stack */
  ___pdr_setthvalue(L, L->top, L1);
  ___pdr_api_incr_top(L);
  preinit_thread(L1, g);
  L1->hookmask = L->hookmask;
  L1->basehookcount = L->basehookcount;
  L1->hook = L->hook;
  ___pdr_resethookcount(L1);
  /* initialize L1 extra space */
  memcpy(___pdr_lua_getextraspace(L1), ___pdr_lua_getextraspace(g->mainthread),
         ___PDR_LUA_EXTRASPACE);
  ___pdr_luai_userstatethread(L, L1);
  stack_init(L1, L);  /* init stack */
  ___pdr_lua_unlock(L);
  return L1;
}


void ___pdr_luaE_freethread (___pdr_lua_State *L, ___pdr_lua_State *L1) {
  LX *l = ___pdr_fromstate(L1);
  ___pdr_luaF_close(L1, L1->stack);  /* close all upvalues for this thread */
  ___pdr_lua_assert(L1->openupval == NULL);
  ___pdr_luai_userstatefree(L, L1);
  freestack(L1);
  ___pdr_luaM_free(L, l);
}


___PDR_LUA_API ___pdr_lua_State *___pdr_lua_newstate (___pdr_lua_Alloc f, void *ud) {
  int i;
  ___pdr_lua_State *L;
  ___pdr_global_State *g;
  LG *l = ___pdr_cast(LG *, (*f)(ud, NULL, ___PDR_LUA_TTHREAD, sizeof(LG)));
  if (l == NULL) return NULL;
  L = &l->l.l;
  g = &l->g;
  L->next = NULL;
  L->tt = ___PDR_LUA_TTHREAD;
  g->currentwhite = ___pdr_bitmask(___PDR_WHITE0BIT);
  L->marked = ___pdr_luaC_white(g);
  preinit_thread(L, g);
  g->frealloc = f;
  g->ud = ud;
  g->mainthread = L;
  g->seed = makeseed(L);
  g->gcrunning = 0;  /* no GC while building state */
  g->GCestimate = 0;
  g->strt.size = g->strt.nuse = 0;
  g->strt.hash = NULL;
  ___pdr_setnilvalue(&g->l_registry);
  g->panic = NULL;
  g->version = NULL;
  g->gcstate = ___pdr_GCSpause;
  g->gckind = ___PDR_KGC_NORMAL;
  g->allgc = g->finobj = g->tobefnz = g->fixedgc = NULL;
  g->sweepgc = NULL;
  g->gray = g->grayagain = NULL;
  g->weak = g->ephemeron = g->allweak = NULL;
  g->twups = NULL;
  g->totalbytes = sizeof(LG);
  g->GCdebt = 0;
  g->gcfinnum = 0;
  g->gcpause = ___PDR_LUAI_GCPAUSE;
  g->gcstepmul = ___PDR_LUAI_GCMUL;
  for (i=0; i < ___PDR_LUA_NUMTAGS; i++) g->mt[i] = NULL;
  if (___pdr_luaD_rawrunprotected(L, f_luaopen, NULL) != ___PDR_LUA_OK) {
    /* memory allocation error: free partial state */
    close_state(L);
    L = NULL;
  }
  return L;
}


___PDR_LUA_API void ___pdr_lua_close (___pdr_lua_State *L) {
  L = ___pdr_G(L)->mainthread;  /* only the main thread can be closed */
  ___pdr_lua_lock(L);
  close_state(L);
}

} // end NS_PDR_SLUA
