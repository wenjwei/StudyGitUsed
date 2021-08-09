/*
** $Id: lgc.h,v 2.91 2015/12/21 13:02:14 roberto Exp $
** Garbage Collector
** See Copyright Notice in lua.h
*/

#ifndef ___pdr_lgc_h___
#define ___pdr_lgc_h___


#include "lobject.h"
#include "lstate.h"

/*
** Collectable objects may have one of three colors: white, which
** means the object is not marked; gray, which means the
** object is marked, but its references may be not marked; and
** black, which means that the object and all its references are marked.
** The main invariant of the garbage collector, while marking objects,
** is that a black object can never point to a white one. Moreover,
** any gray object must be in a "gray list" (gray, grayagain, weak,
** allweak, ephemeron) so that it can be visited again before finishing
** the collection cycle. These lists have no meaning when the invariant
** is not being enforced (e.g., sweep phase).
*/



/* how much to allocate before next GC step */
#if !defined(___PDR_GCSTEPSIZE)
/* ~100 small strings */
#define ___PDR_GCSTEPSIZE	(___pdr_cast_int(100 * sizeof(___pdr_TString)))
#endif


/*
** Possible states of the Garbage Collector
*/
#define ___pdr_GCSpropagate	0
#define ___pdr_GCSatomic	1
#define ___pdr_GCSswpallgc	2
#define ___pdr_GCSswpfinobj	3
#define ___pdr_GCSswptobefnz	4
#define ___pdr_GCSswpend	5
#define ___pdr_GCScallfin	6
#define ___pdr_GCSpause	7


#define ___pdr_issweepphase(g)  \
	(___pdr_GCSswpallgc <= (g)->gcstate && (g)->gcstate <= ___pdr_GCSswpend)


/*
** macro to tell when main invariant (white objects cannot point to black
** ones) must be kept. During a collection, the sweep
** phase may break the invariant, as objects turned white may point to
** still-black objects. The invariant is restored when sweep ends and
** all objects are white again.
*/

#define ___pdr_keepinvariant(g)	((g)->gcstate <= ___pdr_GCSatomic)


/*
** some useful bit tricks
*/
#define ___pdr_resetbits(x,m)		((x) &= ___pdr_cast(___pdr_lu_byte, ~(m)))
#define ___pdr_setbits(x,m)		((x) |= (m))
#define ___pdr_testbits(x,m)		((x) & (m))
#define ___pdr_bitmask(b)		(1<<(b))
#define ___pdr_bit2mask(b1,b2)		(___pdr_bitmask(b1) | ___pdr_bitmask(b2))
#define ___pdr_l_setbit(x,b)		___pdr_setbits(x, ___pdr_bitmask(b))
#define ___pdr_resetbit(x,b)		___pdr_resetbits(x, ___pdr_bitmask(b))
#define ___pdr_testbit(x,b)		___pdr_testbits(x, ___pdr_bitmask(b))


/* Layout for bit use in 'marked' field: */
#define ___PDR_WHITE0BIT	0  /* object is white (type 0) */
#define ___PDR_WHITE1BIT	1  /* object is white (type 1) */
#define ___PDR_BLACKBIT	2  /* object is black */
#define ___PDR_FINALIZEDBIT	3  /* object has been marked for finalization */
/* bit 7 is currently used by tests (luaL_checkmemory) */

#define ___PDR_WHITEBITS	___pdr_bit2mask(___PDR_WHITE0BIT, ___PDR_WHITE1BIT)


#define ___pdr_iswhite(x)      ___pdr_testbits((x)->marked, ___PDR_WHITEBITS)
#define ___pdr_isblack(x)      ___pdr_testbit((x)->marked, ___PDR_BLACKBIT)
#define ___pdr_isgray(x)  /* neither white nor black */  \
	(!___pdr_testbits((x)->marked, ___PDR_WHITEBITS | ___pdr_bitmask(___PDR_BLACKBIT)))

#define ___pdr_tofinalize(x)	___pdr_testbit((x)->marked, ___PDR_FINALIZEDBIT)

