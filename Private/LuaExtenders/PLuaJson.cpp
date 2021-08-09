// Fill out your copyright notice in the Description page of Project Settings.

#include "PLuaJson.h"
#include "lauxlib.h"
#include "Runtime/Json/Public/Json.h"

namespace pandora
{
namespace json
{
static int pandora_lua_json_decoder(NS_PDR_SLUA::___pdr_lua_State *L);
static int pandora_lua_json_encoder(NS_PDR_SLUA::___pdr_lua_State *L);

static NS_PDR_SLUA::___pdr_luaL_Reg pandora_lua_json_methods[] = {
    {"decode",                      pandora_lua_json_decoder},
    {"encode",                      pandora_lua_json_encoder},
    {NULL, NULL}
};

int lua_open(NS_PDR_SLUA::___pdr_lua_State *L)
{
#if ___PDR_LUA_VERSION_NUM > 501 && !defined(___PDR_LUA_COMPAT_MODULE)
    ___pdr_lua_newtable(L);
    ___pdr_luaL_setfuncs(L, pandora_lua_json_methods, 0);
#else
    ___pdr_luaL_openlib(L, "cjson", pandora_lua_json_methods, 0);
#endif
    return 1;
}

static int check_lua_arrlen(NS_PDR_SLUA::___pdr_lua_State *L);
static int obj_table_encoder(NS_PDR_SLUA::___pdr_lua_State *L, TSharedPtr<FJsonObject> & node);
static int arr_table_encoder(NS_PDR_SLUA::___pdr_lua_State *L, TArray<TSharedPtr<FJsonValue>> & nodeArr, int len);

static void obj_array_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, const FString & nodename, const TSharedPtr<FJsonValue> & node);
static void obj_object_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, const FString & nodename, const TSharedPtr<FJsonValue> & node);
static void obj_number_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, const FString & nodename, const TSharedPtr<FJsonValue> & node);
static void obj_string_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, const FString & nodename, const TSharedPtr<FJsonValue> & node);
static void obj_boolean_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, const FString & nodename, const TSharedPtr<FJsonValue> & node);
static void arr_array_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, int idx, const TSharedPtr<FJsonValue> & node);
static void arr_object_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, int idx, const TSharedPtr<FJsonValue> & node);
static void arr_number_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, int idx, const TSharedPtr<FJsonValue> & node);
static void arr_string_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, int idx, const TSharedPtr<FJsonValue> & node);
static void arr_boolean_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, int idx, const TSharedPtr<FJsonValue> & node);
typedef void(*json_obj_item_decoder)(NS_PDR_SLUA::___pdr_lua_State *, const FString &, const TSharedPtr<FJsonValue> &);
typedef void(*json_arr_item_decoder)(NS_PDR_SLUA::___pdr_lua_State *, int idx, const TSharedPtr<FJsonValue> &);
static TMap<EJson, json_obj_item_decoder> obj_item_decoders;
static TMap<EJson, json_arr_item_decoder> arr_item_decoders;

static int obj_string_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, const FString & name, TSharedPtr<FJsonObject> & obj);
static int obj_number_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, const FString & name, TSharedPtr<FJsonObject> & obj);
static int obj_boolean_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, const FString & name, TSharedPtr<FJsonObject> & obj);
static int obj_table_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, const FString & name, TSharedPtr<FJsonObject> & obj);
static int obj_nil_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, const FString & name, TSharedPtr<FJsonObject> & obj);
static int arr_string_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, TArray<TSharedPtr<FJsonValue>> & arr);
static int arr_number_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, TArray<TSharedPtr<FJsonValue>> & arr);
static int arr_boolean_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, TArray<TSharedPtr<FJsonValue>> & arr);
static int arr_table_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, TArray<TSharedPtr<FJsonValue>> & arr);
static int arr_nil_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, TArray<TSharedPtr<FJsonValue>> & arr);
typedef int(*json_obj_item_encoder)(NS_PDR_SLUA::___pdr_lua_State *, const FString &, TSharedPtr<FJsonObject> &);
typedef int(*json_arr_item_encoder)(NS_PDR_SLUA::___pdr_lua_State *, TArray<TSharedPtr<FJsonValue>> &);
static TMap<int, json_obj_item_encoder> obj_item_encoders;
static TMap<int, json_arr_item_encoder> arr_item_encoders;


