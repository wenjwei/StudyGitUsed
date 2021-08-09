/*
** $Id: lgc.c,v 2.215 2016/12/22 13:08:50 roberto Exp $
** Garbage Collector
** See Copyright Notice in lua.h
*/

#define ___pdr_lgc_c
#define ___PDR_LUA_CORE

#include "lgc.h"
#include "lprefix.h"

#include <string.h>

#include "lua.h"
#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "ltm.h"

namespace NS_PDR_SLUA {

/*
** internal state for collector while inside the atomic phase. The
** collector should never be in this state while running regular code.
*/
#define ___pdr_GCSinsideatomic		(___pdr_GCSpause + 1)

/*
** cost of sweeping one element (the size of a small object divided
** by some adjust for the sweep speed)
*/
#define ___PDR_GCSWEEPCOST	((sizeof(___pdr_TString) + 4) / 4)

/* maximum number of elements to sweep in each single step */
#define ___PDR_GCSWEEPMAX	(___pdr_cast_int((___PDR_GCSTEPSIZE / ___PDR_GCSWEEPCOST) / 4))

/* cost of calling one finalizer */
#define ___PDR_GCFINALIZECOST	___PDR_GCSWEEPCOST


/*
** macro to adjust 'stepmul': 'stepmul' is actually used like
** 'stepmul / STEPMULADJ' (value chosen by tests)
*/
#define ___PDR_STEPMULADJ		200


/*
** macro to adjust 'pause': 'pause' is actually used like
** 'pause / PAUSEADJ' (value chosen by tests)
*/
#define ___PDR_PAUSEADJ		100


/*
** 'makewhite' erases all color bits then sets only the current white
** bit
*/
#define ___pdr_maskcolors	(~(___pdr_bitmask(___PDR_BLACKBIT) | ___PDR_WHITEBITS))
#define ___pdr_makewhite(g,x)	\
 (x->marked = ___pdr_cast_byte((x->marked & ___pdr_maskcolors) | ___pdr_luaC_white(g)))

#define ___pdr_white2gray(x)	___pdr_resetbits(x->marked, ___PDR_WHITEBITS)
#define ___pdr_black2gray(x)	___pdr_resetbit(x->marked, ___PDR_BLACKBIT)


#define ___pdr_valiswhite(x)   (___pdr_iscollectable(x) && ___pdr_iswhite(___pdr_gcvalue(x)))

#define ___pdr_checkdeadkey(n)	___pdr_lua_assert(!___pdr_ttisdeadkey(___pdr_gkey(n)) || ___pdr_ttisnil(___pdr_gval(n)))


#define ___pdr_checkconsistency(obj)  \
  ___pdr_lua_longassert(!___pdr_iscollectable(obj) || ___pdr_righttt(obj))


#define ___pdr_markvalue(g,o) { ___pdr_checkconsistency(o); \
  if (___pdr_valiswhite(o)) reallymarkobject(g,___pdr_gcvalue(o)); }

#define ___pdr_markobject(g,t)	{ if (___pdr_iswhite(t)) reallymarkobject(g, ___pdr_obj2gco(t)); }

/*
** mark an object that can be NULL (either because it is really optional,
** or it was stripped as debug info, or inside an uncompleted structure)
*/
#define ___pdr_markobjectN(g,t)	{ if (t) ___pdr_markobject(g,t); }

static void reallymarkobject (___pdr_global_State *g, ___pdr_GCObject *o);


/*
** {======================================================
** Generic functions
** =======================================================
*/


/*
** one after last element in a hash array
*/
#define ___pdr_gnodelast(h)	___pdr_gnode(h, ___pdr_cast(size_t, ___pdr_sizenode(h)))


/*
** link collectable object 'o' into list pointed by 'p'
*/
#define ___pdr_linkgclist(o,p)	((o)->gclist = (p), (p) = ___pdr_obj2gco(o))


/*
** If key is not marked, mark its entry as dead. This allows key to be
** collected, but keeps its entry in the table.  A dead node is needed
** when Lua looks up for a key (it may be part of a chain) and when
** traversing a weak table (key might be removed from the table during
** traversal). Other places never manipulate dead keys, because its
** associated nil value is enough to signal that the entry is logically
** empty.
*/
static void removeentry (___pdr_Node *n) {
  ___pdr_lua_assert(___pdr_ttisnil(___pdr_gval(n)));
  if (___pdr_valiswhite(___pdr_gkey(n)))
    ___pdr_setdeadvalue(___pdr_wgkey(n));  /* unused and unmarked key; remove it */
}


/*
** tells whether a key or value can be cleared from a weak
** table. Non-collectable objects are never removed from weak
** tables. Strings behave as 'values', so are never removed too. for
** other objects: if really collected, cannot keep them; for objects
** being finalized, keep them in keys, but not in values
*/
static int iscleared (___pdr_global_State *g, const ___pdr_TValue *o) {
  if (!___pdr_iscollectable(o)) return 0;
  else if (___pdr_ttisstring(o)) {
    ___pdr_markobject(g, ___pdr_tsvalue(o));  /* strings are 'values', so are never weak */
    return 0;
  }
  else return ___pdr_iswhite(___pdr_gcvalue(o));
}


/*
** barrier that moves collector forward, that is, mark the white object
** being pointed by a black object. (If in sweep phase, clear the black
** object to white [sweep it] to avoid other barrier calls for this
** same object.)
*/
void ___pdr_luaC_barrier_ (___pdr_lua_State *L, ___pdr_GCObject *o, ___pdr_GCObject *v) {
  ___pdr_global_State *g = ___pdr_G(L);
  ___pdr_lua_assert(___pdr_isblack(o) && ___pdr_iswhite(v) && !___pdr_isdead(g, v) && !___pdr_isdead(g, o));
  if (___pdr_keepinvariant(g))  /* must keep invariant? */
    reallymarkobject(g, v);  /* restore invariant */
  else {  /* sweep phase */
    ___pdr_lua_assert(___pdr_issweepphase(g));
    ___pdr_makewhite(g, o);  /* mark main obj. as white to avoid other barriers */
  }
}


