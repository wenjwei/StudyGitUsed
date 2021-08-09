#ifndef ___PDR_MIME_H___
#define ___PDR_MIME_H___
/*=========================================================================*\
* Core MIME support
* LuaSocket toolkit
*
* This module provides functions to implement transfer content encodings
* and formatting conforming to RFC 2045. It is used by mime.lua, which
* provide a higher level interface to this functionality. 
\*=========================================================================*/
#include "lua.h"

/*-------------------------------------------------------------------------*\
* Current MIME library version
\*-------------------------------------------------------------------------*/
#define ___PDR_MIME_VERSION    "MIME 1.0.3"
#define ___PDR_MIME_COPYRIGHT  "Copyright (C) 2004-2013 Diego Nehab"
#define ___PDR_MIME_AUTHORS    "Diego Nehab"

/*-------------------------------------------------------------------------*\
* This macro prefixes all exported API functions
\*-------------------------------------------------------------------------*/
#ifndef ___PDR_MIME_API
#define ___PDR_MIME_API
#endif

namespace NS_PDR_SLUA {    

___PDR_MIME_API int luaopen_mime_core(___pdr_lua_State *L);

} // end NS_PDR_SLUA

#endif /* MIME_H */
