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
#define ___PDR_LUA_LIB
#include "CoreMinimal.h"
#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif
#include "lua/lua.hpp"
#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif
#include "Misc/CoreMisc.h"
#include "UObject/UnrealType.h"
#include "UObject/WeakObjectPtr.h"
#include "Blueprint/UserWidget.h"
#include "PDRSluaUtil.h"
#include "PDRLuaArray.h"
#include "PDRLuaMap.h"
#include "Runtime/Launch/Resources/Version.h"

#ifndef SLUA_CPPINST
#define SLUA_CPPINST "__cppinst"
#endif 

// checkUD will report error if ud had been freed
#define CheckUD(Type,L,P) auto UD = LuaObject::checkUD<Type>(L,P);
// UD may be freed by Engine, so skip it in gc phase
#define CheckUDGC(Type,L,P) auto UD = LuaObject::checkUD<Type>(L,P,false); \
	if(!UD) return 0;

#define RegMetaMethodByName(L,NAME,METHOD) \
    ___pdr_lua_pushcfunction(L,METHOD); \
    ___pdr_lua_setfield(L,-2,NAME);

#define RegMetaMethod(L,METHOD) RegMetaMethodByName(L,#METHOD,METHOD)

#define NewUD(T, v, f) auto ud = ___pdr_lua_newuserdata(L, sizeof(UserData<T*>)); \
	if (!ud) ___pdr_luaL_error(L, "out of memory to new ud"); \
	auto udptr = reinterpret_cast< UserData<T*>* >(ud); \
	udptr->parent = nullptr; \
	udptr->ud = const_cast<T*>(v); \
    udptr->flag = f;

#define CheckSelf(T) \
	auto udptr = reinterpret_cast<UserData<T*>*>(___pdr_lua_touserdata(L, 1)); \
	if(!udptr) ___pdr_luaL_error(L, "self ptr missing"); \
	if (udptr->flag & UD_HADFREE) \
		___pdr_luaL_error(L, "checkValue error, obj parent has been freed"); \
	auto self = udptr->ud

#define IsRealOutParam(propflag) ((propflag&CPF_OutParm) && !(propflag&CPF_ConstParm) && !(propflag&CPF_BlueprintReadOnly))

class UPLatentDelegate;

namespace NS_PDR_SLUA {

    class LuaVar;

    struct AutoStack {
        AutoStack(___pdr_lua_State* l) {
            this->L = l;
            this->top = ___pdr_lua_gettop(L);
        }
        ~AutoStack() {
			___pdr_lua_settop(this->L,this->top);
        }
		___pdr_lua_State* L;
        int top;
    };

    struct PANDORA_API LuaStruct : public FGCObject {
        uint8* buf;
        uint32 size;
        UScriptStruct* uss;

        LuaStruct(uint8* buf,uint32 size,UScriptStruct* uss);
        ~LuaStruct();

		virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

#if (ENGINE_MINOR_VERSION>=20) && (ENGINE_MAJOR_VERSION>=4)
		virtual FString GetReferencerName() const override
		{
			return "LuaStruct";
		}
#endif
    };

		
	#define UD_NOFLAG 0
	#define UD_AUTOGC 1 // flag userdata should call __gc and maintain by lua
	#define UD_HADFREE 1<<2 // flag userdata had been freed
	#define UD_SHAREDPTR 1<<3 // it's a TSharedPtr in userdata instead of raw pointer
	#define UD_SHAREDREF 1<<4 // it's a TSharedRef in userdata instead of raw pointer
	#define UD_THREADSAFEPTR 1<<5 // it's a TSharedptr with thread-safe mode in userdata instead of raw pointer
	#define UD_UOBJECT 1<<6 // flag it's an UObject
	#define UD_USTRUCT 1<<7 // flag it's an UStruct
	#define UD_WEAKUPTR 1<<8 // flag it's a weak UObject ptr
	#define UD_REFERENCE 1<<9

	struct UDBase {
		uint32 flag;
		void* parent;
	};

	// Memory layout of GenericUserData and UserData should be same
	struct GenericUserData : public UDBase {
		void* ud;
	};

	template<class T>
	struct UserData : public UDBase {
		static_assert(sizeof(T) == sizeof(void*), "Userdata type should size equal to sizeof(void*)");
		T ud; 
	};

    DefTypeName(LuaArray);
    DefTypeName(LuaMap);

    template<typename T>
    struct LuaOwnedPtr {
        T* ptr;
        LuaOwnedPtr(T* p):ptr(p) {}
		T* operator->() const {
			return ptr;
		}
    };

	template<typename T,ESPMode mode>
	struct SharedRefUD {
	private:
		TSharedRef<T, mode> ref;
	public:
		typedef T type;
		SharedRefUD(const TSharedRef<T, mode>& other) :ref(other) {}
		
		T* get(___pdr_lua_State *L) {
			return &ref.Get();
		}
	};

	template<typename T, ESPMode mode>
	struct SharedPtrUD {
	private:
		TSharedPtr<T, mode> ptr;
	public:
		typedef T type;
		SharedPtrUD(const TSharedPtr<T, mode>& other) :ptr(other) {}

