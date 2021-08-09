#include "PDNSResolver.h"
#include "SocketSubsystem.h"
#include "PLuaStateMgr.h"
#include "PDRLuaCppBinding.h"
#include "IPAddress.h"
#include "HAL/RunnableThread.h"
#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include "HideWindowsPlatformTypes.h"
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
//#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>
#endif

namespace pandora
{
//using namespace NS_PDR_SLUA;

inline int PushFAddressInfoResultData(NS_PDR_SLUA::___pdr_lua_State* L, const FString& ip)
{
    ___pdr_lua_newtable(L);
    ___pdr_lua_pushstring(L, TCHAR_TO_UTF8(*ip));
    ___pdr_lua_setfield(L, -2, "IP");
    return 1;
}

inline int PushFAddressInfoResult(NS_PDR_SLUA::___pdr_lua_State* L, const PDNSResolver::DNSCache& result)
{
    ___pdr_lua_newtable(L);
    ___pdr_lua_pushstring(L, TCHAR_TO_UTF8(*result.HostName));
    ___pdr_lua_setfield(L, -2, "HostName");
    ___pdr_lua_pushstring(L, TCHAR_TO_UTF8(*result.ServiceName));
    ___pdr_lua_setfield(L, -2, "ServiceName");
    ___pdr_lua_newtable(L);
    const TArray<FString>& results = result.Results;
    for (int i = 0; i < results.Num(); ++i)
    {
        PushFAddressInfoResultData(L, results[i]);
        ___pdr_lua_seti(L, -2, i + 1);
    }
    ___pdr_lua_setfield(L, -2, "Results");

    return 1;
}

inline void SetHintsFamily(addrinfo& hints, ESocketProtocolFamily family)
{
    switch (family)
    {
    case ESocketProtocolFamily::IPv4:
        hints.ai_family = AF_INET;
        break;
    case ESocketProtocolFamily::IPv6:
        hints.ai_family = AF_INET6;
        break;
    case ESocketProtocolFamily::None:
        hints.ai_family = AF_UNSPEC;
        break;
    default:
        hints.ai_family = AF_UNSPEC;
        break;
    }
}

inline void SetHintsSockType(addrinfo& hints, ESocketType socktype)
{
    switch (socktype) 
    {
    case ESocketType::SOCKTYPE_Datagram:
        hints.ai_socktype = SOCK_DGRAM;
        break;
    case ESocketType::SOCKTYPE_Streaming:
        hints.ai_socktype = SOCK_STREAM;
        break;
    case ESocketType::SOCKTYPE_Unknown:
        hints.ai_socktype = SOCK_STREAM;
        break;
    }
}

PDNSResolverWorker::PDNSResolverWorker()
{
    working.Increment();
    thread = FRunnableThread::Create(this, TEXT("PDNSResolverWorker"), 0, TPri_Normal);
    if (thread == nullptr)
        working.Reset();
}

PDNSResolverWorker::~PDNSResolverWorker()
{
    if (thread != nullptr)
    {
        delete thread;
        thread = nullptr;
    }
}

bool PDNSResolverWorker::Init()
{
    return true;
}

uint32 PDNSResolverWorker::Run()
{
    PDNSResolver& resolver = PDNSResolver::Get();
    while (!resolver.requests.IsEmpty() && IsWorking())
    {
        PDNSResolver::DNSResolveRequest req;
        if (!resolver.requests.Dequeue(req))
            continue;

        PDNSResolver::Get().requestCount.Decrement();
        PDNSResolver::DNSResolveResponse resp;

        struct addrinfo* result = NULL;
        struct addrinfo* ptr = NULL;
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));

        SetHintsFamily(hints, req.ProtocolType);
        SetHintsSockType(hints, req.SocketType);
        hints.ai_protocol = IPPROTO_TCP;
        int err = getaddrinfo(TCHAR_TO_UTF8(*req.HostName), 0, &hints, &result);
        PDNSResolver::DNSCache dnscache;
        dnscache.HostName = req.HostName;
        dnscache.ServiceName = req.ServiceName;
        resp.Result = dnscache;
        if (err == 0)
        {
            for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
            {
                char s[64] = "";
                const char* p = NULL;
                if (ptr->ai_family == AF_INET)
                {
                    struct sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*) ptr->ai_addr;
                    p = inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, s, sizeof(*sockaddr_ipv4));
                    if (p)
                    {
                        resp.Result.Results.Add(UTF8_TO_TCHAR(p));
                    }
                }
                else if (ptr->ai_family == AF_INET6)
                {
                    struct sockaddr_in6* sockaddr_ipv6 = (struct sockaddr_in6*) ptr->ai_addr;
                    p = inet_ntop(AF_INET6, &sockaddr_ipv6->sin6_addr, s, sizeof(*sockaddr_ipv6));
                    if (p)
                    {
                        resp.Result.Results.Add(UTF8_TO_TCHAR(p));
                    }
                }
            }
            freeaddrinfo(result);
        }

        resolver.responses.Enqueue(resp);

        // run slowly
        FPlatformProcess::Sleep(0.1);
    }

    return 0;
}

void PDNSResolverWorker::Stop()
{
    working.Reset();
}

void PDNSResolverWorker::Exit()
{
    working.Reset();
}

bool PDNSResolverWorker::IsWorking()
{
    return (!(working.GetValue() == 0));
}

PDNSResolver::PDNSResolver()
{
    requestCount.Reset();
}

PDNSResolver& PDNSResolver::Get()
{
    static PDNSResolver _pandoraDNSResolver;
    return _pandoraDNSResolver;
}

