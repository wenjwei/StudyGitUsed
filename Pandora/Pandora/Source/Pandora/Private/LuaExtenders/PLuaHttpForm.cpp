
#include "PLuaHttpForm.h"
#include "StringConv.h"
#include "GenericPlatformHttp.h"
#include "PDRslua.h"
#include "PDRLuaObject.h"
#include "PDRLuaCppBinding.h"
#include "luaadapter.h"
#include "PLuaFileSystem.h"

namespace pandora
{
PLuaHttpForm::PLuaHttpForm(): 
    _boundary(FString(TEXT("--------------PHTTPFORMBOUNDARYxxx")))
{}
PLuaHttpForm::~PLuaHttpForm() {}

inline void FStringToUTF8TArrayUINT8(TArray<uint8>& dest, const FString& src)
{
    FTCHARToUTF8 convert(*src);
    dest.Empty();
    dest.Append((uint8*)convert.Get(), convert.Length());
}

inline void UTF8TArrayUINT8ToFString(FString & dest, const TArray<uint8>& src)
{
    FUTF8ToTCHAR convert((ANSICHAR*)src.GetData(), src.Num());
    dest = convert.Get();
}

inline bool IsSevenBitClean(const TArray<uint8>& target)
{
    for (int i = 0; i < target.Num(); ++i)
    {
        if (target[i] < 0 || target[i] > 127)
            return false;
    }
    return true;
}

void PLuaHttpForm::AddField(const FString& fieldName, const FString & value)
{
    _fieldNames.Add(fieldName);
    _fileNames.Add(TEXT(""));
    TArray<uint8> arr;
    FStringToUTF8TArrayUINT8(arr, value);
    _formData.Add(arr);
    _types.Add(TEXT("text/plain; charset=\"utf-8\""));
}

void PLuaHttpForm::AddBinaryData(const FString& fieldName, const TArray<uint8>& contents, const FString& fileName /* = TEXT("") */, const FString& mimeType /* = TEXT("") */)
{
    _containFiles = true;

    FString fname = fileName;
    if (fname == TEXT(""))
        fname = fieldName;

    _fieldNames.Add(fieldName);
    _fileNames.Add(fname);
    _formData.Add(contents);
    _types.Add(TEXT("application/octet-stream"));
}

FString PLuaHttpForm::GetContentType()
{
    if (_containFiles)
        return TEXT("multipart/form-data; boundary=\"") + _boundary + TEXT("\"");
    else
        return TEXT("application/x-www-form-urlencoded");
}

TArray<uint8> PLuaHttpForm::GetRawData()
{
    TArray<uint8> contents;
    if (_containFiles)
    {
        TArray<uint8> dDash;
        FStringToUTF8TArrayUINT8(dDash, TEXT("--"));
        TArray<uint8> crlf;
        FStringToUTF8TArrayUINT8(crlf, TEXT("\r\n"));
        TArray<uint8> contentTypeHeader;
        FStringToUTF8TArrayUINT8(contentTypeHeader, TEXT("Content-Type: "));
        TArray<uint8> dispositionHeader;
        FStringToUTF8TArrayUINT8(dispositionHeader, TEXT("Content-disposition: form-data; name=\""));
        TArray<uint8> endQuote;
        FStringToUTF8TArrayUINT8(endQuote, TEXT("\""));
        TArray<uint8> fileNameField;
        FStringToUTF8TArrayUINT8(fileNameField, TEXT("; filename=\""));
        TArray<uint8> boundaryArr;
        FStringToUTF8TArrayUINT8(boundaryArr, _boundary);

        TArray<uint8> temp;
        for (int i = 0; i < _formData.Num(); ++i)
        {
            contents.Append(crlf);
            contents.Append(dDash);
            contents.Append(boundaryArr);
            contents.Append(crlf);
            contents.Append(contentTypeHeader);

            FStringToUTF8TArrayUINT8(temp, _types[i]);
            contents.Append(temp);
            contents.Append(crlf);
            contents.Append(dispositionHeader);
            FStringToUTF8TArrayUINT8(temp, _fieldNames[i]);
            contents.Append(temp);
            contents.Append(endQuote);
                
            if (_fileNames[i] != TEXT(""))
            {
                contents.Append(fileNameField);
                FStringToUTF8TArrayUINT8(temp, _fileNames[i]);
                contents.Append(temp);
                contents.Append(endQuote);
            }

            contents.Append(crlf);
            contents.Append(crlf);
                
            contents.Append(_formData[i]);
        }
        contents.Append(crlf);
        contents.Append(dDash);
        contents.Append(boundaryArr);
        contents.Append(dDash);
    }
    else
    {
        FString _and = TEXT("&");
        FString _equ = TEXT("=");
        FString result = TEXT("");
        FString temp;
        for (int i = 0; i < _formData.Num(); ++i)
        {
            if (i > 0) result += _and;

            result += _fieldNames[i];
            result += _equ;
            UTF8TArrayUINT8ToFString(temp, _formData[i]);
            result += temp;
        }
        result = FGenericPlatformHttp::UrlEncode(result);
        FStringToUTF8TArrayUINT8(contents, result);
    }

    return contents;
}

PLuaHttpFormFactory::PLuaHttpFormFactory() {}
PLuaHttpFormFactory::~PLuaHttpFormFactory() {}

PLuaHttpFormFactory& PLuaHttpFormFactory::Get()
{
    static PLuaHttpFormFactory ___pHttpFormFactoryInstance;
    return ___pHttpFormFactoryInstance;
}

TSharedPtr<PLuaHttpForm> PLuaHttpFormFactory::CreateHttpForm()
{
    return MakeShareable(new PLuaHttpForm);
}
}