		T* get(___pdr_lua_State *L) {
			return ptr.Get();
		}
	};

	struct WeakUObjectUD {
		FWeakObjectPtr ud;
		WeakUObjectUD(FWeakObjectPtr ptr):ud(ptr) {

		}

		bool isValid() {
			return ud.IsValid();
		}

		UObject* get() {
			if(isValid()) return ud.Get();
			else return nullptr;
		}
	};

    class PANDORA_API LuaObject
    {
    private:

		#define CHECK_UD_VALID(ptr) if (ptr && ptr->flag&UD_HADFREE) { \
		if (checkfree) \
			___pdr_luaL_error(L, "arg %d had been freed(%p), can't be used", ___pdr_lua_absindex(L, p), ptr->ud); \
		else \
			return nullptr; \
		}

		template<typename T>
		static T* maybeAnUDTable(___pdr_lua_State* L, int p,bool checkfree) {
			if(___pdr_lua_istable(L, p)) {
                AutoStack as(L);
				___pdr_lua_getfield(L, p, SLUA_CPPINST);
				if (___pdr_lua_type(L, -1) == ___PDR_LUA_TUSERDATA)
					return checkUD<T>(L, ___pdr_lua_absindex(L, -1), checkfree);
			}
			return nullptr;
		}

        // testudata, if T is base of uobject but isn't uobject, try to  cast it to T
        template<typename T>
        static typename std::enable_if<std::is_base_of<UObject,T>::value && !std::is_same<UObject,T>::value, T*>::type 
		testudata(___pdr_lua_State* L,int p, bool checkfree=true) {
            UserData<UObject*>* ptr = (UserData<UObject*>*)___pdr_luaL_testudata(L,p,"UObject");
			CHECK_UD_VALID(ptr);
			T* t = nullptr;
			// if it's a weak UObject, rawget it
			if (ptr && ptr->flag&UD_WEAKUPTR) {
				auto wptr = (UserData<WeakUObjectUD*>*)ptr;
				t = Cast<T>(wptr->ud->get());
			}
			else t = ptr?Cast<T>(ptr->ud):nullptr;

			if (!t && ___pdr_lua_isuserdata(L, p)) {
				___pdr_luaL_getmetafield(L, p, "__name");
				if (___pdr_lua_isnil(L, -1)) {
					return t;
				}
				FString clsname(___pdr_lua_tostring(L, -1));
				___pdr_lua_pop(L, 1);
				// skip first char may be 'U' or 'A'
				if (clsname.Find(T::StaticClass()->GetName()) == 1) {
					UserData<T*>* tptr = (UserData<T*>*) ___pdr_lua_touserdata(L, p);
					t = tptr ? tptr->ud : nullptr;
				}
			}
			else if (!t)
				return maybeAnUDTable<T>(L, p,checkfree);

			// check UObject is valid
			if (isUObjectValid(t)) return t;
            return nullptr;
        }

        // testudata, if T is uobject
        template<typename T>
        static typename std::enable_if<std::is_same<UObject,T>::value, T*>::type 
		testudata(___pdr_lua_State* L,int p, bool checkfree=true) {
            auto ptr = (UserData<T*>*)___pdr_luaL_testudata(L,p,"UObject");
			CHECK_UD_VALID(ptr);
			if (!ptr) return maybeAnUDTable<T>(L, p, checkfree);
			// if it's a weak UObject ptr
			if (ptr->flag&UD_WEAKUPTR) {
				auto wptr = (UserData<WeakUObjectUD*>*)ptr;
				return wptr->ud->get();
			}
			else if (isUObjectValid(ptr->ud) || !checkfree)
				return ptr->ud;
			return nullptr;
        }

		template<class T>
		static typename std::enable_if<!std::is_base_of<TSharedFromThis<T>,T>::value, T*>::type
		unboxSharedUD(___pdr_lua_State* L, UserData<T*>* ptr) {
			// ptr may be a ThreadSafe sharedref or NotThreadSafe sharedref
			// but we don't care about it, we just Get raw ptr from it
			static_assert(sizeof(SharedPtrUD<T, ESPMode::NotThreadSafe>) == sizeof(SharedPtrUD<T, ESPMode::ThreadSafe>), "unexpected static assert");
			auto sptr = (UserData<SharedPtrUD<T, ESPMode::NotThreadSafe>*>*)ptr;
			// unbox shared ptr to rawptr
			return sptr->ud->get(L);
		}

		template<class T>
		static typename std::enable_if<std::is_base_of<TSharedFromThis<T>, T>::value, T*>::type
		unboxSharedUD(___pdr_lua_State* L, UserData<T*>* ptr) {
			// never run here
			ensureMsgf(false, TEXT("You cannot use a TSharedPtr of one mode with a type which inherits TSharedFromThis of another mode."));
			return nullptr;
		}

		
		template<class T>
		static T* unboxSharedUDRef(___pdr_lua_State* L, UserData<T*>* ptr) {
			// ptr may be a ThreadSafe sharedref or NotThreadSafe sharedref
			// but we don't care about it, we just Get raw ptr from it
			static_assert(sizeof(SharedRefUD<T, ESPMode::NotThreadSafe>) == sizeof(SharedRefUD<T, ESPMode::ThreadSafe>), "unexpected static assert");
			auto sptr = (UserData<SharedRefUD<T, ESPMode::NotThreadSafe>*>*)ptr;
			// unbox shared ptr to rawptr
			return sptr->ud->get(L);
		}