/*
** barrier that moves collector backward, that is, mark the black object
** pointing to a white object as gray again.
*/
void ___pdr_luaC_barrierback_ (___pdr_lua_State *L, ___pdr_Table *t) {
  ___pdr_global_State *g = ___pdr_G(L);
  ___pdr_lua_assert(___pdr_isblack(t) && !___pdr_isdead(g, t));
  ___pdr_black2gray(t);  /* make table gray (again) */
  ___pdr_linkgclist(t, g->grayagain);
}


/*
** barrier for assignments to closed upvalues. Because upvalues are
** shared among closures, it is impossible to know the color of all
** closures pointing to it. So, we assume that the object being assigned
** must be marked.
*/
void ___pdr_luaC_upvalbarrier_ (___pdr_lua_State *L, ___pdr_UpVal *uv) {
  ___pdr_global_State *g = ___pdr_G(L);
  ___pdr_GCObject *o = ___pdr_gcvalue(uv->v);
  ___pdr_lua_assert(!___pdr_upisopen(uv));  /* ensured by macro luaC_upvalbarrier */
  if (___pdr_keepinvariant(g))
    ___pdr_markobject(g, o);
}


void ___pdr_luaC_fix (___pdr_lua_State *L, ___pdr_GCObject *o) {
  ___pdr_global_State *g = ___pdr_G(L);
  ___pdr_lua_assert(g->allgc == o);  /* object must be 1st in 'allgc' list! */
  ___pdr_white2gray(o);  /* they will be gray forever */
  g->allgc = o->next;  /* remove object from 'allgc' list */
  o->next = g->fixedgc;  /* link it to 'fixedgc' list */
  g->fixedgc = o;
}


/*
** create a new collectable object (with given type and size) and link
** it to 'allgc' list.
*/
___pdr_GCObject *___pdr_luaC_newobj (___pdr_lua_State *L, int tt, size_t sz) {
  ___pdr_global_State *g = ___pdr_G(L);
  ___pdr_GCObject *o = ___pdr_cast(___pdr_GCObject *, ___pdr_luaM_newobject(L, ___pdr_novariant(tt), sz));
  o->marked = ___pdr_luaC_white(g);
  o->tt = tt;
  o->next = g->allgc;
  g->allgc = o;
  return o;
}

/* }====================================================== */



/*
** {======================================================
** Mark functions
** =======================================================
*/


/*
** mark an object. Userdata, strings, and closed upvalues are visited
** and turned black here. Other objects are marked gray and added
** to appropriate list to be visited (and turned black) later. (Open
** upvalues are already linked in 'headuv' list.)
*/
static void reallymarkobject (___pdr_global_State *g, ___pdr_GCObject *o) {
 reentry:
  ___pdr_white2gray(o);
  switch (o->tt) {
    case ___PDR_LUA_TSHRSTR: {
      ___pdr_gray2black(o);
      g->GCmemtrav += ___pdr_sizelstring(___pdr_gco2ts(o)->shrlen);
      break;
    }
    case ___PDR_LUA_TLNGSTR: {
      ___pdr_gray2black(o);
      g->GCmemtrav += ___pdr_sizelstring(___pdr_gco2ts(o)->u.lnglen);
      break;
    }
    case ___PDR_LUA_TUSERDATA: {
      ___pdr_TValue uvalue;
      ___pdr_markobjectN(g, ___pdr_gco2u(o)->metatable);  /* mark its metatable */
      ___pdr_gray2black(o);
      g->GCmemtrav += ___pdr_sizeudata(___pdr_gco2u(o));
      ___pdr_getuservalue(g->mainthread, ___pdr_gco2u(o), &uvalue);
      if (___pdr_valiswhite(&uvalue)) {  /* markvalue(g, &uvalue); */
        o = ___pdr_gcvalue(&uvalue);
        goto reentry;
      }
      break;
    }
    case ___PDR_LUA_TLCL: {
      ___pdr_linkgclist(___pdr_gco2lcl(o), g->gray);
      break;
    }
    case ___PDR_LUA_TCCL: {
      ___pdr_linkgclist(___pdr_gco2ccl(o), g->gray);
      break;
    }
    case ___PDR_LUA_TTABLE: {
      ___pdr_linkgclist(___pdr_gco2t(o), g->gray);
      break;
    }
    case ___PDR_LUA_TTHREAD: {
      ___pdr_linkgclist(___pdr_gco2th(o), g->gray);
      break;
    }
    case ___PDR_LUA_TPROTO: {
      ___pdr_linkgclist(___pdr_gco2p(o), g->gray);
      break;
    }
    default: ___pdr_lua_assert(0); break;
  }
}


/*
** mark metamethods for basic types
*/
static void markmt (___pdr_global_State *g) {
  int i;
  for (i=0; i < ___PDR_LUA_NUMTAGS; i++)
    ___pdr_markobjectN(g, g->mt[i]);
}


/*
** mark all objects in list of being-finalized
*/
static void markbeingfnz (___pdr_global_State *g) {
  ___pdr_GCObject *o;
  for (o = g->tobefnz; o != NULL; o = o->next)
    ___pdr_markobject(g, o);
}


/*
** Mark all values stored in marked open upvalues from non-marked threads.
** (Values from marked threads were already marked when traversing the
** thread.) Remove from the list threads that no longer have upvalues and
** not-marked threads.
*/
static void remarkupvals (___pdr_global_State *g) {
  ___pdr_lua_State *thread;
  ___pdr_lua_State **p = &g->twups;
  while ((thread = *p) != NULL) {
    ___pdr_lua_assert(!___pdr_isblack(thread));  /* threads are never black */
    if (___pdr_isgray(thread) && thread->openupval != NULL)
      p = &thread->twups;  /* keep marked thread with upvalues in the list */
    else {  /* thread is not marked or without upvalues */
      ___pdr_UpVal *uv;
      *p = thread->twups;  /* remove thread from the list */
      thread->twups = thread;  /* mark that it is out of list */
      for (uv = thread->openupval; uv != NULL; uv = uv->u.open.next) {
        if (uv->u.open.touched) {
          ___pdr_markvalue(g, uv->v);  /* remark upvalue's value */
          uv->u.open.touched = 0;
        }
      }
    }
  }
}


