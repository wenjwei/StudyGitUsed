// Tencent is pleased to support the open source community by making sluaunreal available.

// Copyright (C) 2018 THL A29 Limited, a Tencent company. All rights reserved.
// Licensed under the BSD 3-Clause License (the "License"); 
// you may not use this file except in compliance with the License. You may obtain a copy of the License at

// https://opensource.org/licenses/BSD-3-Clause

// Unless required by applicable law or agreed to in writing, 
// software distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.

#pragma once
#include "CoreMinimal.h"
#include "PDRLuaState.h"
#include "PDRLuaBlueprintLibrary.h"

#if !((ENGINE_MINOR_VERSION>21) && (ENGINE_MAJOR_VERSION>=4))
#include "Kismet/GameplayStatics.h"
#endif



#include "PDRLuaBase.generated.h"

namespace NS_PDR_SLUA {

// special tick function
#define UFUNCTION_TICK ((UFunction*)-1)

	struct LuaSuperOrRpc {
		class LuaBase* base;
		LuaSuperOrRpc(class LuaBase* pBase) :base(pBase) {}
	};

	struct LuaSuper : public LuaSuperOrRpc {
		LuaSuper(class LuaBase* pBase) :LuaSuperOrRpc(pBase) {}
	};

	struct LuaRpc : public LuaSuperOrRpc {
		LuaRpc(class LuaBase* pBase) :LuaSuperOrRpc(pBase) {}
	};

	class PANDORA_API LuaBase {
	public:
		enum IndexFlag {
			IF_NONE,
			IF_SUPER,
			IF_RPC,
		};

		virtual bool luaImplemented(UFunction* func, void* params);
		virtual ~LuaBase() {}

		const FWeakObjectPtr& getContext() const {
			return context;
		}

		const IndexFlag getIndexFlag() const {
			return indexFlag;
		}
	protected:
		
		inline UGameInstance* getGameInstance(AActor* self) {
			return self->GetGameInstance();
		}

		inline UGameInstance* getGameInstance(UActorComponent* self) {
			return self->GetOwner()->GetGameInstance();
		}

		inline UGameInstance* getGameInstance(UUserWidget* self) {
#if (ENGINE_MINOR_VERSION>21) && (ENGINE_MAJOR_VERSION>=4)
			return self->GetGameInstance();
#else
			return UGameplayStatics::GetGameInstance(self);
#endif
		}

		template<typename T>
		static int genericGC(___pdr_lua_State* L) {
			CheckUDGC(T, L, 1);
			delete UD;
			return 0;
		}

		template<typename T>
		bool init(T* ptrT, const char* typeName, const FString& stateName, const FString& luaPath)
		{
			if (luaPath.IsEmpty())
				return false;
			
			ensure(ptrT);
			auto ls = LuaState::get(getGameInstance(ptrT));
			if (stateName.Len() != 0) ls = LuaState::get(stateName);
			if (!ls) return false;

			luaSelfTable = ls->doFile(TCHAR_TO_UTF8(*luaPath));
			if (!luaSelfTable.isTable())
				return false;

			context = ptrT;
			auto L = ls->getLuaState();
			// setup __cppinst
			// we use rawpush to bind objptr and SLUA_CPPINST
			luaSelfTable.push(L);
			LuaObject::push(L, ptrT, true);
			___pdr_lua_setfield(L, -2, SLUA_CPPINST);

			LuaObject::pushType(L, new LuaSuper(this) , "LuaSuper", supermt, genericGC<LuaSuper>);
			___pdr_lua_setfield(L, -2, "Super");

			LuaObject::pushType(L, new LuaRpc(this), "LuaRpc", rpcmt, genericGC<LuaRpc>);
			___pdr_lua_setfield(L, -2, "Rpc");

			int top = ___pdr_lua_gettop(L);
			bindOverrideFunc(ptrT);
			int newtop = ___pdr_lua_gettop(L);

			// setup metatable
			if (!metaTable.isValid()) {
				___pdr_luaL_newmetatable(L, typeName);
				___pdr_lua_pushcfunction(L, __index);
				___pdr_lua_setfield(L, -2, "__index");
				___pdr_lua_pushcfunction(L, __newindex);
				___pdr_lua_setfield(L, -2, "__newindex");
				metaTable.set(L, -1);
				___pdr_lua_pop(L, 1);
			}
			metaTable.push(L);
			___pdr_lua_setmetatable(L, -2);

			// pop luaSelfTable
			___pdr_lua_pop(L, 1);
			return true;
		}

#if ((ENGINE_MINOR_VERSION<19) && (ENGINE_MAJOR_VERSION>=4))
        static void hookBpScript(UFunction* func, Native hookfunc);
#else
        static void hookBpScript(UFunction* func, FNativeFuncPtr hookfunc);
#endif
        void bindOverrideFunc(UObject* obj);
#if ((ENGINE_MINOR_VERSION<19) && (ENGINE_MAJOR_VERSION>=4))
        void luaOverrideFunc(FFrame& Stack, RESULT_DECL);
#else
        DECLARE_FUNCTION(luaOverrideFunc);
#endif

		static int supermt(___pdr_lua_State* L);
		static int rpcmt(___pdr_lua_State* L);

		// store deltaTime for super call
		float deltaTime;

		// call member function in luaSelfTable
		LuaVar callMember(FString name, const TArray<FPLuaBPVar>& args);

		bool postInit(const char* tickFlag,bool rawget=true);
		virtual void tick(float DeltaTime);
		// should override this function to support super::tick
		virtual void superTick(___pdr_lua_State* L);
		virtual void superTick() = 0;
		virtual int superOrRpcCall(___pdr_lua_State* L, UFunction* func);
		static int __index(___pdr_lua_State* L);
		static int __newindex(___pdr_lua_State* L);
		static int __superIndex(___pdr_lua_State* L);
		static int __rpcIndex(___pdr_lua_State* L);
		static int __superTick(___pdr_lua_State* L);
		static int __superCall(___pdr_lua_State* L);
		static int __rpcCall(___pdr_lua_State* L);

		LuaVar luaSelfTable;
		LuaVar tickFunction;
		FWeakObjectPtr context;
		LuaVar metaTable;
		IndexFlag indexFlag = IF_NONE;
		UFunction* currentFunction = nullptr;
	};

	DefTypeName(LuaSuper);
	DefTypeName(LuaRpc);
}

UINTERFACE()
class UPLuaTableObjectInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class IPLuaTableObjectInterface {
	GENERATED_IINTERFACE_BODY()

public:
	static bool isValid(IPLuaTableObjectInterface* luaTableObj);
	static int push(NS_PDR_SLUA::___pdr_lua_State* L, IPLuaTableObjectInterface* luaTableObj);

	virtual NS_PDR_SLUA::LuaVar getSelfTable() const = 0;
};