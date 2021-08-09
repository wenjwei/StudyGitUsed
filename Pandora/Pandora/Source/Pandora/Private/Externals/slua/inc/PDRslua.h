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
#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif
#include "lua/lua.hpp"
#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif
#include "PDRSluaLog.h"
//#include "SLuaUtil.h"
#include "Pandora/Private/Externals/slua/inc/PDRSluaUtil.h"
#include "PDRLuaObject.h"
#include "PDRLuaState.h"
#include "PDRLuaVar.h"
#include "PDRLuaArray.h"
#include "PDRLuaMap.h"
#include "PDRLuaBase.h"
#include "PDRLuaActor.h"
#include "PDRLuaDelegate.h"
#include "PDRLuaCppBinding.h"
#include "PDRLuaCppBindingPost.h"