/*
** mark root set and reset all gray lists, to start a new collection
*/
static void restartcollection (___pdr_global_State *g) {
  g->gray = g->grayagain = NULL;
  g->weak = g->allweak = g->ephemeron = NULL;
  ___pdr_markobject(g, g->mainthread);
  ___pdr_markvalue(g, &g->l_registry);
  markmt(g);
  markbeingfnz(g);  /* mark any finalizing object left from previous cycle */
}

/* }====================================================== */


/*
** {======================================================
** Traverse functions
** =======================================================
*/

/*
** Traverse a table with weak values and link it to proper list. During
** propagate phase, keep it in 'grayagain' list, to be revisited in the
** atomic phase. In the atomic phase, if table has any white value,
** put it in 'weak' list, to be cleared.
*/
static void traverseweakvalue (___pdr_global_State *g, ___pdr_Table *h) {
  ___pdr_Node *n, *limit = ___pdr_gnodelast(h);
  /* if there is array part, assume it may have white values (it is not
     worth traversing it now just to check) */
  int hasclears = (h->sizearray > 0);
  for (n = ___pdr_gnode(h, 0); n < limit; n++) {  /* traverse hash part */
    ___pdr_checkdeadkey(n);
    if (___pdr_ttisnil(___pdr_gval(n)))  /* entry is empty? */
      removeentry(n);  /* remove it */
    else {
      ___pdr_lua_assert(!___pdr_ttisnil(___pdr_gkey(n)));
      ___pdr_markvalue(g, ___pdr_gkey(n));  /* mark key */
      if (!hasclears && iscleared(g, ___pdr_gval(n)))  /* is there a white value? */
        hasclears = 1;  /* table will have to be cleared */
    }
  }
  if (g->gcstate == ___pdr_GCSpropagate)
    ___pdr_linkgclist(h, g->grayagain);  /* must retraverse it in atomic phase */
  else if (hasclears)
    ___pdr_linkgclist(h, g->weak);  /* has to be cleared later */
}


/*
** Traverse an ephemeron table and link it to proper list. Returns true
** iff any object was marked during this traversal (which implies that
** convergence has to continue). During propagation phase, keep table
** in 'grayagain' list, to be visited again in the atomic phase. In
** the atomic phase, if table has any white->white entry, it has to
** be revisited during ephemeron convergence (as that key may turn
** black). Otherwise, if it has any white key, table has to be cleared
** (in the atomic phase).
*/
static int traverseephemeron (___pdr_global_State *g, ___pdr_Table *h) {
  int marked = 0;  /* true if an object is marked in this traversal */
  int hasclears = 0;  /* true if table has white keys */
  int hasww = 0;  /* true if table has entry "white-key -> white-value" */
  ___pdr_Node *n, *limit = ___pdr_gnodelast(h);
  unsigned int i;
  /* traverse array part */
  for (i = 0; i < h->sizearray; i++) {
    if (___pdr_valiswhite(&h->array[i])) {
      marked = 1;
      reallymarkobject(g, ___pdr_gcvalue(&h->array[i]));
    }
  }
  /* traverse hash part */
  for (n = ___pdr_gnode(h, 0); n < limit; n++) {
    ___pdr_checkdeadkey(n);
    if (___pdr_ttisnil(___pdr_gval(n)))  /* entry is empty? */
      removeentry(n);  /* remove it */
    else if (iscleared(g, ___pdr_gkey(n))) {  /* key is not marked (yet)? */
      hasclears = 1;  /* table must be cleared */
      if (___pdr_valiswhite(___pdr_gval(n)))  /* value not marked yet? */
        hasww = 1;  /* white-white entry */
    }
    else if (___pdr_valiswhite(___pdr_gval(n))) {  /* value not marked yet? */
      marked = 1;
      reallymarkobject(g, ___pdr_gcvalue(___pdr_gval(n)));  /* mark it now */
    }
  }
  /* link table into proper list */
  if (g->gcstate == ___pdr_GCSpropagate)
    ___pdr_linkgclist(h, g->grayagain);  /* must retraverse it in atomic phase */
  else if (hasww)  /* table has white->white entries? */
    ___pdr_linkgclist(h, g->ephemeron);  /* have to propagate again */
  else if (hasclears)  /* table has white keys? */
    ___pdr_linkgclist(h, g->allweak);  /* may have to clean white keys */
  return marked;
}


static void traversestrongtable (___pdr_global_State *g, ___pdr_Table *h) {
  ___pdr_Node *n, *limit = ___pdr_gnodelast(h);
  unsigned int i;
  for (i = 0; i < h->sizearray; i++)  /* traverse array part */
    ___pdr_markvalue(g, &h->array[i]);
  for (n = ___pdr_gnode(h, 0); n < limit; n++) {  /* traverse hash part */
    ___pdr_checkdeadkey(n);
    if (___pdr_ttisnil(___pdr_gval(n)))  /* entry is empty? */
      removeentry(n);  /* remove it */
    else {
      ___pdr_lua_assert(!___pdr_ttisnil(___pdr_gkey(n)));
      ___pdr_markvalue(g, ___pdr_gkey(n));  /* mark key */
      ___pdr_markvalue(g, ___pdr_gval(n));  /* mark value */
    }
  }
}


static ___pdr_lu_mem traversetable (___pdr_global_State *g, ___pdr_Table *h) {
  const char *weakkey, *weakvalue;
  const ___pdr_TValue *mode = ___pdr_gfasttm(g, h->metatable, PDR_TM_MODE);
  ___pdr_markobjectN(g, h->metatable);
  if (mode && ___pdr_ttisstring(mode) &&  /* is there a weak mode? */
      ((weakkey = strchr(___pdr_svalue(mode), 'k')),
       (weakvalue = strchr(___pdr_svalue(mode), 'v')),
       (weakkey || weakvalue))) {  /* is really weak? */
    ___pdr_black2gray(h);  /* keep table gray */
    if (!weakkey)  /* strong keys? */
      traverseweakvalue(g, h);
    else if (!weakvalue)  /* strong values? */
      traverseephemeron(g, h);
    else  /* all weak */
      ___pdr_linkgclist(h, g->allweak);  /* nothing to traverse now */
  }
  else  /* not weak */
    traversestrongtable(g, h);
  return sizeof(___pdr_Table) + sizeof(___pdr_TValue) * h->sizearray +
                         sizeof(___pdr_Node) * ___pdr_cast(size_t, ___pdr_allocsizenode(h));
}


