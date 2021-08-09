/*
** $Id: ldump.c,v 2.37 2015/10/08 15:53:49 roberto Exp $
** save precompiled Lua chunks
** See Copyright Notice in lua.h
*/

#define ___pdr_ldump_c
#define ___PDR_LUA_CORE

#include "lprefix.h"

#include <stddef.h>

#include "lua.h"
#include "lobject.h"
#include "lstate.h"
#include "lundump.h"

namespace NS_PDR_SLUA {

#ifdef ___PDR_LUAC

typedef struct {
  ___pdr_lua_State *L;
  ___pdr_lua_Writer writer;
  void *data;
  int strip;
  int status;
} DumpState;


/*
** All high-level dumps go through DumpVector; you can change it to
** change the endianness of the result
*/
#define ___pdr_DumpVector(v,n,D)	DumpBlock(v,(n)*sizeof((v)[0]),D)

#define ___pdr_DumpLiteral(s,D)	DumpBlock(s, sizeof(s) - sizeof(char), D)


static void DumpBlock (const void *b, size_t size, DumpState *D) {
  if (D->status == 0 && size > 0) {
    ___pdr_lua_unlock(D->L);
    D->status = (*D->writer)(D->L, b, size, D->data);
    ___pdr_lua_lock(D->L);
  }
}


#define ___pdr_DumpVar(x,D)		___pdr_DumpVector(&x,1,D)


static void DumpByte (int y, DumpState *D) {
  ___pdr_lu_byte x = (___pdr_lu_byte)y;
  ___pdr_DumpVar(x, D);
}


static void DumpInt (int x, DumpState *D) {
  ___pdr_DumpVar(x, D);
}


static void DumpNumber (___pdr_lua_Number x, DumpState *D) {
  ___pdr_DumpVar(x, D);
}


static void DumpInteger (___pdr_lua_Integer x, DumpState *D) {
  ___pdr_DumpVar(x, D);
}


static void DumpString (const ___pdr_TString *s, DumpState *D) {
  if (s == NULL)
    DumpByte(0, D);
  else {
    int size = ___pdr_tsslen(s) + 1;  /* include trailing '\0' */
    const char *str = ___pdr_getstr(s);
    if (size < 0xFF)
      DumpByte(___pdr_cast_int(size), D);
    else {
      DumpByte(0xFF, D);
      ___pdr_DumpVar(size, D);
    }
    ___pdr_DumpVector(str, size - 1, D);  /* no need to save '\0' */
  }
}


static void DumpCode (const ___pdr_Proto *f, DumpState *D) {
  DumpInt(f->sizecode, D);
  ___pdr_DumpVector(f->code, f->sizecode, D);
}


static void DumpFunction(const ___pdr_Proto *f, ___pdr_TString *psource, DumpState *D);

static void DumpConstants (const ___pdr_Proto *f, DumpState *D) {
  int i;
  int n = f->sizek;
  DumpInt(n, D);
  for (i = 0; i < n; i++) {
    const ___pdr_TValue *o = &f->k[i];
    DumpByte(___pdr_ttype(o), D);
    switch (___pdr_ttype(o)) {
    case ___PDR_LUA_TNIL:
      break;
    case ___PDR_LUA_TBOOLEAN:
      DumpByte(___pdr_bvalue(o), D);
      break;
    case ___PDR_LUA_TNUMFLT:
      DumpNumber(___pdr_fltvalue(o), D);
      break;
    case ___PDR_LUA_TNUMINT:
      DumpInteger(___pdr_ivalue(o), D);
      break;
    case ___PDR_LUA_TSHRSTR:
    case ___PDR_LUA_TLNGSTR:
      DumpString(___pdr_tsvalue(o), D);
      break;
    default:
      ___pdr_lua_assert(0);
    }
  }
}


static void DumpProtos (const ___pdr_Proto *f, DumpState *D) {
  int i;
  int n = f->sizep;
  DumpInt(n, D);
  for (i = 0; i < n; i++)
    DumpFunction(f->p[i], f->source, D);
}


static void DumpUpvalues (const ___pdr_Proto *f, DumpState *D) {
  int i, n = f->sizeupvalues;
  DumpInt(n, D);
  for (i = 0; i < n; i++) {
    DumpByte(f->upvalues[i].instack, D);
    DumpByte(f->upvalues[i].idx, D);
  }
}


static void DumpDebug (const ___pdr_Proto *f, DumpState *D) {
  int i, n;
  n = (D->strip) ? 0 : f->sizelineinfo;
  DumpInt(n, D);
  ___pdr_DumpVector(f->lineinfo, n, D);
  n = (D->strip) ? 0 : f->sizelocvars;
  DumpInt(n, D);
  for (i = 0; i < n; i++) {
    DumpString(f->locvars[i].varname, D);
    DumpInt(f->locvars[i].startpc, D);
    DumpInt(f->locvars[i].endpc, D);
  }
  n = (D->strip) ? 0 : f->sizeupvalues;
  DumpInt(n, D);
  for (i = 0; i < n; i++)
    DumpString(f->upvalues[i].name, D);
}


static void DumpFunction (const ___pdr_Proto *f, ___pdr_TString *psource, DumpState *D) {
  if (D->strip || f->source == psource)
    DumpString(NULL, D);  /* no debug info or same source as its parent */
  else
    DumpString(f->source, D);
  DumpInt(f->linedefined, D);
  DumpInt(f->lastlinedefined, D);
  DumpByte(f->numparams, D);
  DumpByte(f->is_vararg, D);
  DumpByte(f->maxstacksize, D);
  DumpCode(f, D);
  DumpConstants(f, D);
  DumpUpvalues(f, D);
  DumpProtos(f, D);
  DumpDebug(f, D);
}


static void DumpHeader (DumpState *D) {
  ___pdr_DumpLiteral(___PDR_LUA_SIGNATURE, D);
  DumpByte(___PDR_LUAC_VERSION, D);
  DumpByte(___PDR_LUAC_FORMAT, D);
  ___pdr_DumpLiteral(___PDR_LUAC_DATA, D);
  DumpByte(sizeof(int), D);
  DumpByte(sizeof(int), D);
  DumpByte(sizeof(___pdr_Instruction), D);
  DumpByte(sizeof(___pdr_lua_Integer), D);
  DumpByte(sizeof(___pdr_lua_Number), D);
  DumpInteger(___PDR_LUAC_INT, D);
  DumpNumber(___PDR_LUAC_NUM, D);
}

/*
** dump Lua function as precompiled chunk
*/
int ___pdr_luaU_dump(___pdr_lua_State *L, const ___pdr_Proto *f, ___pdr_lua_Writer w, void *data,
              int strip) {
  DumpState D;
  D.L = L;
  D.writer = w;
  D.data = data;
  D.strip = strip;
  D.status = 0;
  DumpHeader(&D);
  DumpByte(f->sizeupvalues, &D);
  DumpFunction(f, NULL, &D);
  return D.status;
}

#endif // end ___PDR_LUAC

} // end NS_PDR_SLUA