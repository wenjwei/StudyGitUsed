#ifndef ___PDR_SELECT_H___
#define ___PDR_SELECT_H___
/*=========================================================================*\
* Select implementation
* LuaSocket toolkit
*
* Each object that can be passed to the select function has to export 
* method getfd() which returns the descriptor to be passed to the
* underlying select function. Another method, dirty(), should return 
* true if there is data ready for reading (required for buffered input).
\*=========================================================================*/

namespace NS_PDR_SLUA {    

int ___pdr_select_open(___pdr_lua_State *L);

} // end NS_PDR_SLUA

#endif /* SELECT_H */