/*
** Traverse a prototype. (While a prototype is being build, its
** arrays can be larger than needed; the extra slots are filled with
** NULL, so the use of 'markobjectN')
*/
static int traverseproto (___pdr_global_State *g, ___pdr_Proto *f) {
  int i;
  if (f->cache && ___pdr_iswhite(f->cache))
    f->cache = NULL;  /* allow cache to be collected */
  ___pdr_markobjectN(g, f->source);
  for (i = 0; i < f->sizek; i++)  /* mark literals */
    ___pdr_markvalue(g, &f->k[i]);
  for (i = 0; i < f->sizeupvalues; i++)  /* mark upvalue names */
    ___pdr_markobjectN(g, f->upvalues[i].name);
  for (i = 0; i < f->sizep; i++)  /* mark nested protos */
    ___pdr_markobjectN(g, f->p[i]);
  for (i = 0; i < f->sizelocvars; i++)  /* mark local-variable names */
    ___pdr_markobjectN(g, f->locvars[i].varname);
  return sizeof(___pdr_Proto) + sizeof(___pdr_Instruction) * f->sizecode +
                         sizeof(___pdr_Proto *) * f->sizep +
                         sizeof(___pdr_TValue) * f->sizek +
                         sizeof(int) * f->sizelineinfo +
                         sizeof(___pdr_LocVar) * f->sizelocvars +
                         sizeof(___pdr_Upvaldesc) * f->sizeupvalues;
}


static ___pdr_lu_mem traverseCclosure (___pdr_global_State *g, ___pdr_CClosure *cl) {
  int i;
  for (i = 0; i < cl->nupvalues; i++)  /* mark its upvalues */
    ___pdr_markvalue(g, &cl->upvalue[i]);
  return ___pdr_sizeCclosure(cl->nupvalues);
}

/*
** open upvalues point to values in a thread, so those values should
** be marked when the thread is traversed except in the atomic phase
** (because then the value cannot be changed by the thread and the
** thread may not be traversed again)
*/
static ___pdr_lu_mem traverseLclosure (___pdr_global_State *g, ___pdr_LClosure *cl) {
  int i;
  ___pdr_markobjectN(g, cl->p);  /* mark its prototype */
  for (i = 0; i < cl->nupvalues; i++) {  /* mark its upvalues */
    ___pdr_UpVal *uv = cl->upvals[i];
    if (uv != NULL) {
      if (___pdr_upisopen(uv) && g->gcstate != ___pdr_GCSinsideatomic)
        uv->u.open.touched = 1;  /* can be marked in 'remarkupvals' */
      else
        ___pdr_markvalue(g, uv->v);
    }
  }
  return ___pdr_sizeLclosure(cl->nupvalues);
}


static ___pdr_lu_mem traversethread (___pdr_global_State *g, ___pdr_lua_State *th) {
  ___pdr_StkId o = th->stack;
  if (o == NULL)
    return 1;  /* stack not completely built yet */
  ___pdr_lua_assert(g->gcstate == ___pdr_GCSinsideatomic ||
             th->openupval == NULL || ___pdr_isintwups(th));
  for (; o < th->top; o++)  /* mark live elements in the stack */
    ___pdr_markvalue(g, o);
  if (g->gcstate == ___pdr_GCSinsideatomic) {  /* final traversal? */
    ___pdr_StkId lim = th->stack + th->stacksize;  /* real end of stack */
    for (; o < lim; o++)  /* clear not-marked stack slice */
      ___pdr_setnilvalue(o);
    /* 'remarkupvals' may have removed thread from 'twups' list */
    if (!___pdr_isintwups(th) && th->openupval != NULL) {
      th->twups = g->twups;  /* link it back to the list */
      g->twups = th;
    }
  }
  else if (g->gckind != ___PDR_KGC_EMERGENCY)
    ___pdr_luaD_shrinkstack(th); /* do not change stack in emergency cycle */
  return (sizeof(___pdr_lua_State) + sizeof(___pdr_TValue) * th->stacksize +
          sizeof(___pdr_CallInfo) * th->nci);
}


/*
** traverse one gray object, turning it to black (except for threads,
** which are always gray).
*/
static void propagatemark (___pdr_global_State *g) {
  ___pdr_lu_mem size;
  ___pdr_GCObject *o = g->gray;
  ___pdr_lua_assert(___pdr_isgray(o));
  ___pdr_gray2black(o);
  switch (o->tt) {
    case ___PDR_LUA_TTABLE: {
      ___pdr_Table *h = ___pdr_gco2t(o);
      g->gray = h->gclist;  /* remove from 'gray' list */
      size = traversetable(g, h);
      break;
    }
    case ___PDR_LUA_TLCL: {
      ___pdr_LClosure *cl = ___pdr_gco2lcl(o);
      g->gray = cl->gclist;  /* remove from 'gray' list */
      size = traverseLclosure(g, cl);
      break;
    }
    case ___PDR_LUA_TCCL: {
      ___pdr_CClosure *cl = ___pdr_gco2ccl(o);
      g->gray = cl->gclist;  /* remove from 'gray' list */
      size = traverseCclosure(g, cl);
      break;
    }
    case ___PDR_LUA_TTHREAD: {
      ___pdr_lua_State *th = ___pdr_gco2th(o);
      g->gray = th->gclist;  /* remove from 'gray' list */
      ___pdr_linkgclist(th, g->grayagain);  /* insert into 'grayagain' list */
      ___pdr_black2gray(o);
      size = traversethread(g, th);
      break;
    }
    case ___PDR_LUA_TPROTO: {
      ___pdr_Proto *p = ___pdr_gco2p(o);
      g->gray = p->gclist;  /* remove from 'gray' list */
      size = traverseproto(g, p);
      break;
    }
    default: ___pdr_lua_assert(0); return;
  }
  g->GCmemtrav += size;
}


static void propagateall (___pdr_global_State *g) {
  while (g->gray) propagatemark(g);
}


