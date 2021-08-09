// Tencent is pleased to support the open source community by making sluaunreal available.

// Copyright (C) 2018 THL A29 Limited, a Tencent company. All rights reserved.
// Licensed under the BSD 3-Clause License (the "License"); 
// you may not use this file except in compliance with the License. You may obtain a copy of the License at

// https://opensource.org/licenses/BSD-3-Clause

// Unless required by applicable law or agreed to in writing, 
// software distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.


#include "PDRLuaState.h"
#include "PDRLuaObject.h"
#include "PDRSluaLib.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
#include "Blueprint/UserWidget.h"
#include "Misc/AssertionMacros.h"
#include "Misc/SecureHash.h"
#include "PDRSluaLog.h"
#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif
#include "lua/lua.hpp"
#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif
#include <map>
#include "PDRLuaWrapper.h"
#include "PDRLuaArray.h"
#include "PDRLuaMap.h"
#include "PDRLuaSocketWrap.h"
#include "PDRLuaMemoryProfile.h"
#include "HAL/RunnableThread.h"
#include "GameDelegates.h"
#include "PDRLatentDelegate.h"
#include "PDRLuaActor.h"
#include "PDRLuaProfiler.h"
#include "Stats.h"

namespace NS_PDR_SLUA {

	const int MaxLuaExecTime = 120; // in second

    int import(___pdr_lua_State *L) {
        const char* name = LuaObject::checkValue<const char*>(L,1);
        if(name) {
            UClass* uclass = FindObject<UClass>(ANY_PACKAGE, UTF8_TO_TCHAR(name));
            if(uclass) return LuaObject::pushClass(L,uclass);
            
			UScriptStruct* ustruct = FindObject<UScriptStruct>(ANY_PACKAGE, UTF8_TO_TCHAR(name));
            if(ustruct) return LuaObject::pushStruct(L,ustruct);

			UEnum* uenum = FindObject<UEnum>(ANY_PACKAGE, UTF8_TO_TCHAR(name));
			if (uenum) return LuaObject::pushEnum(L, uenum);
            
            ___pdr_luaL_error(L,"Can't find class named %s",name);
        }
        return 0;
    }
    
    int print(___pdr_lua_State *L) {
        FString str;
        int top = ___pdr_lua_gettop(L);
        for(int n=1;n<=top;n++) {
            size_t len;
            const char* s = ___pdr_luaL_tolstring(L, n, &len);
            str+="\t";
            if(s) str+=UTF8_TO_TCHAR(s);
        }
        Log::Log("%s",TCHAR_TO_UTF8(*str));
        return 0;
    }

	int dofile(___pdr_lua_State *L) {
		auto fn = ___pdr_luaL_checkstring(L, 1);
		auto ls = LuaState::get(L);
		ensure(ls);
		auto var = ls->doFile(fn);
		if (var.isValid()) {
			return var.push(L);
		}
		return 0;
	}

    int error(___pdr_lua_State* L) {
        const char* err = ___pdr_lua_tostring(L,1);
        ___pdr_luaL_traceback(L,L,err,1);
        err = ___pdr_lua_tostring(L,2);
        ___pdr_lua_pop(L,1);
		auto ls = LuaState::get(L);
		ls->onError(err);
        return 0;
    }

	void LuaState::onError(const char* err) {
		if (errorDelegate) errorDelegate(err);
		else Log::Error("%s", err);
	}

     #if WITH_EDITOR
     // used for debug
	int LuaState::getStringFromMD5(___pdr_lua_State* L) {
		const char* md5String = ___pdr_lua_tostring(L, 1);
		LuaState* state = LuaState::get(L);
		FString md5FString = UTF8_TO_TCHAR(md5String);
		bool hasValue = state->debugStringMap.Contains(md5FString);
		if (hasValue) {
			auto value = state->debugStringMap[md5FString];
			___pdr_lua_pushstring(L, TCHAR_TO_UTF8(*value));
		}
		else {
			___pdr_lua_pushstring(L, "");
		}
		return 1;
	}
    #endif