int lua_init(NS_PDR_SLUA::___pdr_lua_State *L)
{
    obj_item_decoders.Add(EJson::Array, obj_array_item_decoder);
    obj_item_decoders.Add(EJson::Object, obj_object_item_decoder);
    obj_item_decoders.Add(EJson::Number, obj_number_item_decoder);
    obj_item_decoders.Add(EJson::String, obj_string_item_decoder);
    obj_item_decoders.Add(EJson::Boolean, obj_boolean_item_decoder);

    arr_item_decoders.Add(EJson::Array, arr_array_item_decoder);
    arr_item_decoders.Add(EJson::Object, arr_object_item_decoder);
    arr_item_decoders.Add(EJson::Number, arr_number_item_decoder);
    arr_item_decoders.Add(EJson::String, arr_string_item_decoder);
    arr_item_decoders.Add(EJson::Boolean, arr_boolean_item_decoder);

    obj_item_encoders.Add(___PDR_LUA_TSTRING, obj_string_item_encoder);
    obj_item_encoders.Add(___PDR_LUA_TNUMBER, obj_number_item_encoder);
    obj_item_encoders.Add(___PDR_LUA_TBOOLEAN, obj_boolean_item_encoder);
    obj_item_encoders.Add(___PDR_LUA_TTABLE, obj_table_item_encoder);
    obj_item_encoders.Add(___PDR_LUA_TNIL, obj_nil_item_encoder);

    arr_item_encoders.Add(___PDR_LUA_TSTRING, arr_string_item_encoder);
    arr_item_encoders.Add(___PDR_LUA_TNUMBER, arr_number_item_encoder);
    arr_item_encoders.Add(___PDR_LUA_TBOOLEAN, arr_boolean_item_encoder);
    arr_item_encoders.Add(___PDR_LUA_TTABLE, arr_table_item_encoder);
    arr_item_encoders.Add(___PDR_LUA_TNIL, arr_nil_item_encoder);

    ___pdr_luaL_getsubtable(L, ___PDR_LUA_REGISTRYINDEX, ___PDR_LUA_PRELOAD_TABLE);

    ___pdr_lua_pushcfunction(L, lua_open);
    ___pdr_lua_setfield(L, -2, "cjson");

    ___pdr_lua_pop(L, 1);
    return 0;
}



static int pandora_lua_json_decoder(NS_PDR_SLUA::___pdr_lua_State* L)
{
    if (!___pdr_lua_isstring(L, 1))
    {
        ___pdr_luaL_argcheck(L, false, 1, "'string' expected");
        ___pdr_lua_pushnil(L);
        return 1;
    }
    size_t len;
    const char* jsonstr = ___pdr_luaL_checklstring(L, 1, &len);

    TSharedPtr<FJsonObject> result;
    TSharedRef<TJsonReader<TCHAR>> reader = TJsonReaderFactory<TCHAR>::Create(FString(UTF8_TO_TCHAR(jsonstr)));
    if (FJsonSerializer::Deserialize(reader, result))
    {
        ___pdr_lua_newtable(L);
        for (const TPair<FString, TSharedPtr<FJsonValue>>& item : result->Values)
        {
            json_obj_item_decoder* func = obj_item_decoders.Find(item.Value->Type);;
            if (func != nullptr)
                (*func)(L, item.Key, item.Value);
        }
        return 1;
    }

    TArray<TSharedPtr<FJsonValue>> arr;
    TSharedRef<TJsonReader<TCHAR>> arrReader = TJsonReaderFactory<TCHAR>::Create(FString(UTF8_TO_TCHAR(jsonstr)));
    if (FJsonSerializer::Deserialize(arrReader, arr))
    //if (FJsonSerializer::Deserialize(reader, arr))
    {
        ___pdr_lua_newtable(L);
        for (int i = 0; i < arr.Num(); ++i)
        {
            json_arr_item_decoder* func = arr_item_decoders.Find(arr[i]->Type);
            if (func != nullptr)
                (*func)(L, i, arr[i]);
        }
        return 1;
    }

    // deserialize failed
    ___pdr_luaL_argcheck(L, false, 1, "json string invalid");
    ___pdr_lua_pushnil(L);
    return 1;
}