static void convergeephemerons (___pdr_global_State *g) {
  int changed;
  do {
    ___pdr_GCObject *w;
    ___pdr_GCObject *next = g->ephemeron;  /* get ephemeron list */
    g->ephemeron = NULL;  /* tables may return to this list when traversed */
    changed = 0;
    while ((w = next) != NULL) {
      next = ___pdr_gco2t(w)->gclist;
      if (traverseephemeron(g, ___pdr_gco2t(w))) {  /* traverse marked some value? */
        propagateall(g);  /* propagate changes */
        changed = 1;  /* will have to revisit all ephemeron tables */
      }
    }
  } while (changed);
}

/* }====================================================== */


/*
** {======================================================
** Sweep Functions
** =======================================================
*/


/*
** clear entries with unmarked keys from all weaktables in list 'l' up
** to element 'f'
*/
static void clearkeys (___pdr_global_State *g, ___pdr_GCObject *l, ___pdr_GCObject *f) {
  for (; l != f; l = ___pdr_gco2t(l)->gclist) {
    ___pdr_Table *h = ___pdr_gco2t(l);
    ___pdr_Node *n, *limit = ___pdr_gnodelast(h);
    for (n = ___pdr_gnode(h, 0); n < limit; n++) {
      if (!___pdr_ttisnil(___pdr_gval(n)) && (iscleared(g, ___pdr_gkey(n)))) {
        ___pdr_setnilvalue(___pdr_gval(n));  /* remove value ... */
        removeentry(n);  /* and remove entry from table */
      }
    }
  }
}


/*
** clear entries with unmarked values from all weaktables in list 'l' up
** to element 'f'
*/
static void clearvalues (___pdr_global_State *g, ___pdr_GCObject *l, ___pdr_GCObject *f) {
  for (; l != f; l = ___pdr_gco2t(l)->gclist) {
    ___pdr_Table *h = ___pdr_gco2t(l);
    ___pdr_Node *n, *limit = ___pdr_gnodelast(h);
    unsigned int i;
    for (i = 0; i < h->sizearray; i++) {
      ___pdr_TValue *o = &h->array[i];
      if (iscleared(g, o))  /* value was collected? */
        ___pdr_setnilvalue(o);  /* remove value */
    }
    for (n = ___pdr_gnode(h, 0); n < limit; n++) {
      if (!___pdr_ttisnil(___pdr_gval(n)) && iscleared(g, ___pdr_gval(n))) {
        ___pdr_setnilvalue(___pdr_gval(n));  /* remove value ... */
        removeentry(n);  /* and remove entry from table */
      }
    }
  }
}


void ___pdr_luaC_upvdeccount (___pdr_lua_State *L, ___pdr_UpVal *uv) {
  ___pdr_lua_assert(uv->refcount > 0);
  uv->refcount--;
  if (uv->refcount == 0 && !___pdr_upisopen(uv))
    ___pdr_luaM_free(L, uv);
}


static void freeLclosure (___pdr_lua_State *L, ___pdr_LClosure *cl) {
  int i;
  for (i = 0; i < cl->nupvalues; i++) {
    ___pdr_UpVal *uv = cl->upvals[i];
    if (uv)
      ___pdr_luaC_upvdeccount(L, uv);
  }
  ___pdr_luaM_freemem(L, cl, ___pdr_sizeLclosure(cl->nupvalues));
}


static void freeobj (___pdr_lua_State *L, ___pdr_GCObject *o) {
  switch (o->tt) {
    case ___PDR_LUA_TPROTO: ___pdr_luaF_freeproto(L, ___pdr_gco2p(o)); break;
    case ___PDR_LUA_TLCL: {
      freeLclosure(L, ___pdr_gco2lcl(o));
      break;
    }
    case ___PDR_LUA_TCCL: {
      ___pdr_luaM_freemem(L, o, ___pdr_sizeCclosure(___pdr_gco2ccl(o)->nupvalues));
      break;
    }
    case ___PDR_LUA_TTABLE: ___pdr_luaH_free(L, ___pdr_gco2t(o)); break;
    case ___PDR_LUA_TTHREAD: ___pdr_luaE_freethread(L, ___pdr_gco2th(o)); break;
    case ___PDR_LUA_TUSERDATA: ___pdr_luaM_freemem(L, o, ___pdr_sizeudata(___pdr_gco2u(o))); break;
    case ___PDR_LUA_TSHRSTR:
      ___pdr_luaS_remove(L, ___pdr_gco2ts(o));  /* remove it from hash table */
      ___pdr_luaM_freemem(L, o, ___pdr_sizelstring(___pdr_gco2ts(o)->shrlen));
      break;
    case ___PDR_LUA_TLNGSTR: {
      ___pdr_luaM_freemem(L, o, ___pdr_sizelstring(___pdr_gco2ts(o)->u.lnglen));
      break;
    }
    default: ___pdr_lua_assert(0);
  }
}


#define ___pdr_sweepwholelist(L,p)	sweeplist(L,p,___PDR_MAX_LUMEM)
static ___pdr_GCObject **sweeplist (___pdr_lua_State *L, ___pdr_GCObject **p, ___pdr_lu_mem count);


/*
** sweep at most 'count' elements from a list of GCObjects erasing dead
** objects, where a dead object is one marked with the old (non current)
** white; change all non-dead objects back to white, preparing for next
** collection cycle. Return where to continue the traversal or NULL if
** list is finished.
*/
static ___pdr_GCObject **sweeplist (___pdr_lua_State *L, ___pdr_GCObject **p, ___pdr_lu_mem count) {
  ___pdr_global_State *g = ___pdr_G(L);
  int ow = ___pdr_otherwhite(g);
  int white = ___pdr_luaC_white(g);  /* current white */
  while (*p != NULL && count-- > 0) {
    ___pdr_GCObject *curr = *p;
    int marked = curr->marked;
    if (___pdr_isdeadm(ow, marked)) {  /* is 'curr' dead? */
      *p = curr->next;  /* remove 'curr' from list */
      freeobj(L, curr);  /* erase 'curr' */
    }
    else {  /* change mark to 'white' */
      curr->marked = ___pdr_cast_byte((marked & ___pdr_maskcolors) | white);
      p = &curr->next;  /* go to next element */
    }
  }
  return (*p == NULL) ? NULL : p;
}