    int LuaState::loader(___pdr_lua_State* L) {
        LuaState* state = LuaState::get(L);
        const char* fn = ___pdr_lua_tostring(L,1);
        uint32 len;
        FString filepath;
        if(uint8* buf = state->loadFile(fn,len,filepath)) {
            AutoDeleteArray<uint8> defer(buf);

            char chunk[256];
            snprintf(chunk,256,"@%s",TCHAR_TO_UTF8(*filepath));
            if(___pdr_luaL_loadbuffer(L,(const char*)buf,len,chunk)==0) {
                return 1;
            }
            else {
                const char* err = ___pdr_lua_tostring(L,-1);
                Log::Error("%s",err);
                ___pdr_lua_pop(L,1);
            }
        }
        else
            Log::Error("Can't load file %s",fn);
        return 0;
    }
    
    uint8* LuaState::loadFile(const char* fn,uint32& len,FString& filepath) {
        if(loadFileDelegate) return loadFileDelegate(fn,len,filepath);
        return nullptr;
    }

    LuaState* LuaState::mainState = nullptr;
    TMap<int,LuaState*> stateMapFromIndex;
    static int StateIndex = 0;

	LuaState::LuaState(const char* name, UGameInstance* gameInstance)
		: loadFileDelegate(nullptr)
		, errorDelegate(nullptr)
		, L(nullptr)
		, cacheObjRef(___PDR_LUA_NOREF)
		, stackCount(0)
		, si(0)
		, deadLoopCheck(nullptr)
    {
        if(name) stateName=UTF8_TO_TCHAR(name);
		this->pGI = gameInstance;
    }

    LuaState::~LuaState()
    {
        close();
    }

    LuaState* LuaState::get(int index) {
        auto it = stateMapFromIndex.Find(index);
        if(it) return *it;
        return nullptr;
    }

    LuaState* LuaState::get(const FString& name) {
        for(auto& pair:stateMapFromIndex) {
            auto state = pair.Value;
            if(state->stateName==name)
                return state;
        }
        return nullptr;
    }

	LuaState* LuaState::get(UGameInstance* pGI) {
		for (auto& pair : stateMapFromIndex) {
			auto state = pair.Value;
			if (state->pGI && state->pGI == pGI)
				return state;
		}
		return nullptr;
	}

    // check lua top , this function can omit
    void LuaState::Tick(float dtime) {
		ensure(IsInGameThread());
		if (!L) return;

		int top = ___pdr_lua_gettop(L);
		if (top != stackCount) {
			stackCount = top;
			Log::Error("Error: lua stack count should be zero , now is %d", top);
		}

#ifdef ENABLE_PROFILER
		LuaProfiler::tick(L);
#endif

		PROFILER_WATCHER(w1);
		if (stateTickFunc.isFunction())
		{
			PROFILER_WATCHER_X(w2,"TickFunc");
			stateTickFunc.call(dtime);
		}

		// try lua gc
		PROFILER_WATCHER_X(w3, "LuaGC");
		if (!enableMultiThreadGC) ___pdr_lua_gc(L, ___PDR_LUA_GCSTEP, 128);
    }

    void LuaState::close() {
        if(mainState==this) mainState = nullptr;

		latentDelegate = nullptr;

		freeDeferObject();

		releaseAllLink();

		cleanupThreads();
        
        if(L) {
            ___pdr_lua_close(L);
			GUObjectArray.RemoveUObjectDeleteListener(this);
			FCoreUObjectDelegates::GetPostGarbageCollect().Remove(pgcHandler);
			FWorldDelegates::OnWorldCleanup.Remove(wcHandler);
            stateMapFromIndex.Remove(si);
            L=nullptr;
        }
		freeDeferObject();
		objRefs.Empty();
		SafeDelete(deadLoopCheck);
    }