static int pandora_lua_json_encoder(NS_PDR_SLUA::___pdr_lua_State* L)
{
    if (!___pdr_lua_istable(L, 1))
    {
        //___pdr_luaL_argcheck(L, false, 1, "'table' expected");
        ___pdr_lua_pushnil(L);
        return 1;
    }

    int len = check_lua_arrlen(L);
    if (len > 0)
    {
        // treat as array object
        TArray<TSharedPtr<FJsonValue>> arr;
        if (arr_table_encoder(L, arr, len) != 0)
        {
            ___pdr_lua_pushnil(L);
            return 1;
        }

        FString output;
        TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&output);
        FJsonSerializer::Serialize(arr, writer);
        FTCHARToUTF8 converter(*output);
        ___pdr_lua_pushlstring(L, converter.Get(), converter.Length());
        return 1;
    }

    if (len == -1)
    {
        // treat as json object
        TSharedPtr<FJsonObject> obj = MakeShareable(new FJsonObject);
        if (obj_table_encoder(L, obj) != 0)
        {
            //___pdr_luaL_argcheck(L, false, 1, "invalid table");
            ___pdr_lua_pushnil(L);
            return 1;
        }

        FString output;
        TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&output);
        FJsonSerializer::Serialize(obj.ToSharedRef(), writer);
        FTCHARToUTF8 converter(*output);
        ___pdr_lua_pushlstring(L, converter.Get(), converter.Length());
        return 1;
    }

    // invalid table
    ___pdr_lua_pushnil(L);
    return 1;
}

static int check_lua_arrlen(NS_PDR_SLUA::___pdr_lua_State *L)
{
    int itemCnt = 0;
    int maxIdx = 0;
    ___pdr_lua_pushnil(L);
    while (___pdr_lua_next(L, -2) != 0)
    {
        if (___pdr_lua_type(L, -2) != ___PDR_LUA_TNUMBER)
        {
            ___pdr_lua_pop(L, 2);
            return -1;
        }
        double k = ___pdr_lua_tonumber(L, -2);
        if (FMath::FloorToInt(k) != k || k < 1)
        {
            ___pdr_lua_pop(L, 2);
            return -1;
        }

        ++itemCnt;
        if (k > maxIdx)
            maxIdx = k;
        ___pdr_lua_pop(L, 1);
    }

    // no element, process as empty object
    if (itemCnt == 0)
        return -1;

    // too much empty slot, treat as object 
    if (itemCnt < maxIdx - 10)
        return -1;

    return maxIdx;
}

static int obj_table_encoder(NS_PDR_SLUA::___pdr_lua_State *L, TSharedPtr<FJsonObject> & node)
{
    ___pdr_lua_pushnil(L);
    while (___pdr_lua_next(L, -2) != 0)
    {
        FString key;
        if (___pdr_lua_type(L, -2) == ___PDR_LUA_TSTRING)
            key = UTF8_TO_TCHAR(___pdr_lua_tostring(L, -2));
        else if (___pdr_lua_type(L, -2) == ___PDR_LUA_TNUMBER)
            key = FString::FromInt(FMath::FloorToInt(___pdr_lua_tonumber(L, -2)));
        else
        {
            // key invalid
            ___pdr_lua_pop(L, 2);
            return -1;
        }

        int vtype = ___pdr_lua_type(L, -1);
        json_obj_item_encoder * encoder = obj_item_encoders.Find(vtype);
        if (encoder == nullptr)
        {
            ___pdr_lua_pop(L, 2);
            return -1;
        }
        int err = (*encoder)(L, key, node);
        if (err != 0)
        {
            ___pdr_lua_pop(L, 2);
            return -1;
        }
        ___pdr_lua_pop(L, 1);
    }
    return 0;
}

static int arr_table_encoder(NS_PDR_SLUA::___pdr_lua_State *L, TArray<TSharedPtr<FJsonValue>> & nodeArr, int len)
{
    for (int i = 1; i <= len; ++i)
    {
        ___pdr_lua_pushinteger(L, i);
        ___pdr_lua_rawget(L, -2);
        int vtype = ___pdr_lua_type(L, -1);
        json_arr_item_encoder * encoder = arr_item_encoders.Find(vtype);
        if (encoder == nullptr)
        {
            ___pdr_lua_pop(L, 1);
            return -1;
        }
        int err = (*encoder)(L, nodeArr);
        if (err != 0)
        {
            ___pdr_lua_pop(L, 1);
            return -1;
        }
        ___pdr_lua_pop(L, 1);
    }
    return 0;
}

