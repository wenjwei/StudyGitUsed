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
#define LUA_LIB
#include "CoreMinimal.h"
#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif
#include "lua/lua.hpp"
#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif
#include "UObject/UnrealType.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "PDRLuaVar.h"
#include "PDRLuaBlueprintLibrary.generated.h"

USTRUCT(BlueprintType)
struct PANDORA_API FPLuaBPVar {
	GENERATED_USTRUCT_BODY()
public:
	FPLuaBPVar(const NS_PDR_SLUA::LuaVar& v) :value(v) {}
	FPLuaBPVar(NS_PDR_SLUA::LuaVar&& v) :value(std::move(v)) {}
	FPLuaBPVar() {}

	NS_PDR_SLUA::LuaVar value;

	static int checkValue(NS_PDR_SLUA::___pdr_lua_State* L, UStructProperty* p, uint8* params, int i);
};

UCLASS()
class PANDORA_API UPLuaBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:

	/** Call a lua function with args */
	UFUNCTION(BlueprintCallable, meta=( DisplayName="Call To Lua With Arguments", WorldContext = "WorldContextObject"), Category="slua")
	static FPLuaBPVar CallToLuaWithArgs(UObject* WorldContextObject, FString FunctionName,const TArray<FPLuaBPVar>& Args,FString StateName);

	UFUNCTION(BlueprintCallable, meta=( DisplayName="Call To Lua", WorldContext = "WorldContextObject"), Category="slua")
	static FPLuaBPVar CallToLua(UObject* WorldContextObject, FString FunctionName,FString StateName);

	UFUNCTION(BlueprintCallable, Category="slua")
	static FPLuaBPVar CreateVarFromInt(int Value);

	UFUNCTION(BlueprintCallable, Category="slua")
	static FPLuaBPVar CreateVarFromString(FString Value);

	UFUNCTION(BlueprintCallable, Category="slua")
	static FPLuaBPVar CreateVarFromNumber(float Value);

	UFUNCTION(BlueprintCallable, Category="slua")
	static FPLuaBPVar CreateVarFromBool(bool Value);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category="slua")
	static FPLuaBPVar CreateVarFromObject(UObject* WorldContextObject, UObject* Value);

	UFUNCTION(BlueprintCallable, Category="slua")
	static int GetIntFromVar(FPLuaBPVar Value,int Index=1);
	
	UFUNCTION(BlueprintCallable, Category="slua")
	static float GetNumberFromVar(FPLuaBPVar Value,int Index=1);

	UFUNCTION(BlueprintCallable, Category="slua")
	static FString GetStringFromVar(FPLuaBPVar Value,int Index=1);

	UFUNCTION(BlueprintCallable, Category="slua")
	static bool GetBoolFromVar(FPLuaBPVar Value,int Index=1);

	UFUNCTION(BlueprintCallable, Category="slua")
	static UObject* GetObjectFromVar(FPLuaBPVar Value,int Index=1);
};