    bool LuaState::init(bool gcFlag) {

        if(deadLoopCheck)
            return false;

        if(!mainState) 
            mainState = this;

		enableMultiThreadGC = gcFlag;
		pgcHandler = FCoreUObjectDelegates::GetPostGarbageCollect().AddRaw(this, &LuaState::onEngineGC);
		wcHandler = FWorldDelegates::OnWorldCleanup.AddRaw(this, &LuaState::onWorldCleanup);
		GUObjectArray.AddUObjectDeleteListener(this);

		latentDelegate = NewObject<UPLatentDelegate>((UObject*)GetTransientPackage(), UPLatentDelegate::StaticClass());
		latentDelegate->bindLuaState(this);

        stackCount = 0;
        si = ++StateIndex;

		propLinks.Empty();
		classMap.clear();
		objRefs.Empty();

#if WITH_EDITOR
		// used for debug
		debugStringMap.Empty();
#endif

		deadLoopCheck = new FDeadLoopCheck();

        // use custom memory alloc func to profile memory footprint
        L = ___pdr_lua_newstate(LuaMemoryProfile::alloc,this);
        ___pdr_lua_atpanic(L,_atPanic);
        // bind this to L
        *((void**)___pdr_lua_getextraspace(L)) = this;
        stateMapFromIndex.Add(si,this);

        // init obj cache table
        ___pdr_lua_newtable(L);
        ___pdr_lua_newtable(L);
        ___pdr_lua_pushstring(L,"kv");
        ___pdr_lua_setfield(L,-2,"__mode");
        ___pdr_lua_setmetatable(L,-2);
        // register it
        cacheObjRef = ___pdr_luaL_ref(L,___PDR_LUA_REGISTRYINDEX);

        ensure(___pdr_lua_gettop(L)==0);
        
        ___pdr_luaL_openlibs(L);
        
        ___pdr_lua_pushcfunction(L,import);
        ___pdr_lua_setglobal(L, "import");
        
        ___pdr_lua_pushcfunction(L,print);
        ___pdr_lua_setglobal(L, "print");

		___pdr_lua_pushcfunction(L, dofile);
		___pdr_lua_setglobal(L, "dofile");

        #if WITH_EDITOR
        // used for debug
		___pdr_lua_pushcfunction(L, getStringFromMD5);
		___pdr_lua_setglobal(L, "getStringFromMD5");
        #endif
		
        ___pdr_lua_pushcfunction(L,loader);
        int loaderFunc = ___pdr_lua_gettop(L);

        ___pdr_lua_getglobal(L,"package");
        ___pdr_lua_getfield(L,-1,"searchers");

        int loaderTable = ___pdr_lua_gettop(L);

        for(int i=___pdr_lua_rawlen(L,loaderTable)+1;i>2;i--) {
            ___pdr_lua_rawgeti(L,loaderTable,i-1);
            ___pdr_lua_rawseti(L,loaderTable,i);
        }
        ___pdr_lua_pushvalue(L,loaderFunc);
        ___pdr_lua_rawseti(L,loaderTable,2);
		___pdr_lua_settop(L, 0);
        
		LuaSocket::init(L);
        LuaObject::init(L);
        SluaUtil::openLib(L);
        LuaClass::reg(L);
        LuaArray::reg(L);
        LuaMap::reg(L);
#ifdef ENABLE_PROFILER
		LuaProfiler::init(L);
#endif
		
		onInitEvent.Broadcast();

		// disable gc in main thread
		if (enableMultiThreadGC) ___pdr_lua_gc(L, ___PDR_LUA_GCSTOP, 0);

        ___pdr_lua_settop(L,0);

        return true;
    }

	void LuaState::attach(UGameInstance* GI) {
		this->pGI = GI;
	}

    int LuaState::_atPanic(___pdr_lua_State* L) {
        const char* err = ___pdr_lua_tostring(L,-1);
        Log::Error("Fatal error: %s",err);
        return 0;
    }

	void LuaState::setLoadFileDelegate(LoadFileDelegate func) {
		loadFileDelegate = func;
	}

	void LuaState::setErrorDelegate(ErrorDelegate func) {
		errorDelegate = func;
	}

	static void* findParent(GenericUserData* parent) {
		auto pp = parent;
		while(true) {
			if (!pp->parent)
				break;
			pp = reinterpret_cast<GenericUserData*>(pp->parent);
		}
		return pp;
	}

	void LuaState::linkProp(void* parent, void* prop) {
		auto parentud = findParent(reinterpret_cast<GenericUserData*>(parent));
		auto propud = reinterpret_cast<GenericUserData*>(prop);
		propud->parent = parentud;
		auto& propList = propLinks.FindOrAdd(parentud);
		propList.Add(propud);
	}

