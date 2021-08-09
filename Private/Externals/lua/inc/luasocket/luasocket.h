#ifndef ___PDR_LUASOCKET_H___
#define ___PDR_LUASOCKET_H___
/*=========================================================================*\
* LuaSocket toolkit
* Networking support for the Lua language
* Diego Nehab
* 9/11/1999
\*=========================================================================*/
#include "lua.h"

/*-------------------------------------------------------------------------*\
* Current socket library version
\*-------------------------------------------------------------------------*/
#define ___PDR_LUASOCKET_VERSION    "LuaSocket 3.0-rc1"
#define ___PDR_LUASOCKET_COPYRIGHT  "Copyright (C) 1999-2013 Diego Nehab"

/*-------------------------------------------------------------------------*\
* This macro prefixes all exported API functions
\*-------------------------------------------------------------------------*/
#ifndef ___PDR_LUASOCKET_API
#define ___PDR_LUASOCKET_API 
#endif

namespace NS_PDR_SLUA {    

/*-------------------------------------------------------------------------*\
* Initializes the library.
\*-------------------------------------------------------------------------*/
___PDR_LUASOCKET_API int ___pdr_luaopen_socket_core(___pdr_lua_State *L);

} // end NS_PDR_SLUA

#endif /* LUASOCKET_H */