        // testudata, if T isn't uobject
        template<typename T>
        static typename std::enable_if<!std::is_base_of<UObject,T>::value && !std::is_same<UObject,T>::value, T*>::type 
		testudata(___pdr_lua_State* L,int p,bool checkfree=true) {
            auto ptr = (UserData<T*>*)___pdr_luaL_testudata(L,p,TypeName<T>::value().c_str());
			CHECK_UD_VALID(ptr);
			// ptr is boxed shared ptr?
			if (ptr) {
				if (ptr->flag&UD_SHAREDPTR) {
					return unboxSharedUD<T>(L,ptr);
				}
				else if (ptr->flag&UD_SHAREDREF) {
					return unboxSharedUDRef<T>(L,ptr);
				}
			}
			if (!ptr) return maybeAnUDTable<T>(L, p, checkfree);
            return ptr?ptr->ud:nullptr;
        }

    public:

        typedef int (*PushPropertyFunction)(___pdr_lua_State* L,UProperty* prop,uint8* parms,bool ref);
        typedef int (*CheckPropertyFunction)(___pdr_lua_State* L,UProperty* prop,uint8* parms,int i);

        static CheckPropertyFunction getChecker(UClass* prop);
        static PushPropertyFunction getPusher(UProperty* prop);
        static CheckPropertyFunction getChecker(UProperty* cls);
        static PushPropertyFunction getPusher(UClass* cls);

		static bool matchType(___pdr_lua_State* L, int p, const char* tn, bool noprefix=false);

		static int classIndex(___pdr_lua_State* L);
		static int classNewindex(___pdr_lua_State* L);

		static void newType(___pdr_lua_State* L, const char* tn);
        static void newTypeWithBase(___pdr_lua_State* L, const char* tn, std::initializer_list<const char*> bases);
		static void addMethod(___pdr_lua_State* L, const char* name, ___pdr_lua_CFunction func, bool isInstance = true);
		static void addGlobalMethod(___pdr_lua_State* L, const char* name, ___pdr_lua_CFunction func);
		static void addField(___pdr_lua_State* L, const char* name, ___pdr_lua_CFunction getter, ___pdr_lua_CFunction setter, bool isInstance = true);
		static void addOperator(___pdr_lua_State* L, const char* name, ___pdr_lua_CFunction func);
		static void finishType(___pdr_lua_State* L, const char* tn, ___pdr_lua_CFunction ctor, ___pdr_lua_CFunction gc, ___pdr_lua_CFunction strHint=nullptr);
		static void fillParam(___pdr_lua_State* L, int i, UFunction* func, uint8* params);
		static int returnValue(___pdr_lua_State* L, UFunction* func, uint8* params);

		// check UObject is valid
		static bool isUObjectValid(UObject* obj) {
			return obj && !obj->IsUnreachable() && !obj->IsPendingKill();
		}
		
		static void callUFunction(___pdr_lua_State* L, UObject* obj, UFunction* func, uint8* params);
		// create new enum type to lua, see DefEnum macro
		template<class T>
		static void newEnum(___pdr_lua_State* L, const char* tn, const char* keys, std::initializer_list<T> values) {
			FString fkey(keys);
			// remove namespace prefix
			fkey.ReplaceInline(*FString::Format(TEXT("{0}::"), { UTF8_TO_TCHAR(tn) }), TEXT(""));
			// remove space
			fkey.ReplaceInline(TEXT(" "),TEXT(""));
			// create enum table
			createTable(L, tn);

			FString key, right;
			for (size_t i = 0; i < values.size() && strSplit(fkey, ",", &key, &right); ++i) {
				___pdr_lua_pushinteger(L, (int)(*(values.begin() + i)));
				___pdr_lua_setfield(L, -2, TCHAR_TO_UTF8(*key));
				fkey = right;
			}
			// pop t
			___pdr_lua_pop(L, 1);
		}
        static void init(___pdr_lua_State* L);

        // check arg at p is exported lua class named __name in field 
        // of metable of the class, if T is base of class or class is T, 
        // return the pointer of class, otherwise return nullptr
        template<typename T>
        static T* checkUD(___pdr_lua_State* L,int p,bool checkfree=true) {
			if (___pdr_lua_isnil(L, p))
			{
				return nullptr;
			}

            T* ret = testudata<T>(L,p, checkfree);
            if(ret) return ret;

            const char *typearg = nullptr;
            int tt = ___pdr_luaL_getmetafield(L, p, "__name");
            if (tt == ___PDR_LUA_TSTRING)
                typearg = ___pdr_lua_tostring(L, -1);

            if(tt!=___PDR_LUA_TNIL) ___pdr_lua_pop(L,1);

            if(checkfree && !typearg)
				___pdr_luaL_error(L,"expect userdata at %d, if you passed an UObject, maybe it's unreachable",p);

			if (LuaObject::isBaseTypeOf(L, typearg, TypeName<T>::value().c_str())) {
				UserData<T*> *udptr = reinterpret_cast<UserData<T*>*>(___pdr_lua_touserdata(L, p));
				CHECK_UD_VALID(udptr);
				return udptr->ud;
			}
            if(checkfree) 
				___pdr_luaL_error(L,"expect userdata %s, but got %s, if you passed an UObject, maybe it's unreachable",TypeName<T>::value().c_str(),typearg);
            return nullptr;
        }

