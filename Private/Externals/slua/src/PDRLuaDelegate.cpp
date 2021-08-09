// Tencent is pleased to support the open source community by making sluaunreal available.

// Copyright (C) 2018 THL A29 Limited, a Tencent company. All rights reserved.
// Licensed under the BSD 3-Clause License (the "License"); 
// you may not use this file except in compliance with the License. You may obtain a copy of the License at

// https://opensource.org/licenses/BSD-3-Clause

// Unless required by applicable law or agreed to in writing, 
// software distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.

#include "PDRLuaDelegate.h"
#include "PDRLuaObject.h"
#include "PDRLuaVar.h"

UPLuaDelegate::UPLuaDelegate(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
    ,luafunction(nullptr)
    ,ufunction(nullptr)
{
}

UPLuaDelegate::~UPLuaDelegate() {
    SafeDelete(luafunction);
}

void UPLuaDelegate::EventTrigger()
{
    ensure(false); // never run here
}

void UPLuaDelegate::ProcessEvent( UFunction* f, void* Parms ) {
    ensure(luafunction!=nullptr && ufunction!=nullptr);
    luafunction->callByUFunction(ufunction,reinterpret_cast<uint8*>(Parms));
}

void UPLuaDelegate::bindFunction(NS_PDR_SLUA::___pdr_lua_State* L,int p,UFunction* ufunc) {
    ___pdr_luaL_checktype(L,p,___PDR_LUA_TFUNCTION);
    ensure(ufunc);
    luafunction = new NS_PDR_SLUA::LuaVar(L,p,NS_PDR_SLUA::LuaVar::LV_FUNCTION);
    ufunction = ufunc;
}

void UPLuaDelegate::bindFunction(NS_PDR_SLUA::___pdr_lua_State* L,int p) {
    ___pdr_luaL_checktype(L,p,___PDR_LUA_TFUNCTION);
    luafunction = new NS_PDR_SLUA::LuaVar(L,p,NS_PDR_SLUA::LuaVar::LV_FUNCTION);
}

void UPLuaDelegate::bindFunction(UFunction *ufunc) {
    ensure(ufunc);
    ufunction = ufunc;
}

void UPLuaDelegate::dispose()
{
	SafeDelete(luafunction);
	ufunction = nullptr;
}
namespace NS_PDR_SLUA {

    struct LuaMultiDelegateWrap {
        FMulticastScriptDelegate* delegate;
        UFunction* ufunc;
#if WITH_EDITOR
		FString pName;
#endif
    };

    DefTypeName(LuaMultiDelegateWrap);

	struct LuaDelegateWrap {
		FScriptDelegate* delegate;
		UFunction* ufunc;
#if WITH_EDITOR
		FString pName;
#endif
	};

	DefTypeName(LuaDelegateWrap);

    int LuaMultiDelegate::Add(___pdr_lua_State* L) {
        CheckUD(LuaMultiDelegateWrap,L,1);

        // bind luafucntion and signature function
        auto obj = NewObject<UPLuaDelegate>((UObject*)GetTransientPackage(),UPLuaDelegate::StaticClass());
#if WITH_EDITOR
		obj->setPropName(UD->pName);
#endif
        obj->bindFunction(L,2,UD->ufunc);

        // add event listener
        FScriptDelegate Delegate;
        Delegate.BindUFunction(obj, TEXT("EventTrigger"));
        UD->delegate->AddUnique(Delegate);

        // add reference
        LuaObject::addRef(L,obj,nullptr,true);

        ___pdr_lua_pushlightuserdata(L,obj);
        return 1;
    }

    int LuaMultiDelegate::Remove(___pdr_lua_State* L) {
        CheckUD(LuaMultiDelegateWrap,L,1);
        if(!___pdr_lua_islightuserdata(L,2))
            ___pdr_luaL_error(L,"arg 2 expect UPLuaDelegate");
        auto obj =  reinterpret_cast<UPLuaDelegate*>(___pdr_lua_touserdata(L,2));
		if (!obj->IsValidLowLevel())
		{
#if UE_BUILD_DEVELOPMENT
			___pdr_luaL_error(L, "Invalid UPLuaDelegate!");
#else
			return 0;
#endif
		}

        FScriptDelegate Delegate;
        Delegate.BindUFunction(obj, TEXT("EventTrigger"));

        // remove delegate
        UD->delegate->Remove(Delegate);

        // remove reference
        LuaObject::removeRef(L,obj);
		obj->dispose();

        return 0;
    }

    int LuaMultiDelegate::Clear(___pdr_lua_State* L) {
        CheckUD(LuaMultiDelegateWrap,L,1);
        auto array = UD->delegate->GetAllObjects();
        for(auto it:array) {
			UPLuaDelegate* delegateObj = Cast<UPLuaDelegate>(it);
			if (delegateObj)
			{
				delegateObj->dispose();
				LuaObject::removeRef(L, it);
			}
        }
        UD->delegate->Clear();
        return 0;
    }

    int LuaMultiDelegate::gc(___pdr_lua_State* L) { 
        CheckUD(LuaMultiDelegateWrap,L,1);
        delete UD;
        return 0;    
    }
   
    int LuaMultiDelegate::setupMT(___pdr_lua_State* L) {
        LuaObject::setupMTSelfSearch(L);

        RegMetaMethod(L,Add);
        RegMetaMethod(L,Remove);
        RegMetaMethod(L,Clear);
        return 0;
    }

    int LuaMultiDelegate::push(___pdr_lua_State* L,FMulticastScriptDelegate* delegate,UFunction* ufunc,FString pName) {
#if WITH_EDITOR
		LuaMultiDelegateWrap* wrapobj = new LuaMultiDelegateWrap{ delegate,ufunc,pName };
#else
		LuaMultiDelegateWrap* wrapobj = new LuaMultiDelegateWrap{ delegate,ufunc };
#endif
        return LuaObject::pushType<LuaMultiDelegateWrap*>(L,wrapobj,"LuaMultiDelegateWrap",setupMT,gc);
    }

    void clear(___pdr_lua_State* L, LuaDelegateWrap* ldw) {
        auto object = ldw->delegate->GetUObject();
		if (object)
		{
			UPLuaDelegate* delegateObj = Cast<UPLuaDelegate>(object);
			if (delegateObj)
			{
				LuaObject::removeRef(L, object);
				delegateObj->dispose();
			}
		}
		ldw->delegate->Clear();
    }

	int LuaDelegate::Bind(___pdr_lua_State* L)
	{
		CheckUD(LuaDelegateWrap, L, 1);

        // clear old delegate object
        if(UD) clear(L,UD);

		// bind luafucntion and signature function
		auto obj = NewObject<UPLuaDelegate>((UObject*)GetTransientPackage(), UPLuaDelegate::StaticClass());
#if WITH_EDITOR
		obj->setPropName(UD->pName);
#endif
		obj->bindFunction(L, 2, UD->ufunc);

		UD->delegate->BindUFunction(obj, TEXT("EventTrigger"));

		// add reference
		LuaObject::addRef(L, obj, nullptr, true);

		___pdr_lua_pushlightuserdata(L, obj);
		return 1;
	}

	int LuaDelegate::Clear(___pdr_lua_State* L)
	{
		CheckUD(LuaDelegateWrap, L, 1);
		if(UD) clear(L,UD);
		return 0;
	}

	int LuaDelegate::gc(___pdr_lua_State* L)
	{
		CheckUD(LuaDelegateWrap, L, 1);
		delete UD;
		return 0;
	}

	int LuaDelegate::setupMT(___pdr_lua_State* L)
	{
		LuaObject::setupMTSelfSearch(L);

		RegMetaMethod(L, Bind);
		RegMetaMethod(L, Clear);
		return 0;
	}

	int LuaDelegate::push(___pdr_lua_State* L, FScriptDelegate* delegate, UFunction* ufunc, FString pName)
	{
#if WITH_EDITOR
		LuaDelegateWrap* wrapobj = new LuaDelegateWrap{ delegate,ufunc,pName };
#else
		LuaDelegateWrap* wrapobj = new LuaDelegateWrap{ delegate,ufunc };
#endif
		return LuaObject::pushType<LuaDelegateWrap*>(L, wrapobj, "LuaDelegateWrap", setupMT, gc);
	}

}