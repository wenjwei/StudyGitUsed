/*
** $Id: lfunc.c,v 2.45 2014/11/02 19:19:04 roberto Exp $
** Auxiliary functions to manipulate prototypes and closures
** See Copyright Notice in lua.h
*/

#define ___pdr_lfunc_c
#define ___PDR_LUA_CORE

#include "lfunc.h"
#include "lprefix.h"

#include <stddef.h>

#include "lua.h"
#include "lgc.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"

namespace NS_PDR_SLUA {

___pdr_CClosure *___pdr_luaF_newCclosure (___pdr_lua_State *L, int n) {
  ___pdr_GCObject *o = ___pdr_luaC_newobj(L, ___PDR_LUA_TCCL, ___pdr_sizeCclosure(n));
  ___pdr_CClosure *c = ___pdr_gco2ccl(o);
  c->nupvalues = ___pdr_cast_byte(n);
  return c;
}


___pdr_LClosure *___pdr_luaF_newLclosure (___pdr_lua_State *L, int n) {
  ___pdr_GCObject *o = ___pdr_luaC_newobj(L, ___PDR_LUA_TLCL, ___pdr_sizeLclosure(n));
  ___pdr_LClosure *c = ___pdr_gco2lcl(o);
  c->p = NULL;
  c->nupvalues = ___pdr_cast_byte(n);
  while (n--) c->upvals[n] = NULL;
  return c;
}

/*
** fill a closure with new closed upvalues
*/
void ___pdr_luaF_initupvals (___pdr_lua_State *L, ___pdr_LClosure *cl) {
  int i;
  for (i = 0; i < cl->nupvalues; i++) {
    ___pdr_UpVal *uv = ___pdr_luaM_new(L, ___pdr_UpVal);
    uv->refcount = 1;
    uv->v = &uv->u.value;  /* make it closed */
    ___pdr_setnilvalue(uv->v);
    cl->upvals[i] = uv;
  }
}


___pdr_UpVal *___pdr_luaF_findupval (___pdr_lua_State *L, ___pdr_StkId level) {
  ___pdr_UpVal **pp = &L->openupval;
  ___pdr_UpVal *p;
  ___pdr_UpVal *uv;
  ___pdr_lua_assert(___pdr_isintwups(L) || L->openupval == NULL);
  while (*pp != NULL && (p = *pp)->v >= level) {
    ___pdr_lua_assert(___pdr_upisopen(p));
    if (p->v == level)  /* found a corresponding upvalue? */
      return p;  /* return it */
    pp = &p->u.open.next;
  }
  /* not found: create a new upvalue */
  uv = ___pdr_luaM_new(L, ___pdr_UpVal);
  uv->refcount = 0;
  uv->u.open.next = *pp;  /* link it to list of open upvalues */
  uv->u.open.touched = 1;
  *pp = uv;
  uv->v = level;  /* current value lives in the stack */
  if (!___pdr_isintwups(L)) {  /* thread not in list of threads with upvalues? */
    L->twups = ___pdr_G(L)->twups;  /* link it to the list */
    ___pdr_G(L)->twups = L;
  }
  return uv;
}


void ___pdr_luaF_close (___pdr_lua_State *L, ___pdr_StkId level) {
  ___pdr_UpVal *uv;
  while (L->openupval != NULL && (uv = L->openupval)->v >= level) {
    ___pdr_lua_assert(___pdr_upisopen(uv));
    L->openupval = uv->u.open.next;  /* remove from 'open' list */
    if (uv->refcount == 0)  /* no references? */
      ___pdr_luaM_free(L, uv);  /* free upvalue */
    else {
      ___pdr_setobj(L, &uv->u.value, uv->v);  /* move value to upvalue slot */
      uv->v = &uv->u.value;  /* now current value lives here */
      ___pdr_luaC_upvalbarrier(L, uv);
    }
  }
}


___pdr_Proto *___pdr_luaF_newproto (___pdr_lua_State *L) {
  ___pdr_GCObject *o = ___pdr_luaC_newobj(L, ___PDR_LUA_TPROTO, sizeof(___pdr_Proto));
  ___pdr_Proto *f = ___pdr_gco2p(o);
  f->k = NULL;
  f->sizek = 0;
  f->p = NULL;
  f->sizep = 0;
  f->code = NULL;
  f->cache = NULL;
  f->sizecode = 0;
  f->lineinfo = NULL;
  f->sizelineinfo = 0;
  f->upvalues = NULL;
  f->sizeupvalues = 0;
  f->numparams = 0;
  f->is_vararg = 0;
  f->maxstacksize = 0;
  f->locvars = NULL;
  f->sizelocvars = 0;
  f->linedefined = 0;
  f->lastlinedefined = 0;
  f->source = NULL;
  return f;
}


void ___pdr_luaF_freeproto (___pdr_lua_State *L, ___pdr_Proto *f) {
  ___pdr_luaM_freearray(L, f->code, f->sizecode);
  ___pdr_luaM_freearray(L, f->p, f->sizep);
  ___pdr_luaM_freearray(L, f->k, f->sizek);
  ___pdr_luaM_freearray(L, f->lineinfo, f->sizelineinfo);
  ___pdr_luaM_freearray(L, f->locvars, f->sizelocvars);
  ___pdr_luaM_freearray(L, f->upvalues, f->sizeupvalues);
  ___pdr_luaM_free(L, f);
}


/*
** Look for n-th local variable at line 'line' in function 'func'.
** Returns NULL if not found.
*/
const char *___pdr_luaF_getlocalname (const ___pdr_Proto *f, int local_number, int pc) {
  int i;
  for (i = 0; i<f->sizelocvars && f->locvars[i].startpc <= pc; i++) {
    if (pc < f->locvars[i].endpc) {  /* is variable active? */
      local_number--;
      if (local_number == 0)
        return ___pdr_getstr(f->locvars[i].varname);
    }
  }
  return NULL;  /* not found */
}

} // end NS_PDR_SLUA