		template<class T>
		static T checkValueOpt(___pdr_lua_State* L, int p, const T& defaultValue=T()) {
			if (!typeMatched<T>(___pdr_lua_type(L,p))) {
				return defaultValue;
			} else {
				return checkValue<T>(L, p);
			}
		}

		template<class T>
		static typename std::enable_if< std::is_pointer<T>::value,T >::type checkReturn(___pdr_lua_State* L, int p) {
			UserData<T> *udptr = reinterpret_cast<UserData<T>*>(___pdr_lua_touserdata(L, p));
			if (udptr->flag & UD_HADFREE)
				___pdr_luaL_error(L, "checkValue error, obj parent has been freed");
			// if it's an UStruct, check struct type name is matched
			if (udptr->flag & UD_USTRUCT) {
				UserData<LuaStruct*> *structptr = reinterpret_cast<UserData<LuaStruct*>*>(udptr);
				LuaStruct* ls = structptr->ud;
				// skip first prefix like 'F','U','A'
				if (sizeof(typename std::remove_pointer<T>::type) == ls->size && strcmp(TypeName<T>::value().c_str()+1, TCHAR_TO_UTF8(*ls->uss->GetName())) == 0)
					return (T)(ls->buf);
				else
					___pdr_luaL_error(L, "checkValue error, type dismatched, expect %s", TypeName<T>::value().c_str());
			}
			return udptr->ud;
		}

		// if T isn't pointer, we should assume UserData box a pointer and dereference it to return
		template<class T>
		static typename std::enable_if< !std::is_pointer<T>::value,T >::type checkReturn(___pdr_lua_State* L, int p) {
			UserData<T*> *udptr = reinterpret_cast<UserData<T*>*>(___pdr_lua_touserdata(L, p));
			if (udptr->flag & UD_HADFREE)
				___pdr_luaL_error(L, "checkValue error, obj parent has been freed");
			// if it's an UStruct, check struct type name is matched
			if (udptr->flag & UD_USTRUCT) {
				UserData<LuaStruct*> *structptr = reinterpret_cast<UserData<LuaStruct*>*>(udptr);
				LuaStruct* ls = structptr->ud;
				// skip first prefix like 'F','U','A'
				if (sizeof(T) == ls->size && strcmp(TypeName<T>::value().c_str() + 1, TCHAR_TO_UTF8(*ls->uss->GetName())) == 0)
					return *((T*)(ls->buf));
				else
					___pdr_luaL_error(L, "checkValue error, type dismatched, expect %s", TypeName<T>::value().c_str());
			}
			return *(udptr->ud);
		}
        
        template<class T>
		static T checkValue(___pdr_lua_State* L, int p) {
            if(std::is_pointer<T>::value && ___pdr_lua_isnil(L,p))
                return T();

            using TT = typename remove_cr<T>::type;
            if(std::is_class<TT>::value && std::is_default_constructible<TT>::value && ___pdr_lua_isnil(L,p))
                return TT();

			static_assert(!std::is_same<wchar_t*, typename remove_ptr_const<T>::type>::value,
				"checkValue does not support parameter const TCHAR*, use FString instead");

			if (!___pdr_lua_isuserdata(L, p))
				___pdr_luaL_error(L, "expect userdata at arg %d", p);

			return checkReturn<T>(L, p);
		}

		template<class T>
		bool checkValue(___pdr_lua_State* L, int p, T& out) {
			if (!typeMatched<T>(___pdr_lua_type(L, p)))
				return false;
			out = checkValue<T>(L, p);
			return true;
		}

		// check value if it's enum
		template<typename T>
		static T checkEnumValue(___pdr_lua_State* L, int p) {
			return static_cast<T>(___pdr_luaL_checkinteger(L, p));
		}

        // check value if it's TArray
        template<class T>
		static T checkTArray(___pdr_lua_State* L, int p) {
            CheckUD(LuaArray,L,p);
			return UD->asTArray<typename T::ElementType>(L);
		}

		// check value if it's TMap
		template<class T>
		static T checkTMap(___pdr_lua_State* L, int p) {
			CheckUD(LuaMap, L, p);
			using KeyType = typename TPairTraits<typename T::ElementType>::KeyType;
			using ValueType = typename TPairTraits<typename T::ElementType>::ValueType;
			return UD->asTMap<KeyType, ValueType>(L);
		}

        template<typename T>
        static void* void_cast( const T* v ) {
            return reinterpret_cast<void *>(const_cast< T* >(v));
        }

        template<typename T>
        static void* void_cast( T* v ) {
            return reinterpret_cast<void *>(v);
        }

