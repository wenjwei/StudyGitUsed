/*
** $Id: lobject.h,v 2.117 2016/08/01 19:51:24 roberto Exp $
** Type definitions for Lua objects
** See Copyright Notice in lua.h
*/


#ifndef ___pdr_lobject_h___
#define ___pdr_lobject_h___

#include <stdarg.h>

#include "llimits.h"
#include "lua.h"

namespace NS_PDR_SLUA {

/*
** Extra tags for non-values
*/
#define ___PDR_LUA_TPROTO	___PDR_LUA_NUMTAGS		/* function prototypes */
#define ___PDR_LUA_TDEADKEY	(___PDR_LUA_NUMTAGS+1)		/* removed keys in tables */

/*
** number of all possible tags (including LUA_TNONE but excluding DEADKEY)
*/
#define ___PDR_LUA_TOTALTAGS	(___PDR_LUA_TPROTO + 2)


/*
** tags for Tagged Values have the following use of bits:
** bits 0-3: actual tag (a LUA_T* value)
** bits 4-5: variant bits
** bit 6: whether value is collectable
*/


/*
** LUA_TFUNCTION variants:
** 0 - Lua function
** 1 - light C function
** 2 - regular C function (closure)
*/

/* Variant tags for functions */
#define ___PDR_LUA_TLCL	(___PDR_LUA_TFUNCTION | (0 << 4))  /* Lua closure */
#define ___PDR_LUA_TLCF	(___PDR_LUA_TFUNCTION | (1 << 4))  /* light C function */
#define ___PDR_LUA_TCCL	(___PDR_LUA_TFUNCTION | (2 << 4))  /* C closure */


/* Variant tags for strings */
#define ___PDR_LUA_TSHRSTR	(___PDR_LUA_TSTRING | (0 << 4))  /* short strings */
#define ___PDR_LUA_TLNGSTR	(___PDR_LUA_TSTRING | (1 << 4))  /* long strings */


/* Variant tags for numbers */
#define ___PDR_LUA_TNUMFLT	(___PDR_LUA_TNUMBER | (0 << 4))  /* float numbers */
#define ___PDR_LUA_TNUMINT	(___PDR_LUA_TNUMBER | (1 << 4))  /* integer numbers */


/* Bit mark for collectable types */
#define ___PDR_BIT_ISCOLLECTABLE	(1 << 6)

/* mark a tag as collectable */
#define __pdr_ctb(t)			((t) | ___PDR_BIT_ISCOLLECTABLE)


/*
** Common type for all collectable objects
*/
typedef struct ___pdr_GCObject ___pdr_GCObject;


/*
** Common Header for all collectable objects (in macro form, to be
** included in other objects)
*/
#define ___pdr_CommonHeader	___pdr_GCObject *next; ___pdr_lu_byte tt; ___pdr_lu_byte marked


/*
** Common type has only the common header
*/
struct ___pdr_GCObject {
  ___pdr_CommonHeader;
};




/*
** Tagged Values. This is the basic representation of values in Lua,
** an actual value plus a tag with its type.
*/

/*
** Union of all Lua values
*/
typedef union ___pdr_Value {
  ___pdr_GCObject *gc;    /* collectable objects */
  void *p;         /* light userdata */
  int b;           /* booleans */
  ___pdr_lua_CFunction f; /* light C functions */
  ___pdr_lua_Integer i;   /* integer numbers */
  ___pdr_lua_Number n;    /* float numbers */
} ___pdr_Value;


#define ___pdr_TValuefields	___pdr_Value value_; int tt_


typedef struct lua_TValue {
  ___pdr_TValuefields;
} ___pdr_TValue;



/* macro defining a nil value */
#define ___PDR_NILCONSTANT	{NULL}, ___PDR_LUA_TNIL


#define ___pdr_val_(o)		((o)->value_)


/* raw type tag of a TValue */
#define ___pdr_rttype(o)	((o)->tt_)

/* tag with no variants (bits 0-3) */
#define ___pdr_novariant(x)	((x) & 0x0F)

/* type tag of a TValue (bits 0-3 for tags + variant bits 4-5) */
#define ___pdr_ttype(o)	(___pdr_rttype(o) & 0x3F)

/* type tag of a TValue with no variants (bits 0-3) */
#define ___pdr_ttnov(o)	(___pdr_novariant(___pdr_rttype(o)))


/* Macros to test type */
#define ___pdr_checktag(o,t)		(___pdr_rttype(o) == (t))
#define ___pdr_checktype(o,t)		(___pdr_ttnov(o) == (t))
#define ___pdr_ttisnumber(o)		___pdr_checktype((o), ___PDR_LUA_TNUMBER)
#define ___pdr_ttisfloat(o)		___pdr_checktag((o), ___PDR_LUA_TNUMFLT)
#define ___pdr_ttisinteger(o)		___pdr_checktag((o), ___PDR_LUA_TNUMINT)
#define ___pdr_ttisnil(o)		___pdr_checktag((o), ___PDR_LUA_TNIL)
#define ___pdr_ttisboolean(o)		___pdr_checktag((o), ___PDR_LUA_TBOOLEAN)
#define ___pdr_ttislightuserdata(o)	___pdr_checktag((o), ___PDR_LUA_TLIGHTUSERDATA)
#define ___pdr_ttisstring(o)		___pdr_checktype((o), ___PDR_LUA_TSTRING)
#define ___pdr_ttisshrstring(o)	___pdr_checktag((o), __pdr_ctb(___PDR_LUA_TSHRSTR))
#define ___pdr_ttislngstring(o)	___pdr_checktag((o), __pdr_ctb(___PDR_LUA_TLNGSTR))
#define ___pdr_ttistable(o)		___pdr_checktag((o), __pdr_ctb(___PDR_LUA_TTABLE))
#define ___pdr_ttisfunction(o)		___pdr_checktype(o, ___PDR_LUA_TFUNCTION)
#define ___pdr_ttisclosure(o)		((___pdr_rttype(o) & 0x1F) == ___PDR_LUA_TFUNCTION)
#define ___pdr_ttisCclosure(o)		___pdr_checktag((o), __pdr_ctb(___PDR_LUA_TCCL))
#define ___pdr_ttisLclosure(o)		___pdr_checktag((o), __pdr_ctb(___PDR_LUA_TLCL))
#define ___pdr_ttislcf(o)		___pdr_checktag((o), ___PDR_LUA_TLCF)
#define ___pdr_ttisfulluserdata(o)	___pdr_checktag((o), __pdr_ctb(___PDR_LUA_TUSERDATA))
#define ___pdr_ttisthread(o)		___pdr_checktag((o), __pdr_ctb(___PDR_LUA_TTHREAD))
#define ___pdr_ttisdeadkey(o)		___pdr_checktag((o), ___PDR_LUA_TDEADKEY)


/* Macros to access values */
#define ___pdr_ivalue(o)	___pdr_check_exp(___pdr_ttisinteger(o), ___pdr_val_(o).i)
#define ___pdr_fltvalue(o)	___pdr_check_exp(___pdr_ttisfloat(o), ___pdr_val_(o).n)
#define ___pdr_nvalue(o)	___pdr_check_exp(___pdr_ttisnumber(o), \
	(___pdr_ttisinteger(o) ? ___pdr_cast_num(___pdr_ivalue(o)) : ___pdr_fltvalue(o)))
