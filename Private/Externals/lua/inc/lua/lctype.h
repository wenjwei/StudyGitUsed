/*
** $Id: lctype.h,v 1.12 2011/07/15 12:50:29 roberto Exp $
** 'ctype' functions for Lua
** See Copyright Notice in lua.h
*/

#ifndef ___pdr_lctype_h___
#define ___pdr_lctype_h___

#include "lua.h"

/*
** WARNING: the functions defined here do not necessarily correspond
** to the similar functions in the standard C ctype.h. They are
** optimized for the specific needs of Lua
*/

#if !defined(___PDR_LUA_USE_CTYPE)

#if 'A' == 65 && '0' == 48
/* ASCII case: can use its own tables; faster and fixed */
#define ___PDR_LUA_USE_CTYPE	0
#else
/* must use standard C ctype */
#define ___PDR_LUA_USE_CTYPE	1
#endif

#endif


#if !___PDR_LUA_USE_CTYPE	/* { */

#include <limits.h>

#include "llimits.h"


#define ___PDR_ALPHABIT     0
#define ___PDR_DIGITBIT     1
#define ___PDR_PRINTBIT     2
#define ___PDR_SPACEBIT     3
#define ___PDR_XDIGITBIT    4


#define ___PDR_MASK(B)		(1 << (B))


/*
** add 1 to char to allow index -1 (EOZ)
*/
#define ___pdr_testprop(c,p)	(___pdr_luai_ctype_[(c)+1] & (p))

/*
** 'lalpha' (Lua alphabetic) and 'lalnum' (Lua alphanumeric) both include '_'
*/
#define ___pdr_lislalpha(c)	___pdr_testprop(c, ___PDR_MASK(___PDR_ALPHABIT))
#define ___pdr_lislalnum(c)	___pdr_testprop(c, (___PDR_MASK(___PDR_ALPHABIT) | ___PDR_MASK(___PDR_DIGITBIT)))
#define ___pdr_lisdigit(c)	___pdr_testprop(c, ___PDR_MASK(___PDR_DIGITBIT))
#define ___pdr_lisspace(c)	___pdr_testprop(c, ___PDR_MASK(___PDR_SPACEBIT))
#define ___pdr_lisprint(c)	___pdr_testprop(c, ___PDR_MASK(___PDR_PRINTBIT))
#define ___pdr_lisxdigit(c)	___pdr_testprop(c, ___PDR_MASK(___PDR_XDIGITBIT))

/*
** this 'ltolower' only works for alphabetic characters
*/
#define ___pdr_ltolower(c)	((c) | ('A' ^ 'a'))

namespace NS_PDR_SLUA {

/* two more entries for 0 and -1 (EOZ) */
___PDR_LUAI_DDEC const ___pdr_lu_byte ___pdr_luai_ctype_[UCHAR_MAX + 2];

} // end NS_PDR_SLUA

#else			/* }{ */

/*
** use standard C ctypes
*/

#include <ctype.h>


#define ___pdr_lislalpha(c)	(isalpha(c) || (c) == '_')
#define ___pdr_lislalnum(c)	(isalnum(c) || (c) == '_')
#define ___pdr_lisdigit(c)	(isdigit(c))
#define ___pdr_lisspace(c)	(isspace(c))
#define ___pdr_lisprint(c)	(isprint(c))
#define ___pdr_lisxdigit(c)	(isxdigit(c))

#define ___pdr_ltolower(c)	(tolower(c))

#endif			/* } */

#endif