	void LuaState::releaseLink(void* prop) {
		auto propud = reinterpret_cast<GenericUserData*>(prop);
		if (propud->flag & UD_AUTOGC) {
			auto propListPtr = propLinks.Find(propud);
			if (propListPtr) 
				for (auto& cprop : *propListPtr) 
					reinterpret_cast<GenericUserData*>(cprop)->flag |= UD_HADFREE;
		} else {
			propud->flag |= UD_HADFREE;
			auto propListPtr = propLinks.Find(propud->parent);
			if (propListPtr) 
				propListPtr->Remove(propud);
		}
	}

	void LuaState::releaseAllLink() {
		for (auto& pair : propLinks) 
			for (auto& prop : pair.Value) 
				reinterpret_cast<GenericUserData*>(prop)->flag |= UD_HADFREE;
		propLinks.Empty();
	}

	// engine will call this function on post gc
	void LuaState::onEngineGC()
	{
		PROFILER_WATCHER(w1);
		// find freed uclass
		for (ClassCache::CacheFuncMap::TIterator it(classMap.cacheFuncMap); it; ++it)
			if (!it.Key().IsValid())
				it.RemoveCurrent();
		
		for (ClassCache::CachePropMap::TIterator it(classMap.cachePropMap); it; ++it)
			if (!it.Key().IsValid())
				it.RemoveCurrent();		
		
		freeDeferObject();

		Log::Log("Unreal engine GC, lua used %d KB",___pdr_lua_gc(L, ___PDR_LUA_GCCOUNT, 0));
	}

	void LuaState::onWorldCleanup(UWorld * World, bool bSessionEnded, bool bCleanupResources)
	{
		PROFILER_WATCHER(w1);
		unlinkUObject(World);
	}

	void LuaState::freeDeferObject()
	{
		// really delete FGCObject
		for (auto ptr : deferDelete)
			delete ptr;
		deferDelete.Empty();
	}

	LuaVar LuaState::doBuffer(const uint8* buf, uint32 len, const char* chunk, LuaVar* pEnv) {
        AutoStack g(L);
        int errfunc = pushErrorHandler(L);

        if(___pdr_luaL_loadbuffer(L, (const char *)buf, len, chunk)) {
            const char* err = ___pdr_lua_tostring(L,-1);
            Log::Error("DoBuffer failed: %s",err);
            return LuaVar();
        }
        
		LuaVar f(L, -1);
		return f.call();
    }

    LuaVar LuaState::doString(const char* str, LuaVar* pEnv) {
        // fix #31 & #30 issue, 
        // vc compile optimize code cause cl.exe dead loop in release mode(no WITH_EDITOR)
        // if turn optimze flag on
        // so just write complex code to bypass link optimize
        // like this, WTF!
        uint32 len = strlen(str);
        #if WITH_EDITOR
		FString md5FString = FMD5::HashAnsiString(UTF8_TO_TCHAR(str));
		debugStringMap.Add(md5FString, UTF8_TO_TCHAR(str));
        return doBuffer((const uint8*)str,len,TCHAR_TO_UTF8(*md5FString),pEnv);
        #else
        return doBuffer((const uint8*)str,len,str,pEnv);
        #endif
    }

    LuaVar LuaState::doFile(const char* fn, LuaVar* pEnv) {
        uint32 len;
        FString filepath;
        if(uint8* buf=loadFile(fn,len,filepath)) {
            char chunk[256];
            snprintf(chunk,256,"@%s",TCHAR_TO_UTF8(*filepath));

            LuaVar r = doBuffer( buf,len,chunk,pEnv );
            delete[] buf;
            return r;
        }
        return LuaVar();
    }

	void LuaState::NotifyUObjectDeleted(const UObjectBase * Object, int32 Index)
	{
		PROFILER_WATCHER(w1);
		unlinkUObject((const UObject*)Object);
	}