/*
** sweep a list until a live object (or end of list)
*/
static ___pdr_GCObject **sweeptolive (___pdr_lua_State *L, ___pdr_GCObject **p) {
  ___pdr_GCObject **old = p;
  do {
    p = sweeplist(L, p, 1);
  } while (p == old);
  return p;
}

/* }====================================================== */


/*
** {======================================================
** Finalization
** =======================================================
*/

/*
** If possible, shrink string table
*/
static void checkSizes (___pdr_lua_State *L, ___pdr_global_State *g) {
  if (g->gckind != ___PDR_KGC_EMERGENCY) {
    ___pdr_l_mem olddebt = g->GCdebt;
    if (g->strt.nuse < g->strt.size / 4)  /* string table too big? */
      ___pdr_luaS_resize(L, g->strt.size / 2);  /* shrink it a little */
    g->GCestimate += g->GCdebt - olddebt;  /* update estimate */
  }
}


static ___pdr_GCObject *udata2finalize (___pdr_global_State *g) {
  ___pdr_GCObject *o = g->tobefnz;  /* get first element */
  ___pdr_lua_assert(___pdr_tofinalize(o));
  g->tobefnz = o->next;  /* remove it from 'tobefnz' list */
  o->next = g->allgc;  /* return it to 'allgc' list */
  g->allgc = o;
  ___pdr_resetbit(o->marked, ___PDR_FINALIZEDBIT);  /* object is "normal" again */
  if (___pdr_issweepphase(g))
    ___pdr_makewhite(g, o);  /* "sweep" object */
  return o;
}


static void dothecall (___pdr_lua_State *L, void *ud) {
  ___PDR_UNUSED(ud);
  ___pdr_luaD_callnoyield(L, L->top - 2, 0);
}


static void GCTM (___pdr_lua_State *L, int propagateerrors) {
  ___pdr_global_State *g = ___pdr_G(L);
  const ___pdr_TValue *tm;
  ___pdr_TValue v;
  ___pdr_setgcovalue(L, &v, udata2finalize(g));
  tm = ___pdr_luaT_gettmbyobj(L, &v, PDR_TM_GC);
  if (tm != NULL && ___pdr_ttisfunction(tm)) {  /* is there a finalizer? */
    int status;
    ___pdr_lu_byte oldah = L->allowhook;
    int running  = g->gcrunning;
    L->allowhook = 0;  /* stop debug hooks during GC metamethod */
    g->gcrunning = 0;  /* avoid GC steps */
    ___pdr_setobj2s(L, L->top, tm);  /* push finalizer... */
    ___pdr_setobj2s(L, L->top + 1, &v);  /* ... and its argument */
    L->top += 2;  /* and (next line) call the finalizer */
    L->ci->callstatus |= ___PDR_CIST_FIN;  /* will run a finalizer */
    status = ___pdr_luaD_pcall(L, dothecall, NULL, ___pdr_savestack(L, L->top - 2), 0);
    L->ci->callstatus &= ~___PDR_CIST_FIN;  /* not running a finalizer anymore */
    L->allowhook = oldah;  /* restore hooks */
    g->gcrunning = running;  /* restore state */
    if (status != ___PDR_LUA_OK && propagateerrors) {  /* error while running __gc? */
      if (status == ___PDR_LUA_ERRRUN) {  /* is there an error object? */
        const char *msg = (___pdr_ttisstring(L->top - 1))
                            ? ___pdr_svalue(L->top - 1)
                            : "no message";
        ___pdr_luaO_pushfstring(L, "error in __gc metamethod (%s)", msg);
        status = ___PDR_LUA_ERRGCMM;  /* error in __gc metamethod */
      }
      ___pdr_luaD_throw(L, status);  /* re-throw error */
    }
  }
}


/*
** call a few (up to 'g->gcfinnum') finalizers
*/
static int runafewfinalizers (___pdr_lua_State *L) {
  ___pdr_global_State *g = ___pdr_G(L);
  unsigned int i;
  ___pdr_lua_assert(!g->tobefnz || g->gcfinnum > 0);
  for (i = 0; g->tobefnz && i < g->gcfinnum; i++)
    GCTM(L, 1);  /* call one finalizer */
  g->gcfinnum = (!g->tobefnz) ? 0  /* nothing more to finalize? */
                    : g->gcfinnum * 2;  /* else call a few more next time */
  return i;
}


/*
** call all pending finalizers
*/
static void callallpendingfinalizers (___pdr_lua_State *L) {
  ___pdr_global_State *g = ___pdr_G(L);
  while (g->tobefnz)
    GCTM(L, 0);
}


/*
** find last 'next' field in list 'p' list (to add elements in its end)
*/
static ___pdr_GCObject **findlast (___pdr_GCObject **p) {
  while (*p != NULL)
    p = &(*p)->next;
  return p;
}


/*
** move all unreachable objects (or 'all' objects) that need
** finalization from list 'finobj' to list 'tobefnz' (to be finalized)
*/
static void separatetobefnz (___pdr_global_State *g, int all) {
  ___pdr_GCObject *curr;
  ___pdr_GCObject **p = &g->finobj;
  ___pdr_GCObject **lastnext = findlast(&g->tobefnz);
  while ((curr = *p) != NULL) {  /* traverse all finalizable objects */
    ___pdr_lua_assert(___pdr_tofinalize(curr));
    if (!(___pdr_iswhite(curr) || all))  /* not being collected? */
      p = &curr->next;  /* don't bother with it */
    else {
      *p = curr->next;  /* remove 'curr' from 'finobj' list */
      curr->next = *lastnext;  /* link at the end of 'tobefnz' list */
      *lastnext = curr;
      lastnext = &curr->next;
    }
  }
}


