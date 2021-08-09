// Tencent is pleased to support the open source community by making sluaunreal available.

// Copyright (C) 2018 THL A29 Limited, a Tencent company. All rights reserved.
// Licensed under the BSD 3-Clause License (the "License"); 
// you may not use this file except in compliance with the License. You may obtain a copy of the License at

// https://opensource.org/licenses/BSD-3-Clause

// Unless required by applicable law or agreed to in writing, 
// software distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.

#ifdef _WIN32
#pragma warning (push)
#pragma warning (disable : 4018)
#endif

#include "PDRLuaBlueprintLibrary.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Blueprint/BlueprintSupport.h"
#include "PDRLuaState.h"
#include "Internationalization/Internationalization.h"

namespace {
    const FName GetVarOutOfBoundsWarning = FName("GetVarOutOfBoundsWarning");    
    const FName GetVarTypeErrorWarning = FName("GetVarTypeErrorWarning");    
}

#define LOCTEXT_NAMESPACE "UPLuaBlueprintLibrary"

UPLuaBlueprintLibrary::UPLuaBlueprintLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
    FBlueprintSupport::RegisterBlueprintWarning(
		FBlueprintWarningDeclaration (
			GetVarOutOfBoundsWarning,
			LOCTEXT("GetOutOfBoundsWarning", "BpVar read access out of bounds")
		)
	);
	FBlueprintSupport::RegisterBlueprintWarning(
		FBlueprintWarningDeclaration (
			GetVarTypeErrorWarning,
			LOCTEXT("GetVarTypeErrorWarning", "BpVar is not speicified type")
		)
	);

}

FPLuaBPVar UPLuaBlueprintLibrary::CallToLuaWithArgs(UObject* WorldContextObject, FString funcname,const TArray<FPLuaBPVar>& args,FString StateName) {
    // using namespace NS_PDR_SLUA;
    // get main state
	auto actor = Cast<AActor>(WorldContextObject);
	ensure(actor);
    auto ls = NS_PDR_SLUA::LuaState::get(actor->GetGameInstance());
    if(StateName.Len()!=0) ls = NS_PDR_SLUA::LuaState::get(StateName);
    if(!ls) return FPLuaBPVar();
    NS_PDR_SLUA::LuaVar f = ls->get(TCHAR_TO_UTF8(*funcname));
    if(!f.isFunction()) {
		NS_PDR_SLUA::Log::Error("Can't find lua member function named %s to call", TCHAR_TO_UTF8(*funcname));
        return NS_PDR_SLUA::LuaVar();
    }

    for(auto& arg:args) {
        arg.value.push(ls->getLuaState());
    }
    return f.callWithNArg(args.Num());
}

FPLuaBPVar UPLuaBlueprintLibrary::CallToLua(UObject* WorldContextObject, FString funcname,FString StateName) {
    // using namespace NS_PDR_SLUA;
    // get main state
	auto actor = Cast<AActor>(WorldContextObject);
	ensure(actor);
	auto ls = NS_PDR_SLUA::LuaState::get(actor->GetGameInstance());
    if(StateName.Len()!=0) ls = NS_PDR_SLUA::LuaState::get(StateName);
    if(!ls) return FPLuaBPVar();
    NS_PDR_SLUA::LuaVar f = ls->get(TCHAR_TO_UTF8(*funcname));
	if (!f.isFunction()) {
		NS_PDR_SLUA::Log::Error("Can't find lua member function named %s to call", TCHAR_TO_UTF8(*funcname));
        return NS_PDR_SLUA::LuaVar();
    }
    return f.callWithNArg(0);
}


FPLuaBPVar UPLuaBlueprintLibrary::CreateVarFromInt(int i) {
    FPLuaBPVar v;
    v.value.set(i);
    return v;
}

FPLuaBPVar UPLuaBlueprintLibrary::CreateVarFromString(FString s) {
    FPLuaBPVar v;
    v.value.set(TCHAR_TO_UTF8(*s),0);
    return v;
}

FPLuaBPVar UPLuaBlueprintLibrary::CreateVarFromNumber(float d) {
    FPLuaBPVar v;
    v.value.set(d);
    return v;
}

FPLuaBPVar UPLuaBlueprintLibrary::CreateVarFromBool(bool b) {
    FPLuaBPVar v;
    v.value.set(b);
    return v;
}

FPLuaBPVar UPLuaBlueprintLibrary::CreateVarFromObject(UObject* WorldContextObject, UObject* o) {
    // using namespace NS_PDR_SLUA;
	auto actor = Cast<AActor>(WorldContextObject);
	ensure(actor);
    auto ls = NS_PDR_SLUA::LuaState::get(actor->GetGameInstance());
    if(!ls) return FPLuaBPVar();
    NS_PDR_SLUA::LuaObject::push(ls->getLuaState(),o);
    NS_PDR_SLUA::LuaVar ret(ls->getLuaState(),-1);
    ___pdr_lua_pop(ls->getLuaState(),1);
    return FPLuaBPVar(ret);
}

