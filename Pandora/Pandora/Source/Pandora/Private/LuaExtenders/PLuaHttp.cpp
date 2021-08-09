// Fill out your copyright notice in the Description page of Project Settings.

#include "PLuaHttp.h"
#include "Modules/ModuleManager.h"
#include "PDRslua.h"
#include "CoreMinimal.h"
#include "HttpModule.h"
#include "IHttpRequest.h"
#include "IHttpResponse.h"
#include "PDRLuaObject.h"
#include "PDRLuaCppBinding.h"
#include "GenericPlatformHttp.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"
#include "luaadapter.h"
#include "PLuaHttpForm.h"

namespace NS_PDR_SLUA
{
DefLuaClass(IHttpResponse)
    DefLuaMethod(GetResponseCode, &IHttpResponse::GetResponseCode)
    DefLuaMethod(GetContentAsString, &IHttpResponse::GetContentAsString)
    DefLuaMethod_With_Imp(GetContentAsLBuffer, false, {
        if (!NS_CB_LUAADAPTER::ismtcreated(lstack))
        {
            UE_LOG(LogTemp, Warning, TEXT("lbuffer is not required yet, please require lbuffer first"));
            ___pdr_lua_pushnil(lstack);
            return 1;
        }
        CheckUD(IHttpResponse, lstack, 1);
        if (!UD) ___pdr_luaL_argcheck(lstack, false, 1, "'IHttpResponse' expected");
        auto arr = UD->GetContent();
        size_t bufsize = arr.Num() > 4 ? arr.Num() : 4;
        NS_CPPBUFFER::p_cppbuffer buffer = NS_CB_LUAADAPTER::newlbuffer(lstack, bufsize);
        NS_CPPBUFFER::cb_write_raw(buffer, reinterpret_cast<char*>(arr.GetData()), arr.Num());
        return 1;
    })
    DefLuaMethod_With_Imp(SavePayload, false, {
        CheckUD(IHttpResponse, lstack, 1);
        if (!UD) ___pdr_luaL_argcheck(lstack, false, 1, "'IHttpResponse' expected");
        if (!___pdr_lua_isstring(lstack, 2)) ___pdr_luaL_argcheck(lstack, false, 2, "'string' expected");
        const char * path = ___pdr_luaL_checkstring(lstack, 2);
        FString fpath(UTF8_TO_TCHAR(path));
        auto arr = UD->GetContent();
        auto& platform = FPlatformFileManager::Get().GetPlatformFile();
        IFileHandle* handle = platform.OpenWrite(*fpath);
        int err = 0;
        if (handle)
        {
            err = handle->Write(arr.GetData(), arr.Num()) ? 0 : -1;
            if (err == 0)
                handle->Flush();
            delete handle;
        }
        else
            err = -1;
        ___pdr_lua_pushinteger(lstack, err);
        return 1;
    })
EndDef(IHttpResponse, nullptr)

DefLuaClass(IHttpRequest)
    DefLuaMethod(GetVerb, &IHttpRequest::GetVerb)
    DefLuaMethod(SetVerb, &IHttpRequest::SetVerb)
    DefLuaMethod(SetURL, &IHttpRequest::SetURL)
    DefLuaMethod_With_Imp(SetContent, false, {
        CheckUD(IHttpRequest, lstack, 1);
        if (!UD) ___pdr_luaL_argcheck(lstack, false, 1, "'IHttpRequest' expected");
        NS_CPPBUFFER::p_cppbuffer content = NS_CB_LUAADAPTER::checkbuffer(lstack, 2);
        size_t len = NS_CPPBUFFER::cb_get_bytes_count(content);
        char* temp = new char[len];
        NS_CPPBUFFER::cb_read_raw(content, temp, len);
        TArray<uint8> data((uint8*)temp, len);
        delete[] temp;
        UD->SetContent(data);
        return 0;
    })
    DefLuaMethod(SetContentAsString, &IHttpRequest::SetContentAsString)
    DefLuaMethod_With_Imp(SetContentAsForm, false, {
        CheckUD(IHttpRequest, lstack, 1);
        if (!UD) ___pdr_luaL_argcheck(lstack, false, 1, "'IHttpRequest' expected");
        pandora::PLuaHttpForm* form = LuaObject::checkValue<pandora::PLuaHttpForm*>(lstack, 2);
        if (!form) ___pdr_luaL_argcheck(lstack, false, 2, "'PLuaHttpForm' expected");
        UD->SetContent(form->GetRawData());
        return 0;
    })
    DefLuaMethod(SetHeader, &IHttpRequest::SetHeader)
    DefLuaMethod(AppendToHeader, &IHttpRequest::AppendToHeader)
    DefLuaMethod(ProcessRequest, &IHttpRequest::ProcessRequest)
    DefLuaMethod(OnRequestProgress, &IHttpRequest::OnRequestProgress)
    DefLuaMethod(OnProcessRequestComplete, &IHttpRequest::OnProcessRequestComplete)
    DefLuaMethod(CancelRequest, &IHttpRequest::CancelRequest)
    DefLuaMethod(GetStatus, &IHttpRequest::GetStatus)
    DefLuaMethod(GetResponse, &IHttpRequest::GetResponse)
#if (ENGINE_MINOR_VERSION>=20) && (ENGINE_MAJOR_VERSION>=4)
    DefLuaMethod(OnHeaderReceived, &IHttpRequest::OnHeaderReceived)
#endif
EndDef(IHttpRequest, nullptr)

DefLuaClass(FHttpModule)
    DefLuaMethod_With_Lambda(Get, true, []()->FHttpModule* {
        FHttpModule& m = FModuleManager::LoadModuleChecked<FHttpModule>(TEXT("Http"));
        return &m;
    })
    DefLuaMethod(CreateRequest, &FHttpModule::CreateRequest)
    DefLuaMethod(GetHttpTimeout, &FHttpModule::GetHttpTimeout)
    DefLuaMethod_With_Lambda(URLDecode, true, [](const FString & input)->FString {
        return FGenericPlatformHttp::UrlDecode(input);
    })
    DefLuaMethod_With_Lambda(URLEncode, true, [](const FString & input)->FString {
        return FGenericPlatformHttp::UrlEncode(input);
    })
EndDef(FHttpModule, nullptr)
}