static void obj_array_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, const FString & nodename, const TSharedPtr<FJsonValue> & node)
{
    FTCHARToUTF8 converter(*nodename);
    ___pdr_lua_pushlstring(L, converter.Get(), converter.Length());
    ___pdr_lua_newtable(L);
    const TArray<TSharedPtr<FJsonValue>> & arr = node->AsArray();
    for (int i = 0; i < arr.Num(); ++i)
    {
        json_arr_item_decoder * func = arr_item_decoders.Find(arr[i]->Type);
        if (func != nullptr)
            (*func)(L, i, arr[i]);
    }

    ___pdr_lua_rawset(L, -3);
}

static void obj_object_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, const FString & nodename, const TSharedPtr<FJsonValue> & node)
{
    FTCHARToUTF8 converter(*nodename);
    ___pdr_lua_pushlstring(L, converter.Get(), converter.Length());
    ___pdr_lua_newtable(L);
    const TSharedPtr<FJsonObject> & obj = node->AsObject();
    //const TMap<FString, TSharedPtr<FJsonValue>>& map = obj->Values
    for (const TPair<FString, TSharedPtr<FJsonValue>> & item : obj->Values)
    {
        json_obj_item_decoder * func = obj_item_decoders.Find(item.Value->Type);
        if (func != nullptr)
            (*func)(L, item.Key, item.Value);
    }
    ___pdr_lua_rawset(L, -3);
}

static void obj_number_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, const FString & nodename, const TSharedPtr<FJsonValue> & node)
{
    FTCHARToUTF8 converter(*nodename);
    ___pdr_lua_pushlstring(L, converter.Get(), converter.Length());
    double val = node->AsNumber();
    if (val == FMath::FloorToInt(val))
        ___pdr_lua_pushinteger(L, FMath::FloorToInt(val));
    else
        ___pdr_lua_pushnumber(L, val);
    ___pdr_lua_rawset(L, -3);
}

static void obj_string_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, const FString & nodename, const TSharedPtr<FJsonValue> & node)
{
    FTCHARToUTF8 converter(*nodename);
    ___pdr_lua_pushlstring(L, converter.Get(), converter.Length());
    FString value = node->AsString();
    FTCHARToUTF8 valueConverter(*value);
    ___pdr_lua_pushlstring(L, valueConverter.Get(), valueConverter.Length());
    ___pdr_lua_rawset(L, -3);
}

static void obj_boolean_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, const FString & nodename, const TSharedPtr<FJsonValue> & node)
{
    FTCHARToUTF8 converter(*nodename);
    ___pdr_lua_pushlstring(L, converter.Get(), converter.Length());
    ___pdr_lua_pushboolean(L, node->AsBool());
    ___pdr_lua_rawset(L, -3);
}

static void arr_array_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, int idx, const TSharedPtr<FJsonValue> & node)
{
    ___pdr_lua_pushinteger(L, idx + 1);
    ___pdr_lua_newtable(L);
    const TArray<TSharedPtr<FJsonValue>> & arr = node->AsArray();
    for (int i = 0; i < arr.Num(); ++i)
    {
        json_arr_item_decoder * func = arr_item_decoders.Find(arr[i]->Type);
        if (func != nullptr)
            (*func)(L, i, arr[i]);
    }
    ___pdr_lua_rawset(L, -3);
}

static void arr_object_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, int idx, const TSharedPtr<FJsonValue> & node)
{
    ___pdr_lua_pushinteger(L, idx + 1);
    ___pdr_lua_newtable(L);
    const TSharedPtr<FJsonObject> & obj = node->AsObject();
    for (const TPair<FString, TSharedPtr<FJsonValue>> & item : obj->Values)
    {
        json_obj_item_decoder * func = obj_item_decoders.Find(item.Value->Type);
        if (func != nullptr)
            (*func)(L, item.Key, item.Value);
    }
    ___pdr_lua_rawset(L, -3);
}

