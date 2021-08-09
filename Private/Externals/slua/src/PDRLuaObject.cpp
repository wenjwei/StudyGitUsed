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

#include "PDRLuaObject.h"
#include "PDRLuaVar.h"
#include "PDRLuaDelegate.h"
#include "PDRLatentDelegate.h"
#include "UObject/StructOnScope.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/Stack.h"
#include "Blueprint/WidgetTree.h"
#include "PDRLuaWidgetTree.h"
#include "PDRLuaArray.h"
#include "PDRLuaMap.h"
#include "PDRSluaLog.h"
#include "PDRLuaState.h"
#include "PDRLuaWrapper.h"
#include "PDRSluaUtil.h"
#include "PDRLuaReference.h"
#include "PDRLuaBase.h"
#include "Engine/UserDefinedEnum.h"

namespace NS_PDR_SLUA { 
	static const FName NAME_LatentInfo = TEXT("LatentInfo");

	TMap<UClass*,LuaObject::PushPropertyFunction> pusherMap;
	TMap<UClass*,LuaObject::CheckPropertyFunction> checkerMap;

	struct ExtensionField {
		bool isFunction = true;
		union {
			struct {
				___pdr_lua_CFunction getter;
				___pdr_lua_CFunction setter;
			};
			___pdr_lua_CFunction func;
		};

		ExtensionField(___pdr_lua_CFunction funcf) : isFunction(true), func(funcf) {
			ensure(funcf);
		}
		ExtensionField(___pdr_lua_CFunction getterf, ___pdr_lua_CFunction setterf) 
			: isFunction(false)
			, getter(getterf)
			, setter(setterf) {}
	};

    #if ENGINE_MINOR_VERSION >= 23 && (PLATFORM_MAC || PLATFORM_IOS)
    struct FNewFrame : public FOutputDevice
        {
        public:
            // Variables.
            UFunction* Node;
            UObject* Object;
            uint8* Code;
            uint8* Locals;
            
            UProperty* MostRecentProperty;
            uint8* MostRecentPropertyAddress;
            
            /** The execution flow stack for compiled Kismet code */
            FlowStackType FlowStack;
            
            /** Previous frame on the stack */
            FFrame* PreviousFrame;
            
            /** contains information on any out parameters */
            FOutParmRec* OutParms;
            
            /** If a class is compiled in then this is set to the property chain for compiled-in functions. In that case, we follow the links to setup the args instead of executing by code. */
            UField* PropertyChainForCompiledIn;
            
            /** Currently executed native function */
            UFunction* CurrentNativeFunction;
            
            bool bArrayContextFailed;
        public:
            
            // Constructors.
            FNewFrame( UObject* InObject, UFunction* InNode, void* InLocals, FFrame* InPreviousFrame = NULL, UField* InPropertyChainForCompiledIn = NULL )
            : Node(InNode)
            , Object(InObject)
            , Code(InNode->Script.GetData())
            , Locals((uint8*)InLocals)
            , MostRecentProperty(NULL)
            , MostRecentPropertyAddress(NULL)
            , PreviousFrame(InPreviousFrame)
            , OutParms(NULL)
            , PropertyChainForCompiledIn(InPropertyChainForCompiledIn)
            , CurrentNativeFunction(NULL)
            , bArrayContextFailed(false)
            {
        #if DO_BLUEPRINT_GUARD
                FBlueprintExceptionTracker::Get().ScriptStack.Push((FFrame *)this);
        #endif
            }
            
            virtual ~FNewFrame()
            {
                #if DO_BLUEPRINT_GUARD
                    FBlueprintExceptionTracker& BlueprintExceptionTracker = FBlueprintExceptionTracker::Get();
                    if (BlueprintExceptionTracker.ScriptStack.Num())
                    {
                        BlueprintExceptionTracker.ScriptStack.Pop(false);
                    }
                #endif
            }
            