#define ___pdr_otherwhite(g)	((g)->currentwhite ^ ___PDR_WHITEBITS)
#define ___pdr_isdeadm(ow,m)	(!(((m) ^ ___PDR_WHITEBITS) & (ow)))
#define ___pdr_isdead(g,v)	___pdr_isdeadm(___pdr_otherwhite(g), (v)->marked)

#define ___pdr_changewhite(x)	((x)->marked ^= ___PDR_WHITEBITS)
#define ___pdr_gray2black(x)	___pdr_l_setbit((x)->marked, ___PDR_BLACKBIT)

#define ___pdr_luaC_white(g)	___pdr_cast(___pdr_lu_byte, (g)->currentwhite & ___PDR_WHITEBITS)


/*
** Does one step of collection when debt becomes positive. 'pre'/'pos'
** allows some adjustments to be done only when needed. macro
** 'condchangemem' is used only for heavy tests (forcing a full
** GC cycle on every opportunity)
*/
#define ___pdr_luaC_condGC(L,pre,pos) \
	{ if (___pdr_G(L)->GCdebt > 0) { pre; ___pdr_luaC_step(L); pos;}; \
	  ___pdr_condchangemem(L,pre,pos); }

/* more often than not, 'pre'/'pos' are empty */
#define ___pdr_luaC_checkGC(L)		___pdr_luaC_condGC(L,(void)0,(void)0)


#define ___pdr_luaC_barrier(L,p,v) (  \
	(___pdr_iscollectable(v) && ___pdr_isblack(p) && ___pdr_iswhite(___pdr_gcvalue(v))) ?  \
	___pdr_luaC_barrier_(L,___pdr_obj2gco(p),___pdr_gcvalue(v)) : ___pdr_cast_void(0))

#define ___pdr_luaC_barrierback(L,p,v) (  \
	(___pdr_iscollectable(v) && ___pdr_isblack(p) && ___pdr_iswhite(___pdr_gcvalue(v))) ? \
	___pdr_luaC_barrierback_(L,p) : ___pdr_cast_void(0))

#define ___pdr_luaC_objbarrier(L,p,o) (  \
	(___pdr_isblack(p) && ___pdr_iswhite(o)) ? \
	___pdr_luaC_barrier_(L,___pdr_obj2gco(p),___pdr_obj2gco(o)) : ___pdr_cast_void(0))

#define ___pdr_luaC_upvalbarrier(L,uv) ( \
	(___pdr_iscollectable((uv)->v) && !___pdr_upisopen(uv)) ? \
         ___pdr_luaC_upvalbarrier_(L,uv) : ___pdr_cast_void(0))

namespace NS_PDR_SLUA {

___PDR_LUAI_FUNC void ___pdr_luaC_fix (___pdr_lua_State *L, ___pdr_GCObject *o);
___PDR_LUAI_FUNC void ___pdr_luaC_freeallobjects (___pdr_lua_State *L);
___PDR_LUAI_FUNC void ___pdr_luaC_step (___pdr_lua_State *L);
___PDR_LUAI_FUNC void ___pdr_luaC_runtilstate (___pdr_lua_State *L, int statesmask);
___PDR_LUAI_FUNC void ___pdr_luaC_fullgc (___pdr_lua_State *L, int isemergency);
___PDR_LUAI_FUNC ___pdr_GCObject *___pdr_luaC_newobj (___pdr_lua_State *L, int tt, size_t sz);
___PDR_LUAI_FUNC void ___pdr_luaC_barrier_ (___pdr_lua_State *L, ___pdr_GCObject *o, ___pdr_GCObject *v);
___PDR_LUAI_FUNC void ___pdr_luaC_barrierback_ (___pdr_lua_State *L, ___pdr_Table *o);
___PDR_LUAI_FUNC void ___pdr_luaC_upvalbarrier_ (___pdr_lua_State *L, ___pdr_UpVal *uv);
___PDR_LUAI_FUNC void ___pdr_luaC_checkfinalizer (___pdr_lua_State *L, ___pdr_GCObject *o, ___pdr_Table *mt);
___PDR_LUAI_FUNC void ___pdr_luaC_upvdeccount (___pdr_lua_State *L, ___pdr_UpVal *uv);

} // end NS_PDR_SLUA

#endif
