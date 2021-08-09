/*
** $Id: ltable.c,v 2.118 2016/11/07 12:38:35 roberto Exp $
** Lua tables (hash)
** See Copyright Notice in lua.h
*/

#define ___pdr_ltable_c
#define ___PDR_LUA_CORE

#include "ltable.h"
#include "lprefix.h"

/*
** Implementation of tables (aka arrays, objects, or hash tables).
** Tables keep its elements in two parts: an array part and a hash part.
** Non-negative integer keys are all candidates to be kept in the array
** part. The actual size of the array is the largest 'n' such that
** more than half the slots between 1 and n are in use.
** Hash uses a mix of chained scatter table with Brent's variation.
** A main invariant of these tables is that, if an element is not
** in its main position (i.e. the 'original' position that its hash gives
** to it), then the colliding element is in its own main position.
** Hence even when the load factor reaches 100%, performance remains good.
*/

#include <math.h>
#include <limits.h>

#include "lua.h"
#include "ldebug.h"
#include "ldo.h"
#include "lgc.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"
#include "lstring.h"
#include "lvm.h"

namespace NS_PDR_SLUA {

/*
** Maximum size of array part (MAXASIZE) is 2^MAXABITS. MAXABITS is
** the largest integer such that MAXASIZE fits in an unsigned int.
*/
#define ___PDR_MAXABITS	___pdr_cast_int(sizeof(int) * CHAR_BIT - 1)
#define ___PDR_MAXASIZE	(1u << ___PDR_MAXABITS)

/*
** Maximum size of hash part is 2^MAXHBITS. MAXHBITS is the largest
** integer such that 2^MAXHBITS fits in a signed int. (Note that the
** maximum number of elements in a table, 2^MAXABITS + 2^MAXHBITS, still
** fits comfortably in an unsigned int.)
*/
#define ___PDR_MAXHBITS	(___PDR_MAXABITS - 1)


#define ___pdr_hashpow2(t,n)		(___pdr_gnode(t, ___pdr_lmod((n), ___pdr_sizenode(t))))

#define ___pdr_hashstr(t,str)       ___pdr_hashpow2(t, (str)->hash)
#define ___pdr_hashboolean(t,p)     ___pdr_hashpow2(t, p)
#define ___pdr_hashint(t,i)         ___pdr_hashpow2(t, i)


/*
** for some types, it is better to avoid modulus by power of 2, as
** they tend to have many 2 factors.
*/
#define ___pdr_hashmod(t,n)	(___pdr_gnode(t, ((n) % ((___pdr_sizenode(t)-1)|1))))


#define ___pdr_hashpointer(t,p)	___pdr_hashmod(t, ___pdr_point2uint(p))


#define ___pdr_dummynode		(&dummynode_)

static const ___pdr_Node dummynode_ = {
  {___PDR_NILCONSTANT},  /* value */
  {{___PDR_NILCONSTANT, 0}}  /* key */
};


/*
** Hash for floating-point numbers.
** The main computation should be just
**     n = frexp(n, &i); return (n * INT_MAX) + i
** but there are some numerical subtleties.
** In a two-complement representation, INT_MAX does not has an exact
** representation as a float, but INT_MIN does; because the absolute
** value of 'frexp' is smaller than 1 (unless 'n' is inf/NaN), the
** absolute value of the product 'frexp * -INT_MIN' is smaller or equal
** to INT_MAX. Next, the use of 'unsigned int' avoids overflows when
** adding 'i'; the use of '~u' (instead of '-u') avoids problems with
** INT_MIN.
*/
#if !defined(___ptr_l_hashfloat)
static int ___ptr_l_hashfloat (___pdr_lua_Number n) {
  int i;
  ___pdr_lua_Integer ni;
  n = ___pdr_l_mathop(frexp)(n, &i) * -___pdr_cast_num(INT_MIN);
  if (!___pdr_lua_numbertointeger(n, &ni)) {  /* is 'n' inf/-inf/NaN? */
    ___pdr_lua_assert(___pdr_luai_numisnan(n) || ___pdr_l_mathop(fabs)(n) == ___pdr_cast_num(HUGE_VAL));
    return 0;
  }
  else {  /* normal case */
    unsigned int u = ___pdr_cast(unsigned int, i) + ___pdr_cast(unsigned int, ni);
    return ___pdr_cast_int(u <= ___pdr_cast(unsigned int, INT_MAX) ? u : ~u);
  }
}
#endif


/*
** returns the 'main' position of an element in a table (that is, the index
** of its hash value)
*/
static ___pdr_Node *mainposition (const ___pdr_Table *t, const ___pdr_TValue *key) {
  switch (___pdr_ttype(key)) {
    case ___PDR_LUA_TNUMINT:
      return ___pdr_hashint(t, ___pdr_ivalue(key));
    case ___PDR_LUA_TNUMFLT:
      return ___pdr_hashmod(t, ___ptr_l_hashfloat(___pdr_fltvalue(key)));
    case ___PDR_LUA_TSHRSTR:
      return ___pdr_hashstr(t, ___pdr_tsvalue(key));
    case ___PDR_LUA_TLNGSTR:
      return ___pdr_hashpow2(t, ___pdr_luaS_hashlongstr(___pdr_tsvalue(key)));
    case ___PDR_LUA_TBOOLEAN:
      return ___pdr_hashboolean(t, ___pdr_bvalue(key));
    case ___PDR_LUA_TLIGHTUSERDATA:
      return ___pdr_hashpointer(t, ___pdr_pvalue(key));
    case ___PDR_LUA_TLCF:
      return ___pdr_hashpointer(t, ___pdr_fvalue(key));
    default:
      ___pdr_lua_assert(!___pdr_ttisdeadkey(key));
      return ___pdr_hashpointer(t, ___pdr_gcvalue(key));
  }
}


/*
** returns the index for 'key' if 'key' is an appropriate key to live in
** the array part of the table, 0 otherwise.
*/
static unsigned int arrayindex (const ___pdr_TValue *key) {
  if (___pdr_ttisinteger(key)) {
    ___pdr_lua_Integer k = ___pdr_ivalue(key);
    if (0 < k && (___pdr_lua_Unsigned)k <= ___PDR_MAXASIZE)
      return ___pdr_cast(unsigned int, k);  /* 'key' is an appropriate array index */
  }
  return 0;  /* 'key' did not match some condition */
}


/*
** returns the index of a 'key' for table traversals. First goes all
** elements in the array part, then elements in the hash part. The
** beginning of a traversal is signaled by 0.
*/
static unsigned int findindex (___pdr_lua_State *L, ___pdr_Table *t, ___pdr_StkId key) {
  unsigned int i;
  if (___pdr_ttisnil(key)) return 0;  /* first iteration */
  i = arrayindex(key);
  if (i != 0 && i <= t->sizearray)  /* is 'key' inside array part? */
    return i;  /* yes; that's the index */
  else {
    int nx;
    ___pdr_Node *n = mainposition(t, key);
    for (;;) {  /* check whether 'key' is somewhere in the chain */
      /* key may be dead already, but it is ok to use it in 'next' */
      if (___pdr_luaV_rawequalobj(___pdr_gkey(n), key) ||
            (___pdr_ttisdeadkey(___pdr_gkey(n)) && ___pdr_iscollectable(key) &&
             ___pdr_deadvalue(___pdr_gkey(n)) == ___pdr_gcvalue(key))) {
        i = ___pdr_cast_int(n - ___pdr_gnode(t, 0));  /* key index in hash table */
        /* hash elements are numbered after array ones */
        return (i + 1) + t->sizearray;
      }
      nx = ___pdr_gnext(n);
      if (nx == 0)
        ___pdr_luaG_runerror(L, "invalid key to 'next'");  /* key not found */
      else n += nx;
    }
  }
}


int ___pdr_luaH_next (___pdr_lua_State *L, ___pdr_Table *t, ___pdr_StkId key) {
  unsigned int i = findindex(L, t, key);  /* find original element */
  for (; i < t->sizearray; i++) {  /* try first array part */
    if (!___pdr_ttisnil(&t->array[i])) {  /* a non-nil value? */
      ___pdr_setivalue(key, i + 1);
      ___pdr_setobj2s(L, key+1, &t->array[i]);
      return 1;
    }
  }
  for (i -= t->sizearray; ___pdr_cast_int(i) < ___pdr_sizenode(t); i++) {  /* hash part */
    if (!___pdr_ttisnil(___pdr_gval(___pdr_gnode(t, i)))) {  /* a non-nil value? */
      ___pdr_setobj2s(L, key, ___pdr_gkey(___pdr_gnode(t, i)));
      ___pdr_setobj2s(L, key+1, ___pdr_gval(___pdr_gnode(t, i)));
      return 1;
    }
  }
  return 0;  /* no more elements */
}


/*
** {=============================================================
** Rehash
** ==============================================================
*/

/*
** Compute the optimal size for the array part of table 't'. 'nums' is a
** "count array" where 'nums[i]' is the number of integers in the table
** between 2^(i - 1) + 1 and 2^i. 'pna' enters with the total number of
** integer keys in the table and leaves with the number of keys that
** will go to the array part; return the optimal size.
*/
static unsigned int computesizes (unsigned int nums[], unsigned int *pna) {
  int i;
  unsigned int twotoi;  /* 2^i (candidate for optimal size) */
  unsigned int a = 0;  /* number of elements smaller than 2^i */
  unsigned int na = 0;  /* number of elements to go to array part */
  unsigned int optimal = 0;  /* optimal size for array part */
  /* loop while keys can fill more than half of total size */
  for (i = 0, twotoi = 1; *pna > twotoi / 2; i++, twotoi *= 2) {
    if (nums[i] > 0) {
      a += nums[i];
      if (a > twotoi/2) {  /* more than half elements present? */
        optimal = twotoi;  /* optimal size (till now) */
        na = a;  /* all elements up to 'optimal' will go to array part */
      }
    }
  }
  ___pdr_lua_assert((optimal == 0 || optimal / 2 < na) && na <= optimal);
  *pna = na;
  return optimal;
}


static int countint (const ___pdr_TValue *key, unsigned int *nums) {
  unsigned int k = arrayindex(key);
  if (k != 0) {  /* is 'key' an appropriate array index? */
    nums[___pdr_luaO_ceillog2(k)]++;  /* count as such */
    return 1;
  }
  else
    return 0;
}


/*
** Count keys in array part of table 't': Fill 'nums[i]' with
** number of keys that will go into corresponding slice and return
** total number of non-nil keys.
*/
static unsigned int numusearray (const ___pdr_Table *t, unsigned int *nums) {
  int lg;
  unsigned int ttlg;  /* 2^lg */
  unsigned int ause = 0;  /* summation of 'nums' */
  unsigned int i = 1;  /* count to traverse all array keys */
  /* traverse each slice */
  for (lg = 0, ttlg = 1; lg <= ___PDR_MAXABITS; lg++, ttlg *= 2) {
    unsigned int lc = 0;  /* counter */
    unsigned int lim = ttlg;
    if (lim > t->sizearray) {
      lim = t->sizearray;  /* adjust upper limit */
      if (i > lim)
        break;  /* no more elements to count */
    }
    /* count elements in range (2^(lg - 1), 2^lg] */
    for (; i <= lim; i++) {
      if (!___pdr_ttisnil(&t->array[i-1]))
        lc++;
    }
    nums[lg] += lc;
    ause += lc;
  }
  return ause;
}


static int numusehash (const ___pdr_Table *t, unsigned int *nums, unsigned int *pna) {
  int totaluse = 0;  /* total number of elements */
  int ause = 0;  /* elements added to 'nums' (can go to array part) */
  int i = ___pdr_sizenode(t);
  while (i--) {
    ___pdr_Node *n = &t->node[i];
    if (!___pdr_ttisnil(___pdr_gval(n))) {
      ause += countint(___pdr_gkey(n), nums);
      totaluse++;
    }
  }
  *pna += ause;
  return totaluse;
}


static void setarrayvector (___pdr_lua_State *L, ___pdr_Table *t, unsigned int size) {
  unsigned int i;
  ___pdr_luaM_reallocvector(L, t->array, t->sizearray, size, ___pdr_TValue);
  for (i=t->sizearray; i<size; i++)
     ___pdr_setnilvalue(&t->array[i]);
  t->sizearray = size;
}


static void setnodevector (___pdr_lua_State *L, ___pdr_Table *t, unsigned int size) {
  if (size == 0) {  /* no elements to hash part? */
    t->node = ___pdr_cast(___pdr_Node *, ___pdr_dummynode);  /* use common 'dummynode' */
    t->lsizenode = 0;
    t->lastfree = NULL;  /* signal that it is using dummy node */
  }
  else {
    int i;
    int lsize = ___pdr_luaO_ceillog2(size);
    if (lsize > ___PDR_MAXHBITS)
      ___pdr_luaG_runerror(L, "table overflow");
    size = ___pdr_twoto(lsize);
    t->node = ___pdr_luaM_newvector(L, size, ___pdr_Node);
    for (i = 0; i < (int)size; i++) {
      ___pdr_Node *n = ___pdr_gnode(t, i);
      ___pdr_gnext(n) = 0;
      ___pdr_setnilvalue(___pdr_wgkey(n));
      ___pdr_setnilvalue(___pdr_gval(n));
    }
    t->lsizenode = ___pdr_cast_byte(lsize);
    t->lastfree = ___pdr_gnode(t, size);  /* all positions are free */
  }
}


void ___pdr_luaH_resize (___pdr_lua_State *L, ___pdr_Table *t, unsigned int nasize,
                                          unsigned int nhsize) {
  unsigned int i;
  int j;
  unsigned int oldasize = t->sizearray;
  int oldhsize = ___pdr_allocsizenode(t);
  ___pdr_Node *nold = t->node;  /* save old hash ... */
  if (nasize > oldasize)  /* array part must grow? */
    setarrayvector(L, t, nasize);
  /* create new hash part with appropriate size */
  setnodevector(L, t, nhsize);
  if (nasize < oldasize) {  /* array part must shrink? */
    t->sizearray = nasize;
    /* re-insert elements from vanishing slice */
    for (i=nasize; i<oldasize; i++) {
      if (!___pdr_ttisnil(&t->array[i]))
        ___pdr_luaH_setint(L, t, i + 1, &t->array[i]);
    }
    /* shrink array */
    ___pdr_luaM_reallocvector(L, t->array, oldasize, nasize, ___pdr_TValue);
  }
  /* re-insert elements from hash part */
  for (j = oldhsize - 1; j >= 0; j--) {
    ___pdr_Node *old = nold + j;
    if (!___pdr_ttisnil(___pdr_gval(old))) {
      /* doesn't need barrier/invalidate cache, as entry was
         already present in the table */
      ___pdr_setobjt2t(L, ___pdr_luaH_set(L, t, ___pdr_gkey(old)), ___pdr_gval(old));
    }
  }
  if (oldhsize > 0)  /* not the dummy node? */
    ___pdr_luaM_freearray(L, nold, ___pdr_cast(size_t, oldhsize)); /* free old hash */
}


void ___pdr_luaH_resizearray (___pdr_lua_State *L, ___pdr_Table *t, unsigned int nasize) {
  int nsize = ___pdr_allocsizenode(t);
  ___pdr_luaH_resize(L, t, nasize, nsize);
}

/*
** nums[i] = number of keys 'k' where 2^(i - 1) < k <= 2^i
*/
static void rehash (___pdr_lua_State *L, ___pdr_Table *t, const ___pdr_TValue *ek) {
  unsigned int asize;  /* optimal size for array part */
  unsigned int na;  /* number of keys in the array part */
  unsigned int nums[___PDR_MAXABITS + 1];
  int i;
  int totaluse;
  for (i = 0; i <= ___PDR_MAXABITS; i++) nums[i] = 0;  /* reset counts */
  na = numusearray(t, nums);  /* count keys in array part */
  totaluse = na;  /* all those keys are integer keys */
  totaluse += numusehash(t, nums, &na);  /* count keys in hash part */
  /* count extra key */
  na += countint(ek, nums);
  totaluse++;
  /* compute new size for array part */
  asize = computesizes(nums, &na);
  /* resize the table to new computed sizes */
  ___pdr_luaH_resize(L, t, asize, totaluse - na);
}



/*
** }=============================================================
*/


___pdr_Table *___pdr_luaH_new (___pdr_lua_State *L) {
  ___pdr_GCObject *o = ___pdr_luaC_newobj(L, ___PDR_LUA_TTABLE, sizeof(___pdr_Table));
  ___pdr_Table *t = ___pdr_gco2t(o);
  t->metatable = NULL;
  t->flags = ___pdr_cast_byte(~0);
  t->array = NULL;
  t->sizearray = 0;
  setnodevector(L, t, 0);
  return t;
}


void ___pdr_luaH_free (___pdr_lua_State *L, ___pdr_Table *t) {
  if (!___pdr_isdummy(t))
    ___pdr_luaM_freearray(L, t->node, ___pdr_cast(size_t, ___pdr_sizenode(t)));
  ___pdr_luaM_freearray(L, t->array, t->sizearray);
  ___pdr_luaM_free(L, t);
}


static ___pdr_Node *getfreepos (___pdr_Table *t) {
  if (!___pdr_isdummy(t)) {
    while (t->lastfree > t->node) {
      t->lastfree--;
      if (___pdr_ttisnil(___pdr_gkey(t->lastfree)))
        return t->lastfree;
    }
  }
  return NULL;  /* could not find a free place */
}



/*
** inserts a new key into a hash table; first, check whether key's main
** position is free. If not, check whether colliding node is in its main
** position or not: if it is not, move colliding node to an empty place and
** put new key in its main position; otherwise (colliding node is in its main
** position), new key goes to an empty position.
*/
___pdr_TValue *___pdr_luaH_newkey (___pdr_lua_State *L, ___pdr_Table *t, const ___pdr_TValue *key) {
  ___pdr_Node *mp;
  ___pdr_TValue aux;
  if (___pdr_ttisnil(key)) ___pdr_luaG_runerror(L, "table index is nil");
  else if (___pdr_ttisfloat(key)) {
    ___pdr_lua_Integer k;
    if (___pdr_luaV_tointeger(key, &k, 0)) {  /* does index fit in an integer? */
      ___pdr_setivalue(&aux, k);
      key = &aux;  /* insert it as an integer */
    }
    else if (___pdr_luai_numisnan(___pdr_fltvalue(key)))
      ___pdr_luaG_runerror(L, "table index is NaN");
  }
  mp = mainposition(t, key);
  if (!___pdr_ttisnil(___pdr_gval(mp)) || ___pdr_isdummy(t)) {  /* main position is taken? */
    ___pdr_Node *othern;
    ___pdr_Node *f = getfreepos(t);  /* get a free place */
    if (f == NULL) {  /* cannot find a free place? */
      rehash(L, t, key);  /* grow table */
      /* whatever called 'newkey' takes care of TM cache */
      return ___pdr_luaH_set(L, t, key);  /* insert key into grown table */
    }
    ___pdr_lua_assert(!___pdr_isdummy(t));
    othern = mainposition(t, ___pdr_gkey(mp));
    if (othern != mp) {  /* is colliding node out of its main position? */
      /* yes; move colliding node into free position */
      while (othern + ___pdr_gnext(othern) != mp)  /* find previous */
        othern += ___pdr_gnext(othern);
      ___pdr_gnext(othern) = ___pdr_cast_int(f - othern);  /* rechain to point to 'f' */
      *f = *mp;  /* copy colliding node into free pos. (mp->next also goes) */
      if (___pdr_gnext(mp) != 0) {
        ___pdr_gnext(f) += ___pdr_cast_int(mp - f);  /* correct 'next' */
        ___pdr_gnext(mp) = 0;  /* now 'mp' is free */
      }
      ___pdr_setnilvalue(___pdr_gval(mp));
    }
    else {  /* colliding node is in its own main position */
      /* new node will go into free position */
      if (___pdr_gnext(mp) != 0)
        ___pdr_gnext(f) = ___pdr_cast_int((mp + ___pdr_gnext(mp)) - f);  /* chain new position */
      else ___pdr_lua_assert(___pdr_gnext(f) == 0);
      ___pdr_gnext(mp) = ___pdr_cast_int(f - mp);
      mp = f;
    }
  }
  ___pdr_setnodekey(L, &mp->i_key, key);
  ___pdr_luaC_barrierback(L, t, key);
  ___pdr_lua_assert(___pdr_ttisnil(___pdr_gval(mp)));
  return ___pdr_gval(mp);
}


/*
** search function for integers
*/
const ___pdr_TValue *___pdr_luaH_getint (___pdr_Table *t, ___pdr_lua_Integer key) {
  /* (1 <= key && key <= t->sizearray) */
  if (___pdr_l_castS2U(key) - 1 < t->sizearray)
    return &t->array[key - 1];
  else {
    ___pdr_Node *n = ___pdr_hashint(t, key);
    for (;;) {  /* check whether 'key' is somewhere in the chain */
      if (___pdr_ttisinteger(___pdr_gkey(n)) && ___pdr_ivalue(___pdr_gkey(n)) == key)
        return ___pdr_gval(n);  /* that's it */
      else {
        int nx = ___pdr_gnext(n);
        if (nx == 0) break;
        n += nx;
      }
    }
    return ___pdr_luaO_nilobject;
  }
}


/*
** search function for short strings
*/
const ___pdr_TValue *___pdr_luaH_getshortstr (___pdr_Table *t, ___pdr_TString *key) {
  ___pdr_Node *n = ___pdr_hashstr(t, key);
  ___pdr_lua_assert(key->tt == ___PDR_LUA_TSHRSTR);
  for (;;) {  /* check whether 'key' is somewhere in the chain */
    const ___pdr_TValue *k = ___pdr_gkey(n);
    if (___pdr_ttisshrstring(k) && ___pdr_eqshrstr(___pdr_tsvalue(k), key))
      return ___pdr_gval(n);  /* that's it */
    else {
      int nx = ___pdr_gnext(n);
      if (nx == 0)
        return ___pdr_luaO_nilobject;  /* not found */
      n += nx;
    }
  }
}


/*
** "Generic" get version. (Not that generic: not valid for integers,
** which may be in array part, nor for floats with integral values.)
*/
static const ___pdr_TValue *getgeneric (___pdr_Table *t, const ___pdr_TValue *key) {
  ___pdr_Node *n = mainposition(t, key);
  for (;;) {  /* check whether 'key' is somewhere in the chain */
    if (___pdr_luaV_rawequalobj(___pdr_gkey(n), key))
      return ___pdr_gval(n);  /* that's it */
    else {
      int nx = ___pdr_gnext(n);
      if (nx == 0)
        return ___pdr_luaO_nilobject;  /* not found */
      n += nx;
    }
  }
}


const ___pdr_TValue *___pdr_luaH_getstr (___pdr_Table *t, ___pdr_TString *key) {
  if (key->tt == ___PDR_LUA_TSHRSTR)
    return ___pdr_luaH_getshortstr(t, key);
  else {  /* for long strings, use generic case */
    ___pdr_TValue ko;
    ___pdr_setsvalue(___pdr_cast(___pdr_lua_State *, NULL), &ko, key);
    return getgeneric(t, &ko);
  }
}


/*
** main search function
*/
const ___pdr_TValue *___pdr_luaH_get (___pdr_Table *t, const ___pdr_TValue *key) {
  switch (___pdr_ttype(key)) {
    case ___PDR_LUA_TSHRSTR: return ___pdr_luaH_getshortstr(t, ___pdr_tsvalue(key));
    case ___PDR_LUA_TNUMINT: return ___pdr_luaH_getint(t, ___pdr_ivalue(key));
    case ___PDR_LUA_TNIL: return ___pdr_luaO_nilobject;
    case ___PDR_LUA_TNUMFLT: {
      ___pdr_lua_Integer k;
      if (___pdr_luaV_tointeger(key, &k, 0)) /* index is int? */
        return ___pdr_luaH_getint(t, k);  /* use specialized version */
      /* else... */
    }  /* FALLTHROUGH */
    default:
      return getgeneric(t, key);
  }
}


/*
** beware: when using this function you probably need to check a GC
** barrier and invalidate the TM cache.
*/
___pdr_TValue *___pdr_luaH_set (___pdr_lua_State *L, ___pdr_Table *t, const ___pdr_TValue *key) {
  const ___pdr_TValue *p = ___pdr_luaH_get(t, key);
  if (p != ___pdr_luaO_nilobject)
    return ___pdr_cast(___pdr_TValue *, p);
  else return ___pdr_luaH_newkey(L, t, key);
}


void ___pdr_luaH_setint (___pdr_lua_State *L, ___pdr_Table *t, ___pdr_lua_Integer key, ___pdr_TValue *value) {
  const ___pdr_TValue *p = ___pdr_luaH_getint(t, key);
  ___pdr_TValue *cell;
  if (p != ___pdr_luaO_nilobject)
    cell = ___pdr_cast(___pdr_TValue *, p);
  else {
    ___pdr_TValue k;
    ___pdr_setivalue(&k, key);
    cell = ___pdr_luaH_newkey(L, t, &k);
  }
  ___pdr_setobj2t(L, cell, value);
}


static int unbound_search (___pdr_Table *t, unsigned int j) {
  unsigned int i = j;  /* i is zero or a present index */
  j++;
  /* find 'i' and 'j' such that i is present and j is not */
  while (!___pdr_ttisnil(___pdr_luaH_getint(t, j))) {
    i = j;
    if (j > ___pdr_cast(unsigned int, ___PDR_MAX_INT)/2) {  /* overflow? */
      /* table was built with bad purposes: resort to linear search */
      i = 1;
      while (!___pdr_ttisnil(___pdr_luaH_getint(t, i))) i++;
      return i - 1;
    }
    j *= 2;
  }
  /* now do a binary search between them */
  while (j - i > 1) {
    unsigned int m = (i+j)/2;
    if (___pdr_ttisnil(___pdr_luaH_getint(t, m))) j = m;
    else i = m;
  }
  return i;
}


/*
** Try to find a boundary in table 't'. A 'boundary' is an integer index
** such that t[i] is non-nil and t[i+1] is nil (and 0 if t[1] is nil).
*/
int ___pdr_luaH_getn (___pdr_Table *t) {
  unsigned int j = t->sizearray;
  if (j > 0 && ___pdr_ttisnil(&t->array[j - 1])) {
    /* there is a boundary in the array part: (binary) search for it */
    unsigned int i = 0;
    while (j - i > 1) {
      unsigned int m = (i+j)/2;
      if (___pdr_ttisnil(&t->array[m - 1])) j = m;
      else i = m;
    }
    return i;
  }
  /* else must find a boundary in hash part */
  else if (___pdr_isdummy(t))  /* hash part is empty? */
    return j;  /* that is easy... */
  else return unbound_search(t, j);
}



#if defined(___PDR_LUA_DEBUG)

___pdr_Node *___pdr_luaH_mainposition (const ___pdr_Table *t, const ___pdr_TValue *key) {
  return mainposition(t, key);
}

int ___pdr_luaH_isdummy (const ___pdr_Table *t) { return ___pdr_isdummy(t); }

#endif

} // end NS_PDR_SLUA