            virtual void Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category ) {};
        };
    #endif
    
    TMap< UClass*, TMap<FString, ExtensionField> > extensionMMap;
    TMap< UClass*, TMap<FString, ExtensionField> > extensionMMap_static;

    namespace ExtensionMethod{
        void init();
    }

    DefTypeName(LuaStruct)

    // construct lua struct
    LuaStruct::LuaStruct(uint8* b,uint32 s,UScriptStruct* u)
        :buf(b),size(s),uss(u) {
    }

    LuaStruct::~LuaStruct() {
		if (buf) {
			uss->DestroyStruct(buf);
			FMemory::Free(buf);
			buf = nullptr;
		}
    }

	void LuaStruct::AddReferencedObjects(FReferenceCollector& Collector) {
		Collector.AddReferencedObject(uss);
		LuaReference::addRefByStruct(Collector, uss, buf);
	}

    void LuaObject::addExtensionMethod(UClass* cls,const char* n,___pdr_lua_CFunction func,bool isStatic) {
        if(isStatic) {
            auto& extmap = extensionMMap_static.FindOrAdd(cls);
            extmap.Add(n, ExtensionField(func));
        }
        else {
            auto& extmap = extensionMMap.FindOrAdd(cls);
            extmap.Add(n, ExtensionField(func));
        }
    }

	void LuaObject::addExtensionProperty(UClass * cls, const char * n, ___pdr_lua_CFunction getter, ___pdr_lua_CFunction setter, bool isStatic)
	{
		if (isStatic) {
			auto& extmap = extensionMMap_static.FindOrAdd(cls);
			extmap.Add(n, ExtensionField(getter,setter));
		}
		else {
			auto& extmap = extensionMMap.FindOrAdd(cls);
			extmap.Add(n, ExtensionField(getter, setter));
		}
	}

    static int findMember(___pdr_lua_State* L,const char* name) {
       int popn = 0;
        if ((++popn, ___pdr_lua_getfield(L, -1, name) != 0)) {
			___pdr_lua_remove(L, -2); // remove mt
            return 1;
        } else if ((++popn, ___pdr_lua_getfield(L, -2, ".get")) && (++popn, ___pdr_lua_getfield(L, -1, name))) {
            ___pdr_lua_pushvalue(L, 1);
            lua_call(L, 1, 1);
			___pdr_lua_remove(L, -2); // remove .get
            return 1;
        } else {
            // find base
            ___pdr_lua_pop(L, popn);
            ___pdr_lua_getfield(L,-1, "__base");
			___pdr_luaL_checktype(L, -1, ___PDR_LUA_TTABLE);
			// for each base
            {
                size_t cnt = ___pdr_lua_rawlen(L,-1);
                int r = 0;
                for(size_t n=0;n<cnt;n++) {
                    ___pdr_lua_geti(L,-1,n+1);
                    const char* tn = ___pdr_lua_tostring(L,-1);
                    ___pdr_lua_pop(L,1); // pop tn
                    ___pdr_luaL_getmetatable(L,tn);
					___pdr_luaL_checktype(L, -1, ___PDR_LUA_TTABLE);
					if (findMember(L, name)) return 1;
                }
                ___pdr_lua_remove(L,-2); // remove __base
                return r;
            }
        }
    }

	static bool setMember(___pdr_lua_State* L, const char* name) {
		int popn = 0;
		if ((++popn, ___pdr_lua_getfield(L, -1, ".set")) && (++popn, ___pdr_lua_getfield(L, -1, name))) {
			// push ud
			___pdr_lua_pushvalue(L, 1);
			// push value
			___pdr_lua_pushvalue(L, 3);
			// call setter
			lua_call(L, 2, 0);
			___pdr_lua_pop(L, 1); // pop .set
			return true;
		}
		else {
			// find base
			___pdr_lua_pop(L, popn);
			___pdr_lua_getfield(L, -1, "__base");
			___pdr_luaL_checktype(L, -1, ___PDR_LUA_TTABLE);
			// for each base
			{
				size_t cnt = ___pdr_lua_rawlen(L, -1);
				for (size_t n = 0; n < cnt; n++) {
					___pdr_lua_geti(L, -1, n + 1);
					const char* tn = ___pdr_lua_tostring(L, -1);
					___pdr_lua_pop(L, 1); // pop tn
					___pdr_luaL_getmetatable(L, tn);
					___pdr_luaL_checktype(L, -1, ___PDR_LUA_TTABLE);
					if (setMember(L, name)) return true;
				}
			}
			// pop __base
			___pdr_lua_pop(L, 1);
			return false;
		}
	}

	int LuaObject::classIndex(___pdr_lua_State* L) {
		___pdr_lua_getmetatable(L, 1);
		const char* name = checkValue<const char*>(L, 2);
		if (!findMember(L, name))
			___pdr_luaL_error(L, "can't get %s", name);
		___pdr_lua_remove(L, -2);
		return 1;
	}

	int LuaObject::classNewindex(___pdr_lua_State* L) {
		___pdr_lua_getmetatable(L, 1);
		const char* name = checkValue<const char*>(L, 2);
		if(!setMember(L, name))
			___pdr_luaL_error(L, "can't set %s", name);
		___pdr_lua_pop(L, 1);
		return 0;
	}

	static void setMetaMethods(___pdr_lua_State* L) {
		___pdr_lua_newtable(L);
		___pdr_lua_pushvalue(L, -1);
		___pdr_lua_setfield(L, -3, ".get"); // upvalue
		___pdr_lua_pushcclosure(L, LuaObject::classIndex, 1);
		___pdr_lua_setfield(L, -2, "__index");
		___pdr_lua_newtable(L);
		___pdr_lua_pushvalue(L, -1);
		___pdr_lua_setfield(L, -3, ".set"); // upvalue
		___pdr_lua_pushcclosure(L, LuaObject::classNewindex, 1);
		___pdr_lua_setfield(L, -2, "__newindex");
	}

	void LuaObject::newType(___pdr_lua_State* L, const char* tn) {
		___pdr_lua_pushglobaltable(L);				    // _G
		___pdr_lua_newtable(L);							// local t = {}
		___pdr_lua_pushvalue(L, -1);
		___pdr_lua_setfield(L, -3, tn);					// _G[tn] = t
		___pdr_lua_remove(L, -2);						// remove global table;

        ___pdr_lua_newtable(L);
		___pdr_lua_pushvalue(L, -1);
		___pdr_lua_setmetatable(L, -3);					// setmetatable(t, mt)
		setMetaMethods(L);

		___pdr_luaL_newmetatable(L, tn);
		setMetaMethods(L);
	}

    void LuaObject::newTypeWithBase(___pdr_lua_State* L, const char* tn, std::initializer_list<const char*> bases) {
		newType(L,tn);

        // create base table
        ___pdr_lua_newtable(L);
        ___pdr_lua_pushvalue(L,-1);
        ___pdr_lua_setfield(L,-3,"__base");
        
        for(auto base:bases) {
            if(strlen(base)>0) {
                ___pdr_lua_pushstring(L,base);
                size_t p = ___pdr_lua_rawlen(L,-2);
                ___pdr_lua_seti(L,-2,p+1);
            }
        }
        // pop __base table
        ___pdr_lua_pop(L,1);
	}

	int LuaObject::push(___pdr_lua_State * L, const LuaLString& lstr)
	{
		___pdr_lua_pushlstring(L, lstr.buf, lstr.len);
		return 1;
	}

	bool LuaObject::isBaseTypeOf(___pdr_lua_State* L,const char* tn,const char* base) {
        AutoStack as(L);
        int t = ___pdr_luaL_getmetatable(L,tn);
        if(t!=___PDR_LUA_TTABLE)
            return false;

        if(___pdr_lua_getfield(L,-1,"__base")==___PDR_LUA_TTABLE) {
            size_t len = ___pdr_lua_rawlen(L,-1);
            for(int n=0;n<len;n++) {
                if(___pdr_lua_geti(L,-1,n+1)==___PDR_LUA_TSTRING) {
                    const char* maybeBase = ___pdr_lua_tostring(L,-1);
                    if(strcmp(maybeBase,base)==0) return true;
                    else return isBaseTypeOf(L,maybeBase,base);
                }
            }
            return false;
        }
        return false;
    }

	void LuaObject::addMethod(___pdr_lua_State* L, const char* name, ___pdr_lua_CFunction func, bool isInstance) {
		___pdr_lua_pushcfunction(L, func);
		___pdr_lua_setfield(L, isInstance ? -2 : -3, name);
	}

    void LuaObject::addGlobalMethod(___pdr_lua_State* L, const char* name, ___pdr_lua_CFunction func) {
		___pdr_lua_pushcfunction(L, func);
        ___pdr_lua_setglobal(L,name);
	}

	void LuaObject::addField(___pdr_lua_State* L, const char* name, ___pdr_lua_CFunction getter, ___pdr_lua_CFunction setter, bool isInstance) {
		___pdr_lua_getfield(L, isInstance ? -1 : -2, ".get");
		___pdr_lua_pushcfunction(L, getter);
		___pdr_lua_setfield(L, -2, name);
		___pdr_lua_pop(L, 1);
		___pdr_lua_getfield(L, isInstance ? -1 : -2, ".set");
		___pdr_lua_pushcfunction(L, setter);
		___pdr_lua_setfield(L, -2, name);
		___pdr_lua_pop(L, 1);
	}

	void LuaObject::addOperator(___pdr_lua_State* L, const char* name, ___pdr_lua_CFunction func) {
		___pdr_lua_pushcfunction(L, func);
		___pdr_lua_setfield(L, -2, name);
	}

	void LuaObject::finishType(___pdr_lua_State* L, const char* tn, ___pdr_lua_CFunction ctor, ___pdr_lua_CFunction gc, ___pdr_lua_CFunction strHint) {
        if(ctor) {
		    ___pdr_lua_pushcclosure(L, ctor, 0);
		    ___pdr_lua_setfield(L, -3, "__call");
        }
        if(gc) {
		    ___pdr_lua_pushcclosure(L, gc, 0); // t, mt, _instance, __gc
    		___pdr_lua_setfield(L, -2, "__gc"); // t, mt, _instance
        }
        if(strHint) {
            ___pdr_lua_pushcfunction(L, strHint);
            ___pdr_lua_setfield(L, -2, "__tostring");
        }
        ___pdr_lua_pop(L,3);
	}

	bool LuaObject::matchType(___pdr_lua_State* L, int p, const char* tn, bool noprefix) {
		AutoStack autoStack(L);
		if (!___pdr_lua_isuserdata(L, p)) {
			return false;
		}
		___pdr_lua_getmetatable(L, p);
		if (___pdr_lua_isnil(L, -1)) {
			return false;
		}
		___pdr_lua_getfield(L, -1, "__name");
		if (___pdr_lua_isnil(L, -1)) {
			return false;
		}
		auto name = ___pdr_luaL_checkstring(L, -1);
		// skip first prefix "F" or "U" or "A"
		if(noprefix) return strcmp(name+1, tn) == 0;
		else return strcmp(name,tn)==0;
	}

    LuaObject::PushPropertyFunction LuaObject::getPusher(UClass* cls) {
        auto it = pusherMap.Find(cls);
        if(it!=nullptr)
            return *it;
        return nullptr;
    }

    LuaObject::CheckPropertyFunction LuaObject::getChecker(UClass* cls) {
        auto it = checkerMap.Find(cls);
        if(it!=nullptr)
            return *it;
        return nullptr;
    }

    LuaObject::PushPropertyFunction LuaObject::getPusher(UProperty* prop) {
        return getPusher(prop->GetClass());
    }

    LuaObject::CheckPropertyFunction LuaObject::getChecker(UProperty* prop) {
        return getChecker(prop->GetClass());        
    }

    

    void regPusher(UClass* cls,LuaObject::PushPropertyFunction func) {
		pusherMap.Add(cls, func);
    }

    void regChecker(UClass* cls,LuaObject::CheckPropertyFunction func) {
		checkerMap.Add(cls, func);
    }

    int classConstruct(___pdr_lua_State* L) {
        UClass* cls = LuaObject::checkValue<UClass*>(L, 1);
		UObject* outter = LuaObject::checkValueOpt<UObject*>(L, 2, (UObject*)GetTransientPackage());
		if (cls && !outter->IsA(cls->ClassWithin)) {
			___pdr_luaL_error(L, "Can't create object in %s", TCHAR_TO_UTF8(*outter->GetClass()->GetName()));
		}
		FName name = LuaObject::checkValueOpt<FName>(L, 3, FName(NAME_None));
        if(cls) {
            UObject* obj = NewObject<UObject>(outter,cls,name);
            if(obj) {
                LuaObject::push(L,obj);
                return 1;
            }
        }
        return 0;
    }

    int searchExtensionMethod(___pdr_lua_State* L,UClass* cls,const char* name,bool isStatic=false) {

        // search class and its super
        TMap<FString,ExtensionField>* mapptr=nullptr;
        while(cls!=nullptr) {
            mapptr = isStatic?extensionMMap_static.Find(cls):extensionMMap.Find(cls);
            if(mapptr!=nullptr) {
                // find field
                auto fieldptr = mapptr->Find(name);
				if (fieldptr != nullptr) {
					// is function
					if (fieldptr->isFunction) {
						___pdr_lua_pushcfunction(L, fieldptr->func);
						return 1;
					} 
					// is property
					else {
						if (!fieldptr->getter) ___pdr_luaL_error(L, "Property %s is set only", name);
						___pdr_lua_pushcfunction(L, fieldptr->getter);
						if (!isStatic) {
							___pdr_lua_pushvalue(L, 1); // push self
							lua_call(L, 1, 1);
						} else 
							lua_call(L, 0, 1);
						return 1;
					}
				}
            }
            cls=cls->GetSuperClass();
        }   
        return 0;
    }

    int searchExtensionMethod(___pdr_lua_State* L,UObject* o,const char* name,bool isStatic=false) {
        auto cls = o->GetClass();
        return searchExtensionMethod(L,cls,name,isStatic);
    }

    int classIndex(___pdr_lua_State* L) {
        UClass* cls = LuaObject::checkValue<UClass*>(L, 1);
        const char* name = LuaObject::checkValue<const char*>(L, 2);
        // get blueprint member
        UFunction* func = cls->FindFunctionByName(UTF8_TO_TCHAR(name));
        if(func) return LuaObject::push(L,func,cls);
        return searchExtensionMethod(L,cls,name,true);
    }

    int structConstruct(___pdr_lua_State* L) {
        UScriptStruct* uss = LuaObject::checkValue<UScriptStruct*>(L, 1);
        if(uss) {
            uint32 size = uss->GetStructureSize() ? uss->GetStructureSize() : 1;
            
            uint8* buf = (uint8*)FMemory::Malloc(size);
            uss->InitializeStruct(buf);
            LuaStruct* ls=new LuaStruct(buf,size,uss);
            LuaObject::push(L,ls);
            return 1;
        }
        return 0;
    }

    int fillParamFromState(___pdr_lua_State* L,UProperty* prop,uint8* params,int i) {

        // if is out param, can accept nil
        uint64 propflag = prop->GetPropertyFlags();
        if(propflag&CPF_OutParm && ___pdr_lua_isnil(L,i))
            return prop->GetSize();

        auto checker = LuaObject::getChecker(prop);
        if(checker) {
            checker(L,prop,params,i);
            return prop->GetSize();
        }
        else {
            FString tn = prop->GetClass()->GetName();
            ___pdr_luaL_error(L,"unsupport param type %s at %d",TCHAR_TO_UTF8(*tn),i);
            return 0;
        }
        
    }

    void LuaObject::fillParam(___pdr_lua_State* L,int i,UFunction* func,uint8* params) {
		auto funcFlag = func->FunctionFlags;
        for(TFieldIterator<UProperty> it(func);it && (it->PropertyFlags&CPF_Parm);++it) {
            UProperty* prop = *it;
            uint64 propflag = prop->GetPropertyFlags();
			if (funcFlag & EFunctionFlags::FUNC_Native) {
				if ((propflag&CPF_ReturnParm))
					continue;
			}
			else if (IsRealOutParam(propflag))
				continue;

			if (prop->GetFName() == NAME_LatentInfo) {
				// bind a callback to the latent function
				___pdr_lua_State *mainThread = ___pdr_G(L)->mainthread;

				UPLatentDelegate *obj = LuaObject::getLatentDelegate(mainThread);
				int threadRef = obj->getThreadRef(L);
				FLatentActionInfo LatentActionInfo(threadRef, GetTypeHash(FGuid::NewGuid()), *UPLatentDelegate::NAME_LatentCallback, obj);

				prop->CopySingleValue(prop->ContainerPtrToValuePtr<void>(params), &LatentActionInfo);
			}
			else {
				fillParamFromState(L, prop, params + prop->GetOffset_ForInternal(), i);
				i++;
			}
        }
    }

	void LuaObject::callRpc(___pdr_lua_State* L, UObject* obj, UFunction* func, uint8* params) {
		// call rpc without outparams
		const bool bHasReturnParam = func->ReturnValueOffset != MAX_uint16;
		uint8* ReturnValueAddress = bHasReturnParam ? ((uint8*)params + func->ReturnValueOffset) : nullptr;
        
        #if ENGINE_MINOR_VERSION >= 23 && (PLATFORM_MAC || PLATFORM_IOS)
            FNewFrame NewStack(obj, func, params, NULL, func->Children);
        #else
            FFrame NewStack(obj, func, params, NULL, func->Children);
        #endif
        
        #if ENGINE_MINOR_VERSION < 25
            if (func->ReturnValueOffset != MAX_uint16) {
                UProperty* ReturnProperty = func->GetReturnProperty();
                if (ensure(ReturnProperty)) {
                    FOutParmRec* RetVal = (FOutParmRec*)FMemory_Alloca(sizeof(FOutParmRec));

                    /* Our context should be that we're in a variable assignment to the return value, so ensure that we have a valid property to return to */
                    RetVal->PropAddr = (uint8*)FMemory_Alloca(ReturnProperty->GetSize());
                    RetVal->Property = ReturnProperty;
                    NewStack.OutParms = RetVal;
                }
            }

            NewStack.Locals = params;
            FOutParmRec** LastOut = &NewStack.OutParms;

            for (UProperty* Property = (UProperty*)func->Children; Property!=nullptr; Property = (UProperty*)Property->Next){
                if (Property->PropertyFlags & CPF_OutParm){

                    CA_SUPPRESS(6263)
                        FOutParmRec* Out = (FOutParmRec*)FMemory_Alloca(sizeof(FOutParmRec));
                    // set the address and property in the out param info
                    // warning: Stack.MostRecentPropertyAddress could be NULL for optional out parameters
                    // if that's the case, we use the extra memory allocated for the out param in the function's locals
                    // so there's always a valid address
                    Out->PropAddr = Property->ContainerPtrToValuePtr<uint8>(NewStack.Locals);
                    Out->Property = Property;

                    // add the new out param info to the stack frame's linked list
                    if (*LastOut) {
                        (*LastOut)->NextOutParm = Out;
                        LastOut = &(*LastOut)->NextOutParm;
                    } else {
                        *LastOut = Out;
                    }
                } else {
                    // copy the result of the expression for this parameter into the appropriate part of the local variable space
                    uint8* Param = Property->ContainerPtrToValuePtr<uint8>(NewStack.Locals);
                    Property->InitializeValue_InContainer(NewStack.Locals);
                }
            }
        #else
            NewStack.OutParms = nullptr;
        #endif
    
        #if ENGINE_MINOR_VERSION >= 23 && (PLATFORM_MAC || PLATFORM_IOS)
            FFrame *frame = (FFrame *)&NewStack;
            func->Invoke(obj, *frame, ReturnValueAddress);
        #else
            func->Invoke(obj, NewStack, ReturnValueAddress);
        #endif
	}

	void LuaObject::callUFunction(___pdr_lua_State* L, UObject* obj, UFunction* func, uint8* params) {
		auto ff = func->FunctionFlags;
		// it's an RPC function
		if (ff&FUNC_Net)
			LuaObject::callRpc(L, obj, func, params);
		else
		// it's a local function
			obj->ProcessEvent(func, params);
	}

    // handle return value and out params
    int LuaObject::returnValue(___pdr_lua_State* L,UFunction* func,uint8* params) {

        // check is function has return value
		const bool bHasReturnParam = func->ReturnValueOffset != MAX_uint16;

		// put return value as head
        int ret = 0;
        if(bHasReturnParam) {
            UProperty* p = func->GetReturnProperty();
            ret += LuaObject::push(L,p,params+p->GetOffset_ForInternal());
        }

		bool isLatentFunction = false;
        // push out parms
        for(TFieldIterator<UProperty> it(func);it;++it) {
            UProperty* p = *it;
            uint64 propflag = p->GetPropertyFlags();
            // skip return param
            if(propflag&CPF_ReturnParm)
                continue;

			if (p->GetFName() == NAME_LatentInfo) {
				isLatentFunction = true;
			}
            else if(IsRealOutParam(propflag)) // out params should be not const and not readonly
                ret += LuaObject::push(L,p,params+p->GetOffset_ForInternal());
        }
        
		if (isLatentFunction) {
			return ___pdr_lua_yield(L, ret);
		}
		else {
			return ret;
		}
    }
   
    int ufuncClosure(___pdr_lua_State* L) {
        ___pdr_lua_pushvalue(L,___pdr_lua_upvalueindex(1));
        void* ud = ___pdr_lua_touserdata(L, -1);
        ___pdr_lua_pop(L, 1); // pop ud of func
        
        if(!ud) ___pdr_luaL_error(L, "Call ufunction error");

        ___pdr_lua_pushvalue(L,___pdr_lua_upvalueindex(2));
        UClass* cls = reinterpret_cast<UClass*>(___pdr_lua_touserdata(L, -1));
        ___pdr_lua_pop(L,1); // pop staticfunc flag
        
        UObject* obj;
        int offset=1;
        // use ClassDefaultObject if is static function call 
        if(cls) obj = cls->ClassDefaultObject;
        // use obj instance if is member function call
        // and offset set 2 to skip self
        else {
            obj = LuaObject::checkValue<UObject*>(L, 1);
            offset++;
        }
        
        UFunction* func = reinterpret_cast<UFunction*>(ud);
        
        uint8* params = (uint8*)FMemory_Alloca(func->ParmsSize);
        FMemory::Memzero(params, func->ParmsSize);
        for (TFieldIterator<UProperty> it(func); it && it->HasAnyPropertyFlags(CPF_Parm); ++it)
        {
            UProperty* localProp = *it;
            checkSlow(localProp);
            if (!localProp->HasAnyPropertyFlags(CPF_ZeroConstructor))
            {
                localProp->InitializeValue_InContainer(params);
            }
        }

        LuaObject::fillParam(L, offset, func, params);
		{
            obj->ProcessEvent(func, params);
		}

		// return value to push lua stack
        int outParamCount = LuaObject::returnValue(L, func, params);
        for (TFieldIterator<UProperty> it(func); it && it->HasAnyPropertyFlags(CPF_Parm); ++it)
        {
            it->DestroyValue_InContainer(params);
        }

        return outParamCount;
    }

    // find ufunction from cache
    UFunction* LuaObject::findCacheFunction(___pdr_lua_State* L, UClass* cls,const char* fname) {
        auto state = LuaState::get(L);
		return state->classMap.findFunc(cls, fname);
    }

    // cache ufunction for reuse
    void LuaObject::cacheFunction(___pdr_lua_State* L,UClass* cls,const char* fname,UFunction* func) {
        auto state = LuaState::get(L);
        state->classMap.cacheFunc(cls,fname,func);
    }

    UProperty* LuaObject::findCacheProperty(___pdr_lua_State* L, UClass* cls, const char* pname)
    {
		auto state = LuaState::get(L);
		return state->classMap.findProp(cls, pname);
    }

    void LuaObject::cacheProperty(___pdr_lua_State* L, UClass* cls, const char* pname, UProperty* property)
    {
		auto state = LuaState::get(L);
		state->classMap.cacheProp(cls, pname, property);
    }

	// cache class property's
	void cachePropertys(___pdr_lua_State* L, UClass* cls) {
		auto PropertyLink = cls->PropertyLink;
		for (UProperty* Property = PropertyLink; Property != NULL; Property = Property->PropertyLinkNext) {
			LuaObject::cacheProperty(L, cls, TCHAR_TO_UTF8(*(Property->GetName())), Property);
		}
	}
    int instanceIndex(___pdr_lua_State* L) {
        UObject* obj = LuaObject::checkValue<UObject*>(L, 1);
        const char* name = LuaObject::checkValue<const char*>(L, 2);

		UClass* cls = obj->GetClass();
        UProperty* up = LuaObject::findCacheProperty(L, cls, name);
        if (up)
        {
            return LuaObject::push(L, up, obj, false);
        }

        UFunction* func = LuaObject::findCacheFunction(L, cls, name);
        if (func)
        {
            return LuaObject::push(L, func);
        }

        // get blueprint member
		FName wname(UTF8_TO_TCHAR(name));
        func = cls->FindFunctionByName(wname);
        if(!func) {
			cachePropertys(L, cls);

			up = LuaObject::findCacheProperty(L, cls, name);
            if (up) {
                return LuaObject::push(L, up, obj, false);
            }
            
            // search extension method
            return searchExtensionMethod(L, obj, name);
        }
        else {   
			LuaObject::cacheFunction(L, cls, name, func);
            return LuaObject::push(L,func);
        }
    }

    int newinstanceIndex(___pdr_lua_State* L) {
        UObject* obj = LuaObject::checkValue<UObject*>(L, 1);
        const char* name = LuaObject::checkValue<const char*>(L, 2);
        UClass* cls = obj->GetClass();
		UProperty* up = LuaObject::findCacheProperty(L, cls, name);
		if (!up)
		{
			cachePropertys(L, cls);
			up = LuaObject::findCacheProperty(L, cls, name);
		}
		if (!up) ___pdr_luaL_error(L, "Property %s not found", name);
        if(up->GetPropertyFlags() & CPF_BlueprintReadOnly)
            ___pdr_luaL_error(L,"Property %s is readonly",name);

        auto checker = LuaObject::getChecker(up);
        if(!checker) ___pdr_luaL_error(L,"Property %s type is not support",name);
        // set property value
        checker(L,up,up->ContainerPtrToValuePtr<uint8>(obj),3);
        return 0;
    }

	UProperty* FindStructPropertyByName(UScriptStruct* scriptStruct, const char* name)
	{
		if (scriptStruct->IsNative())
		{
			return scriptStruct->FindPropertyByName(UTF8_TO_TCHAR(name));
		}

		FString propName = UTF8_TO_TCHAR(name);
		for (UProperty* Property = scriptStruct->PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext)
		{
			FString fieldName = Property->GetName();
			if (fieldName.StartsWith(propName, ESearchCase::CaseSensitive))
			{
				int index = fieldName.Len();
				for (int i = 0; i < 2; ++i)
				{
					int findIndex = fieldName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromEnd, index);
					if (findIndex != INDEX_NONE)
					{
						index = findIndex;
					}
				}
				if (propName.Len() == index)
				{
					return Property;
				}
			}
		}

		return nullptr;
	}

    int instanceStructIndex(___pdr_lua_State* L) {
        LuaStruct* ls = LuaObject::checkValue<LuaStruct*>(L, 1);
        const char* name = LuaObject::checkValue<const char*>(L, 2);
        
        auto* cls = ls->uss;
        UProperty* up = FindStructPropertyByName(cls, name);
        if(!up) return 0;
        return LuaObject::push(L,up,ls->buf+up->GetOffset_ForInternal(),false);
    }

    int newinstanceStructIndex(___pdr_lua_State* L) {
        LuaStruct* ls = LuaObject::checkValue<LuaStruct*>(L, 1);
        const char* name = LuaObject::checkValue<const char*>(L, 2);

        auto* cls = ls->uss;
        UProperty* up = FindStructPropertyByName(cls, name);
        if (!up) ___pdr_luaL_error(L, "Can't find property named %s", name);
        if (up->GetPropertyFlags() & CPF_BlueprintReadOnly)
            ___pdr_luaL_error(L, "Property %s is readonly", name);

        auto checker = LuaObject::getChecker(up);
        if(!checker) ___pdr_luaL_error(L,"Property %s type is not support",name);

        checker(L, up, ls->buf + up->GetOffset_ForInternal(), 3);
        return 0;
    }

    int instanceIndexSelf(___pdr_lua_State* L) {
        ___pdr_lua_getmetatable(L,1);
        const char* name = LuaObject::checkValue<const char*>(L, 2);
        
        ___pdr_lua_getfield(L,-1,name);
        ___pdr_lua_remove(L,-2); // remove mt of ud
        return 1;
    }

	int LuaObject::objectToString(___pdr_lua_State* L)
	{
        const int BufMax = 128;
        static char buffer[BufMax] = { 0 };
		UObject* obj = LuaObject::testudata<UObject>(L, 1);
		if (obj) {
			auto clsname = obj->GetClass()->GetFName().ToString();
			auto objname = obj->GetFName().ToString();
			snprintf(buffer, BufMax, "%s: %s %p",TCHAR_TO_UTF8(*clsname),TCHAR_TO_UTF8(*objname), obj);
		}
        else {
            // if ud isn't a uobject, get __name of metatable to cast it to string
            const void* ptr = ___pdr_lua_topointer(L,1);
            int tt = ___pdr_luaL_getmetafield(L,1,"__name");
            // should have __name field
            if(tt==___PDR_LUA_TSTRING) {
                const char* metaname = ___pdr_lua_tostring(L,-1);
                snprintf(buffer, BufMax, "%s: %p", metaname,ptr);
            }
            if(tt!=___PDR_LUA_TNIL)
                ___pdr_lua_pop(L,1);
        }

		___pdr_lua_pushstring(L, buffer);
		return 1;
	}

	void LuaObject::setupMetaTable(___pdr_lua_State* L, const char* tn, ___pdr_lua_CFunction setupmt, ___pdr_lua_CFunction gc)
	{
		if (___pdr_luaL_newmetatable(L, tn)) {
			if (setupmt)
				setupmt(L);
			if (gc) {
				___pdr_lua_pushcfunction(L, gc);
				___pdr_lua_setfield(L, -2, "__gc");
			}
		}
		___pdr_lua_setmetatable(L, -2);
	}

	void LuaObject::setupMetaTable(___pdr_lua_State* L, const char* tn, ___pdr_lua_CFunction setupmt, int gc)
	{
		if (___pdr_luaL_newmetatable(L, tn)) {
			if (setupmt)
				setupmt(L);
			if (gc) {
				___pdr_lua_pushvalue(L, gc);
				___pdr_lua_setfield(L, -2, "__gc");
			}
		}
		___pdr_lua_setmetatable(L, -2);
	}

	void LuaObject::setupMetaTable(___pdr_lua_State* L, const char* tn, ___pdr_lua_CFunction gc)
	{
		___pdr_luaL_getmetatable(L, tn);
		if (___pdr_lua_isnil(L, -1))
			___pdr_luaL_error(L, "Can't find type %s exported", tn);

		___pdr_lua_pushcfunction(L, gc);
		___pdr_lua_setfield(L, -2, "__gc");
		___pdr_lua_setmetatable(L, -2);
	}

    template<typename T>
    int pushUProperty(___pdr_lua_State* L,UProperty* prop,uint8* parms,bool ref) {
        auto p=Cast<T>(prop);
        ensure(p);
        return LuaObject::push(L,p->GetPropertyValue(parms));
    }

	int pushEnumProperty(___pdr_lua_State* L, UProperty* prop, uint8* parms,bool ref) {
		auto p = Cast<UEnumProperty>(prop);
		ensure(p);
		auto p2 = p->GetUnderlyingProperty();
		ensure(p2);
		int i = p2->GetSignedIntPropertyValue(parms);
		return LuaObject::push(L, i);
	}

    int pushUArrayProperty(___pdr_lua_State* L,UProperty* prop,uint8* parms,bool ref) {
        auto p = Cast<UArrayProperty>(prop);
        ensure(p);
        FScriptArray* v = p->GetPropertyValuePtr(parms);
		return LuaArray::push(L, p->Inner, v);
    }

    int pushUMapProperty(___pdr_lua_State* L,UProperty* prop,uint8* parms,bool ref) {
        auto p = Cast<UMapProperty>(prop);
        ensure(p);
		FScriptMap* v = p->GetPropertyValuePtr(parms);
		return LuaMap::push(L, p->KeyProp, p->ValueProp, v);
    }

	int pushUWeakProperty(___pdr_lua_State* L, UProperty* prop, uint8* parms,bool ref) {
		auto p = Cast<UWeakObjectProperty>(prop);
		ensure(p);
		FWeakObjectPtr v = p->GetPropertyValue(parms);
		return LuaObject::push(L, v);
	}

    int checkUArrayProperty(___pdr_lua_State* L,UProperty* prop,uint8* parms,int i) {
        auto p = Cast<UArrayProperty>(prop);
        ensure(p);
        CheckUD(LuaArray,L,i);
        LuaArray::clone((FScriptArray*)parms,p->Inner,UD->get());
        return 0;
    }

	int checkUMapProperty(___pdr_lua_State* L, UProperty* prop, uint8* parms, int i) {
		auto p = Cast<UMapProperty>(prop);
		ensure(p);
		CheckUD(LuaMap, L, i);
        LuaMap::clone((FScriptMap*)parms,p->KeyProp,p->ValueProp,UD->get());
		return 0;
	}

    int pushUStructProperty(___pdr_lua_State* L,UProperty* prop,uint8* parms,bool ref) {
        auto p = Cast<UStructProperty>(prop);
        ensure(p);
        auto uss = p->Struct;

		if (LuaWrapper::pushValue(L, p, uss, parms))
			return 1;

		if (uss->GetName() == "LuaBPVar") {
			((FPLuaBPVar*)parms)->value.push(L);
			return 1;
		}

		uint32 size = uss->GetStructureSize() ? uss->GetStructureSize() : 1;
		uint8* buf = (uint8*)FMemory::Malloc(size);
		uss->InitializeStruct(buf);
		uss->CopyScriptStruct(buf, parms);
		return LuaObject::push(L, new LuaStruct(buf,size,uss));
    }  

	int pushUDelegateProperty(___pdr_lua_State* L, UProperty* prop, uint8* parms, bool ref) {
		auto p = Cast<UDelegateProperty>(prop);
		ensure(p);
		FScriptDelegate* delegate = p->GetPropertyValuePtr(parms);
		return LuaDelegate::push(L, delegate, p->SignatureFunction, prop->GetNameCPP());
	}

    int pushUMulticastDelegateProperty(___pdr_lua_State* L,UProperty* prop,uint8* parms,bool ref) {
        auto p = Cast<UMulticastDelegateProperty>(prop);
        ensure(p);
#if (ENGINE_MINOR_VERSION>=23) && (ENGINE_MAJOR_VERSION>=4)
		FMulticastScriptDelegate* delegate = const_cast<FMulticastScriptDelegate*>(p->GetMulticastDelegate(parms));
#else
        FMulticastScriptDelegate* delegate = p->GetPropertyValuePtr(parms);
#endif
		return LuaMultiDelegate::push(L, delegate, p->SignatureFunction, prop->GetNameCPP());
    }