		template<class T>
		static int push(___pdr_lua_State* L, const char* fn, const T* v, uint32 flag = UD_NOFLAG) {
            if(getFromCache(L,void_cast(v),fn)) return 1;
			___pdr_luaL_getmetatable(L,fn);
			// if v is the UnrealType
			UScriptStruct* uss = nullptr;
			if (___pdr_lua_isnil(L, -1) && isUnrealStruct(fn, &uss)) {
				___pdr_lua_pop(L, 1); // pop nil
				uint32 size = uss->GetStructureSize() ? uss->GetStructureSize() : 1;
				ensure(size == sizeof(T));
				uint8* buf = (uint8*)FMemory::Malloc(size);
				uss->InitializeStruct(buf);
				uss->CopyScriptStruct(buf, v);
				cacheObj(L, void_cast(v));
				return push(L, new LuaStruct(buf, size, uss));
			}
			NewUD(T, v, flag);
			___pdr_lua_pushvalue(L, -2);
			___pdr_lua_setmetatable(L, -2);
			___pdr_lua_remove(L, -2); // remove metatable of fn
            cacheObj(L,void_cast(v));
            return 1;
		}

		static void releaseLink(___pdr_lua_State* L, void* prop);
		static void linkProp(___pdr_lua_State* L, void* parent, void* prop);

		template<class T>
		static int pushAndLink(___pdr_lua_State* L, const void* parent, const char* tn, const T* v) {
			if (getFromCache(L, void_cast(v), tn)) return 1;
			NewUD(T, v, UD_NOFLAG);
			___pdr_luaL_getmetatable(L, tn);
			___pdr_lua_setmetatable(L, -2);
			cacheObj(L, void_cast(v));
			linkProp(L, void_cast(parent), void_cast(udptr));
			return 1;
		}

        typedef void SetupMetaTableFunc(___pdr_lua_State* L,const char* tn, ___pdr_lua_CFunction setupmt, ___pdr_lua_CFunction gc);

        template<class T, bool F = IsUObject<T>::value >
        static int pushType(___pdr_lua_State* L,T cls,const char* tn, ___pdr_lua_CFunction setupmt=nullptr, ___pdr_lua_CFunction gc=nullptr) {
            if(!cls) {
				___pdr_lua_pushnil(L);
                return 1;
            }
            UserData<T>* ud = reinterpret_cast< UserData<T>* >(___pdr_lua_newuserdata(L, sizeof(UserData<T>)));
			ud->parent = nullptr;
            ud->ud = cls;
            ud->flag = gc!=nullptr?UD_AUTOGC:UD_NOFLAG;
			if (F) ud->flag |= UD_UOBJECT;
            setupMetaTable(L,tn,setupmt,gc);
            return 1;
        }

		// for weak UObject

		static int gcWeakUObject(___pdr_lua_State* L) {
			___pdr_luaL_checktype(L, 1, ___PDR_LUA_TUSERDATA);
			UserData<WeakUObjectUD*>* ud = reinterpret_cast<UserData<WeakUObjectUD*>*>(___pdr_lua_touserdata(L, 1));
			ensure(ud->flag&UD_WEAKUPTR);
			ud->flag |= UD_HADFREE;
			SafeDelete(ud->ud);
			return 0;
		}

		static int pushWeakType(___pdr_lua_State* L, WeakUObjectUD* cls) {
			UserData<WeakUObjectUD*>* ud = reinterpret_cast<UserData<WeakUObjectUD*>*>(___pdr_lua_newuserdata(L, sizeof(UserData<WeakUObjectUD*>)));
			ud->parent = nullptr;
			ud->ud = cls;
			ud->flag = UD_WEAKUPTR | UD_AUTOGC;
			setupMetaTable(L, "UObject", setupInstanceMT, gcWeakUObject);
			return 1;
		}

		// for TSharePtr version

		template<class T, ESPMode mode>
		static int gcSharedUD(___pdr_lua_State* L) {
			___pdr_luaL_checktype(L, 1, ___PDR_LUA_TUSERDATA);
			UserData<T*>* ud = reinterpret_cast<UserData<T*>*>(___pdr_lua_touserdata(L, 1));
			ud->flag |= UD_HADFREE;
			SafeDelete(ud->ud);
			return 0;
		}

		template<class BOXPUD, ESPMode mode, bool F>
		static int pushSharedType(___pdr_lua_State* L, BOXPUD* cls, const char* tn, int flag) {
			UserData<BOXPUD*>* ud = reinterpret_cast<UserData<BOXPUD*>*>(___pdr_lua_newuserdata(L, sizeof(UserData<BOXPUD*>)));
			ud->parent = nullptr;
			ud->ud = cls;
			ud->flag = UD_AUTOGC | flag;
			if (F) ud->flag |= UD_UOBJECT;
			if (mode == ESPMode::ThreadSafe) ud->flag |= UD_THREADSAFEPTR;
			setupMetaTable(L, tn, gcSharedUD<BOXPUD, mode>);
			return 1;
		}

