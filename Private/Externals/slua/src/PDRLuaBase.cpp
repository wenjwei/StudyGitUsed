// Tencent is pleased to support the open source community by making sluaunreal available.

// Copyright (C) 2018 THL A29 Limited, a Tencent company. All rights reserved.
// Licensed under the BSD 3-Clause License (the "License"); 
// you may not use this file except in compliance with the License. You may obtain a copy of the License at

// https://opensource.org/licenses/BSD-3-Clause

// Unless required by applicable law or agreed to in writing, 
// software distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.


#include "PDRLuaBase.h"
#include "PDRLuaUserWidget.h"
#include "PDRLuaActor.h"

// engine function
#if ((ENGINE_MINOR_VERSION<19) && (ENGINE_MAJOR_VERSION>=4))
extern uint8 GRegisterNative(int32 NativeBytecodeIndex, const Native& Func);
#else
extern uint8 GRegisterNative(int32 NativeBytecodeIndex, const FNativeFuncPtr& Func);
#endif
#define Ex_LuaHook (EX_Max-1)

UPLuaTableObjectInterface::UPLuaTableObjectInterface(const class FObjectInitializer& OI)
	:Super(OI) {}

namespace NS_PDR_SLUA {

	bool LuaBase::luaImplemented(UFunction * func, void * params)
	{
		if (indexFlag!=IF_NONE && func==currentFunction) return false;

		if (!func->HasAnyFunctionFlags(EFunctionFlags::FUNC_BlueprintEvent))
			return false;

		if (!luaSelfTable.isTable())
			return false;

		NS_PDR_SLUA::LuaVar lfunc = luaSelfTable.getFromTable<NS_PDR_SLUA::LuaVar>((const char*)TCHAR_TO_UTF8(*func->GetName()), true);
		if (!lfunc.isValid()) return false;

		return lfunc.callByUFunction(func, (uint8*)params, &luaSelfTable);
	}

	// Called every frame
	void LuaBase::tick(float DeltaTime)
	{
		deltaTime = DeltaTime;
		if (!tickFunction.isValid()) {
			superTick();
			return;
		}
		tickFunction.call(luaSelfTable, DeltaTime);
	}

	void LuaBase::superTick(___pdr_lua_State* L)
	{
		deltaTime = ___pdr_luaL_checknumber(L, 2);
		superTick();
	}

	int LuaBase::superOrRpcCall(___pdr_lua_State* L,UFunction* func)
	{
		UObject* obj = context.Get();
		if (!obj) return 0;

		FStructOnScope params(func);
		LuaObject::fillParam(L, 2, func, params.GetStructMemory());
		{
			// call function with params
			LuaObject::callUFunction(L, obj, func, params.GetStructMemory());
		}
		// return value to push lua stack
		return LuaObject::returnValue(L, func, params.GetStructMemory());
	}

	int LuaBase::__index(NS_PDR_SLUA::___pdr_lua_State * L)
	{
		___pdr_lua_pushstring(L, SLUA_CPPINST);
		___pdr_lua_rawget(L, 1);
		if (!___pdr_lua_isuserdata(L, -1))
			___pdr_luaL_error(L, "expect LuaBase table at arg 1");
		// push key
		___pdr_lua_pushvalue(L, 2);
		// get field from real actor
		___pdr_lua_gettable(L, -2);
		return 1;
	}

	static int setParent(NS_PDR_SLUA::___pdr_lua_State* L) {
		// set field to obj, may raise an error
		___pdr_lua_settable(L, 1);
		return 0;
	}