namespace NS_PDR_SLUA
{
using pandora::PLuaHttpForm;
using pandora::PLuaHttpFormFactory;
using pandora::PPlatformFileWrapper;

DefLuaClass(PLuaHttpForm)
    DefLuaMethod(AddField, &PLuaHttpForm::AddField)
    DefLuaMethod(GetContentType, &PLuaHttpForm::GetContentType)
    DefLuaMethod_With_Imp(AddBinaryData, false, {
        CheckUD(PLuaHttpForm, lstack, 1);
        if (!UD) ___pdr_luaL_argcheck(lstack, false, 1, "'PLuaHttpForm' expected");
        const char* cstr_fieldName = ___pdr_luaL_checkstring(lstack, 2);
        FString fieldName(UTF8_TO_TCHAR(cstr_fieldName));
        NS_CPPBUFFER::p_cppbuffer data = NS_CB_LUAADAPTER::checkbuffer(lstack, 3);
        const char* cstr_fileName = ___pdr_luaL_checkstring(lstack, 4);
        FString fileName(UTF8_TO_TCHAR(cstr_fileName));
        const char* cstr_mimeType = ___pdr_luaL_checkstring(lstack, 5);
        FString mimeType(UTF8_TO_TCHAR(cstr_mimeType));
        size_t len = NS_CPPBUFFER::cb_get_bytes_count(data);
        char* temp = new char[len];
        NS_CPPBUFFER::cb_read_raw(data, temp, len, 0);
        TArray<uint8> dataarr((uint8*)temp, len);
        delete[] temp;
        UD->AddBinaryData(fieldName, dataarr, fileName, mimeType);
        return 0;
    })
    DefLuaMethod_With_Imp(AddLocalFileAsBinaryData, false, {
        CheckUD(PLuaHttpForm, lstack, 1);
        if (!UD) ___pdr_luaL_argcheck(lstack, false, 1, "'PLuaHttpForm' expected");
        const char* cstr_fieldName = ___pdr_luaL_checkstring(lstack, 2);
        FString fieldName(UTF8_TO_TCHAR(cstr_fieldName));
        const char* cstr_filePath = ___pdr_luaL_checkstring(lstack, 3);
        FString filePath(UTF8_TO_TCHAR(cstr_filePath));
        if (!PPlatformFileWrapper::FileExists(filePath)) {
            ___pdr_luaL_argcheck(lstack, false, 3, "file not exist");
        }
        const char* cstr_fileName = ___pdr_luaL_checkstring(lstack, 4);
        FString fileName(UTF8_TO_TCHAR(cstr_fileName));
        const char* cstr_mimeType = ___pdr_luaL_checkstring(lstack, 5);
        FString mimeType(UTF8_TO_TCHAR(cstr_mimeType));
        TArray<uint8> fileData;
        PPlatformFileWrapper::LoadFileToArray(filePath, fileData);
        UD->AddBinaryData(fieldName, fileData, fileName, mimeType);
        return 0;
    })
    DefLuaMethod_With_Imp(GetRawData, false, {
        if (!NS_CB_LUAADAPTER::ismtcreated(lstack))
        {
            UE_LOG(LogTemp, Warning, TEXT("lbuffer is not required yet, please require lbuffer first"));
            ___pdr_lua_pushnil(lstack);
            return 1;
        }
        CheckUD(PLuaHttpForm, lstack, 1);
        if (!UD) ___pdr_luaL_argcheck(lstack, false, 1, "'PLuaHttpForm' expected");
        TArray<uint8>arr = UD->GetRawData();
        NS_CPPBUFFER::p_cppbuffer data = NS_CB_LUAADAPTER::newlbuffer(lstack, arr.Num());
        NS_CPPBUFFER::cb_write_raw(data, (const char*)(arr.GetData()), arr.Num());
        return 1;
    })
EndDef(PLuaHttpForm, nullptr)

DefLuaClass(PLuaHttpFormFactory)
    DefLuaMethod_With_Lambda(Get, true, []()->PLuaHttpFormFactory* {
        return &PLuaHttpFormFactory::Get();
    })
    DefLuaMethod(CreateHttpForm, &PLuaHttpFormFactory::CreateHttpForm)
EndDef(PLuaHttpFormFactory, nullptr)
}