		template<class T,ESPMode mode, bool F = IsUObject<T>::value>
		static int pushType(___pdr_lua_State* L, SharedPtrUD<T, mode>* cls, const char* tn) {
			if (!cls) {
				___pdr_lua_pushnil(L);
				return 1;
			}
			using BOXPUD = SharedPtrUD<T, mode>;
			return pushSharedType<BOXPUD,mode,F>(L, cls, tn, UD_SHAREDPTR);
		}

		template<class T, ESPMode mode, bool F = IsUObject<T>::value>
		static int pushType(___pdr_lua_State* L, SharedRefUD<T, mode>* cls, const char* tn) {
			if (!cls) {
				___pdr_lua_pushnil(L);
				return 1;
			}
			using BOXPUD = SharedRefUD<T, mode>;
			return pushSharedType<BOXPUD, mode,F>(L, cls, tn, UD_SHAREDREF);
		}

        static void addRef(___pdr_lua_State* L,UObject* obj, void* ud, bool ref);
        static void removeRef(___pdr_lua_State* L,UObject* obj);

        template<typename T>
        static int pushGCObject(___pdr_lua_State* L,T obj,const char* tn, ___pdr_lua_CFunction setupmt, ___pdr_lua_CFunction gc,bool ref) {
            if(getFromCache(L,obj,tn)) return 1;
			___pdr_lua_pushcclosure(L,gc,0);
            int f = ___pdr_lua_gettop(L);
            int r = pushType<T>(L,obj,tn,setupmt,f);
			___pdr_lua_remove(L,f); // remove wraped gc function
			if (r) {
				addRef(L, obj, ___pdr_lua_touserdata(L, -1), ref);
				cacheObj(L, obj);
			}
            return r;
        }

        template<typename T>
        static int pushObject(___pdr_lua_State* L,T obj,const char* tn, ___pdr_lua_CFunction setupmt=nullptr) {
            if(getFromCache(L,obj,tn)) return 1;
            int r = pushType<T>(L,obj,tn,setupmt,nullptr);
            if(r) cacheObj(L,obj);
            return r;
        }

		static int setupMTSelfSearch(___pdr_lua_State* L);
        
        static int pushClass(___pdr_lua_State* L,UClass* cls);
        static int pushStruct(___pdr_lua_State* L,UScriptStruct* cls);
		static int pushEnum(___pdr_lua_State* L, UEnum* e);
		static int push(___pdr_lua_State* L, UObject* obj, bool rawpush=false, bool ref=true);
		inline static int push(___pdr_lua_State* L, const UObject* obj) {
			return push(L, const_cast<UObject*>(obj));
		}
		static int push(___pdr_lua_State* L, FWeakObjectPtr ptr);
		static int push(___pdr_lua_State* L, FScriptDelegate* obj);
		static int push(___pdr_lua_State* L, LuaStruct* ls);
		static int push(___pdr_lua_State* L, double v);
        static int push(___pdr_lua_State* L, int64 v);
        static int push(___pdr_lua_State* L, uint64 v);
        static int push(___pdr_lua_State* L, int8 v);
        static int push(___pdr_lua_State* L, uint8 v);
        static int push(___pdr_lua_State* L, int16 v);
        static int push(___pdr_lua_State* L, uint16 v);
		static int push(___pdr_lua_State* L, float v);
		static int push(___pdr_lua_State* L, int v);
		static int push(___pdr_lua_State* L, bool v);
		static int push(___pdr_lua_State* L, uint32 v);
		static int push(___pdr_lua_State* L, void* v);
		static int push(___pdr_lua_State* L, const FText& v);
		static int push(___pdr_lua_State* L, const FString& str);
		static int push(___pdr_lua_State* L, const FName& str);
		static int push(___pdr_lua_State* L, const char* str);
		static int push(___pdr_lua_State* L, const LuaVar& v);
        static int push(___pdr_lua_State* L, UFunction* func, UClass* cls=nullptr);
		static int push(___pdr_lua_State* L, const LuaLString& lstr);
		static int push(___pdr_lua_State* L, UProperty* up, uint8* parms, bool ref=true);
		static int push(___pdr_lua_State* L, UProperty* up, UObject* obj, bool ref=true);

        // check tn is base of base
        static bool isBaseTypeOf(___pdr_lua_State* L,const char* tn,const char* base);

        template<typename T>
        static int push(___pdr_lua_State* L,T* ptr,typename std::enable_if<!std::is_base_of<UObject,T>::value && !Has_LUA_typename<T>::value>::type* = nullptr) {
            return push(L, TypeName<T>::value().c_str(), ptr);
        }

		// it's an override for non-uobject, non-ptr, only accept struct or class value
		template<typename T>
		static int push(___pdr_lua_State* L, const T& v, typename std::enable_if<!std::is_base_of<UObject, T>::value && std::is_class<T>::value>::type* = nullptr) {
			T* newPtr = new T(v);
			return push<T>(L, TypeName<T>::value().c_str(), newPtr, UD_AUTOGC);
		}

		// if T has a member function named LUA_typename,
		// used this branch
		template<typename T>
		static int push(___pdr_lua_State* L, T* ptr, typename std::enable_if<!std::is_base_of<UObject, T>::value && Has_LUA_typename<T>::value>::type* = nullptr) {
			return push(L, ptr->LUA_typename().c_str(), ptr);
		}