	int LuaBase::__newindex(NS_PDR_SLUA::___pdr_lua_State * L)
	{
		___pdr_lua_pushstring(L, SLUA_CPPINST);
		___pdr_lua_rawget(L, 1);
		if (!___pdr_lua_isuserdata(L, -1))
			___pdr_luaL_error(L, "expect LuaBase table at arg 1");

		___pdr_lua_pushcfunction(L, setParent);
		// push cpp inst
		___pdr_lua_pushvalue(L, -2);
		// push key
		___pdr_lua_pushvalue(L, 2);
		// push value
		___pdr_lua_pushvalue(L, 3);
		// set ok?
		if (___pdr_lua_pcall(L, 3, 0, 0)) {
			___pdr_lua_pop(L, 1);
			// push key
			___pdr_lua_pushvalue(L, 2);
			// push value
			___pdr_lua_pushvalue(L, 3);
			// rawset to table
			___pdr_lua_rawset(L, 1);
		}
		return 0;
	}

#if ((ENGINE_MINOR_VERSION<19) && (ENGINE_MAJOR_VERSION>=4))
    void LuaBase::hookBpScript(UFunction* func, Native hookfunc) {
#else
    void LuaBase::hookBpScript(UFunction* func, FNativeFuncPtr hookfunc) {
#endif
		static bool regExCode = false;
		if (!regExCode) {
			GRegisterNative(Ex_LuaHook, hookfunc);
			regExCode = true;
		}
		// if func had hooked
		if (func->Script.Num() > 5 && func->Script[5] == Ex_LuaHook)
			return;
		// if script isn't empty
		if (func->Script.Num() > 0) {
			// goto 8(a uint32 value) to skip return
			uint8 code[] = { EX_JumpIfNot,8,0,0,0,Ex_LuaHook,EX_Return,EX_Nothing };
			func->Script.Insert(code, sizeof(code), 0);
		}
	}

	LuaBase* checkBase(UObject* obj) {
		if (auto uit = Cast<UPLuaUserWidget>(obj))
			return uit;
		else if (auto ait = Cast<APLuaActor>(obj))
			return ait;
		else if (auto pit = Cast<APLuaPawn>(obj))
			return pit;
		else if (auto cit = Cast<APLuaCharacter>(obj))
			return cit;
		else if (auto coit = Cast<APLuaController>(obj))
			return coit;
		else if (auto pcit = Cast<APLuaPlayerController>(obj))
			return pcit;
		else if (auto acit = Cast<UPLuaActorComponent>(obj))
			return acit;
		else if (auto gmit = Cast<APLuaGameModeBase>(obj))
			return gmit;
		else if (auto hit = Cast<APLuaHUD>(obj))
			return hit;
		else
			return nullptr;
	}

#if ((ENGINE_MINOR_VERSION<19) && (ENGINE_MAJOR_VERSION>=4))
    void LuaBase::luaOverrideFunc(FFrame& Stack, RESULT_DECL)
#else
    DEFINE_FUNCTION(LuaBase::luaOverrideFunc)
#endif
	{
		UFunction* func = Stack.Node;
		ensure(func);
		LuaBase* lb = checkBase(Stack.Object);

		// maybe lb is nullptr, some member function with same name in different class
		// we don't care about it
		if (!lb) {
			*(bool*)RESULT_PARAM = false;
			return;
		}

		ensure(lb);

		if (lb->indexFlag==IF_SUPER && lb->currentFunction==func) {
			*(bool*)RESULT_PARAM = false;
			return;
		}

		void* params = Stack.Locals;

		LuaVar& luaSelfTable = lb->luaSelfTable;
		NS_PDR_SLUA::LuaVar lfunc = luaSelfTable.getFromTable<NS_PDR_SLUA::LuaVar>(func->GetName(), true);
		if (lfunc.isValid()) {
			lfunc.callByUFunction(func, (uint8*)params, &luaSelfTable, Stack.OutParms);
			*(bool*)RESULT_PARAM = true;
		}
		else {
			// if RESULT_PARAM is true, don't execute code behind this hook
			// otherwise execute code continue
			// don't have lua override function, continue execute bp code
			*(bool*)RESULT_PARAM = false;
		}

		
	}

	void LuaBase::bindOverrideFunc(UObject* obj)
	{
		ensure(obj && luaSelfTable.isValid());
		UClass* cls = obj->GetClass();
		ensure(cls);

		EFunctionFlags availableFlag = FUNC_BlueprintEvent;
		for (TFieldIterator<UFunction> it(cls); it; ++it) {
			if (!(it->FunctionFlags&availableFlag))
				continue;
			if (luaSelfTable.getFromTable<LuaVar>(it->GetName(), true).isFunction()) {
#if ((ENGINE_MINOR_VERSION<19) && (ENGINE_MAJOR_VERSION>=4))
                //hookBpScript(*it, (Native)&luaOverrideFunc);
#else
                hookBpScript(*it, (FNativeFuncPtr)&luaOverrideFunc);
#endif
			}
		}
	}

	template<typename T>
	UFunction* getSuperOrRpcFunction(___pdr_lua_State* L) {
		CheckUD(T, L, 1);
		___pdr_lua_getmetatable(L, 1);
		const char* name = LuaObject::checkValue<const char*>(L, 2);

		___pdr_lua_getfield(L, -1, name);
		___pdr_lua_remove(L, -2); // remove mt of ud
		if (!___pdr_lua_isnil(L, -1)) {
			return nullptr;
		}

		UObject* obj = UD->base->getContext().Get();
		if (!obj)
			___pdr_luaL_error(L, "Context is invalid");
		if (UD->base->getIndexFlag() == LuaBase::IF_RPC)
			___pdr_luaL_error(L, "Can't call super in RPC function");

		UFunction* func = obj->GetClass()->FindFunctionByName(UTF8_TO_TCHAR(name));
		if (!func || (func->FunctionFlags&FUNC_BlueprintEvent) == 0)
			___pdr_luaL_error(L, "Can't find function %s in super", name);

		return func;
	}

	int LuaBase::__superIndex(___pdr_lua_State* L) {
		
		UFunction* func = getSuperOrRpcFunction<LuaSuper>(L);
		if (!func) return 1;

		___pdr_lua_pushlightuserdata(L, func);
		___pdr_lua_pushcclosure(L, __superCall, 1);
		return 1;
	}

	int LuaBase::__rpcIndex(___pdr_lua_State* L) {

		UFunction* func = getSuperOrRpcFunction<LuaRpc>(L);
		if (!func) return 1;

		___pdr_lua_pushlightuserdata(L, func);
		___pdr_lua_pushcclosure(L, __rpcCall, 1);
		return 1;
	}

	int LuaBase::__superTick(___pdr_lua_State* L) {
		CheckUD(LuaSuper, L, 1);
		UD->base->indexFlag = IF_SUPER;
		UD->base->superTick(L);
		UD->base->indexFlag = IF_NONE;
		return 0;
	}

	int LuaBase::__superCall(___pdr_lua_State* L)
	{
		CheckUD(LuaSuper, L, 1);
		___pdr_lua_pushvalue(L, ___pdr_lua_upvalueindex(1));
		UFunction* func = (UFunction*) ___pdr_lua_touserdata(L, -1);
		if (!func || !func->IsValidLowLevel())
			___pdr_luaL_error(L, "Super function is isvalid");
		___pdr_lua_pop(L, 1);
		auto lbase = UD->base;
		ensure(lbase);
		lbase->currentFunction = func;
		lbase->indexFlag = IF_SUPER;
		int ret = lbase->superOrRpcCall(L, func);
		lbase->indexFlag = IF_NONE;
		lbase->currentFunction = nullptr;
		return ret;
	}

	int LuaBase::__rpcCall(___pdr_lua_State* L)
	{
		CheckUD(LuaRpc, L, 1);
		___pdr_lua_pushvalue(L, ___pdr_lua_upvalueindex(1));
		UFunction* func = (UFunction*)___pdr_lua_touserdata(L, -1);
		if (!func || !func->IsValidLowLevel())
			___pdr_luaL_error(L, "Super function is isvalid");
		___pdr_lua_pop(L, 1);
		auto lbase = UD->base;
		ensure(lbase);
		lbase->currentFunction = func;
		lbase->indexFlag = IF_RPC;
		int ret = lbase->superOrRpcCall(L, func);
		lbase->indexFlag = IF_NONE;
		lbase->currentFunction = nullptr;
		return ret;
	}

	int LuaBase::supermt(___pdr_lua_State* L)
	{
		LuaObject::setupMTSelfSearch(L);
		RegMetaMethodByName(L, "Tick", __superTick);
		RegMetaMethodByName(L, "__index", __superIndex);
		return 0;
	}

	int LuaBase::rpcmt(___pdr_lua_State* L)
	{
		LuaObject::setupMTSelfSearch(L);
		RegMetaMethodByName(L, "__index", __rpcIndex);
		return 0;
	}

	LuaVar LuaBase::callMember(FString func, const TArray<FPLuaBPVar>& args)
	{
		NS_PDR_SLUA::LuaVar lfunc = luaSelfTable.getFromTable<NS_PDR_SLUA::LuaVar>((const char*)TCHAR_TO_UTF8(*func), true);
		if (!lfunc.isFunction()) {
			Log::Error("Can't find lua member function named %s to call", TCHAR_TO_UTF8(*func));
			return false;
		}
		
		auto L = luaSelfTable.getState();
		// push self
		luaSelfTable.push(L);
		// push arg to lua
		for (auto& arg : args) {
			arg.value.push(L);
		}
		return lfunc.callWithNArg(args.Num()+1);
	}

	bool LuaBase::postInit(const char* tickFlag,bool rawget)
	{
		if (!luaSelfTable.isTable())
			return false;

		if (luaSelfTable.isTable()) {
			tickFunction = luaSelfTable.getFromTable<NS_PDR_SLUA::LuaVar>("Tick", true);
		}

		return luaSelfTable.getFromTable<bool>(tickFlag, rawget);
	}
}

bool IPLuaTableObjectInterface::isValid(IPLuaTableObjectInterface * luaTableObj)
{
	return luaTableObj && luaTableObj->getSelfTable().isTable();
}

int IPLuaTableObjectInterface::push(NS_PDR_SLUA::___pdr_lua_State * L, IPLuaTableObjectInterface * luaTableObj)
{
	if (!isValid(luaTableObj)) {
		NS_PDR_SLUA::Log::Error("Can't get a valid lua self table, push nil instead.");
		return NS_PDR_SLUA::LuaObject::pushNil(L);
	}
	auto self = luaTableObj->getSelfTable();
	return self.push(L);
}
