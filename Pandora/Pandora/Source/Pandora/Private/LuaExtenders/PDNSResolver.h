#pragma once

#include "Containers/Map.h"
#include "Containers/Set.h"
#include "Containers/Queue.h"

#if ((ENGINE_MINOR_VERSION<19) && (ENGINE_MAJOR_VERSION>=4))
#include "PSocketEnums.h"
#else
#include "AddressInfoTypes.h"
#endif

#include "HAL/CriticalSection.h" 
#include "HAL/Runnable.h"
#include "TimerManager.h"
#include "PDRLuaVar.h"

class FRunnableThread;

namespace pandora
{
class PDNSResolver;

class PDNSResolverWorker : public FRunnable
{
private:
    FRunnableThread * thread;
    FThreadSafeCounter working;
public:
    PDNSResolverWorker();
    virtual ~PDNSResolverWorker();

    virtual bool Init();
    virtual uint32 Run();
    virtual void Stop();
    virtual void Exit();

    bool IsWorking();
};


class PDNSResolver
{
    friend PDNSResolverWorker;

public:
    struct DNSResolveRequest
    {
        FString HostName;
        FString ServiceName;
        EAddressInfoFlags QueryFlags;
        ESocketProtocolFamily ProtocolType;
        ESocketType SocketType;

        DNSResolveRequest() {}

        DNSResolveRequest(const FString& host, const FString& service,
            EAddressInfoFlags flags, ESocketProtocolFamily family, ESocketType socktype) :
            HostName(host), ServiceName(service), QueryFlags(flags), ProtocolType(family), SocketType(socktype)
        { }

        DNSResolveRequest(const DNSResolveRequest& src)
        {
            HostName = src.HostName;
            ServiceName = src.ServiceName;
            QueryFlags = src.QueryFlags;
            ProtocolType = src.ProtocolType;
            SocketType = src.SocketType;
        }
    };

    struct DNSCache {
        FString HostName;
        FString ServiceName;
        TArray<FString> Results;
    };

    struct DNSResolveResponse
    {
        DNSCache Result;

        DNSResolveResponse() {}
    };


    //typedef void(*OnAddressResolvedCallback)(const FAddressInfoResult& result);

    virtual ~PDNSResolver() {};
    static PDNSResolver& Get();

    /*
    * c like async interface, return true when address is resolved, otherwise return false
    * when true is returned, result will fill in the first argument
    */
    bool GetAddressInfo(DNSCache& result, const FString & host, const FString & service,
        EAddressInfoFlags queryFlags, ESocketProtocolFamily protocolType, ESocketType socketType);

    /*
    * async interface, start a resolve task
    * when task done, cb will be called
    */
    void GetAddressInfo(const FString & host, const FString & service,
        EAddressInfoFlags queryFlags, ESocketProtocolFamily protocolType, ESocketType socketType, NS_PDR_SLUA::LuaVar cb);

    //void GetAddressInfo(const FString & host, const FString & service,
        //EAddressInfoFlags queryFlags, ESocketProtocolFamily protocolType, ESocketType socketType, OnAddressResolvedCallback cb);

    FString GetKey(const FString& host, const FString& port);

    void ReceiveResult();

    // only clear callbacks
    // if any host is resolving, leave it there
    void Clear();

private:
    PDNSResolverWorker * worker = nullptr;
    FThreadSafeCounter requestCount;
    TMap<FString, DNSCache> cache;
    TSet<FString> processings;
    TMap<FString, TArray<NS_PDR_SLUA::LuaVar>> luaCallbacks;
    //TMap<FString, TSet<OnAddressResolvedCallback>> callbacks;
    TQueue<DNSResolveRequest, EQueueMode::Spsc> requests;
    TQueue<DNSResolveResponse, EQueueMode::Spsc> responses;
    FTimerHandle receiverHandle;
    bool isReceiving;
        
    PDNSResolver();
    void WakeupWorker();
    void StartReceiver();
    void StopReceiver();
    void OnAddressResolved(const FString & key, const DNSCache& result);
    void DoLuaCallbacks(const FString & key, const DNSCache& result);
};
}