/*
** if object 'o' has a finalizer, remove it from 'allgc' list (must
** search the list to find it) and link it in 'finobj' list.
*/
void ___pdr_luaC_checkfinalizer (___pdr_lua_State *L, ___pdr_GCObject *o, ___pdr_Table *mt) {
  ___pdr_global_State *g = ___pdr_G(L);
  if (___pdr_tofinalize(o) ||                 /* obj. is already marked... */
      ___pdr_gfasttm(g, mt, PDR_TM_GC) == NULL)   /* or has no finalizer? */
    return;  /* nothing to be done */
  else {  /* move 'o' to 'finobj' list */
    ___pdr_GCObject **p;
    if (___pdr_issweepphase(g)) {
      ___pdr_makewhite(g, o);  /* "sweep" object 'o' */
      if (g->sweepgc == &o->next)  /* should not remove 'sweepgc' object */
        g->sweepgc = sweeptolive(L, g->sweepgc);  /* change 'sweepgc' */
    }
    /* search for pointer pointing to 'o' */
    for (p = &g->allgc; *p != o; p = &(*p)->next) { /* empty */ }
    *p = o->next;  /* remove 'o' from 'allgc' list */
    o->next = g->finobj;  /* link it in 'finobj' list */
    g->finobj = o;
    ___pdr_l_setbit(o->marked, ___PDR_FINALIZEDBIT);  /* mark it as such */
  }
}

/* }====================================================== */



/*
** {======================================================
** GC control
** =======================================================
*/


/*
** Set a reasonable "time" to wait before starting a new GC cycle; cycle
** will start when memory use hits threshold. (Division by 'estimate'
** should be OK: it cannot be zero (because Lua cannot even start with
** less than PAUSEADJ bytes).
*/
static void setpause (___pdr_global_State *g) {
  ___pdr_l_mem threshold, debt;
  ___pdr_l_mem estimate = g->GCestimate / ___PDR_PAUSEADJ;  /* adjust 'estimate' */
  ___pdr_lua_assert(estimate > 0);
  threshold = (g->gcpause < ___PDR_MAX_LMEM / estimate)  /* overflow? */
            ? estimate * g->gcpause  /* no overflow */
            : ___PDR_MAX_LMEM;  /* overflow; truncate to maximum */
  debt = ___pdr_gettotalbytes(g) - threshold;
  ___pdr_luaE_setdebt(g, debt);
}


/*
** Enter first sweep phase.
** The call to 'sweeplist' tries to make pointer point to an object
** inside the list (instead of to the header), so that the real sweep do
** not need to skip objects created between "now" and the start of the
** real sweep.
*/
static void entersweep (___pdr_lua_State *L) {
  ___pdr_global_State *g = ___pdr_G(L);
  g->gcstate = ___pdr_GCSswpallgc;
  ___pdr_lua_assert(g->sweepgc == NULL);
  g->sweepgc = sweeplist(L, &g->allgc, 1);
}


void ___pdr_luaC_freeallobjects (___pdr_lua_State *L) {
  ___pdr_global_State *g = ___pdr_G(L);
  separatetobefnz(g, 1);  /* separate all objects with finalizers */
  ___pdr_lua_assert(g->finobj == NULL);
  callallpendingfinalizers(L);
  ___pdr_lua_assert(g->tobefnz == NULL);
  g->currentwhite = ___PDR_WHITEBITS; /* this "white" makes all objects look dead */
  g->gckind = ___PDR_KGC_NORMAL;
  ___pdr_sweepwholelist(L, &g->finobj);
  ___pdr_sweepwholelist(L, &g->allgc);
  ___pdr_sweepwholelist(L, &g->fixedgc);  /* collect fixed objects */
  ___pdr_lua_assert(g->strt.nuse == 0);
}


static ___pdr_l_mem atomic (___pdr_lua_State *L) {
  ___pdr_global_State *g = ___pdr_G(L);
  ___pdr_l_mem work;
  ___pdr_GCObject *origweak, *origall;
  ___pdr_GCObject *grayagain = g->grayagain;  /* save original list */
  ___pdr_lua_assert(g->ephemeron == NULL && g->weak == NULL);
  ___pdr_lua_assert(!___pdr_iswhite(g->mainthread));
  g->gcstate = ___pdr_GCSinsideatomic;
  g->GCmemtrav = 0;  /* start counting work */
  ___pdr_markobject(g, L);  /* mark running thread */
  /* registry and global metatables may be changed by API */
  ___pdr_markvalue(g, &g->l_registry);
  markmt(g);  /* mark global metatables */
  /* remark occasional upvalues of (maybe) dead threads */
  remarkupvals(g);
  propagateall(g);  /* propagate changes */
  work = g->GCmemtrav;  /* stop counting (do not recount 'grayagain') */
  g->gray = grayagain;
  propagateall(g);  /* traverse 'grayagain' list */
  g->GCmemtrav = 0;  /* restart counting */
  convergeephemerons(g);
  /* at this point, all strongly accessible objects are marked. */
  /* Clear values from weak tables, before checking finalizers */
  clearvalues(g, g->weak, NULL);
  clearvalues(g, g->allweak, NULL);
  origweak = g->weak; origall = g->allweak;
  work += g->GCmemtrav;  /* stop counting (objects being finalized) */
  separatetobefnz(g, 0);  /* separate objects to be finalized */
  g->gcfinnum = 1;  /* there may be objects to be finalized */
  markbeingfnz(g);  /* mark objects that will be finalized */
  propagateall(g);  /* remark, to propagate 'resurrection' */
  g->GCmemtrav = 0;  /* restart counting */
  convergeephemerons(g);
  /* at this point, all resurrected objects are marked. */
  /* remove dead objects from weak tables */
  clearkeys(g, g->ephemeron, NULL);  /* clear keys from all ephemeron tables */
  clearkeys(g, g->allweak, NULL);  /* clear keys from all 'allweak' tables */
  /* clear values from resurrected weak tables */
  clearvalues(g, g->weak, origweak);
  clearvalues(g, g->allweak, origall);
  ___pdr_luaS_clearcache(g);
  g->currentwhite = ___pdr_cast_byte(___pdr_otherwhite(g));  /* flip current white */
  work += g->GCmemtrav;  /* complete counting */
  return work;  /* estimate of memory marked by 'atomic' */
}


static ___pdr_lu_mem sweepstep (___pdr_lua_State *L, ___pdr_global_State *g,
                         int nextstate, ___pdr_GCObject **nextlist) {
  if (g->sweepgc) {
    ___pdr_l_mem olddebt = g->GCdebt;
    g->sweepgc = sweeplist(L, g->sweepgc, ___PDR_GCSWEEPMAX);
    g->GCestimate += g->GCdebt - olddebt;  /* update estimate */
    if (g->sweepgc)  /* is there still something to sweep? */
      return (___PDR_GCSWEEPMAX * ___PDR_GCSWEEPCOST);
  }
  /* else enter next state */
  g->gcstate = nextstate;
  g->sweepgc = nextlist;
  return 0;
}


