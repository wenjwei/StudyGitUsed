#pragma once


#include "Containers/List.h"
#include "Containers/UnrealString.h"
#include "SharedPointer.h"

namespace pandora
{
/*
* HTTP form utilities
* encode with utf-8 standard
*/
class PLuaHttpForm
{
public:
    PLuaHttpForm();
    virtual ~PLuaHttpForm();

    void AddField(const FString& fieldName, const FString & value);
    void AddBinaryData(const FString& fieldName, const TArray<uint8>& contents, const FString& fileName = TEXT(""), const FString& mimeType = TEXT(""));
    FString GetContentType();
    TArray<uint8> GetRawData();

private:
    TArray<TArray<uint8>> _formData;
    TArray<FString> _fieldNames;
    TArray<FString> _fileNames;
    TArray<FString> _types;
    FString _boundary;
    bool _containFiles;
};

class PLuaHttpFormFactory
{
public:
    static PLuaHttpFormFactory& Get();
    virtual ~PLuaHttpFormFactory();

    TSharedPtr<PLuaHttpForm> CreateHttpForm();

private:
    PLuaHttpFormFactory();
};
}