#define ___pdr_gcvalue(o)	___pdr_check_exp(___pdr_iscollectable(o), ___pdr_val_(o).gc)
#define ___pdr_pvalue(o)	___pdr_check_exp(___pdr_ttislightuserdata(o), ___pdr_val_(o).p)
#define ___pdr_tsvalue(o)	___pdr_check_exp(___pdr_ttisstring(o), ___pdr_gco2ts(___pdr_val_(o).gc))
#define ___pdr_uvalue(o)	___pdr_check_exp(___pdr_ttisfulluserdata(o), ___pdr_gco2u(___pdr_val_(o).gc))
#define ___pdr_clvalue(o)	___pdr_check_exp(___pdr_ttisclosure(o), ___pdr_gco2cl(___pdr_val_(o).gc))
#define ___pdr_clLvalue(o)	___pdr_check_exp(___pdr_ttisLclosure(o), ___pdr_gco2lcl(___pdr_val_(o).gc))
#define ___pdr_clCvalue(o)	___pdr_check_exp(___pdr_ttisCclosure(o), ___pdr_gco2ccl(___pdr_val_(o).gc))
#define ___pdr_fvalue(o)	___pdr_check_exp(___pdr_ttislcf(o), ___pdr_val_(o).f)
#define ___pdr_hvalue(o)	___pdr_check_exp(___pdr_ttistable(o), ___pdr_gco2t(___pdr_val_(o).gc))
#define ___pdr_bvalue(o)	___pdr_check_exp(___pdr_ttisboolean(o), ___pdr_val_(o).b)
#define ___pdr_thvalue(o)	___pdr_check_exp(___pdr_ttisthread(o), ___pdr_gco2th(___pdr_val_(o).gc))
/* a dead value may get the 'gc' field, but cannot access its contents */
#define ___pdr_deadvalue(o)	___pdr_check_exp(___pdr_ttisdeadkey(o), ___pdr_cast(void *, ___pdr_val_(o).gc))

