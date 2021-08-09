#include "PLuaTimer.h"
#include "PLuaStateMgr.h"

namespace pandora 
{
//using namespace NS_PDR_SLUA;

TMap<int, FTimerHandle> PLuaTimer::_timerMap;
TMap<int, NS_PDR_SLUA::LuaVar> PLuaTimer::_luaFunctionMap;
int PLuaTimer::_currentTimerID = 0;

int PLuaTimer::GetTimerId() 
{
    _currentTimerID += 1;
    int timerID = _currentTimerID;

    // -1 is a special value, when a timer is invalid, it's ID should be -1
    if (timerID == -1)
    {
        _currentTimerID += 1;
        timerID = _currentTimerID;
    }
    return timerID;
}

int PLuaTimer::SetTimer(NS_PDR_SLUA::___pdr_lua_State* L)
{
    if (___pdr_lua_gettop(L) != 3) 
    {
        ___pdr_lua_pushinteger(L, -1);
        return 1;
    }

    auto instance = PLuaStateMgr::GetGameInstance();
    if (!IsValid(instance)) 
    {
        ___pdr_lua_pushinteger(L, -1);
        return 1;
    }

    float rate = ___pdr_luaL_checknumber(L, 1);
    bool isLoop = ___pdr_lua_toboolean(L, 2) != 0;
    NS_PDR_SLUA::LuaVar callback(L, 3, NS_PDR_SLUA::LuaVar::LV_FUNCTION);
    FTimerHandle timerHandle;
    FTimerDelegate timerDelegate;
    auto timerID = GetTimerId();
    timerDelegate.BindLambda([timerID]() 
    {
        NS_PDR_SLUA::LuaVar lfunc;
        if (!PLuaTimer::GetLuaFunction(timerID, lfunc))
            return;
        lfunc.call();
    });
    instance->GetTimerManager().SetTimer(timerHandle, timerDelegate, rate, isLoop);
    if (!timerHandle.IsValid()) 
    {
        ___pdr_lua_pushinteger(L, -1);
        return 1;
    }

    _timerMap.Add(timerID, timerHandle);
    _luaFunctionMap.Add(timerID, callback);
        
    ___pdr_lua_pushinteger(L, timerID);
    return 1;
}

int PLuaTimer::ClearTimer(NS_PDR_SLUA::___pdr_lua_State* L)
{
    if (___pdr_lua_gettop(L) != 1) 
    {
        ___pdr_lua_pushboolean(L, false);
        return 1;
    }

    int timerID = ___pdr_luaL_checkinteger(L, 1);
    if (timerID < 0) 
    {
        ___pdr_lua_pushboolean(L, false);
        return 1;
    }

    if (_luaFunctionMap.Contains(timerID))
        _luaFunctionMap.Remove(timerID);

    auto pTimerHandle = _timerMap.Find(timerID);
    if (!pTimerHandle) 
    {
        ___pdr_lua_pushboolean(L, false);
        return 1;
    }

    auto& timerHandle = *pTimerHandle;
    if (!timerHandle.IsValid()) 
    {
        ___pdr_lua_pushboolean(L, false);
        return 1;
    }

    auto instance = PLuaStateMgr::GetGameInstance();
    if (!IsValid(instance)) 
    {
        ___pdr_lua_pushboolean(L, false);
        return 1;
    }

    instance->GetTimerManager().ClearTimer(timerHandle);
    _timerMap.Remove(timerID);
    ___pdr_lua_pushboolean(L, true);
    return 1;
}

void PLuaTimer::RegisterToLua(NS_PDR_SLUA::___pdr_lua_State* L)
{
    ___pdr_lua_newtable(L);
    RegMetaMethod(L, SetTimer);
    RegMetaMethod(L, ClearTimer);
    ___pdr_lua_setglobal(L, "PLuaTimer");
}

void PLuaTimer::Clear() 
{
    auto instance = PLuaStateMgr::GetGameInstance();
    if (!IsValid(instance)) 
    {
        return;
    }

    for (auto& p : _timerMap) 
    {
        auto& timerHandle = p.Value;
        if (timerHandle.IsValid()) 
        {
            instance->GetTimerManager().ClearTimer(timerHandle);
        }
    }

    _timerMap.Empty();
    _luaFunctionMap.Empty();
    _currentTimerID = 0;
}

bool PLuaTimer::GetLuaFunction(int timerID, NS_PDR_SLUA::LuaVar & luaFunction)
{
    if (!_luaFunctionMap.Contains(timerID))
        return false;
    luaFunction = _luaFunctionMap[timerID];
    return true;
}

}

