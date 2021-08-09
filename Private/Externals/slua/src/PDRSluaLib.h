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
#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif
#include "lua/lua.hpp"
#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif

namespace NS_PDR_SLUA {

    // used for lua interface
    class SluaUtil {
    public:
        static void openLib(___pdr_lua_State* L);
        static void reg(___pdr_lua_State* L,const char* fn,___pdr_lua_CFunction f);
    private:
        static int loadUI(___pdr_lua_State* L);
        static int loadClass(___pdr_lua_State* L);
		static int createDelegate(___pdr_lua_State* L);

		// remote profile
		static int setTickFunction(___pdr_lua_State* L);
		static int makeProfilePackage(___pdr_lua_State* L);
		static int getMicroseconds(___pdr_lua_State* L);
		static int getMiliseconds(___pdr_lua_State* L);
		static int loadObject(___pdr_lua_State* L);
		static int threadGC(___pdr_lua_State* L);
		// dump all uobject that referenced by lua
		static int dumpUObjects(___pdr_lua_State* L);
		// return whether an userdata is valid?
		static int isValid(___pdr_lua_State* L);
    };

}