		template<typename T>
		static int push(___pdr_lua_State* L, LuaOwnedPtr<T> ptr, typename std::enable_if<!std::is_base_of<UObject, T>::value && Has_LUA_typename<T>::value>::type* = nullptr) {
			return push(L, ptr->LUA_typename().c_str(), ptr.ptr, UD_AUTOGC);
		}

		template<typename T>
		static int push(___pdr_lua_State* L, LuaOwnedPtr<T> ptr, typename std::enable_if<!std::is_base_of<UObject, T>::value && !Has_LUA_typename<T>::value>::type* = nullptr) {
			return push(L, TypeName<T>::value().c_str(), ptr.ptr, UD_AUTOGC);
		}

		static int gcSharedPtr(___pdr_lua_State *L) {
			return 0;
		}

		template<typename T, ESPMode mode>
		static int push(___pdr_lua_State* L, const TSharedPtr<T, mode>& ptr) {
			// get raw ptr from sharedptr
			T* rawptr = ptr.Get();
			// get typename 
			auto tn = TypeName<T>::value();
			if (getFromCache(L, rawptr, tn.c_str())) return 1;
			int r = pushType<T>(L, new SharedPtrUD<T, mode>(ptr), tn.c_str());
			if (r) cacheObj(L, rawptr);
			return r;
		}

		template<typename T, ESPMode mode>
		static int push(___pdr_lua_State* L, const TSharedRef<T, mode>& ref) {
			// get raw ptr from sharedptr
			T& rawref = ref.Get();
			// get typename 
			auto tn = TypeName<T>::value();
			if (getFromCache(L, &rawref, tn.c_str())) return 1;
			int r = pushType<T>(L, new SharedRefUD<T, mode>(ref), tn.c_str());
			if (r) cacheObj(L, &rawref);
			return r;
		}

		// for TBaseDelegate
		template<class R, class ...ARGS>
		static int push(___pdr_lua_State* L, TBaseDelegate<R, ARGS...>& delegate);
		

        template<typename T>
        static int push(___pdr_lua_State* L,T v,typename std::enable_if<std::is_enum<T>::value>::type* = nullptr) {
            return push(L,static_cast<int>(v));
        }

		template<typename T>
		static int push(___pdr_lua_State* L, const TArray<T>& v) {
			return LuaArray::push(L, v);
		}

		template<typename K,typename V>
		static int push(___pdr_lua_State* L, const TMap<K,V>& v) {
			return LuaMap::push(L, v);
		}

		// static int push(___pdr_lua_State* L, FScriptArray* array);
        
        static int pushNil(___pdr_lua_State* L) {
			___pdr_lua_pushnil(L);
            return 1;
        }

		static void addExtensionMethod(UClass* cls, const char* n, ___pdr_lua_CFunction func, bool isStatic = false);
		static void addExtensionProperty(UClass* cls, const char* n, ___pdr_lua_CFunction getter, ___pdr_lua_CFunction setter, bool isStatic = false);

        static UFunction* findCacheFunction(___pdr_lua_State* L,UClass* cls,const char* fname);
        static void cacheFunction(___pdr_lua_State* L, UClass* cls,const char* fame,UFunction* func);

        static UProperty* findCacheProperty(___pdr_lua_State* L, UClass* cls, const char* pname);
        static void cacheProperty(___pdr_lua_State* L, UClass* cls, const char* pname, UProperty* property);

        static bool getFromCache(___pdr_lua_State* L, void* obj, const char* tn, bool check = true);
		static void cacheObj(___pdr_lua_State* L, void* obj);
		static void removeFromCache(___pdr_lua_State* L, void* obj);
		static UPLatentDelegate* getLatentDelegate(___pdr_lua_State* L);
		static void deleteFGCObject(___pdr_lua_State* L,FGCObject* obj);
    private:
        static int setupClassMT(___pdr_lua_State* L);
        static int setupInstanceMT(___pdr_lua_State* L);
        static int setupInstanceStructMT(___pdr_lua_State* L);
        static int setupStructMT(___pdr_lua_State* L);

        static int gcObject(___pdr_lua_State* L);
        static int gcClass(___pdr_lua_State* L);
        static int gcStructClass(___pdr_lua_State* L);
		static int gcStruct(___pdr_lua_State* L);
        static int objectToString(___pdr_lua_State* L);
        static void setupMetaTable(___pdr_lua_State* L,const char* tn,___pdr_lua_CFunction setupmt,___pdr_lua_CFunction gc);
		static void setupMetaTable(___pdr_lua_State* L, const char* tn, ___pdr_lua_CFunction setupmt, int gc);
		static void setupMetaTable(___pdr_lua_State* L, const char* tn, ___pdr_lua_CFunction gc);
		static void callRpc(___pdr_lua_State* L, UObject* obj, UFunction* func, uint8* params);

