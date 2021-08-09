// Tencent is pleased to support the open source community by making sluaunreal available.

// Copyright (C) 2018 THL A29 Limited, a Tencent company. All rights reserved.
// Licensed under the BSD 3-Clause License (the "License"); 
// you may not use this file except in compliance with the License. You may obtain a copy of the License at

// https://opensource.org/licenses/BSD-3-Clause

// Unless required by applicable law or agreed to in writing, 
// software distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.

#include "PDRLuaSocketWrap.h"
#include "LuaSocketExt.h"
#include "PDRLuaObject.h"
#include "PDRLuaState.h"
#include "luaadapter.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif
#include "luasocket/luasocket.h"
#include "luasocket/mime.h"
#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif


namespace NS_PDR_SLUA {

    namespace LuaSocket {

        int luaopen_url(___pdr_lua_State *L) {
            #include "luasocket/url.lua.bin"
            ___pdr_luaL_loadbuffer(L, (const char *)url_bytes, sizeof(url_bytes), "url.lua");
            ___pdr_lua_pcall(L, 0, ___PDR_LUA_MULTRET, 0);
            return 1;
        }

        int luaopen_tp(___pdr_lua_State *L) {
            #include "luasocket/tp.lua.bin"
            ___pdr_luaL_loadbuffer(L, (const char *)tp_bytes, sizeof(tp_bytes), "tp.lua");
            ___pdr_lua_pcall(L, 0, ___PDR_LUA_MULTRET, 0);
            return 1;
        }

        int luaopen_socket(___pdr_lua_State *L) {
            NS_LUASOCKET_EXT::Extend(L);
            #include "luasocket/socket.lua.bin"
            ___pdr_luaL_loadbuffer(L, (const char *)socket_bytes, sizeof(socket_bytes), "socket.lua");
            ___pdr_lua_pcall(L, 0, ___PDR_LUA_MULTRET, 0);
            return 1;
        }

        int luaopen_smtp(___pdr_lua_State *L) {
            #include "luasocket/smtp.lua.bin"
            ___pdr_luaL_loadbuffer(L, (const char *)smtp_bytes, sizeof(smtp_bytes), "smtp.lua");
            ___pdr_lua_pcall(L, 0, ___PDR_LUA_MULTRET, 0);
            return 1;
        }

        int luaopen_mime(___pdr_lua_State *L) {
            #include "luasocket/mime.lua.bin"
            ___pdr_luaL_loadbuffer(L, (const char *)mime_bytes, sizeof(mime_bytes), "mime.lua");
            ___pdr_lua_pcall(L, 0, ___PDR_LUA_MULTRET, 0);
            return 1;
        }

        int luaopen_mbox(___pdr_lua_State *L) {
            #include "luasocket/mbox.lua.bin"
            ___pdr_luaL_loadbuffer(L, (const char *)mbox_bytes, sizeof(mbox_bytes), "mbox.lua");
            ___pdr_lua_pcall(L, 0, ___PDR_LUA_MULTRET, 0);
            return 1;
        }

        int luaopen_ltn12(___pdr_lua_State *L) {
            #include "luasocket/ltn12.lua.bin"
            ___pdr_luaL_loadbuffer(L, (const char *)ltn12_bytes, sizeof(ltn12_bytes), "ltn12.lua");
            ___pdr_lua_pcall(L, 0, ___PDR_LUA_MULTRET, 0);
            return 1;
        }

        int luaopen_socket_headers(___pdr_lua_State *L) {
            #include "luasocket/headers.lua.bin"
            ___pdr_luaL_loadbuffer(L, (const char *)headers_bytes, sizeof(headers_bytes), "headers.lua");
            ___pdr_lua_pcall(L, 0, ___PDR_LUA_MULTRET, 0);
            return 1;
        }

        int luaopen_http(___pdr_lua_State *L) {
            #include "luasocket/http.lua.bin"
            ___pdr_luaL_loadbuffer(L, (const char *)http_bytes, sizeof(http_bytes), "http.lua");
            ___pdr_lua_pcall(L, 0, ___PDR_LUA_MULTRET, 0);
            return 1;
        }

        int luaopen_ftp(___pdr_lua_State *L) {
            #include "luasocket/ftp.lua.bin"
            ___pdr_luaL_loadbuffer(L, (const char *)ftp_bytes, sizeof(ftp_bytes), "ftp.lua");
            ___pdr_lua_pcall(L, 0, ___PDR_LUA_MULTRET, 0);
            return 1;
        }

        void init(___pdr_lua_State *L) {
            ___pdr_luaL_getsubtable(L, ___PDR_LUA_REGISTRYINDEX, ___PDR_LUA_PRELOAD_TABLE);

            ___pdr_lua_pushcfunction(L, ___pdr_luaopen_socket_core);
            ___pdr_lua_setfield(L, -2, "socket.core");

            ___pdr_lua_pushcfunction(L, luaopen_socket_headers);
            ___pdr_lua_setfield(L, -2, "socket.headers");

            ___pdr_lua_pushcfunction(L, luaopen_mime_core);
            ___pdr_lua_setfield(L, -2, "mime.core");

            ___pdr_lua_pushcfunction(L, luaopen_url);
            ___pdr_lua_setfield(L, -2, "socket.url");

            ___pdr_lua_pushcfunction(L, luaopen_tp);
            ___pdr_lua_setfield(L, -2, "socket.tp");

            ___pdr_lua_pushcfunction(L, luaopen_socket);
            ___pdr_lua_setfield(L, -2, "socket");

            ___pdr_lua_pushcfunction(L, luaopen_smtp);
            ___pdr_lua_setfield(L, -2, "socket.smtp");

            ___pdr_lua_pushcfunction(L, luaopen_mime);
            ___pdr_lua_setfield(L, -2, "mime");

            ___pdr_lua_pushcfunction(L, luaopen_mbox);
            ___pdr_lua_setfield(L, -2, "mbox");

            ___pdr_lua_pushcfunction(L, luaopen_ltn12);
            ___pdr_lua_setfield(L, -2, "ltn12");
             
            ___pdr_lua_pushcfunction(L, luaopen_http);
            ___pdr_lua_setfield(L, -2, "socket.http");

            ___pdr_lua_pushcfunction(L, luaopen_ftp);
            ___pdr_lua_setfield(L, -2, "socket.ftp");

			___pdr_lua_pushcfunction(L, NS_CB_LUAADAPTER::open);
			___pdr_lua_setfield(L, -2, "lbuffer");

            ___pdr_lua_pop(L, 1);
        }
    }
}