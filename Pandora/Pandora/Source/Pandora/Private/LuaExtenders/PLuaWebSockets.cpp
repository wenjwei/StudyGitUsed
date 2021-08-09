#include "PLuaWebSockets.h"

#if PDR_ENABLE_WEBSOCKETS
#include "Modules/ModuleManager.h"
#include "PDRslua.h"
#include "WebSockets/Public/WebSocketsModule.h"
#include "IWebSocket.h"
#include "UnrealString.h"
#include "Containers/Map.h"
#include "SharedPointer.h"
#include "PDRLuaCppBinding.h"
#include "cppbuffer.h"
#include "luaadapter.h"

namespace pandora
{
#define PLUAWEBSOCKET_BIND_EVENT(NAME) socket->NAME().AddRaw(this, &PLuaWebSocketWrapper::NAME)
PLuaWebSocketWrapper::PLuaWebSocketWrapper(const TSharedRef<IWebSocket>& webSocket, NS_CPPBUFFER::CB_EENDIAN dataEndian) 
    : socket(webSocket), endian(dataEndian)
{
    PLUAWEBSOCKET_BIND_EVENT(OnConnected);
    PLUAWEBSOCKET_BIND_EVENT(OnConnectionError);
    PLUAWEBSOCKET_BIND_EVENT(OnClosed);
    PLUAWEBSOCKET_BIND_EVENT(OnMessage);
    PLUAWEBSOCKET_BIND_EVENT(OnRawMessage);
}
#undef PLUAWEBSOCKET_BIND_EVENT

#define PLUAWEBSOCKET_FREE_LUA_FUNC(NAME) if (NAME.isValid()) NAME.free();
PLuaWebSocketWrapper::~PLuaWebSocketWrapper()
{
    PLUAWEBSOCKET_FREE_LUA_FUNC(luaFuncOnConnected);
    PLUAWEBSOCKET_FREE_LUA_FUNC(luaFuncOnConnectionError);
    PLUAWEBSOCKET_FREE_LUA_FUNC(luaFuncOnClosed);
    PLUAWEBSOCKET_FREE_LUA_FUNC(luaFuncOnMessage);
    PLUAWEBSOCKET_FREE_LUA_FUNC(luaFuncOnRawMessage);
    Close();
}
#undef PLUAWEBSOCKET_FREE_LUA_FUNC

void PLuaWebSocketWrapper::Connect()
{
    socket->Connect();
}

void PLuaWebSocketWrapper::Close()
{
    socket->Close();
}

bool PLuaWebSocketWrapper::IsConnected()
{
    return socket->IsConnected();
}

void PLuaWebSocketWrapper::Send(const FString& data)
{
    socket->Send(data);
}

void PLuaWebSocketWrapper::Send(const void* data, SIZE_T size, bool isBinary /* = false */)
{
    socket->Send(data, size, isBinary);
}

#define PLUAWEBSOCKET_BIND_FUNC_BODY(NAME) \
void PLuaWebSocketWrapper::Bind##NAME(NS_PDR_SLUA::LuaVar func) {\
    if (luaFunc##NAME.isValid()) luaFunc##NAME.free();\
    luaFunc##NAME = func;\
}

PLUAWEBSOCKET_BIND_FUNC_BODY(OnConnected)
PLUAWEBSOCKET_BIND_FUNC_BODY(OnConnectionError)
PLUAWEBSOCKET_BIND_FUNC_BODY(OnClosed)
PLUAWEBSOCKET_BIND_FUNC_BODY(OnMessage)
PLUAWEBSOCKET_BIND_FUNC_BODY(OnRawMessage)

#undef PLUAWEBSOCKET_BIND_FUNC_BODY

void PLuaWebSocketWrapper::OnConnected()
{
    if (!luaFuncOnConnected.isValid())
        return;
    luaFuncOnConnected.call();
}

void PLuaWebSocketWrapper::OnConnectionError(const FString& err)
{
    if (!luaFuncOnConnectionError.isValid())
        return;
    luaFuncOnConnectionError.call(err);
}

void PLuaWebSocketWrapper::OnClosed(int32 statusCode, const FString& reason, bool wasClean)
{
    if (!luaFuncOnClosed.isValid())
        return;
    luaFuncOnClosed.call(statusCode, reason, wasClean);
}

void PLuaWebSocketWrapper::OnMessage(const FString& message)
{
    if (!luaFuncOnMessage.isValid())
        return;
    luaFuncOnMessage.call(message);
}

void PLuaWebSocketWrapper::OnRawMessage(const void* data, SIZE_T size, SIZE_T bytesRemaining)
{
    if (!luaFuncOnRawMessage.isValid())
        return;
    NS_PDR_SLUA::___pdr_lua_State* L = luaFuncOnRawMessage.getState();
    if (!L)
        return;
    // TODO: BUILD A LUABUFFER AND COPY DATA INTO IT
    NS_CPPBUFFER::p_cppbuffer buffer = NS_CB_LUAADAPTER::newlbuffer(L, size, endian);
    NS_CPPBUFFER::cb_write_raw(buffer, (char*)data, size);
    ___pdr_lua_pushinteger(L, bytesRemaining);
    luaFuncOnRawMessage.callWithNArg(2);
}
    
int CreateWebSocket(NS_PDR_SLUA::___pdr_lua_State* L)
{
    ___pdr_lua_settop(L, 5);
    FWebSocketsModule* m = NS_PDR_SLUA::LuaObject::checkUD<FWebSocketsModule>(L, 1);
    if (!m)
        ___pdr_luaL_argcheck(L, false, 1, "'FWebSocketsModule' expected");
    if (!___pdr_lua_isstring(L, 2))
        ___pdr_luaL_argcheck(L, false, 2, "'string' expected");
    FString url = UTF8_TO_TCHAR(___pdr_luaL_checkstring(L, 2));
    FString protocol = UTF8_TO_TCHAR("wss"); // default;
    if (!___pdr_lua_isnil(L, 3))
    {
        if (!___pdr_lua_isstring(L, 3))
            ___pdr_luaL_argcheck(L, false, 3, "'string' expected");
        protocol = UTF8_TO_TCHAR(___pdr_luaL_checkstring(L, 3));
    }
    TMap<FString, FString> upgradeHeaders;
    if (!___pdr_lua_isnil(L, 4))
    {
        if (!___pdr_lua_istable(L, 4))
            ___pdr_luaL_argcheck(L, false, 4, "'table' expected");
        ___pdr_lua_pushnil(L);
        FString key, val;
        while (___pdr_lua_next(L, -2) != 0)
        {
            if (!___pdr_lua_isstring(L, -2) || !___pdr_lua_isstring(L, -1))
                ___pdr_luaL_argcheck(L, false, 4, "table content invalid");

            key = UTF8_TO_TCHAR(___pdr_luaL_checkstring(L, -2));
            val = UTF8_TO_TCHAR(___pdr_luaL_checkstring(L, -1));
            upgradeHeaders.Add(key, val);
            ___pdr_lua_pop(L, 1);
        }
        ___pdr_lua_pop(L, 1);
    }

    NS_CPPBUFFER::CB_EENDIAN endian = NS_CPPBUFFER::CB_BIG_ENDIAN;
    if (!___pdr_lua_isnil(L, 5))
    {
        if (!___pdr_lua_isinteger(L, 5)) ___pdr_luaL_argcheck(L, false, 5, "'integer' expected");
        ___PDR_LUA_INTEGER val = ___pdr_luaL_checkinteger(L, 5);
        if (val != 0 && val != 1) ___pdr_luaL_argcheck(L, false, 5, "only accept 0 or 1, 0 for big endian, 1 for little endian");
        if (val == 1) endian = NS_CPPBUFFER::CB_EENDIAN::CB_LITTLE_ENDIAN;
    }
        
    TSharedRef<IWebSocket> webSocket = m->CreateWebSocket(url, protocol, upgradeHeaders);
    TSharedPtr<PLuaWebSocketWrapper> wrapper = MakeShareable<PLuaWebSocketWrapper>(new PLuaWebSocketWrapper(webSocket, endian));
    return NS_PDR_SLUA::LuaObject::push(L, wrapper);
}
}