#define ___pdr_l_isfalse(o)	(___pdr_ttisnil(o) || (___pdr_ttisboolean(o) && ___pdr_bvalue(o) == 0))


#define ___pdr_iscollectable(o)	(___pdr_rttype(o) & ___PDR_BIT_ISCOLLECTABLE)


/* Macros for internal tests */
#define ___pdr_righttt(obj)		(___pdr_ttype(obj) == ___pdr_gcvalue(obj)->tt)

#define ___pdr_checkliveness(L,obj) \
	___pdr_lua_longassert(!___pdr_iscollectable(obj) || \
		(___pdr_righttt(obj) && (L == NULL || !___pdr_isdead(___pdr_G(L),___pdr_gcvalue(obj)))))


/* Macros to set values */
#define ___pdr_settt_(o,t)	((o)->tt_=(t))

#define ___pdr_setfltvalue(obj,x) \
  { ___pdr_TValue *io=(obj); ___pdr_val_(io).n=(x); ___pdr_settt_(io, ___PDR_LUA_TNUMFLT); }

#define ___pdr_chgfltvalue(obj,x) \
  { ___pdr_TValue *io=(obj); ___pdr_lua_assert(___pdr_ttisfloat(io)); ___pdr_val_(io).n=(x); }

#define ___pdr_setivalue(obj,x) \
  { ___pdr_TValue *io=(obj); ___pdr_val_(io).i=(x); ___pdr_settt_(io, ___PDR_LUA_TNUMINT); }

#define ___pdr_chgivalue(obj,x) \
  { ___pdr_TValue *io=(obj); ___pdr_lua_assert(___pdr_ttisinteger(io)); ___pdr_val_(io).i=(x); }

#define ___pdr_setnilvalue(obj) ___pdr_settt_(obj, ___PDR_LUA_TNIL)

#define ___pdr_setfvalue(obj,x) \
  { ___pdr_TValue *io=(obj); ___pdr_val_(io).f=(x); ___pdr_settt_(io, ___PDR_LUA_TLCF); }

#define ___pdr_setpvalue(obj,x) \
  { ___pdr_TValue *io=(obj); ___pdr_val_(io).p=(x); ___pdr_settt_(io, ___PDR_LUA_TLIGHTUSERDATA); }

#define ___pdr_setbvalue(obj,x) \
  { ___pdr_TValue *io=(obj); ___pdr_val_(io).b=(x); ___pdr_settt_(io, ___PDR_LUA_TBOOLEAN); }

#define ___pdr_setgcovalue(L,obj,x) \
  { ___pdr_TValue *io = (obj); ___pdr_GCObject *i_g=(x); \
    ___pdr_val_(io).gc = i_g; ___pdr_settt_(io, __pdr_ctb(i_g->tt)); }

#define ___pdr_setsvalue(L,obj,x) \
  { ___pdr_TValue *io = (obj); ___pdr_TString *x_ = (x); \
    ___pdr_val_(io).gc = ___pdr_obj2gco(x_); ___pdr_settt_(io, __pdr_ctb(x_->tt)); \
    ___pdr_checkliveness(L,io); }

#define ___pdr_setuvalue(L,obj,x) \
  { ___pdr_TValue *io = (obj); ___pdr_Udata *x_ = (x); \
    ___pdr_val_(io).gc = ___pdr_obj2gco(x_); ___pdr_settt_(io, __pdr_ctb(___PDR_LUA_TUSERDATA)); \
    ___pdr_checkliveness(L,io); }