	void LuaState::unlinkUObject(const UObject * Object)
	{
		// find Object from objRefs, maybe nothing
		auto udptr = objRefs.Find(Object);
		// maybe Object not push to lua
		if (!udptr) {
			return;
		}

		GenericUserData* ud = *udptr;

		// remove should put here avoid ud is invalid
		// remove ref, Object must be an UObject in slua
		objRefs.Remove(const_cast<UObject*>(Object));

		if (!ud || ud->flag & UD_HADFREE)
			return;

		// indicate ud had be free
		ud->flag |= UD_HADFREE;
		// remove cache
		ensure(ud->ud == Object);
		LuaObject::removeFromCache(L, (void*)Object);
	}

	void LuaState::AddReferencedObjects(FReferenceCollector & Collector)
	{
		for (UObjectRefMap::TIterator it(objRefs); it; ++it)
		{
			UObject* item = it.Key();
			GenericUserData* userData = it.Value();
			if (userData && !(userData->flag & UD_REFERENCE))
			{
				continue;
			}
			Collector.AddReferencedObject(item);
		}
		// do more gc step in collecting thread
		// ___pdr_lua_gc can be call async in bg thread in some isolate position
		// but this position equivalent to main thread
		// we just try and find some proper async position
		if (enableMultiThreadGC && L) ___pdr_lua_gc(L, ___PDR_LUA_GCSTEP, 128);
	}
#if (ENGINE_MINOR_VERSION>=23) && (ENGINE_MAJOR_VERSION>=4)
	void LuaState::OnUObjectArrayShutdown() {
		// nothing todo, we don't add any listener to FUObjectDeleteListener
	}
#endif

	int LuaState::pushErrorHandler(___pdr_lua_State* L) {
        auto ls = get(L);
        ensure(ls!=nullptr);
        return ls->_pushErrorHandler(L);
    }
	