#if (ENGINE_MINOR_VERSION>=23) && (ENGINE_MAJOR_VERSION>=4)
	int pushUMulticastInlineDelegateProperty(___pdr_lua_State* L, UProperty* prop, uint8* parms, bool ref) {
		auto p = Cast<UMulticastInlineDelegateProperty>(prop);
		ensure(p);
		FMulticastScriptDelegate* delegate = const_cast<FMulticastScriptDelegate*>(p->GetMulticastDelegate(parms));
		return LuaMultiDelegate::push(L, delegate, p->SignatureFunction, prop->GetNameCPP());
	}
#endif

#if (ENGINE_MINOR_VERSION>=24) && (ENGINE_MAJOR_VERSION>=4)
	int pushUMulticastSparseDelegateProperty(___pdr_lua_State* L, UProperty* prop, uint8* parms, bool ref) {
		auto p = Cast<UMulticastSparseDelegateProperty>(prop);
		ensure(p);
		FMulticastScriptDelegate* delegate = const_cast<FMulticastScriptDelegate*>(p->GetMulticastDelegate(parms));
		return LuaMultiDelegate::push(L, delegate, p->SignatureFunction, prop->GetNameCPP());
	}
#endif

    int checkUDelegateProperty(___pdr_lua_State* L,UProperty* prop,uint8* parms,int i) {
        auto p = Cast<UDelegateProperty>(prop);
        ensure(p);
        CheckUD(UObject,L,i);
        // bind SignatureFunction
        if(auto dobj=Cast<UPLuaDelegate>(UD)) dobj->bindFunction(p->SignatureFunction);
        else ___pdr_luaL_error(L,"arg 1 expect an UDelegateObject");

        FScriptDelegate d;
        d.BindUFunction(UD, TEXT("EventTrigger"));

        p->SetPropertyValue(parms,d);
        return 0;
    }
	 
    int pushUObjectProperty(___pdr_lua_State* L,UProperty* prop,uint8* parms,bool ref) {
        auto p = Cast<UObjectProperty>(prop);
        ensure(p);   
        UObject* o = p->GetPropertyValue(parms);
        if(auto tr=Cast<UWidgetTree>(o))
            return LuaWidgetTree::push(L,tr);
        else
            return LuaObject::push(L,o,false,ref);
    }

    template<typename T>
    int checkUProperty(___pdr_lua_State* L,UProperty* prop,uint8* parms,int i) {
        auto p = Cast<T>(prop);
        ensure(p);
        p->SetPropertyValue(parms,LuaObject::checkValue<typename T::TCppType>(L,i));
        return 0;
    }

    template<>
	int checkUProperty<UEnumProperty>(___pdr_lua_State* L, UProperty* prop, uint8* parms, int i) {
		auto p = Cast<UEnumProperty>(prop);
		ensure(p);
		auto v = (int64)LuaObject::checkValue<int>(L, i);
		p->CopyCompleteValue(parms, &v);
		return 0;
	}

	template<>
	int checkUProperty<UObjectProperty>(___pdr_lua_State* L, UProperty* prop, uint8* parms, int i) {
		auto p = Cast<UObjectProperty>(prop);
		ensure(p);
		UObject* arg = LuaObject::checkValue<UObject*>(L, i);
		if (arg && arg->GetClass() != p->PropertyClass && !arg->GetClass()->IsChildOf(p->PropertyClass))
			___pdr_luaL_error(L, "arg %d expect %s, but got %s", i,
				p->PropertyClass ? TCHAR_TO_UTF8(*p->PropertyClass->GetName()) : "", 
				arg->GetClass() ? TCHAR_TO_UTF8(*arg->GetClass()->GetName()) : "");

		p->SetPropertyValue(parms, arg);
		return LuaObject::push(L, arg);
	}

    int checkUStructProperty(___pdr_lua_State* L,UProperty* prop,uint8* parms,int i) {
        auto p = Cast<UStructProperty>(prop);
        ensure(p);
        auto uss = p->Struct;

		// if it's LuaBPVar
		if (uss->GetName() == "LuaBPVar")
			return FPLuaBPVar::checkValue(L, p, parms, i);

		// skip first char to match type
		if (LuaObject::matchType(L, i, TCHAR_TO_UTF8(*uss->GetName()),true)) {
			if (LuaWrapper::checkValue(L, p, uss, parms, i))
				return 0;
		}

		LuaStruct* ls = LuaObject::checkValue<LuaStruct*>(L, i);
		if(!ls)
			___pdr_luaL_error(L, "expect struct but got nil");

		if (p->GetSize() != ls->size)
			___pdr_luaL_error(L, "expect struct size == %d, but got %d", p->GetSize(), ls->size);
		p->CopyCompleteValue(parms, ls->buf);
		return 0;
    }
	
	int pushUClassProperty(___pdr_lua_State* L, UProperty* prop, uint8* parms, bool ref) {
		auto p = Cast<UClassProperty>(prop);
		ensure(p);
		UClass* cls = Cast<UClass>(p->GetPropertyValue(parms));
		return LuaObject::pushClass(L, cls);
	}

	int checkUClassProperty(___pdr_lua_State* L, UProperty* prop, uint8* parms, int i) {
		auto p = Cast<UClassProperty>(prop);
		ensure(p);
		p->SetPropertyValue(parms, LuaObject::checkValue<UClass*>(L, i));
		return 0;
	}

	bool checkType(___pdr_lua_State* L, int p, const char* tn) {
		if (!___pdr_lua_isuserdata(L, p))
			return false;
		int tt = ___pdr_luaL_getmetafield(L, p, "__name");
		if (tt==___PDR_LUA_TSTRING && strcmp(tn, ___pdr_lua_tostring(L, -1)) == 0)
		{
			___pdr_lua_pop(L, 1);
			return true;
		}
		if(tt!=___PDR_LUA_TNIL)
            ___pdr_lua_pop(L, 1);
		return false;
	}

    // search obj from registry, push cached obj and return true if find it
    bool LuaObject::getFromCache(___pdr_lua_State* L,void* obj,const char* tn,bool check) {
        LuaState* ls = LuaState::get(L);
        ensure(ls->cacheObjRef!=___PDR_LUA_NOREF);
        ___pdr_lua_geti(L,___PDR_LUA_REGISTRYINDEX,ls->cacheObjRef);
        // should be a table
        ensure(___pdr_lua_type(L,-1)==___PDR_LUA_TTABLE);
        // push obj as key
        ___pdr_lua_pushlightuserdata(L,obj);
        // get key from table
        ___pdr_lua_rawget(L,-2);
        ___pdr_lua_remove(L,-2); // remove cache table
        
        if(___pdr_lua_isnil(L,-1)) {
            ___pdr_lua_pop(L,1);
			return false;
        }
		if (!check)
			return true;
		// check type of ud matched
		return checkType(L, -1, tn);
    }

    void LuaObject::addRef(___pdr_lua_State* L,UObject* obj,void* ud,bool ref) {
        auto sl = LuaState::get(L);
        sl->addRef(obj,ud,ref);
    }


    void LuaObject::removeRef(___pdr_lua_State* L,UObject* obj) {
        auto sl = LuaState::get(L);
        sl->unlinkUObject(obj);
    }

	void LuaObject::releaseLink(___pdr_lua_State* L, void* prop) {
		LuaState* ls = LuaState::get(L);
		ls->releaseLink(prop);
	}

	void LuaObject::linkProp(___pdr_lua_State* L, void* parent, void* prop) {
		LuaState* ls = LuaState::get(L);
		ls->linkProp(parent,prop);
	}

    void LuaObject::cacheObj(___pdr_lua_State* L,void* obj) {
        LuaState* ls = LuaState::get(L);
        ___pdr_lua_geti(L,___PDR_LUA_REGISTRYINDEX,ls->cacheObjRef);
        ___pdr_lua_pushlightuserdata(L,obj);
        ___pdr_lua_pushvalue(L,-3); // obj userdata
        ___pdr_lua_rawset(L,-3);
        ___pdr_lua_pop(L,1); // pop cache table        
    }

	void LuaObject::removeFromCache(___pdr_lua_State * L, void* obj)
	{
		// get cache table
		LuaState* ls = LuaState::get(L);
		___pdr_lua_geti(L, ___PDR_LUA_REGISTRYINDEX, ls->cacheObjRef);
		ensure(___pdr_lua_type(L, -1) == ___PDR_LUA_TTABLE);
		___pdr_lua_pushlightuserdata(L, obj);
		___pdr_lua_pushnil(L);
		// cache[obj] = nil
		___pdr_lua_rawset(L, -3);
		// pop cache table;
		___pdr_lua_pop(L, 1);
	}

	void LuaObject::deleteFGCObject(___pdr_lua_State* L, FGCObject * obj)
	{
		auto ls = LuaState::get(L);
		ensure(ls);
		ls->deferDelete.Add(obj);
	}
	
	UPLatentDelegate* LuaObject::getLatentDelegate(___pdr_lua_State* L)
	{
		LuaState* ls = LuaState::get(L);
		return ls->getLatentDelegate();
	}

	void LuaObject::createTable(___pdr_lua_State* L, const char * tn)
	{
		auto ls = LuaState::get(L);
		ensure(ls);
		LuaVar t = ls->createTable(tn);
		t.push(L);
	}


    int LuaObject::pushClass(___pdr_lua_State* L,UClass* cls) {
        if(!cls) {
            ___pdr_lua_pushnil(L);
            return 1;
        }
		return pushGCObject<UClass*>(L, cls, "UClass", setupClassMT, gcClass, true);
    }

    int LuaObject::pushStruct(___pdr_lua_State* L,UScriptStruct* cls) {
        if(!cls) {
            ___pdr_lua_pushnil(L);
            return 1;
        }    
        return pushGCObject<UScriptStruct*>(L,cls,"UScriptStruct",setupStructMT,gcStructClass,true);
    }

	int LuaObject::pushEnum(___pdr_lua_State * L, UEnum * e)
	{
		bool isbpEnum = Cast<UUserDefinedEnum>(e) != nullptr;
		// return a enum as table
		___pdr_lua_newtable(L);
		int num = e->NumEnums();
		for (int i = 0; i < num; i++) {
			FString name;
			// if is bp enum, can't get name as key
			if(isbpEnum)
				name = e->GetDisplayNameTextByIndex(i).ToString();
			else
				name = e->GetNameStringByIndex(i);
			int64 value = e->GetValueByIndex(i);
			___pdr_lua_pushinteger(L, value);
			___pdr_lua_setfield(L, -2, TCHAR_TO_UTF8(*name));
		}
		return 1;
	}

    int LuaObject::gcObject(___pdr_lua_State* L) {
		CheckUDGC(UObject,L,1);
        removeRef(L,UD);
        return 0;
    }

    int LuaObject::gcClass(___pdr_lua_State* L) {
		CheckUDGC(UClass,L,1);
        removeRef(L,UD);
        return 0;
    }

    int LuaObject::gcStructClass(___pdr_lua_State* L) {
		CheckUDGC(UScriptStruct,L,1);
        removeRef(L,UD);
        return 0;
    }

	int LuaObject::gcStruct(___pdr_lua_State* L) {
		CheckUDGC(LuaStruct, L, 1);
		deleteFGCObject(L,UD);
		return 0;
	}

    int LuaObject::push(___pdr_lua_State* L, UObject* obj, bool rawpush, bool ref) {
		if (!obj) return pushNil(L);
		if (!rawpush) {
			if (auto it = Cast<IPLuaTableObjectInterface>(obj)) {
				return IPLuaTableObjectInterface::push(L, it);
			}
		}
		if (auto e = Cast<UEnum>(obj))
			return pushEnum(L, e);
		else if (auto c = Cast<UClass>(obj))
			return pushClass(L, c);
		else if (auto s = Cast<UScriptStruct>(obj))
			return pushStruct(L, s);
		else
			return pushGCObject<UObject*>(L,obj,"UObject",setupInstanceMT,gcObject,ref);
    }

	int LuaObject::push(___pdr_lua_State* L, FWeakObjectPtr ptr) {
		if (!ptr.IsValid()) {
			___pdr_lua_pushnil(L);
			return 1;
		}
		UObject* obj = ptr.Get();
		if (getFromCache(L, obj, "UObject")) return 1;
		int r = pushWeakType(L, new WeakUObjectUD(ptr));
		if (r) cacheObj(L, obj);
		return r;
	}
    
    template<typename T>
    inline void regPusher() {
		pusherMap.Add(T::StaticClass(), pushUProperty<T>);
    }

    template<typename T>
    inline void regChecker() {
		checkerMap.Add(T::StaticClass(), checkUProperty<T>);
    }

    void LuaObject::init(___pdr_lua_State* L) {
		regPusher<UIntProperty>();
		regPusher<UUInt32Property>();
        regPusher<UInt64Property>();
        regPusher<UUInt64Property>();
		regPusher<UInt16Property>();
		regPusher<UUInt16Property>();
		regPusher<UInt8Property>();
		regPusher<UByteProperty>(); // uint8
		regPusher<UFloatProperty>();
		regPusher<UDoubleProperty>();
        regPusher<UBoolProperty>();
        regPusher<UTextProperty>();
        regPusher<UStrProperty>();
        regPusher<UNameProperty>();
		
		regPusher(UDelegateProperty::StaticClass(), pushUDelegateProperty);
        regPusher(UMulticastDelegateProperty::StaticClass(),pushUMulticastDelegateProperty);
#if (ENGINE_MINOR_VERSION>=23) && (ENGINE_MAJOR_VERSION>=4)
		regPusher(UMulticastInlineDelegateProperty::StaticClass(), pushUMulticastInlineDelegateProperty);
#endif
#if (ENGINE_MINOR_VERSION>=24) && (ENGINE_MAJOR_VERSION>=4)
		regPusher(UMulticastSparseDelegateProperty::StaticClass(), pushUMulticastSparseDelegateProperty);
#endif
        regPusher(UObjectProperty::StaticClass(),pushUObjectProperty);
        regPusher(UArrayProperty::StaticClass(),pushUArrayProperty);
        regPusher(UMapProperty::StaticClass(),pushUMapProperty);
        regPusher(UStructProperty::StaticClass(),pushUStructProperty);
		regPusher(UEnumProperty::StaticClass(), pushEnumProperty);
		regPusher(UClassProperty::StaticClass(), pushUClassProperty);
		regPusher(UWeakObjectProperty::StaticClass(), pushUWeakProperty);
		
		regChecker<UIntProperty>();
		regChecker<UUInt32Property>();
        regChecker<UInt64Property>();
        regChecker<UUInt64Property>();
		regChecker<UInt16Property>();
		regChecker<UUInt16Property>();
		regChecker<UInt8Property>();
		regChecker<UByteProperty>(); // uint8
		regChecker<UFloatProperty>();
		regChecker<UDoubleProperty>();
		regChecker<UBoolProperty>();
        regChecker<UNameProperty>();
        regChecker<UTextProperty>();
		regChecker<UObjectProperty>();
        regChecker<UStrProperty>();
        regChecker<UEnumProperty>();

        regChecker(UArrayProperty::StaticClass(),checkUArrayProperty);
        regChecker(UMapProperty::StaticClass(),checkUMapProperty);
        regChecker(UDelegateProperty::StaticClass(),checkUDelegateProperty);
        regChecker(UStructProperty::StaticClass(),checkUStructProperty);
		regChecker(UClassProperty::StaticClass(), checkUClassProperty);
		
		LuaWrapper::init(L);
        ExtensionMethod::init();
    }

    int LuaObject::push(___pdr_lua_State* L,UFunction* func,UClass* cls)  {
        ___pdr_lua_pushlightuserdata(L, func);
        if(cls) {
            ___pdr_lua_pushlightuserdata(L, cls);
            ___pdr_lua_pushcclosure(L, ufuncClosure, 2);
        }
        else
            ___pdr_lua_pushcclosure(L, ufuncClosure, 1);
        return 1;
    }

    int LuaObject::push(___pdr_lua_State* L,UProperty* prop,uint8* parms,bool ref) {
        auto pusher = getPusher(prop);
        if (pusher)
            return pusher(L,prop,parms,ref);
        else {
            FString name = prop->GetClass()->GetName();
            Log::Error("unsupport type %s to push",TCHAR_TO_UTF8(*name));
            return 0;
        }
    }

	int LuaObject::push(___pdr_lua_State* L, UProperty* up, UObject* obj, bool ref) {
		auto cls = up->GetClass();
		// if it's an UArrayProperty
		if (cls==UArrayProperty::StaticClass())
			return LuaArray::push(L, Cast<UArrayProperty>(up), obj);
        // if it's an UMapProperty
        else if(cls==UMapProperty::StaticClass())
            return LuaMap::push(L,Cast<UMapProperty>(up),obj);
		else
			return push(L, up, up->ContainerPtrToValuePtr<uint8>(obj), ref);
	}

	int LuaObject::push(___pdr_lua_State* L, LuaStruct* ls) {
		return pushType<LuaStruct*>(L, ls, "LuaStruct", setupInstanceStructMT, gcStruct);
	}

	int LuaObject::push(___pdr_lua_State* L, double v) {
		___pdr_lua_pushnumber(L, v);
		return 1;
	}

	int LuaObject::push(___pdr_lua_State* L, float v) {
		___pdr_lua_pushnumber(L, v);
		return 1;
	}

    int LuaObject::push(___pdr_lua_State* L, int64 v) {
		___pdr_lua_pushinteger(L, v);
		return 1;
	}

    int LuaObject::push(___pdr_lua_State* L, uint64 v) {
		___pdr_lua_pushinteger(L, v);
		return 1;
	}

	int LuaObject::push(___pdr_lua_State* L, int v) {
		___pdr_lua_pushinteger(L, v);
		return 1;
	}

    int LuaObject::push(___pdr_lua_State* L, bool v) {
		___pdr_lua_pushboolean(L, v);
		return 1;
	}

	int LuaObject::push(___pdr_lua_State* L, uint32 v) {
		___pdr_lua_pushinteger(L, v);
		return 1;
	}

    int LuaObject::push(___pdr_lua_State* L, int16 v) {
		___pdr_lua_pushinteger(L, v);
		return 1;
	}

    int LuaObject::push(___pdr_lua_State* L, uint16 v) {
		___pdr_lua_pushinteger(L, v);
		return 1;
	}

    int LuaObject::push(___pdr_lua_State* L, int8 v) {
		___pdr_lua_pushinteger(L, v);
		return 1;
	}

    int LuaObject::push(___pdr_lua_State* L, uint8 v) {
		___pdr_lua_pushinteger(L, v);
		return 1;
	}

    int LuaObject::push(___pdr_lua_State* L, const LuaVar& v) {
        return v.push(L);
	}

    int LuaObject::push(___pdr_lua_State* L, void* ptr) {
		___pdr_lua_pushlightuserdata(L,ptr);
		return 1;
	}

	int LuaObject::push(___pdr_lua_State* L, const FText& v) {
		FString str = v.ToString();
		___pdr_lua_pushstring(L, TCHAR_TO_UTF8(*str));
		return 1;
	}

	int LuaObject::push(___pdr_lua_State* L, const FString& str) {
		___pdr_lua_pushstring(L, TCHAR_TO_UTF8(*str));
		return 1;
	}

    int LuaObject::push(___pdr_lua_State* L, const FName& name) {
		___pdr_lua_pushstring(L, TCHAR_TO_UTF8(*name.ToString()));
		return 1;
	}

    int LuaObject::push(___pdr_lua_State* L, const char* str) {
		___pdr_lua_pushstring(L, str);
		return 1;
	}

    int LuaObject::setupMTSelfSearch(___pdr_lua_State* L) {
        ___pdr_lua_pushcfunction(L,instanceIndexSelf);
		___pdr_lua_setfield(L, -2, "__index");
		___pdr_lua_pushcfunction(L, objectToString);
		___pdr_lua_setfield(L, -2, "__tostring");
        return 0;
    }


    int LuaObject::setupClassMT(___pdr_lua_State* L) {
        ___pdr_lua_pushcfunction(L,classConstruct);
        ___pdr_lua_setfield(L, -2, "__call");
        ___pdr_lua_pushcfunction(L,NS_PDR_SLUA::classIndex);
		___pdr_lua_setfield(L, -2, "__index");
		___pdr_lua_pushcfunction(L, objectToString);
		___pdr_lua_setfield(L, -2, "__tostring");
        return 0;
    }

    int LuaObject::setupStructMT(___pdr_lua_State* L) {
        ___pdr_lua_pushcfunction(L,structConstruct);
		___pdr_lua_setfield(L, -2, "__call");
		___pdr_lua_pushcfunction(L, objectToString);
		___pdr_lua_setfield(L, -2, "__tostring");
        return 0;
    }

    int LuaObject::setupInstanceMT(___pdr_lua_State* L) {
        ___pdr_lua_pushcfunction(L,instanceIndex);
        ___pdr_lua_setfield(L, -2, "__index");
        ___pdr_lua_pushcfunction(L,newinstanceIndex);
        ___pdr_lua_setfield(L, -2, "__newindex");
		___pdr_lua_pushcfunction(L, objectToString);
		___pdr_lua_setfield(L, -2, "__tostring");
        return 0;
    }

    int LuaObject::setupInstanceStructMT(___pdr_lua_State* L) {
        ___pdr_lua_pushcfunction(L,instanceStructIndex);
        ___pdr_lua_setfield(L, -2, "__index");
        ___pdr_lua_pushcfunction(L, newinstanceStructIndex);
		___pdr_lua_setfield(L, -2, "__newindex");
		___pdr_lua_pushcfunction(L, objectToString);
		___pdr_lua_setfield(L, -2, "__tostring");
        return 0;
    }
}

#ifdef _WIN32
#pragma warning (pop)
#endif