#define ___pdr_setthvalue(L,obj,x) \
  { ___pdr_TValue *io = (obj); ___pdr_lua_State *x_ = (x); \
    ___pdr_val_(io).gc = ___pdr_obj2gco(x_); ___pdr_settt_(io, __pdr_ctb(___PDR_LUA_TTHREAD)); \
    ___pdr_checkliveness(L,io); }

#define ___pdr_setclLvalue(L,obj,x) \
  { ___pdr_TValue *io = (obj); ___pdr_LClosure *x_ = (x); \
    ___pdr_val_(io).gc = ___pdr_obj2gco(x_); ___pdr_settt_(io, __pdr_ctb(___PDR_LUA_TLCL)); \
    ___pdr_checkliveness(L,io); }

#define ___pdr_setclCvalue(L,obj,x) \
  { ___pdr_TValue *io = (obj); ___pdr_CClosure *x_ = (x); \
    ___pdr_val_(io).gc = ___pdr_obj2gco(x_); ___pdr_settt_(io, __pdr_ctb(___PDR_LUA_TCCL)); \
    ___pdr_checkliveness(L,io); }

#define ___pdr_sethvalue(L,obj,x) \
  { ___pdr_TValue *io = (obj); ___pdr_Table *x_ = (x); \
    ___pdr_val_(io).gc = ___pdr_obj2gco(x_); ___pdr_settt_(io, __pdr_ctb(___PDR_LUA_TTABLE)); \
    ___pdr_checkliveness(L,io); }

#define ___pdr_setdeadvalue(obj)	___pdr_settt_(obj, ___PDR_LUA_TDEADKEY)



#define ___pdr_setobj(L,obj1,obj2) \
	{ ___pdr_TValue *io1=(obj1); *io1 = *(obj2); \
	  (void)L; ___pdr_checkliveness(L,io1); }


/*
** different types of assignments, according to destination
*/

/* from stack to (same) stack */
#define ___pdr_setobjs2s	___pdr_setobj
/* to stack (not from same stack) */
#define ___pdr_setobj2s	___pdr_setobj
#define ___pdr_setsvalue2s	___pdr_setsvalue
#define ___pdr_sethvalue2s	___pdr_sethvalue
#define ___pdr_setptvalue2s	setptvalue
/* from table to same table */
#define ___pdr_setobjt2t	___pdr_setobj
/* to new object */
#define ___pdr_setobj2n	___pdr_setobj
#define ___pdr_setsvalue2n	___pdr_setsvalue

/* to table (define it as an expression to be used in macros) */
#define ___pdr_setobj2t(L,o1,o2)  ((void)L, *(o1)=*(o2), ___pdr_checkliveness(L,(o1)))




/*
** {======================================================
** types and prototypes
** =======================================================
*/


typedef ___pdr_TValue *___pdr_StkId;  /* index to stack elements */




/*
** Header for string value; string bytes follow the end of this structure
** (aligned according to 'UTString'; see next).
*/
typedef struct ___pdr_TString {
  ___pdr_CommonHeader;
  ___pdr_lu_byte extra;  /* reserved words for short strings; "has hash" for longs */
  ___pdr_lu_byte shrlen;  /* length for short strings */
  unsigned int hash;
  union {
    size_t lnglen;  /* length for long strings */
    struct ___pdr_TString *hnext;  /* linked list for hash table */
  } u;
} ___pdr_TString;


/*
** Ensures that address after this type is always fully aligned.
*/
typedef union ___pdr_UTString {
  ___pdr_L_Umaxalign dummy;  /* ensures maximum alignment for strings */
  ___pdr_TString tsv;
} ___pdr_UTString;


/*
** Get the actual string (array of bytes) from a 'TString'.
** (Access to 'extra' ensures that value is really a 'TString'.)
*/
#define ___pdr_getstr(ts)  \
  ___pdr_check_exp(sizeof((ts)->extra), ___pdr_cast(char *, (ts)) + sizeof(___pdr_UTString))


/* get the actual string (array of bytes) from a Lua value */
#define ___pdr_svalue(o)       ___pdr_getstr(___pdr_tsvalue(o))

