#ifndef ___PDR_AUXILIAR_H___
#define ___PDR_AUXILIAR_H___
/*=========================================================================*\
* Auxiliar routines for class hierarchy manipulation
* LuaSocket toolkit (but completely independent of other LuaSocket modules)
*
* A LuaSocket class is a name associated with Lua metatables. A LuaSocket 
* group is a name associated with a class. A class can belong to any number 
* of groups. This module provides the functionality to:
*
*   - create new classes 
*   - add classes to groups 
*   - set the class of objects
*   - check if an object belongs to a given class or group
*   - get the userdata associated to objects
*   - print objects in a pretty way
*
* LuaSocket class names follow the convention <module>{<class>}. Modules
* can define any number of classes and groups. The module tcp.c, for
* example, defines the classes tcp{master}, tcp{client} and tcp{server} and
* the groups tcp{client,server} and tcp{any}. Module functions can then
* perform type-checking on their arguments by either class or group.
*
* LuaSocket metatables define the __index metamethod as being a table. This
* table has one field for each method supported by the class, and a field
* "class" with the class name.
*
* The mapping from class name to the corresponding metatable and the
* reverse mapping are done using lauxlib. 
\*=========================================================================*/

#include "lua.h"
#include "lauxlib.h"

namespace NS_PDR_SLUA {

int ___pdr_auxiliar_open(___pdr_lua_State *L);
void ___pdr_auxiliar_newclass(___pdr_lua_State *L, const char *classname, ___pdr_luaL_Reg *func);
void ___pdr_auxiliar_add2group(___pdr_lua_State *L, const char *classname, const char *group);
void ___pdr_auxiliar_setclass(___pdr_lua_State *L, const char *classname, int objidx);
void *___pdr_auxiliar_checkclass(___pdr_lua_State *L, const char *classname, int objidx);
void *___pdr_auxiliar_checkgroup(___pdr_lua_State *L, const char *groupname, int objidx);
void *___pdr_auxiliar_getclassudata(___pdr_lua_State *L, const char *groupname, int objidx);
void *___pdr_auxiliar_getgroupudata(___pdr_lua_State *L, const char *groupname, int objidx);
int ___pdr_auxiliar_checkboolean(___pdr_lua_State *L, int objidx);
int ___pdr_auxiliar_tostring(___pdr_lua_State *L);
int ___pdr_auxiliar_typeerror(___pdr_lua_State *L, int narg, const char *tname);

} // end NS_PDR_SLUA

#endif /* AUXILIAR_H */
