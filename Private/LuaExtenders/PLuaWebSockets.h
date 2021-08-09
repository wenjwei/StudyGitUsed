#pragma once

#if PDR_ENABLE_WEBSOCKETS
#include "Platform.h"
#include "SharedPointer.h"
#include "IWebSocket.h"
#include "PDRLuaObject.h"
#include "PDRslua.h"
#include "cppbuffer.h"

class FString;

namespace pandora
{
class PLuaWebSocketWrapper
{
public:
    PLuaWebSocketWrapper(const TSharedRef<IWebSocket>& , NS_CPPBUFFER::CB_EENDIAN);
    ~PLuaWebSocketWrapper();
    void Connect();
    void Close();
    bool IsConnected();
    void Send(const FString& data);
    void Send(const void* data, SIZE_T size, bool isBinary = false);
    void BindOnConnected(NS_PDR_SLUA::LuaVar func);
    void BindOnConnectionError(NS_PDR_SLUA::LuaVar func);
    void BindOnClosed(NS_PDR_SLUA::LuaVar func);
    void BindOnMessage(NS_PDR_SLUA::LuaVar func);
    void BindOnRawMessage(NS_PDR_SLUA::LuaVar func);

private:
    TSharedRef<IWebSocket> socket;
    NS_CPPBUFFER::CB_EENDIAN endian;
    NS_PDR_SLUA::LuaVar luaFuncOnConnected;
    NS_PDR_SLUA::LuaVar luaFuncOnConnectionError;
    NS_PDR_SLUA::LuaVar luaFuncOnClosed;
    NS_PDR_SLUA::LuaVar luaFuncOnMessage;
    NS_PDR_SLUA::LuaVar luaFuncOnRawMessage;
    void OnConnected();
    void OnConnectionError(const FString& err);
    void OnClosed(int32 statusCode, const FString& reason, bool wasClean);
    void OnMessage(const FString& message);
    void OnRawMessage(const void* data, SIZE_T size, SIZE_T bytesRemaining);
};
}

#endif