/* get string length from 'TString *s' */
#define ___pdr_tsslen(s)	((s)->tt == ___PDR_LUA_TSHRSTR ? (s)->shrlen : (s)->u.lnglen)

/* get string length from 'TValue *o' */
#define ___pdr_vslen(o)	___pdr_tsslen(___pdr_tsvalue(o))


/*
** Header for userdata; memory area follows the end of this structure
** (aligned according to 'UUdata'; see next).
*/
typedef struct ___pdr_Udata {
  ___pdr_CommonHeader;
  ___pdr_lu_byte ttuv_;  /* user value's tag */
  struct ___pdr_Table *metatable;
  size_t len;  /* number of bytes */
  union ___pdr_Value user_;  /* user value */
} ___pdr_Udata;


/*
** Ensures that address after this type is always fully aligned.
*/
typedef union ___pdr_UUdata {
  ___pdr_L_Umaxalign dummy;  /* ensures maximum alignment for 'local' udata */
  ___pdr_Udata uv;
} ___pdr_UUdata;


/*
**  Get the address of memory block inside 'Udata'.
** (Access to 'ttuv_' ensures that value is really a 'Udata'.)
*/
#define ___pdr_getudatamem(u)  \
  ___pdr_check_exp(sizeof((u)->ttuv_), (___pdr_cast(char*, (u)) + sizeof(___pdr_UUdata)))

#define ___pdr_setuservalue(L,u,o) \
	{ const ___pdr_TValue *io=(o); ___pdr_Udata *iu = (u); \
	  iu->user_ = io->value_; iu->ttuv_ = ___pdr_rttype(io); \
	  ___pdr_checkliveness(L,io); }


#define ___pdr_getuservalue(L,u,o) \
	{ ___pdr_TValue *io=(o); const ___pdr_Udata *iu = (u); \
	  io->value_ = iu->user_; ___pdr_settt_(io, iu->ttuv_); \
	  ___pdr_checkliveness(L,io); }


/*
** Description of an upvalue for function prototypes
*/
typedef struct ___pdr_Upvaldesc {
  ___pdr_TString *name;  /* upvalue name (for debug information) */
  ___pdr_lu_byte instack;  /* whether it is in stack (register) */
  ___pdr_lu_byte idx;  /* index of upvalue (in stack or in outer function's list) */
} ___pdr_Upvaldesc;


/*
** Description of a local variable for function prototypes
** (used for debug information)
*/
typedef struct ___pdr_LocVar {
  ___pdr_TString *varname;
  int startpc;  /* first point where variable is active */
  int endpc;    /* first point where variable is dead */
} ___pdr_LocVar;


/*
** Function Prototypes
*/
typedef struct ___pdr_Proto {
  ___pdr_CommonHeader;
  ___pdr_lu_byte numparams;  /* number of fixed parameters */
  ___pdr_lu_byte is_vararg;
  ___pdr_lu_byte maxstacksize;  /* number of registers needed by this function */
  int sizeupvalues;  /* size of 'upvalues' */
  int sizek;  /* size of 'k' */
  int sizecode;
  int sizelineinfo;
  int sizep;  /* size of 'p' */
  int sizelocvars;
  int linedefined;  /* debug information  */
  int lastlinedefined;  /* debug information  */
  ___pdr_TValue *k;  /* constants used by the function */
  ___pdr_Instruction *code;  /* opcodes */
  struct ___pdr_Proto **p;  /* functions defined inside the function */
  int *lineinfo;  /* map from opcodes to source lines (debug information) */
  ___pdr_LocVar *locvars;  /* information about local variables (debug information) */
  ___pdr_Upvaldesc *upvalues;  /* upvalue information */
  struct ___pdr_LClosure *cache;  /* last-created closure with this prototype */
  ___pdr_TString  *source;  /* used for debug information */
  ___pdr_GCObject *gclist;
} ___pdr_Proto;



/*
** Lua Upvalues
*/
typedef struct ___pdr_UpVal ___pdr_UpVal;


/*
** Closures
*/

#define ___pdr_ClosureHeader \
	___pdr_CommonHeader; ___pdr_lu_byte nupvalues; ___pdr_GCObject *gclist