	TStatId LuaState::GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(LuaState, STATGROUP_Game);
	}

	int LuaState::addThread(___pdr_lua_State *thread)
	{
		int isMainThread = ___pdr_lua_pushthread(thread);
		if (isMainThread == 1)
		{
			___pdr_lua_pop(thread, 1);

			___pdr_luaL_error(thread, "Can't call latent action in main lua thread!");
			return ___PDR_LUA_REFNIL;
		}

		___pdr_lua_xmove(thread, L, 1);
		___pdr_lua_pop(thread, 1);

		ensure(___pdr_lua_isthread(L, -1));

		int threadRef = ___pdr_luaL_ref(L, ___PDR_LUA_REGISTRYINDEX);
		threadToRef.Add(thread, threadRef);
		refToThread.Add(threadRef, thread);

		return threadRef;
	}

	void LuaState::resumeThread(int threadRef)
	{
		QUICK_SCOPE_CYCLE_COUNTER(Lua_LatentCallback);

		___pdr_lua_State **threadPtr = refToThread.Find(threadRef);
		if (threadPtr)
		{
			___pdr_lua_State *thread = *threadPtr;
			bool threadIsDead = false;

			if (___pdr_lua_status(thread) == ___PDR_LUA_OK && ___pdr_lua_gettop(thread) == 0)
			{
				Log::Error("cannot resume dead coroutine");
				threadIsDead = true;
			}
			else
			{
				int status = ___pdr_lua_resume(thread, L, 0);
				if (status == ___PDR_LUA_OK || status == ___PDR_LUA_YIELD)
				{
					int nres = ___pdr_lua_gettop(thread);
					if (!___pdr_lua_checkstack(L, nres + 1))
					{
						___pdr_lua_pop(thread, nres);  /* remove results anyway */
						Log::Error("too many results to resume");
						threadIsDead = true;
					}
					else
					{
						___pdr_lua_xmove(thread, L, nres);  /* move yielded values */

						if (status == ___PDR_LUA_OK)
						{
							threadIsDead = true;
						}
					}
				}
				else
				{
					___pdr_lua_xmove(thread, L, 1);  /* move error message */
					const char* err = ___pdr_lua_tostring(L, -1);
					___pdr_luaL_traceback(L, thread, err, 0);
					err = ___pdr_lua_tostring(L, -1);
					Log::Error("%s", err);
					___pdr_lua_pop(L, 1);

					threadIsDead = true;
				}
			}

			if (threadIsDead)
			{
				threadToRef.Remove(thread);
				refToThread.Remove(threadRef);
				___pdr_luaL_unref(L, ___PDR_LUA_REGISTRYINDEX, threadRef);
			}
		}
	}

	int LuaState::findThread(___pdr_lua_State *thread)
	{
		int32 *threadRefPtr = threadToRef.Find(thread);
		return threadRefPtr ? *threadRefPtr : ___PDR_LUA_REFNIL;
	}

	void LuaState::cleanupThreads()
	{
		for (TMap<___pdr_lua_State*, int32>::TIterator It(threadToRef); It; ++It)
		{
			___pdr_lua_State *thread = It.Key();
			int32 threadRef = It.Value();
			if (threadRef != ___PDR_LUA_REFNIL)
			{
				___pdr_luaL_unref(L, ___PDR_LUA_REGISTRYINDEX, threadRef);
			}
		}
		threadToRef.Empty();
		refToThread.Empty();
	}

	UPLatentDelegate* LuaState::getLatentDelegate() const
	{
		return latentDelegate;
	}

	int LuaState::_pushErrorHandler(___pdr_lua_State* state) {
        ___pdr_lua_pushcfunction(state,error);
        return ___pdr_lua_gettop(state);
    }

    LuaVar LuaState::get(const char* key) {
        // push global table
        ___pdr_lua_pushglobaltable(L);

        FString path(key);
        FString left,right;
        LuaVar rt;
        while(strSplit(path,".",&left,&right)) {
            if(___pdr_lua_type(L,-1)!=___PDR_LUA_TTABLE) break;
            ___pdr_lua_pushstring(L,TCHAR_TO_UTF8(*left));
            ___pdr_lua_gettable(L,-2);
            rt.set(L,-1);
            ___pdr_lua_remove(L,-2);
            if(rt.isNil()) break;
            path = right;
        }
        ___pdr_lua_pop(L,1);
        return rt;
    }

	bool LuaState::set(const char * key, LuaVar v)
	{
		// push global table
		AutoStack as(L);
		___pdr_lua_pushglobaltable(L);

		FString path(key);
		FString left, right;
		LuaVar rt;
		while (strSplit(path, ".", &left, &right)) {
			if (___pdr_lua_type(L, -1) != ___PDR_LUA_TTABLE) return false;
			___pdr_lua_pushstring(L, TCHAR_TO_UTF8(*left));
			___pdr_lua_gettable(L, -2);
			if (___pdr_lua_isnil(L, -1))
			{
				// pop nil
				___pdr_lua_pop(L, 1);
				if (right.IsEmpty()) {
					___pdr_lua_pushstring(L, TCHAR_TO_UTF8(*left));
					v.push(L);
					___pdr_lua_rawset(L, -3);
					return true;
				}
				else {
					___pdr_lua_newtable(L);
					___pdr_lua_pushstring(L, TCHAR_TO_UTF8(*left));
					// push table again
					___pdr_lua_pushvalue(L, -2);
					___pdr_lua_rawset(L, -4);
					// stack leave table for next get
				}
			}
			else
			{
				// if sub field isn't table, set failed
				if (!right.IsEmpty() && !___pdr_lua_istable(L, -1))
					return false;

				if (___pdr_lua_istable(L, -1) && right.IsEmpty()) {
					___pdr_lua_pushstring(L, TCHAR_TO_UTF8(*left));
					v.push(L);
					___pdr_lua_rawset(L, -3);
					return true;
				}
			}
			path = right;
		}
		return false;
	}

    LuaVar LuaState::createTable() {
        ___pdr_lua_newtable(L);
        LuaVar ret(L,-1);
        ___pdr_lua_pop(L,1);
        return ret;
    }

	LuaVar LuaState::createTable(const char * key)
	{
		___pdr_lua_newtable(L);
		LuaVar ret(L, -1);
		set(key, ret);
		___pdr_lua_pop(L, 1);
		return ret;
	}

	void LuaState::setTickFunction(LuaVar func)
	{
		stateTickFunc = func;
	}

	void LuaState::addRef(UObject* obj, void* ud, bool ref)
	{
		auto* udptr = objRefs.Find(obj);
		// if any obj find in objRefs, it should be flag freed and removed
		if (udptr) {
			(*udptr)->flag |= UD_HADFREE;
			objRefs.Remove(obj);
		}

		GenericUserData* userData = (GenericUserData*)ud;
		if (ref && userData) {
			userData->flag |= UD_REFERENCE;
		}
		objRefs.Add(obj,userData);
	}

	FDeadLoopCheck::FDeadLoopCheck()
		: timeoutEvent(nullptr)
		, timeoutCounter(0)
		, stopCounter(0)
		, frameCounter(0)
	{
		thread = FRunnableThread::Create(this, TEXT("FLuaDeadLoopCheck"), 0, TPri_BelowNormal);
	}

	FDeadLoopCheck::~FDeadLoopCheck()
	{
		Stop();
		thread->WaitForCompletion();
		SafeDelete(thread);
	}

	uint32 FDeadLoopCheck::Run()
	{
		while (stopCounter.GetValue() == 0) {
			FPlatformProcess::Sleep(1.0f);
			if (frameCounter.GetValue() != 0) {
				timeoutCounter.Increment();
				if(timeoutCounter.GetValue() >= MaxLuaExecTime)
					onScriptTimeout();
			}
		}
		return 0;
	}

	void FDeadLoopCheck::Stop()
	{
		stopCounter.Increment();
	}

	int FDeadLoopCheck::scriptEnter(ScriptTimeoutEvent* pEvent)
	{
		int ret = frameCounter.Increment();
		if ( ret == 1) {
			timeoutCounter.Set(0);
			timeoutEvent.store(pEvent);
		}
		return ret;
	}

	int FDeadLoopCheck::scriptLeave()
	{
		return frameCounter.Decrement();
	}

	void FDeadLoopCheck::onScriptTimeout()
	{
		auto pEvent = timeoutEvent.load();
		if (pEvent) {
			timeoutEvent.store(nullptr);
			pEvent->onTimeout();
		}
	}

	LuaScriptCallGuard::LuaScriptCallGuard(___pdr_lua_State * L_)
		:L(L_)
	{
		auto ls = LuaState::get(L);
		ls->deadLoopCheck->scriptEnter(this);
	}

	LuaScriptCallGuard::~LuaScriptCallGuard()
	{
		auto ls = LuaState::get(L);
		ls->deadLoopCheck->scriptLeave();
	}

	void LuaScriptCallGuard::onTimeout()
	{
		auto hook = ___pdr_lua_gethook(L);
		// if debugger isn't exists
		if (hook == nullptr) {
			// this function thread safe
			___pdr_lua_sethook(L, scriptTimeout, ___PDR_LUA_MASKLINE, 0);
		}
	}

	void LuaScriptCallGuard::scriptTimeout(___pdr_lua_State *L, ___pdr_lua_Debug *ar)
	{
		// only report once
		___pdr_lua_sethook(L, nullptr, 0, 0);
		___pdr_luaL_error(L, "script exec timeout");
	}

	UFunction* LuaState::ClassCache::findFunc(UClass* uclass, const char* fname)
	{
		auto item = cacheFuncMap.Find(uclass);
		if (!item) return nullptr;
		auto func = item->Find(UTF8_TO_TCHAR(fname));
		if(func!=nullptr)
			return func->IsValid() ? func->Get() : nullptr;
		return nullptr;
	}

	UProperty* LuaState::ClassCache::findProp(UClass* uclass, const char* pname)
	{
		auto item = cachePropMap.Find(uclass);
		if (!item) return nullptr;
		auto prop = item->Find(UTF8_TO_TCHAR(pname));
		if (prop != nullptr)
			return prop->IsValid() ? prop->Get() : nullptr;
		return nullptr;
	}

	void LuaState::ClassCache::cacheFunc(UClass* uclass, const char* fname, UFunction* func)
	{
		auto& item = cacheFuncMap.FindOrAdd(uclass);
		item.Add(UTF8_TO_TCHAR(fname), func);
	}

	void LuaState::ClassCache::cacheProp(UClass* uclass, const char* pname, UProperty* prop)
	{
		auto& item = cachePropMap.FindOrAdd(uclass);
		item.Add(UTF8_TO_TCHAR(pname), prop);
	}
}