static void arr_number_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, int idx, const TSharedPtr<FJsonValue> & node)
{
    ___pdr_lua_pushinteger(L, idx + 1);
    double val = node->AsNumber();
    if (val == FMath::FloorToInt(val))
        ___pdr_lua_pushinteger(L, FMath::FloorToInt(val));
    else
        ___pdr_lua_pushnumber(L, val);
    ___pdr_lua_rawset(L, -3);
}

static void arr_string_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, int idx, const TSharedPtr<FJsonValue> & node)
{
    ___pdr_lua_pushinteger(L, idx + 1);
    FString value = node->AsString();
    FTCHARToUTF8 converter(*value);
    ___pdr_lua_pushlstring(L, converter.Get(), converter.Length());
    ___pdr_lua_rawset(L, -3);
}

static void arr_boolean_item_decoder(NS_PDR_SLUA::___pdr_lua_State *L, int idx, const TSharedPtr<FJsonValue> & node)
{
    ___pdr_lua_pushinteger(L, idx + 1);
    ___pdr_lua_pushboolean(L, node->AsBool());
    ___pdr_lua_rawset(L, -3);
}

static int obj_string_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, const FString & name, TSharedPtr<FJsonObject> & obj)
{
    obj->SetStringField(name, UTF8_TO_TCHAR(___pdr_lua_tostring(L, -1)));
    return 0;
}

static int obj_number_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, const FString & name, TSharedPtr<FJsonObject> & obj)
{
    obj->SetNumberField(name, ___pdr_lua_tonumber(L, -1));
    return 0;
}

static int obj_boolean_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, const FString & name, TSharedPtr<FJsonObject> & obj)
{
    if (___pdr_lua_toboolean(L, -1))
        obj->SetBoolField(name, true);
    else
        obj->SetBoolField(name, false);
    return 0;
}

static int obj_table_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, const FString & name, TSharedPtr<FJsonObject> & obj)
{
    int err = 0;
    int len = check_lua_arrlen(L);
    if (len == -2)
        return -1;
    if (len == -1)
    {
        TSharedPtr<FJsonObject> newNode = MakeShareable(new FJsonObject);
        err = obj_table_encoder(L, newNode);
        if (err != 0)
            return err;
        obj->SetObjectField(name, newNode);
    }
    else
    {
        TArray<TSharedPtr<FJsonValue>> nodeArr;
        err = arr_table_encoder(L, nodeArr, len);
        if (err != 0)
            return err;
        obj->SetArrayField(name, nodeArr);
    }
    return err;
}

static int obj_nil_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, const FString & name, TSharedPtr<FJsonObject> & obj)
{
    // just do nothing
    return 0;
}

static int arr_string_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, TArray<TSharedPtr<FJsonValue>> & arr)
{
    arr.Add(MakeShareable(new FJsonValueString(FString(UTF8_TO_TCHAR(___pdr_lua_tostring(L, -1))))));
    return 0;
}

static int arr_number_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, TArray<TSharedPtr<FJsonValue>> & arr)
{
    arr.Add(MakeShareable(new FJsonValueNumber(___pdr_lua_tonumber(L, -1))));
    return 0;
}

static int arr_boolean_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, TArray<TSharedPtr<FJsonValue>> & arr)
{
    if (___pdr_lua_toboolean(L, -1))
        arr.Add((MakeShareable(new FJsonValueBoolean(true))));
    else
        arr.Add((MakeShareable(new FJsonValueBoolean(false))));
    return 0;
}

static int arr_table_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, TArray<TSharedPtr<FJsonValue>> & arr)
{

    int err = 0;
    int len = check_lua_arrlen(L);
    if (len == -2)
        return -1;
    if (len == -1)
    {
        TSharedPtr<FJsonObject> newNode = MakeShareable(new FJsonObject);
        err = obj_table_encoder(L, newNode);
        if (err != 0)
            return err;
        arr.Add(MakeShareable(new FJsonValueObject(newNode)));
    }
    else
    {
        TArray<TSharedPtr<FJsonValue>> nodeArr;
        err = arr_table_encoder(L, nodeArr, len);
        if (err != 0)
            return err;
        arr.Add(MakeShareable(new FJsonValueArray(nodeArr)));
    }

    return err;
}

static int arr_nil_item_encoder(NS_PDR_SLUA::___pdr_lua_State * L, TArray<TSharedPtr<FJsonValue>> & arr)
{
    arr.Add(MakeShareable(new FJsonValueNull));
    return 0;
}
}
}