        template<class T, bool F = IsUObject<T>::value>
        static int pushType(___pdr_lua_State* L,T cls,const char* tn,___pdr_lua_CFunction setupmt,int gc) {
            if(!cls) {
                ___pdr_lua_pushnil(L);
                return 1;
            }
                
            UserData<T>* ud = reinterpret_cast< UserData<T>* >(___pdr_lua_newuserdata(L, sizeof(UserData<T>)));
			ud->parent = nullptr;
            ud->ud = cls;
            ud->flag = F|UD_AUTOGC;
			if (F) ud->flag |= UD_UOBJECT;
            setupMetaTable(L,tn,setupmt,gc);
            return 1;
        }
		static void createTable(___pdr_lua_State* L, const char* tn);
    };

    template<>
    inline UClass* LuaObject::checkValue(___pdr_lua_State* L, int p) {
        CheckUD(UClass, L, p);
        return UD;
    }

    template<>
    inline UObject* LuaObject::checkValue(___pdr_lua_State* L, int p) {
		CheckUD(UObject, L, p);
		return UD;
    }

    template<>
    inline UScriptStruct* LuaObject::checkValue(___pdr_lua_State* L, int p) {
        CheckUD(UScriptStruct, L, p);
        return UD;
    }

    template<>
    inline LuaStruct* LuaObject::checkValue(___pdr_lua_State* L, int p) {
        CheckUD(LuaStruct, L, p);
        return UD;
    }

    template<>
    inline const char* LuaObject::checkValue(___pdr_lua_State* L, int p) {
        return ___pdr_luaL_checkstring(L, p);
    }

    template<>
    inline float LuaObject::checkValue(___pdr_lua_State* L, int p) {
        return (float)___pdr_luaL_checknumber(L, p);
    }

    template<>
    inline double LuaObject::checkValue(___pdr_lua_State* L, int p) {
        return ___pdr_luaL_checknumber(L, p);
    }

    template<>
    inline int LuaObject::checkValue(___pdr_lua_State* L, int p) {
        return ___pdr_luaL_checkinteger(L, p);
    }

    template<>
    inline uint32 LuaObject::checkValue(___pdr_lua_State* L, int p) {
        return (uint32) ___pdr_luaL_checkinteger(L, p);
    }

    template<>
    inline int8 LuaObject::checkValue(___pdr_lua_State* L, int p) {
        return (int8) ___pdr_luaL_checkinteger(L, p);
    }

    template<>
    inline uint8 LuaObject::checkValue(___pdr_lua_State* L, int p) {
        return (uint8) ___pdr_luaL_checkinteger(L, p);
    }

    template<>
    inline int16 LuaObject::checkValue(___pdr_lua_State* L, int p) {
        return (int16) ___pdr_luaL_checkinteger(L, p);
    }

    template<>
    inline uint16 LuaObject::checkValue(___pdr_lua_State* L, int p) {
        return (uint16) ___pdr_luaL_checkinteger(L, p);
    }

    template<>
    inline int64 LuaObject::checkValue(___pdr_lua_State* L, int p) {
        return ___pdr_luaL_checkinteger(L, p);
    }

    template<>
    inline uint64 LuaObject::checkValue(___pdr_lua_State* L, int p) {
        return ___pdr_luaL_checkinteger(L, p);
    }

    template<>
    inline bool LuaObject::checkValue(___pdr_lua_State* L, int p) {
        ___pdr_luaL_checktype(L, p, ___PDR_LUA_TBOOLEAN);
        return !!___pdr_lua_toboolean(L, p);
    }

    template<>
    inline FText LuaObject::checkValue(___pdr_lua_State* L, int p) {
        const char* s = ___pdr_luaL_checkstring(L, p);
        return FText::FromString(UTF8_TO_TCHAR(s));
    }

    template<>
    inline FString LuaObject::checkValue(___pdr_lua_State* L, int p) {
        const char* s = ___pdr_luaL_checkstring(L, p);
        return FString(UTF8_TO_TCHAR(s));
    }

    template<>
    inline FName LuaObject::checkValue(___pdr_lua_State* L, int p) {
        const char* s = ___pdr_luaL_checkstring(L, p);
        return FName(UTF8_TO_TCHAR(s));
    }

    template<>
    inline void* LuaObject::checkValue(___pdr_lua_State* L, int p) {
        ___pdr_luaL_checktype(L,p,___PDR_LUA_TLIGHTUSERDATA);
        return ___pdr_lua_touserdata(L,p);
    }

	template<>
	inline int LuaObject::pushType<LuaStruct*, false>(___pdr_lua_State* L, LuaStruct* cls,
		const char* tn, ___pdr_lua_CFunction setupmt, ___pdr_lua_CFunction gc) {
		if (!cls) {
			___pdr_lua_pushnil(L);
			return 1;
		}
		UserData<LuaStruct*>* ud = reinterpret_cast<UserData<LuaStruct*>*>(___pdr_lua_newuserdata(L, sizeof(UserData<LuaStruct*>)));
		ud->parent = nullptr;
		ud->ud = cls;
		ud->flag = gc != nullptr ? UD_AUTOGC : UD_NOFLAG;
		ud->flag |= UD_USTRUCT;
		setupMetaTable(L, tn, setupmt, gc);
		return 1;
	}
}