namespace NS_PDR_SLUA
{
using pandora::PLuaWebSocketWrapper;
#define PLUAWEBSOCKET_EXPORT_BIND_FUNC_BODY(NAME) \
    DefLuaMethod_With_Imp(Bind##NAME, false, { \
        CheckUD(PLuaWebSocketWrapper, lstack, 1); \
        if (!UD) ___pdr_luaL_argcheck(lstack, false, 1, "'PLuaWebSocketWrapper' expected"); \
        if (!___pdr_lua_isfunction(lstack, 2)) ___pdr_luaL_argcheck(lstack, false, 2, "'function' expected"); \
        LuaVar func = LuaVar(lstack, 2, LuaVar::Type::LV_FUNCTION); \
        UD->Bind##NAME(func); \
        return 0; \
    })

DefLuaClass(PLuaWebSocketWrapper)
    DefLuaMethod(Connect, &PLuaWebSocketWrapper::Connect)
    DefLuaMethod(Close, &PLuaWebSocketWrapper::Close)
    DefLuaMethod(IsConnected, &PLuaWebSocketWrapper::IsConnected)
    DefLuaMethod_With_Imp(SendMessage, false, {
        CheckUD(PLuaWebSocketWrapper, lstack, 1);
        if (!UD) ___pdr_luaL_argcheck(lstack, false, 1, "'PLuaWebSocketWrapper' expected");
        if (!___pdr_lua_isstring(lstack, 2)) ___pdr_luaL_argcheck(lstack, false, 2, "'string' expected");
        FString content = UTF8_TO_TCHAR(___pdr_luaL_checkstring(lstack, 2));
        UD->Send(content);
        return 0;
        })
    DefLuaMethod_With_Imp(SendBuffer, false, {
        CheckUD(PLuaWebSocketWrapper, lstack, 1);
        if (!UD) ___pdr_luaL_argcheck(lstack, false, 1, "'PLuaWebSocketWrapper' expected");
        NS_CPPBUFFER::p_cppbuffer buffer = NS_CB_LUAADAPTER::checkbuffer(lstack, 2);
        size_t size = NS_CPPBUFFER::cb_get_bytes_count(buffer);
        void * data = FMemory::Malloc(size);
        NS_CPPBUFFER::cb_read_raw(buffer, (char*)data, size);
        UD->Send(data, size, true);
        FMemory::Free(data);
        return 0;
        })
    PLUAWEBSOCKET_EXPORT_BIND_FUNC_BODY(OnConnected)
    PLUAWEBSOCKET_EXPORT_BIND_FUNC_BODY(OnConnectionError)
    PLUAWEBSOCKET_EXPORT_BIND_FUNC_BODY(OnClosed)
    PLUAWEBSOCKET_EXPORT_BIND_FUNC_BODY(OnMessage)
    PLUAWEBSOCKET_EXPORT_BIND_FUNC_BODY(OnRawMessage)
EndDef(PLuaWebSocketWrapper, nullptr)

#undef PLUAWEBSOCKET_EXPORT_BIND_FUNC_BODY

DefLuaClass(FWebSocketsModule)
    DefLuaMethod_With_Lambda(Get, true, []()->FWebSocketsModule* {
        FWebSocketsModule& m = FModuleManager::LoadModuleChecked<FWebSocketsModule>(TEXT("WebSockets"));
        return &m;
    })
    DefLuaMethod_With_Imp(CreateWebSocket, false, {
        return pandora::CreateWebSocket(lstack);
    })
EndDef(FWebSocketsModule, nullptr)
}

#endif
