/*
** $Id: lundump.c,v 2.44 2015/11/02 16:09:30 roberto Exp $
** load precompiled Lua chunks
** See Copyright Notice in lua.h
*/

#define ___pdr_lundump_c
#define ___PDR_LUA_CORE

#include "lundump.h"
#include "lprefix.h"

#include <string.h>

#include "lua.h"
#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "lmem.h"
#include "lobject.h"
#include "lstring.h"
#include "lzio.h"

namespace NS_PDR_SLUA {

#if !defined(___pdr_luai_verifycode)
#define ___pdr_luai_verifycode(L,b,f)  /* empty */
#endif


typedef struct {
  ___pdr_lua_State *L;
  ___pdr_ZIO *Z;
  const char *name;
} LoadState;


static ___pdr_l_noret error(LoadState *S, const char *why) {
  ___pdr_luaO_pushfstring(S->L, "%s: %s precompiled chunk", S->name, why);
  ___pdr_luaD_throw(S->L, ___PDR_LUA_ERRSYNTAX);
}


/*
** All high-level loads go through LoadVector; you can change it to
** adapt to the endianness of the input
*/
#define ___pdr_LoadVector(S,b,n)	LoadBlock(S,b,(n)*sizeof((b)[0]))

static void LoadBlock (LoadState *S, void *b, size_t size) {
  if (___pdr_luaZ_read(S->Z, b, size) != 0)
    error(S, "truncated");
}


#define ___pdr_LoadVar(S,x)		___pdr_LoadVector(S,&x,1)


static ___pdr_lu_byte LoadByte (LoadState *S) {
  ___pdr_lu_byte x;
  ___pdr_LoadVar(S, x);
  return x;
}


static int LoadInt (LoadState *S) {
  int x;
  ___pdr_LoadVar(S, x);
  return x;
}


static ___pdr_lua_Number LoadNumber (LoadState *S) {
  ___pdr_lua_Number x;
  ___pdr_LoadVar(S, x);
  return x;
}


static ___pdr_lua_Integer LoadInteger (LoadState *S) {
  ___pdr_lua_Integer x;
  ___pdr_LoadVar(S, x);
  return x;
}


static ___pdr_TString *LoadString (LoadState *S) {
  int size = LoadByte(S);
  if (size == 0xFF)
    ___pdr_LoadVar(S, size);
  if (size == 0)
    return NULL;
  else if (--size <= ___PDR_LUAI_MAXSHORTLEN) {  /* short string? */
    char buff[___PDR_LUAI_MAXSHORTLEN];
    ___pdr_LoadVector(S, buff, size);
    return ___pdr_luaS_newlstr(S->L, buff, size);
  }
  else {  /* long string */
    ___pdr_TString *ts = ___pdr_luaS_createlngstrobj(S->L, size);
    ___pdr_LoadVector(S, ___pdr_getstr(ts), size);  /* load directly in final place */
    return ts;
  }
}


static void LoadCode (LoadState *S, ___pdr_Proto *f) {
  int n = LoadInt(S);
  f->code = ___pdr_luaM_newvector(S->L, n, ___pdr_Instruction);
  f->sizecode = n;
  ___pdr_LoadVector(S, f->code, n);
}


static void LoadFunction(LoadState *S, ___pdr_Proto *f, ___pdr_TString *psource);


static void LoadConstants (LoadState *S, ___pdr_Proto *f) {
  int i;
  int n = LoadInt(S);
  f->k = ___pdr_luaM_newvector(S->L, n, ___pdr_TValue);
  f->sizek = n;
  for (i = 0; i < n; i++)
    ___pdr_setnilvalue(&f->k[i]);
  for (i = 0; i < n; i++) {
    ___pdr_TValue *o = &f->k[i];
    int t = LoadByte(S);
    switch (t) {
    case ___PDR_LUA_TNIL:
      ___pdr_setnilvalue(o);
      break;
    case ___PDR_LUA_TBOOLEAN:
      ___pdr_setbvalue(o, LoadByte(S));
      break;
    case ___PDR_LUA_TNUMFLT:
      ___pdr_setfltvalue(o, LoadNumber(S));
      break;
    case ___PDR_LUA_TNUMINT:
      ___pdr_setivalue(o, LoadInteger(S));
      break;
    case ___PDR_LUA_TSHRSTR:
    case ___PDR_LUA_TLNGSTR:
      ___pdr_setsvalue2n(S->L, o, LoadString(S));
      break;
    default:
      ___pdr_lua_assert(0);
    }
  }
}


static void LoadProtos (LoadState *S, ___pdr_Proto *f) {
  int i;
  int n = LoadInt(S);
  f->p = ___pdr_luaM_newvector(S->L, n, ___pdr_Proto *);
  f->sizep = n;
  for (i = 0; i < n; i++)
    f->p[i] = NULL;
  for (i = 0; i < n; i++) {
    f->p[i] = ___pdr_luaF_newproto(S->L);
    LoadFunction(S, f->p[i], f->source);
  }
}


static void LoadUpvalues (LoadState *S, ___pdr_Proto *f) {
  int i, n;
  n = LoadInt(S);
  f->upvalues = ___pdr_luaM_newvector(S->L, n, ___pdr_Upvaldesc);
  f->sizeupvalues = n;
  for (i = 0; i < n; i++)
    f->upvalues[i].name = NULL;
  for (i = 0; i < n; i++) {
    f->upvalues[i].instack = LoadByte(S);
    f->upvalues[i].idx = LoadByte(S);
  }
}


static void LoadDebug (LoadState *S, ___pdr_Proto *f) {
  int i, n;
  n = LoadInt(S);
  f->lineinfo = ___pdr_luaM_newvector(S->L, n, int);
  f->sizelineinfo = n;
  ___pdr_LoadVector(S, f->lineinfo, n);
  n = LoadInt(S);
  f->locvars = ___pdr_luaM_newvector(S->L, n, ___pdr_LocVar);
  f->sizelocvars = n;
  for (i = 0; i < n; i++)
    f->locvars[i].varname = NULL;
  for (i = 0; i < n; i++) {
    f->locvars[i].varname = LoadString(S);
    f->locvars[i].startpc = LoadInt(S);
    f->locvars[i].endpc = LoadInt(S);
  }
  n = LoadInt(S);
  for (i = 0; i < n; i++)
    f->upvalues[i].name = LoadString(S);
}


static void LoadFunction (LoadState *S, ___pdr_Proto *f, ___pdr_TString *psource) {
  f->source = LoadString(S);
  if (f->source == NULL)  /* no source in dump? */
    f->source = psource;  /* reuse parent's source */
  f->linedefined = LoadInt(S);
  f->lastlinedefined = LoadInt(S);
  f->numparams = LoadByte(S);
  f->is_vararg = LoadByte(S);
  f->maxstacksize = LoadByte(S);
  LoadCode(S, f);
  LoadConstants(S, f);
  LoadUpvalues(S, f);
  LoadProtos(S, f);
  LoadDebug(S, f);
}


static void checkliteral (LoadState *S, const char *s, const char *msg) {
  char buff[sizeof(___PDR_LUA_SIGNATURE) + sizeof(___PDR_LUAC_DATA)]; /* larger than both */
  size_t len = strlen(s);
  ___pdr_LoadVector(S, buff, len);
  if (memcmp(s, buff, len) != 0)
    error(S, msg);
}


static void fchecksize (LoadState *S, size_t size, const char *tname) {
  if (LoadByte(S) != size)
    error(S, ___pdr_luaO_pushfstring(S->L, "%s size mismatch in", tname));
}


#define ___pdr_checksize(S,t)	fchecksize(S,sizeof(t),#t)

static void checkHeader (LoadState *S) {
  const char *tmp = ___PDR_LUA_SIGNATURE;
  checkliteral(S, tmp + 1, "not a");  /* 1st char already checked */
  if (LoadByte(S) != ___PDR_LUAC_VERSION)
    error(S, "version mismatch in");
  if (LoadByte(S) != ___PDR_LUAC_FORMAT)
    error(S, "format mismatch in");
  checkliteral(S, ___PDR_LUAC_DATA, "corrupted");
  ___pdr_checksize(S, int);
  ___pdr_checksize(S, int);
  ___pdr_checksize(S, ___pdr_Instruction);
  ___pdr_checksize(S, ___pdr_lua_Integer);
  ___pdr_checksize(S, ___pdr_lua_Number);
  if (LoadInteger(S) != ___PDR_LUAC_INT)
    error(S, "endianness mismatch in");
  if (LoadNumber(S) != ___PDR_LUAC_NUM)
    error(S, "float format mismatch in");
}


/*
** load precompiled chunk
*/
___pdr_LClosure *___pdr_luaU_undump(___pdr_lua_State *L, ___pdr_ZIO *Z, const char *name) {
  LoadState S;
  ___pdr_LClosure *cl;
  if (*name == '@' || *name == '=')
    S.name = name + 1;
  else if (*name == ___PDR_LUA_SIGNATURE[0])
    S.name = "binary string";
  else
    S.name = name;
  S.L = L;
  S.Z = Z;
  checkHeader(&S);
  cl = ___pdr_luaF_newLclosure(L, LoadByte(&S));
  ___pdr_setclLvalue(L, L->top, cl);
  ___pdr_luaD_inctop(L);
  cl->p = ___pdr_luaF_newproto(L);
  LoadFunction(&S, cl->p, NULL);
  ___pdr_lua_assert(cl->nupvalues == cl->p->sizeupvalues);
  ___pdr_luai_verifycode(L, buff, cl->p);
  return cl;
}

} // end NS_PDR_SLUA