bool PDNSResolver::GetAddressInfo(DNSCache& result, const FString & host, const FString & service,
    EAddressInfoFlags queryFlags, ESocketProtocolFamily protocolType, ESocketType socketType)
{
    FString key = GetKey(host, service);
    if (cache.Contains(key))
    {
        result = cache[key];
        return true;
    }

    if (processings.Contains(key))
        return false;

    processings.Add(key);

    // if not receiving, start receiving
    if (!isReceiving)
        StartReceiver();

    // push request, wait for worker to resolve and return
    if (requests.Enqueue(DNSResolveRequest(host, service, queryFlags, protocolType, socketType)))
    {
        requestCount.Increment();
    }
    return false;
}

void PDNSResolver::GetAddressInfo(const FString & host, const FString & service,
    EAddressInfoFlags queryFlags, ESocketProtocolFamily protocolType, ESocketType socketType, NS_PDR_SLUA::LuaVar cb)
{
    FString key = GetKey(host, service);
    DNSCache result;
    if (GetAddressInfo(result, host, service, queryFlags, protocolType, socketType))
    {
        PushFAddressInfoResult(cb.getState(), result);
        cb.callWithNArg(1);
        return;
    }

    if (!luaCallbacks.Contains(key))
    {
        luaCallbacks.Add(key, TArray<NS_PDR_SLUA::LuaVar>());
    }
    TArray<NS_PDR_SLUA::LuaVar>& lcbarr = luaCallbacks[key];
    lcbarr.Add(cb);
}

FString PDNSResolver::GetKey(const FString& host, const FString& port)
{
    return host + ":" + port;
}

void PDNSResolver::ReceiveResult()
{
    if (processings.Num() == 0 && /*requests.IsEmpty()*/ requestCount.GetValue() == 0 && responses.IsEmpty())
    {
        // no more data is needed to process, exit
        if (worker)
        {
            if (!worker->IsWorking())
            {
                delete worker;
                worker = nullptr;
                // stop ticking
                StopReceiver();
            }
        }
        else
        {
            // stop ticking
            StopReceiver();
        }
    }

    WakeupWorker();

    while (!responses.IsEmpty())
    {
        DNSResolveResponse resp;
        if (!responses.Dequeue(resp))
            break;
        DNSCache result = resp.Result;
        FString key = GetKey(result.HostName, result.ServiceName);
        processings.Remove(key);
        // only when we get valid result, we add it to cache
        if (result.Results.Num() > 0)
            cache.Add(key, result);
        OnAddressResolved(key, result);
    }
}

void PDNSResolver::Clear()
{
    for (auto& i : luaCallbacks)
        i.Value.Empty();
    luaCallbacks.Empty();
}

void PDNSResolver::WakeupWorker()
{
    //if (requests.IsEmpty())
    if (requestCount.GetValue() == 0)
        // no more data for worker to consume, do not wake it up
        return;

    if (worker != nullptr)
    {
        // if worker stop working, then restart it
        if (!worker->IsWorking())
        {
            delete worker;
            worker = nullptr;
            worker = new PDNSResolverWorker();
        }
    }
    else
    {
        // if no worker is created, create one
        worker = new PDNSResolverWorker();
    }
}

void PDNSResolver::StartReceiver()
{
    if (isReceiving)
        return;

    FTimerDelegate tDelegate;
    tDelegate.BindLambda([this]() {
        this->ReceiveResult();
    });
    PLuaStateMgr::GetGameInstance()->GetTimerManager().SetTimer(receiverHandle, tDelegate, 0.1f, true);
    isReceiving = true;
}

void PDNSResolver::StopReceiver()
{
    if (!isReceiving)
        return;

    if (receiverHandle.IsValid())
        PLuaStateMgr::GetGameInstance()->GetTimerManager().ClearTimer(receiverHandle);
    isReceiving = false;
}

void PDNSResolver::OnAddressResolved(const FString & key, const DNSCache& result)
{
    DoLuaCallbacks(key, result);
}

void PDNSResolver::DoLuaCallbacks(const FString & key, const DNSCache& result)
{
    if (!luaCallbacks.Contains(key))
        return;

    TArray<NS_PDR_SLUA::LuaVar> lcbarr = luaCallbacks[key];
    for (int i = 0; i < lcbarr.Num(); ++i)
    {
        if (lcbarr[i].isValid())
        {
            PushFAddressInfoResult(lcbarr[i].getState(), result);
            lcbarr[i].callWithNArg(1);
        }
    }

    luaCallbacks.Remove(key);
}
}

namespace NS_PDR_SLUA
{
using pandora::PDNSResolver;

DefLuaClass(PDNSResolver)
    DefLuaMethod_With_Lambda(Get, true, []()->PDNSResolver*{
        return &PDNSResolver::Get();
    })
    DefLuaMethod_With_Imp(GetAddressInfo, false, {
        CheckUD(PDNSResolver, lstack, 1);
        if (!UD) ___pdr_luaL_argcheck(lstack, false, 1, "'PDNSResolver' expected");
        const char * host = ___pdr_luaL_checkstring(lstack, 2);
        const char * serv = ___pdr_luaL_checkstring(lstack, 3);
        int flags = ___pdr_luaL_checkinteger(lstack, 4);
        int family = ___pdr_luaL_checkinteger(lstack, 5);
        int sockType = ___pdr_luaL_checkinteger(lstack, 6);
        LuaVar cb = LuaVar(lstack, 7, LuaVar::LV_FUNCTION);
        UD->GetAddressInfo(UTF8_TO_TCHAR(host), UTF8_TO_TCHAR(serv), 
            (EAddressInfoFlags)flags, (ESocketProtocolFamily)family, (ESocketType)sockType, cb);
        return 0;
    })
EndDef(PDNSResolver, nullptr)
}