int FPLuaBPVar::checkValue(NS_PDR_SLUA::___pdr_lua_State* L, UStructProperty* p, uint8* params, int i)
{
	FPLuaBPVar ret;
	ret.value.set(L, i);
	p->CopyCompleteValue(params, &ret);
	return 0;
}


namespace PDR_BP_UTIL {
    // using namespace NS_PDR_SLUA;

    bool getValue(const NS_PDR_SLUA::LuaVar& lv,int index,int& value) {
        if(index==1 && lv.count()>=index && lv.isInt()) {
            value = lv.asInt();
            return true;
        }
        else if(lv.isTable() || lv.isTuple()) {
            NS_PDR_SLUA::LuaVar v = lv.getAt(index);
            return getValue(v,1,value);
        }
        else
            return false;
    }

    bool getValue(const NS_PDR_SLUA::LuaVar& lv,int index,bool& value) {
        if(index==1 && lv.count()>=index && lv.isBool()) {
            value = lv.asBool();
            return true;
        }
        else if(lv.isTable() || lv.isTuple()) {
            NS_PDR_SLUA::LuaVar v = lv.getAt(index);
            return getValue(v,1,value);
        }
        else
            return false;
    }

    bool getValue(const NS_PDR_SLUA::LuaVar& lv,int index,float& value) {
        if(index==1 && lv.count()>=index && lv.isNumber()) {
            value = lv.asFloat();
            return true;
        }
        else if(lv.isTable() || lv.isTuple()) {
            NS_PDR_SLUA::LuaVar v = lv.getAt(index);
            return getValue(v,1,value);
        }
        else
            return false;
    }

    bool getValue(const NS_PDR_SLUA::LuaVar& lv,int index,FString& value) {
        if(index==1 && lv.count()>=index && lv.isString()) {
            value = UTF8_TO_TCHAR(lv.asString());
            return true;
        }
        else if(lv.isTable() || lv.isTuple()) {
            NS_PDR_SLUA::LuaVar v = lv.getAt(index);
            return getValue(v,1,value);
        }
        else
            return false;
    }

    bool getValue(const NS_PDR_SLUA::LuaVar& lv,int index,UObject*& value) {
        if(index==1 && lv.count()>=index && lv.isUserdata("UObject")) {
            value = lv.asUserdata<UObject>("UObject");
            return true;
        }
        else if(lv.isTable() || lv.isTuple()) {
            NS_PDR_SLUA::LuaVar v = lv.getAt(index);
            return getValue(v,1,value);
        }
        else
            return false;
    }

    template<class T>
    T getValueFromVar(const FPLuaBPVar& Value,int Index) {
        // using namespace NS_PDR_SLUA;
        const NS_PDR_SLUA::LuaVar& lv = Value.value;
        if(Index<=lv.count()) {
            T v;
            if(getValue(lv,Index,v))
                return v;
            else
                FFrame::KismetExecutionMessage(*FString::Printf(TEXT("Attempted to index an item from an invalid type!")),
                    ELogVerbosity::Warning, GetVarTypeErrorWarning);
        }
        else {
            FFrame::KismetExecutionMessage(*FString::Printf(TEXT("Attempted to index an item from an invalid index from BpVar [%d/%d]!"),
                 Index, lv.count()), ELogVerbosity::Warning, GetVarOutOfBoundsWarning);
        }
        return T();
    }
}



int UPLuaBlueprintLibrary::GetIntFromVar(FPLuaBPVar Value,int Index) {
    return PDR_BP_UTIL::getValueFromVar<int>(Value,Index);
}

float UPLuaBlueprintLibrary::GetNumberFromVar(FPLuaBPVar Value,int Index) {
    return PDR_BP_UTIL::getValueFromVar<float>(Value,Index);
}

bool UPLuaBlueprintLibrary::GetBoolFromVar(FPLuaBPVar Value,int Index) {
    return PDR_BP_UTIL::getValueFromVar<bool>(Value,Index);
}

FString UPLuaBlueprintLibrary::GetStringFromVar(FPLuaBPVar Value,int Index) {
    return PDR_BP_UTIL::getValueFromVar<FString>(Value,Index);
}

UObject* UPLuaBlueprintLibrary::GetObjectFromVar(FPLuaBPVar Value,int Index) {
    return PDR_BP_UTIL::getValueFromVar<UObject*>(Value,Index);
}

#undef LOCTEXT_NAMESPACE

#ifdef _WIN32
#pragma warning (pop)
#endif