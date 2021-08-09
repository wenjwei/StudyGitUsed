#pragma once

#include "Components/WaterfallScrollView.h"
#include "PDRLuaObject.h"
#include "PDRLuaCppBinding.h"

namespace pandora
{
namespace extension
{
static void LuaMethodExtension() 
{
    using NS_PDR_SLUA::LuaObject;
    using NS_PDR_SLUA::___pdr_lua_State;
    REG_EXTENSION_METHOD_IMP(UWaterfallScrollView, "SetItemFillDelegate", {
        CheckUD(UWaterfallScrollView, L, 1);
        if (!UD) ___pdr_luaL_argcheck(L, false, 1, "'UWaterfallScrollView' expected");
        if (!___pdr_lua_isfunction(L, 2)) ___pdr_luaL_argcheck(L, false, 2, "'function' expected");
        NS_PDR_SLUA::LuaVar func = NS_PDR_SLUA::LuaVar(L, 2, NS_PDR_SLUA::LuaVar::Type::LV_FUNCTION);
        UD->onFillItem = func;
        return 0;
        });

    REG_EXTENSION_METHOD_IMP(UWaterfallScrollView, "SetItemRecycleDelegate", {
        CheckUD(UWaterfallScrollView, L, 1);
        if (!UD) ___pdr_luaL_argcheck(L, false, 1, "'UWaterfallScrollView' expected");
        if (!___pdr_lua_isfunction(L, 2)) ___pdr_luaL_argcheck(L, false, 2, "'function' expected");
        NS_PDR_SLUA::LuaVar func = NS_PDR_SLUA::LuaVar(L, 2, NS_PDR_SLUA::LuaVar::Type::LV_FUNCTION);
        UD->onRecycleItem = func;
        return 0;
        });
};
}
}
