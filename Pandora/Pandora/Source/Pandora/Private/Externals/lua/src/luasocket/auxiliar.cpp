/*=========================================================================*\
* Auxiliar routines for class hierarchy manipulation
* LuaSocket toolkit
\*=========================================================================*/
#include "auxiliar.h"
#include <string.h>
#include <stdio.h>


namespace NS_PDR_SLUA {

/*=========================================================================*\
* Exported functions
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Initializes the module
\*-------------------------------------------------------------------------*/
int ___pdr_auxiliar_open(___pdr_lua_State *L) {
    (void) L;
    return 0;
}

/*-------------------------------------------------------------------------*\
* Creates a new class with given methods
* Methods whose names start with __ are passed directly to the metatable.
\*-------------------------------------------------------------------------*/
void ___pdr_auxiliar_newclass(___pdr_lua_State *L, const char *classname, ___pdr_luaL_Reg *func) {
    ___pdr_luaL_newmetatable(L, classname); /* mt */
    /* create __index table to place methods */
    ___pdr_lua_pushstring(L, "__index");    /* mt,"__index" */
    ___pdr_lua_newtable(L);                 /* mt,"__index",it */ 
    /* put class name into class metatable */
    ___pdr_lua_pushstring(L, "class");      /* mt,"__index",it,"class" */
    ___pdr_lua_pushstring(L, classname);    /* mt,"__index",it,"class",classname */
    ___pdr_lua_rawset(L, -3);               /* mt,"__index",it */
    /* pass all methods that start with _ to the metatable, and all others
     * to the index table */
    for (; func->name; func++) {     /* mt,"__index",it */
        ___pdr_lua_pushstring(L, func->name);
        ___pdr_lua_pushcfunction(L, func->func);
        ___pdr_lua_rawset(L, func->name[0] == '_' ? -5: -3);
    }
    ___pdr_lua_rawset(L, -3);               /* mt */
    ___pdr_lua_pop(L, 1);
}

/*-------------------------------------------------------------------------*\
* Prints the value of a class in a nice way
\*-------------------------------------------------------------------------*/
int ___pdr_auxiliar_tostring(___pdr_lua_State *L) {
    char buf[32];
    if (!___pdr_lua_getmetatable(L, 1)) goto error;
    ___pdr_lua_pushstring(L, "__index");
    ___pdr_lua_gettable(L, -2);
    if (!___pdr_lua_istable(L, -1)) goto error;
    ___pdr_lua_pushstring(L, "class");
    ___pdr_lua_gettable(L, -2);
    if (!___pdr_lua_isstring(L, -1)) goto error;
    sprintf(buf, "%p", ___pdr_lua_touserdata(L, 1));
    ___pdr_lua_pushfstring(L, "%s: %s", ___pdr_lua_tostring(L, -1), buf);
    return 1;
error:
    ___pdr_lua_pushstring(L, "invalid object passed to 'auxiliar.c:__tostring'");
    ___pdr_lua_error(L);
    return 1;
}

/*-------------------------------------------------------------------------*\
* Insert class into group
\*-------------------------------------------------------------------------*/
void ___pdr_auxiliar_add2group(___pdr_lua_State *L, const char *classname, const char *groupname) {
    ___pdr_luaL_getmetatable(L, classname);
    ___pdr_lua_pushstring(L, groupname);
    ___pdr_lua_pushboolean(L, 1);
    ___pdr_lua_rawset(L, -3);
    ___pdr_lua_pop(L, 1);
}

/*-------------------------------------------------------------------------*\
* Make sure argument is a boolean
\*-------------------------------------------------------------------------*/
int ___pdr_auxiliar_checkboolean(___pdr_lua_State *L, int objidx) {
    if (!___pdr_lua_isboolean(L, objidx))
        ___pdr_auxiliar_typeerror(L, objidx, ___pdr_lua_typename(L, ___PDR_LUA_TBOOLEAN));
    return ___pdr_lua_toboolean(L, objidx);
}

/*-------------------------------------------------------------------------*\
* Return userdata pointer if object belongs to a given class, abort with 
* error otherwise
\*-------------------------------------------------------------------------*/
void *___pdr_auxiliar_checkclass(___pdr_lua_State *L, const char *classname, int objidx) {
    void *data = ___pdr_auxiliar_getclassudata(L, classname, objidx);
    if (!data) {
        char msg[45];
        sprintf(msg, "%.35s expected", classname);
        ___pdr_luaL_argerror(L, objidx, msg);
    }
    return data;
}

/*-------------------------------------------------------------------------*\
* Return userdata pointer if object belongs to a given group, abort with 
* error otherwise
\*-------------------------------------------------------------------------*/
void *___pdr_auxiliar_checkgroup(___pdr_lua_State *L, const char *groupname, int objidx) {
    void *data = ___pdr_auxiliar_getgroupudata(L, groupname, objidx);
    if (!data) {
        char msg[45];
        sprintf(msg, "%.35s expected", groupname);
        ___pdr_luaL_argerror(L, objidx, msg);
    }
    return data;
}

/*-------------------------------------------------------------------------*\
* Set object class
\*-------------------------------------------------------------------------*/
void ___pdr_auxiliar_setclass(___pdr_lua_State *L, const char *classname, int objidx) {
    ___pdr_luaL_getmetatable(L, classname);
    if (objidx < 0) objidx--;
    ___pdr_lua_setmetatable(L, objidx);
}

/*-------------------------------------------------------------------------*\
* Get a userdata pointer if object belongs to a given group. Return NULL 
* otherwise
\*-------------------------------------------------------------------------*/
void *___pdr_auxiliar_getgroupudata(___pdr_lua_State *L, const char *groupname, int objidx) {
    if (!___pdr_lua_getmetatable(L, objidx))
        return NULL;
    ___pdr_lua_pushstring(L, groupname);
    ___pdr_lua_rawget(L, -2);
    if (___pdr_lua_isnil(L, -1)) {
        ___pdr_lua_pop(L, 2);
        return NULL;
    } else {
        ___pdr_lua_pop(L, 2);
        return ___pdr_lua_touserdata(L, objidx);
    }
}

/*-------------------------------------------------------------------------*\
* Get a userdata pointer if object belongs to a given class. Return NULL 
* otherwise
\*-------------------------------------------------------------------------*/
void *___pdr_auxiliar_getclassudata(___pdr_lua_State *L, const char *classname, int objidx) {
    return ___pdr_luaL_checkudata(L, objidx, classname);
}

/*-------------------------------------------------------------------------*\
* Throws error when argument does not have correct type.
* Used to be part of lauxlib in Lua 5.1, was dropped from 5.2.
\*-------------------------------------------------------------------------*/
int ___pdr_auxiliar_typeerror (___pdr_lua_State *L, int narg, const char *tname) {
  const char *msg = ___pdr_lua_pushfstring(L, "%s expected, got %s", tname, 
      ___pdr_luaL_typename(L, narg));
  return ___pdr_luaL_argerror(L, narg, msg);
}


} // end NS_PDR_SLUA