static ___pdr_lu_mem singlestep (___pdr_lua_State *L) {
  ___pdr_global_State *g = ___pdr_G(L);
  switch (g->gcstate) {
    case ___pdr_GCSpause: {
      g->GCmemtrav = g->strt.size * sizeof(___pdr_GCObject*);
      restartcollection(g);
      g->gcstate = ___pdr_GCSpropagate;
      return g->GCmemtrav;
    }
    case ___pdr_GCSpropagate: {
      g->GCmemtrav = 0;
      ___pdr_lua_assert(g->gray);
      propagatemark(g);
       if (g->gray == NULL)  /* no more gray objects? */
        g->gcstate = ___pdr_GCSatomic;  /* finish propagate phase */
      return g->GCmemtrav;  /* memory traversed in this step */
    }
    case ___pdr_GCSatomic: {
      ___pdr_lu_mem work;
      propagateall(g);  /* make sure gray list is empty */
      work = atomic(L);  /* work is what was traversed by 'atomic' */
      entersweep(L);
      g->GCestimate = ___pdr_gettotalbytes(g);  /* first estimate */;
      return work;
    }
    case ___pdr_GCSswpallgc: {  /* sweep "regular" objects */
      return sweepstep(L, g, ___pdr_GCSswpfinobj, &g->finobj);
    }
    case ___pdr_GCSswpfinobj: {  /* sweep objects with finalizers */
      return sweepstep(L, g, ___pdr_GCSswptobefnz, &g->tobefnz);
    }
    case ___pdr_GCSswptobefnz: {  /* sweep objects to be finalized */
      return sweepstep(L, g, ___pdr_GCSswpend, NULL);
    }
    case ___pdr_GCSswpend: {  /* finish sweeps */
      ___pdr_makewhite(g, g->mainthread);  /* sweep main thread */
      checkSizes(L, g);
      g->gcstate = ___pdr_GCScallfin;
      return 0;
    }
    case ___pdr_GCScallfin: {  /* call remaining finalizers */
      if (g->tobefnz && g->gckind != ___PDR_KGC_EMERGENCY) {
        int n = runafewfinalizers(L);
        return (n * ___PDR_GCFINALIZECOST);
      }
      else {  /* emergency mode or no more finalizers */
        g->gcstate = ___pdr_GCSpause;  /* finish collection */
        return 0;
      }
    }
    default: ___pdr_lua_assert(0); return 0;
  }
}


/*
** advances the garbage collector until it reaches a state allowed
** by 'statemask'
*/
void ___pdr_luaC_runtilstate (___pdr_lua_State *L, int statesmask) {
  ___pdr_global_State *g = ___pdr_G(L);
  while (!___pdr_testbit(statesmask, g->gcstate))
    singlestep(L);
}


/*
** get GC debt and convert it from Kb to 'work units' (avoid zero debt
** and overflows)
*/
static ___pdr_l_mem getdebt (___pdr_global_State *g) {
  ___pdr_l_mem debt = g->GCdebt;
  int stepmul = g->gcstepmul;
  if (debt <= 0) return 0;  /* minimal debt */
  else {
    debt = (debt / ___PDR_STEPMULADJ) + 1;
    debt = (debt < ___PDR_MAX_LMEM / stepmul) ? debt * stepmul : ___PDR_MAX_LMEM;
    return debt;
  }
}

/*
** performs a basic GC step when collector is running
*/
void ___pdr_luaC_step (___pdr_lua_State *L) {
  ___pdr_global_State *g = ___pdr_G(L);
  ___pdr_l_mem debt = getdebt(g);  /* GC deficit (be paid now) */
  if (!g->gcrunning) {  /* not running? */
    ___pdr_luaE_setdebt(g, -___PDR_GCSTEPSIZE * 10);  /* avoid being called too often */
    return;
  }
  do {  /* repeat until pause or enough "credit" (negative debt) */
    ___pdr_lu_mem work = singlestep(L);  /* perform one single step */
    debt -= work;
  } while (debt > -___PDR_GCSTEPSIZE && g->gcstate != ___pdr_GCSpause);
  if (g->gcstate == ___pdr_GCSpause)
    setpause(g);  /* pause until next cycle */
  else {
    debt = (debt / g->gcstepmul) * ___PDR_STEPMULADJ;  /* convert 'work units' to Kb */
    ___pdr_luaE_setdebt(g, debt);
    runafewfinalizers(L);
  }
}


/*
** Performs a full GC cycle; if 'isemergency', set a flag to avoid
** some operations which could change the interpreter state in some
** unexpected ways (running finalizers and shrinking some structures).
** Before running the collection, check 'keepinvariant'; if it is true,
** there may be some objects marked as black, so the collector has
** to sweep all objects to turn them back to white (as white has not
** changed, nothing will be collected).
*/
void ___pdr_luaC_fullgc (___pdr_lua_State *L, int isemergency) {
  ___pdr_global_State *g = ___pdr_G(L);
  ___pdr_lua_assert(g->gckind == ___PDR_KGC_NORMAL);
  if (isemergency) g->gckind = ___PDR_KGC_EMERGENCY;  /* set flag */
  if (___pdr_keepinvariant(g)) {  /* black objects? */
    entersweep(L); /* sweep everything to turn them back to white */
  }
  /* finish any pending sweep phase to start a new cycle */
  ___pdr_luaC_runtilstate(L, ___pdr_bitmask(___pdr_GCSpause));
  ___pdr_luaC_runtilstate(L, ~___pdr_bitmask(___pdr_GCSpause));  /* start new collection */
  ___pdr_luaC_runtilstate(L, ___pdr_bitmask(___pdr_GCScallfin));  /* run up to finalizers */
  /* estimate must be correct after a full GC cycle */
  ___pdr_lua_assert(g->GCestimate == ___pdr_gettotalbytes(g));
  ___pdr_luaC_runtilstate(L, ___pdr_bitmask(___pdr_GCSpause));  /* finish collection */
  g->gckind = ___PDR_KGC_NORMAL;
  setpause(g);
}

/* }====================================================== */

} // end NS_PDR_SLUA