typedef struct ___pdr_CClosure {
  ___pdr_ClosureHeader;
  ___pdr_lua_CFunction f;
  ___pdr_TValue upvalue[1];  /* list of upvalues */
} ___pdr_CClosure;


typedef struct ___pdr_LClosure {
  ___pdr_ClosureHeader;
  struct ___pdr_Proto *p;
  ___pdr_UpVal *upvals[1];  /* list of upvalues */
} ___pdr_LClosure;


typedef union ___pdr_Closure {
  ___pdr_CClosure c;
  ___pdr_LClosure l;
} ___pdr_Closure;


#define ___pdr_isLfunction(o)	___pdr_ttisLclosure(o)

#define ___pdr_getproto(o)	(___pdr_clLvalue(o)->p)


/*
** Tables
*/

typedef union ___pdr_TKey {
  struct {
    ___pdr_TValuefields;
    int next;  /* for chaining (offset for next node) */
  } nk;
  ___pdr_TValue tvk;
} ___pdr_TKey;


/* copy a value into a key without messing up field 'next' */
#define ___pdr_setnodekey(L,key,obj) \
	{ ___pdr_TKey *k_=(key); const ___pdr_TValue *io_=(obj); \
	  k_->nk.value_ = io_->value_; k_->nk.tt_ = io_->tt_; \
	  (void)L; ___pdr_checkliveness(L,io_); }


typedef struct ___pdr_Node {
  ___pdr_TValue i_val;
  ___pdr_TKey i_key;
} ___pdr_Node;


typedef struct ___pdr_Table {
  ___pdr_CommonHeader;
  ___pdr_lu_byte flags;  /* 1<<p means tagmethod(p) is not present */
  ___pdr_lu_byte lsizenode;  /* log2 of size of 'node' array */
  unsigned int sizearray;  /* size of 'array' array */
  ___pdr_TValue *array;  /* array part */
  ___pdr_Node *node;
  ___pdr_Node *lastfree;  /* any free position is before this position */
  struct ___pdr_Table *metatable;
  ___pdr_GCObject *gclist;
} ___pdr_Table;



/*
** 'module' operation for hashing (size is always a power of 2)
*/
#define ___pdr_lmod(s,size) \
	(___pdr_check_exp((size&(size-1))==0, (___pdr_cast(int, (s) & ((size)-1)))))


#define ___pdr_twoto(x)	(1LL<<(x))
#define ___pdr_sizenode(t)	(___pdr_twoto((t)->lsizenode))


/*
** (address of) a fixed nil value
*/
#define ___pdr_luaO_nilobject		(&___pdr_luaO_nilobject_)


___PDR_LUAI_DDEC const ___pdr_TValue ___pdr_luaO_nilobject_;

/* size of buffer for 'luaO_utf8esc' function */
#define ___PDR_UTF8BUFFSZ	8

___PDR_LUAI_FUNC int ___pdr_luaO_int2fb (unsigned int x);
___PDR_LUAI_FUNC int ___pdr_luaO_fb2int (int x);
___PDR_LUAI_FUNC int ___pdr_luaO_utf8esc (char *buff, unsigned long x);
___PDR_LUAI_FUNC int ___pdr_luaO_ceillog2 (unsigned int x);
___PDR_LUAI_FUNC void ___pdr_luaO_arith (___pdr_lua_State *L, int op, const ___pdr_TValue *p1,
                           const ___pdr_TValue *p2, ___pdr_TValue *res);
___PDR_LUAI_FUNC size_t ___pdr_luaO_str2num (const char *s, ___pdr_TValue *o);
___PDR_LUAI_FUNC int ___pdr_luaO_hexavalue (int c);
___PDR_LUAI_FUNC void ___pdr_luaO_tostring (___pdr_lua_State *L, ___pdr_StkId obj);
___PDR_LUAI_FUNC const char *___pdr_luaO_pushvfstring (___pdr_lua_State *L, const char *fmt,
                                                       va_list argp);
___PDR_LUAI_FUNC const char *___pdr_luaO_pushfstring (___pdr_lua_State *L, const char *fmt, ...);
___PDR_LUAI_FUNC void ___pdr_luaO_chunkid (char *out, const char *source, size_t len);

} // end NS_PDR_SLUA

#endif

