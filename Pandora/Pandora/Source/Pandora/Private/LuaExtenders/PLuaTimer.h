#pragma once

#include "PDRLuaObject.h"

namespace pandora 
{
//using namespace NS_PDR_SLUA;

class PLuaTimer
{
public:
    static void RegisterToLua(NS_PDR_SLUA::___pdr_lua_State* L);
    static void Clear();
    static bool GetLuaFunction(int tid, NS_PDR_SLUA::LuaVar & luaFunction);

private:
    static TMap<int, FTimerHandle> _timerMap;
    static TMap<int, NS_PDR_SLUA::LuaVar> _luaFunctionMap;
    static int _currentTimerID;

    static int SetTimer(NS_PDR_SLUA::___pdr_lua_State* L);
    static int GetTimerId();
    static int ClearTimer(NS_PDR_SLUA::___pdr_lua_State* L);
};

}
