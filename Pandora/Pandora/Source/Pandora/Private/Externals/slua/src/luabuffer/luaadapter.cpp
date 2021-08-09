
#include "luaadapter.h"
#include "cppbuffer.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "limits.h"
#include "lstate.h"

#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif

#define ERR_TYPE_INVALID -1
#define ERR_STRING_EXCEED_LIMIT -2
#define ERR_FORMAT_STRING_INVALID -3
#define ERR_CSTRING_ENDING_NOT_FOUND -4


#define ADAPTER_WRITE_NUMERIC(buffer,idx,err,T,LR,HR,CHKNUMTYPE,CHKNUM)  \
    if (!___pdr_lua_isnumber(L,idx)) \
        err = ERR_TYPE_INVALID; \
    if (err == 0) {\
        T val; \
        CHKNUMTYPE n = CHKNUM(L, idx); \
        if (n < LR || n > HR) \
            ___pdr_luaL_argcheck(L, false, idx, "number exceed limit"); \
        val = (T) n; \
        err = NS_CPPBUFFER::cb_write_convertable_data(buffer, val); \
    }


#define ADAPTER_WRITE_NUMERIC_FUNC_BODY(T,LR,HR,CHKNUMTYPE,CHKNUM)  \
    NS_CPPBUFFER::t_cppbuffer * buf = NS_CB_LUAADAPTER::checkbuffer(L, 1); \
    int err = 0; \
    ADAPTER_WRITE_NUMERIC(buf, 2, err, T, LR, HR, CHKNUMTYPE, CHKNUM) \
    if (err == ERR_TYPE_INVALID) \
        ___pdr_luaL_argcheck(L, false, 2, "'number' expected"); \
    ___pdr_lua_pushinteger(L, err); \
    return 1; 


#define ADAPTER_READ_NUMERIC(buffer,err,T,PUSHNUM)  \
    T val; \
    err = NS_CPPBUFFER::cb_read_convertable_data(buffer, val); \
    if (err == 0) \
        PUSHNUM(L, val); \
    else \
        ___pdr_lua_pushnil(L);


#define ADAPTER_READ_NUMERIC_FUNC_BODY(T,PUSHNUM)  \
    NS_CPPBUFFER::t_cppbuffer * buf = NS_CB_LUAADAPTER::checkbuffer(L, 1); \
    int err = 0; \
    ADAPTER_READ_NUMERIC(buf, err, T, PUSHNUM) \
    ___pdr_lua_pushinteger(L, err); \
    return 2; 


#define ADAPTER_WRITE_NUMERIC_LIST_FUNC_BODY(T, CHKNUM) \
    NS_CPPBUFFER::t_cppbuffer * buf = NS_CB_LUAADAPTER::checkbuffer(L, 1); \
    if (!___pdr_lua_istable(L, 2)) \
        ___pdr_luaL_argcheck(L, false, 2, "'table' expected"); \
    ___PDR_LUA_INTEGER len = ___pdr_lua_rawlen(L, 2); \
    if (len <= 0) { \
        ___pdr_lua_pushinteger(L, 0); \
        return 1; \
    } \
    int err = 0; \
    for (int i = 0; i < len; ++i) { \
        ___pdr_lua_rawgeti(L, 2, i+1); \
        if (!___pdr_lua_isnumber(L, 3)) { \
            ___pdr_lua_pop(L, 1); \
            err = ERR_TYPE_INVALID; \
            NS_CPPBUFFER::cb_peekwrite(buf, i*sizeof(T), false); \
            break; \
        } \
        T val = (T)CHKNUM(L, 3); \
        NS_CPPBUFFER::cb_write_convertable_data(buf, val); \
        ___pdr_lua_pop(L, 1); \
    } \
    ___pdr_lua_pushinteger(L, err); \
    return 1; 


#define ADAPTER_READ_NUMERIC_LIST_FUNC_BODY(T,PUSHNUM) \
    NS_CPPBUFFER::t_cppbuffer * buf = NS_CB_LUAADAPTER::checkbuffer(L, 1); \
    if (!___pdr_lua_isnumber(L, 2)) \
        ___pdr_luaL_argcheck(L, false, 2, "'number' expected"); \
    ___PDR_LUA_INTEGER len = ___pdr_luaL_checkinteger(L, 2); \
    size_t pread_snapshot = buf->pread; \
    size_t pwrite_snapshot = buf->pwrite; \
    size_t count_snapshot = buf->count; \
    ___pdr_lua_newtable(L); \
    int err = 0; \
    for (int i = 0; i < len; ++i) { \
        T val; \
        err = NS_CPPBUFFER::cb_read_convertable_data(buf, val); \
        if (err != 0) { \
            ___pdr_lua_pop(L, 1); \
            break; \
        } \
        PUSHNUM(L, val); \
        ___pdr_lua_seti(L, -2, i+1); \
    } \
    if (err != 0) { \
        buf->pread = pread_snapshot; \
        buf->pwrite = pwrite_snapshot; \
        buf->count = count_snapshot; \
        ___pdr_lua_pushnil(L); \
        ___pdr_lua_pushinteger(L, err); \
        return 2; \
    } \
    ___pdr_lua_pushinteger(L, err); \
    return 2;


#define ADAPTER_WRITE_PRECEDED_STRING(buffer,idx,err,len,HR,PRECEDED_TYPE) \
    if (!___pdr_lua_isstring(L, idx)) \
        err = ERR_TYPE_INVALID; \
    size_t len = 0; \
    const char * str = ___pdr_luaL_checklstring(L, idx, &len); \
    if (len > HR) \
        err = ERR_STRING_EXCEED_LIMIT; \
    err = NS_CPPBUFFER::cb_write_convertable_data(buffer, (PRECEDED_TYPE)len); \
    if (err == 0) {\
        err = NS_CPPBUFFER::cb_write_string(buffer, str, len); \
        if (err != 0) \
            NS_CPPBUFFER::cb_peekwrite(buffer, sizeof(PRECEDED_TYPE), false); \
    }


#define ADAPTER_READ_PRECEDED_STRING(buffer,err,len,PRECEDED_TYPE) \
    PRECEDED_TYPE len = 0; \
    err = NS_CPPBUFFER::cb_read_convertable_data(buffer, len); \
    if (err == 0) { \
        char * str = static_cast<char*>(CB_MALLOC(len)); \
        err = NS_CPPBUFFER::cb_read_string(buffer, str, len); \
        if (err == 0) { \
            ___pdr_lua_pushlstring(L, str, len); \
        } \
        CB_FREE(str); \
    } \


static void create_lbuffer_metatable(NS_PDR_SLUA::___pdr_lua_State * L, NS_PDR_SLUA::___pdr_luaL_Reg *funcs);

static int lbuffer_method_destroy(NS_PDR_SLUA::___pdr_lua_State * L);
static int lbuffer_method_tostring(NS_PDR_SLUA::___pdr_lua_State * L);
static int lbuffer_method_clear(NS_PDR_SLUA::___pdr_lua_State * L);
static int lbuffer_method_flush(NS_PDR_SLUA::___pdr_lua_State * L);
static int lbuffer_method_get_capacity(NS_PDR_SLUA::___pdr_lua_State * L);
static int lbuffer_method_get_emptysize(NS_PDR_SLUA::___pdr_lua_State * L);

/// write methods
static int lbuffer_method_write_sbyte(NS_PDR_SLUA::___pdr_lua_State * L)        { ADAPTER_WRITE_NUMERIC_FUNC_BODY(char, SCHAR_MIN, SCHAR_MAX, ___PDR_LUA_INTEGER, ___pdr_luaL_checkinteger) };
static int lbuffer_method_write_byte(NS_PDR_SLUA::___pdr_lua_State * L)         { ADAPTER_WRITE_NUMERIC_FUNC_BODY(unsigned char, 0, UCHAR_MAX, ___PDR_LUA_INTEGER, ___pdr_luaL_checkinteger); };
static int lbuffer_method_write_short(NS_PDR_SLUA::___pdr_lua_State * L)        { ADAPTER_WRITE_NUMERIC_FUNC_BODY(short, SHRT_MIN, SHRT_MAX, ___PDR_LUA_INTEGER, ___pdr_luaL_checkinteger); };
static int lbuffer_method_write_ushort(NS_PDR_SLUA::___pdr_lua_State * L)       { ADAPTER_WRITE_NUMERIC_FUNC_BODY(unsigned short, 0, USHRT_MAX, ___PDR_LUA_INTEGER, ___pdr_luaL_checkinteger); };
static int lbuffer_method_write_int(NS_PDR_SLUA::___pdr_lua_State * L)          { ADAPTER_WRITE_NUMERIC_FUNC_BODY(int, INT_MIN, INT_MAX, ___PDR_LUA_INTEGER, ___pdr_luaL_checkinteger); };
static int lbuffer_method_write_uint(NS_PDR_SLUA::___pdr_lua_State * L)         { ADAPTER_WRITE_NUMERIC_FUNC_BODY(unsigned int, 0, UINT_MAX, ___PDR_LUA_INTEGER, ___pdr_luaL_checkinteger); };
static int lbuffer_method_write_int64(NS_PDR_SLUA::___pdr_lua_State * L)        { ADAPTER_WRITE_NUMERIC_FUNC_BODY(long long, LLONG_MIN, LLONG_MAX, ___PDR_LUA_INTEGER, ___pdr_luaL_checkinteger); };
static int lbuffer_method_write_float(NS_PDR_SLUA::___pdr_lua_State * L)        { ADAPTER_WRITE_NUMERIC_FUNC_BODY(float, -3.4E+38, 3.4E+38, double, ___pdr_luaL_checknumber); };
static int lbuffer_method_write_double(NS_PDR_SLUA::___pdr_lua_State * L)       { ADAPTER_WRITE_NUMERIC_FUNC_BODY(double, -1.7E+308, 1.7E+308, double, ___pdr_luaL_checknumber); };

/// write list methods
static int lbuffer_method_write_sbyte_list(NS_PDR_SLUA::___pdr_lua_State * L)   { ADAPTER_WRITE_NUMERIC_LIST_FUNC_BODY(char, ___pdr_luaL_checkinteger) };
static int lbuffer_method_write_byte_list(NS_PDR_SLUA::___pdr_lua_State * L)    { ADAPTER_WRITE_NUMERIC_LIST_FUNC_BODY(unsigned char, ___pdr_luaL_checkinteger) };
static int lbuffer_method_write_short_list(NS_PDR_SLUA::___pdr_lua_State * L)   { ADAPTER_WRITE_NUMERIC_LIST_FUNC_BODY(short, ___pdr_luaL_checkinteger) };
static int lbuffer_method_write_ushort_list(NS_PDR_SLUA::___pdr_lua_State * L)  { ADAPTER_WRITE_NUMERIC_LIST_FUNC_BODY(unsigned short, ___pdr_luaL_checkinteger) };
static int lbuffer_method_write_int_list(NS_PDR_SLUA::___pdr_lua_State * L)     { ADAPTER_WRITE_NUMERIC_LIST_FUNC_BODY(int, ___pdr_luaL_checkinteger) };
static int lbuffer_method_write_uint_list(NS_PDR_SLUA::___pdr_lua_State * L)    { ADAPTER_WRITE_NUMERIC_LIST_FUNC_BODY(unsigned int, ___pdr_luaL_checkinteger) };
static int lbuffer_method_write_int64_list(NS_PDR_SLUA::___pdr_lua_State * L)   { ADAPTER_WRITE_NUMERIC_LIST_FUNC_BODY(long long, ___pdr_luaL_checkinteger) };
static int lbuffer_method_write_float_list(NS_PDR_SLUA::___pdr_lua_State * L)   { ADAPTER_WRITE_NUMERIC_LIST_FUNC_BODY(float, ___pdr_luaL_checknumber) };
static int lbuffer_method_write_double_list(NS_PDR_SLUA::___pdr_lua_State * L)  { ADAPTER_WRITE_NUMERIC_LIST_FUNC_BODY(double, ___pdr_luaL_checknumber) };


static int lbuffer_method_write_string(NS_PDR_SLUA::___pdr_lua_State * L);
static int lbuffer_method_write_cstring(NS_PDR_SLUA::___pdr_lua_State * L);
static int lbuffer_method_write_buffer(NS_PDR_SLUA::___pdr_lua_State * L);
static int lbuffer_method_write_format(NS_PDR_SLUA::___pdr_lua_State * L);

/// read methods
static int lbuffer_method_read_sbyte(NS_PDR_SLUA::___pdr_lua_State * L)         { ADAPTER_READ_NUMERIC_FUNC_BODY(char, ___pdr_lua_pushinteger); };
static int lbuffer_method_read_byte(NS_PDR_SLUA::___pdr_lua_State * L)          { ADAPTER_READ_NUMERIC_FUNC_BODY(unsigned char, ___pdr_lua_pushinteger); };
static int lbuffer_method_read_short(NS_PDR_SLUA::___pdr_lua_State * L)         { ADAPTER_READ_NUMERIC_FUNC_BODY(short, ___pdr_lua_pushinteger); };
static int lbuffer_method_read_ushort(NS_PDR_SLUA::___pdr_lua_State * L)        { ADAPTER_READ_NUMERIC_FUNC_BODY(unsigned short, ___pdr_lua_pushinteger); };
static int lbuffer_method_read_int(NS_PDR_SLUA::___pdr_lua_State * L)           { ADAPTER_READ_NUMERIC_FUNC_BODY(int, ___pdr_lua_pushinteger); };
static int lbuffer_method_read_uint(NS_PDR_SLUA::___pdr_lua_State * L)          { ADAPTER_READ_NUMERIC_FUNC_BODY(unsigned int, ___pdr_lua_pushinteger); };
static int lbuffer_method_read_int64(NS_PDR_SLUA::___pdr_lua_State * L)         { ADAPTER_READ_NUMERIC_FUNC_BODY(long long, ___pdr_lua_pushinteger); };
static int lbuffer_method_read_float(NS_PDR_SLUA::___pdr_lua_State * L)         { ADAPTER_READ_NUMERIC_FUNC_BODY(float, ___pdr_lua_pushnumber); };
static int lbuffer_method_read_double(NS_PDR_SLUA::___pdr_lua_State * L)        { ADAPTER_READ_NUMERIC_FUNC_BODY(double, ___pdr_lua_pushnumber); };

/// read list methods
static int lbuffer_method_read_sbyte_list(NS_PDR_SLUA::___pdr_lua_State * L)    { ADAPTER_READ_NUMERIC_LIST_FUNC_BODY(char, ___pdr_lua_pushinteger); };
static int lbuffer_method_read_byte_list(NS_PDR_SLUA::___pdr_lua_State * L)     { ADAPTER_READ_NUMERIC_LIST_FUNC_BODY(unsigned char, ___pdr_lua_pushinteger); };
static int lbuffer_method_read_short_list(NS_PDR_SLUA::___pdr_lua_State * L)    { ADAPTER_READ_NUMERIC_LIST_FUNC_BODY(short, ___pdr_lua_pushinteger); };
static int lbuffer_method_read_ushort_list(NS_PDR_SLUA::___pdr_lua_State * L)   { ADAPTER_READ_NUMERIC_LIST_FUNC_BODY(unsigned short, ___pdr_lua_pushinteger); };
static int lbuffer_method_read_int_list(NS_PDR_SLUA::___pdr_lua_State * L)      { ADAPTER_READ_NUMERIC_LIST_FUNC_BODY(int, ___pdr_lua_pushinteger); };
static int lbuffer_method_read_uint_list(NS_PDR_SLUA::___pdr_lua_State * L)     { ADAPTER_READ_NUMERIC_LIST_FUNC_BODY(unsigned int, ___pdr_lua_pushinteger); };
static int lbuffer_method_read_int64_list(NS_PDR_SLUA::___pdr_lua_State * L)    { ADAPTER_READ_NUMERIC_LIST_FUNC_BODY(long long, ___pdr_lua_pushinteger); };
static int lbuffer_method_read_float_list(NS_PDR_SLUA::___pdr_lua_State * L)    { ADAPTER_READ_NUMERIC_LIST_FUNC_BODY(float, ___pdr_lua_pushnumber); };
static int lbuffer_method_read_double_list(NS_PDR_SLUA::___pdr_lua_State * L)   { ADAPTER_READ_NUMERIC_LIST_FUNC_BODY(double, ___pdr_lua_pushnumber); };

static int lbuffer_method_read_string(NS_PDR_SLUA::___pdr_lua_State * L);
static int lbuffer_method_read_cstring(NS_PDR_SLUA::___pdr_lua_State * L);
static int lbuffer_method_read_buffer(NS_PDR_SLUA::___pdr_lua_State * L);
static int lbuffer_method_read_format(NS_PDR_SLUA::___pdr_lua_State * L);

static int lbuffer_method_skip(NS_PDR_SLUA::___pdr_lua_State * L);
static int lbuffer_method_count(NS_PDR_SLUA::___pdr_lua_State * L);
static int lbuffer_method_peek_write(NS_PDR_SLUA::___pdr_lua_State * L);
static int lbuffer_method_peek_read(NS_PDR_SLUA::___pdr_lua_State * L);

static int lbuffer_global_create(NS_PDR_SLUA::___pdr_lua_State * L);

// member functions
static NS_PDR_SLUA::___pdr_luaL_Reg lbuffer_methods[] = {
    {"__gc",                lbuffer_method_destroy},
    {"__tostring",          lbuffer_method_tostring},
    {"clear",               lbuffer_method_clear},
    {"flush",               lbuffer_method_flush},
    {"capacity",            lbuffer_method_get_capacity},
    {"emptysize",           lbuffer_method_get_emptysize},
    {"writesbyte",          lbuffer_method_write_sbyte},
    {"writebyte",           lbuffer_method_write_byte},
    {"writeshort",          lbuffer_method_write_short},
    {"writeushort",         lbuffer_method_write_ushort},
    {"writeint",            lbuffer_method_write_int},
    {"writeuint",           lbuffer_method_write_uint},
    {"writeint64",          lbuffer_method_write_int64},
    {"writefloat",          lbuffer_method_write_float},
    {"writedouble",         lbuffer_method_write_double},
    {"writelsbyte",         lbuffer_method_write_sbyte_list},
    {"writelbyte",          lbuffer_method_write_byte_list},
    {"writelshort",         lbuffer_method_write_short_list},
    {"writelushort",        lbuffer_method_write_ushort_list},
    {"writelint",           lbuffer_method_write_int_list},
    {"writeluint",          lbuffer_method_write_uint_list},
    {"writelint64",         lbuffer_method_write_int64_list},
    {"writelfloat",         lbuffer_method_write_float_list},
    {"writeldouble",        lbuffer_method_write_double_list},
    {"writestring",         lbuffer_method_write_string},
    {"writecstring",        lbuffer_method_write_cstring},
    {"writebuffer",         lbuffer_method_write_buffer},
    {"writeformat",         lbuffer_method_write_format},
    {"readsbyte",           lbuffer_method_read_sbyte},
    {"readbyte",            lbuffer_method_read_byte},
    {"readshort",           lbuffer_method_read_short},
    {"readushort",          lbuffer_method_read_ushort},
    {"readint",             lbuffer_method_read_int},
    {"readuint",            lbuffer_method_read_uint},
    {"readint64",           lbuffer_method_read_int64},
    {"readfloat",           lbuffer_method_read_float},
    {"readdouble",          lbuffer_method_read_double},
    {"readlsbyte",          lbuffer_method_read_sbyte_list},
    {"readlbyte",           lbuffer_method_read_byte_list},
    {"readlshort",          lbuffer_method_read_short_list},
    {"readlushort",         lbuffer_method_read_ushort_list},
    {"readlint",            lbuffer_method_read_int_list},
    {"readluint",           lbuffer_method_read_uint_list},
    {"readlint64",          lbuffer_method_read_int64_list},
    {"readlfloat",          lbuffer_method_read_float_list},
    {"readldouble",         lbuffer_method_read_double_list},
    {"readstring",          lbuffer_method_read_string},
    {"readcstring",         lbuffer_method_read_cstring},
    {"readbuffer",          lbuffer_method_read_buffer},
    {"readformat",          lbuffer_method_read_format},
    {"peekwrite",           lbuffer_method_peek_write},
    {"peekread",            lbuffer_method_peek_read},
    {"skip",                lbuffer_method_skip},
    {"count",               lbuffer_method_count},
    {NULL, NULL}
};

// static functions 
static NS_PDR_SLUA::___pdr_luaL_Reg lbuffer_func[] = {
    {"new",                 lbuffer_global_create},
    {NULL, NULL}
};


static int lbuffer_method_destroy(NS_PDR_SLUA::___pdr_lua_State * L)
{
    NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(L, 1);
	if (buf->buf != nullptr)
	{
		CB_FREE(buf->buf);
	}
    buf->buf = nullptr;
    return 0;
}

static int lbuffer_method_tostring(NS_PDR_SLUA::___pdr_lua_State * L)
{
    NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(L, 1);
    size_t size = NS_CPPBUFFER::cb_get_bytes_count(buf);
    unsigned char * output_buf;
    if (size > 0)
    {
		output_buf = static_cast<unsigned char*>(CB_MALLOC(size));
        char * hex_buf = static_cast<char*>(CB_MALLOC(size * 2 + 1));
        NS_CPPBUFFER::cb_read_raw(buf, (char*)output_buf, size);
        unsigned char hex_str[] = "0123456789abcdef";
        for (int i = 0; i < size; i++) {
            hex_buf[i * 2 + 0] = hex_str[(output_buf[i] >> 4) & 0x0F];
            hex_buf[i * 2 + 1] = hex_str[(output_buf[i]) & 0x0F];
        }
        hex_buf[size * 2] = '\0';
        ___pdr_lua_pushlstring(L, hex_buf, size * 2 + 1);
        CB_FREE(hex_buf);
        CB_FREE(output_buf);
    }
    else
    {
        ___pdr_lua_pushstring(L, "");
    }
    return 1;
}

static int lbuffer_method_clear(NS_PDR_SLUA::___pdr_lua_State * L)
{
    NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(L, 1);
    NS_CPPBUFFER::cb_clear(buf);
    return 0;
}

/* equivalent to tostring then clear */
static int lbuffer_method_flush(NS_PDR_SLUA::___pdr_lua_State * L)
{
    NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(L, 1);
    size_t size = NS_CPPBUFFER::cb_get_bytes_count(buf);
    char * output_buf;
    if (size > 0)
    {
		output_buf = static_cast<char*>(CB_MALLOC(size));
        NS_CPPBUFFER::cb_read_raw(buf, output_buf, size);
        NS_CPPBUFFER::cb_clear(buf);
        ___pdr_lua_pushlstring(L, output_buf, size);
        CB_FREE(output_buf);
    }
    else
    {
        ___pdr_lua_pushstring(L, "");
    }

    return 1;
}

static int lbuffer_method_get_capacity(NS_PDR_SLUA::___pdr_lua_State *L)
{
    NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(L, 1);
    ___pdr_lua_pushinteger(L, buf->capacity);
    return 1;
}

static int lbuffer_method_get_emptysize(NS_PDR_SLUA::___pdr_lua_State * L)
{
    NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(L, 1);
    size_t size = NS_CPPBUFFER::cb_get_empty_size(buf);
    ___pdr_lua_pushinteger(L, size);
    return 1;
}

static int lbuffer_method_write_string(NS_PDR_SLUA::___pdr_lua_State * L)
{
    NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(L, 1);
    size_t len;
    if (!___pdr_lua_isstring(L, 2))
        ___pdr_luaL_argcheck(L, false, 2, "'string' expected");

    const char * str = ___pdr_luaL_checklstring(L, 2, &len);
    int err = NS_CPPBUFFER::cb_write_string(buf, str, len);
    ___pdr_lua_pushinteger(L, err);
    return 1;
}

static int lbuffer_method_write_cstring(NS_PDR_SLUA::___pdr_lua_State* L)
{
    NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(L, 1);
    size_t len;
    if (!___pdr_lua_isstring(L, 2))
        ___pdr_luaL_argcheck(L, false, 2, "'string' expected");

    const char * str = ___pdr_luaL_checklstring(L, 2, &len);
    int err = NS_CPPBUFFER::cb_write_string(buf, str, len);
    if (err != 0)
    {
        ___pdr_lua_pushinteger(L, err);
        return 1;
    }
    if (str[len - 1] != '\0')
    {
        err = NS_CPPBUFFER::cb_write_convertable_data<char>(buf, '\0');
    }
    ___pdr_lua_pushinteger(L, err);
    return 1;
}

static int lbuffer_method_write_buffer(NS_PDR_SLUA::___pdr_lua_State * L)
{
    NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(L, 1);
    NS_CPPBUFFER::p_cppbuffer srcbuf = NS_CB_LUAADAPTER::checkbuffer(L, 2);

    if (!___pdr_lua_isnumber(L, 3))
        ___pdr_luaL_argcheck(L, false, 3, "'number' expected");
    if (!___pdr_lua_isnumber(L, 4))
        ___pdr_luaL_argcheck(L, false, 4, "'number' expected");
    
    ___PDR_LUA_INTEGER offset = ___pdr_luaL_checkinteger(L, 3);
    ___PDR_LUA_INTEGER len = ___pdr_luaL_checkinteger(L, 4);
    int err = NS_CPPBUFFER::cb_buffer_cpy(srcbuf, buf, offset, len);
    ___pdr_lua_pushinteger(L, err);
    return 1;
}

static int lbuffer_method_write_format(NS_PDR_SLUA::___pdr_lua_State * L)
{
    NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(L, 1);
    if (!___pdr_lua_isstring(L, 2))
        ___pdr_luaL_argcheck(L, false, 2, "'string' expected");
    size_t len;
    const char * format = ___pdr_luaL_checklstring(L, 2, &len);
    size_t count = buf->count;
    int err = 0;
    for (size_t i = 0; i < len; ++i)
    {
        char f = format[i];
        int stackidx = i + 3;
        switch (f)
        {
        case 'f': // write float
        { ADAPTER_WRITE_NUMERIC(buf, stackidx, err, float, -3.4E+38, 3.4E+38, double, ___pdr_luaL_checknumber) }; break;
        case 'd': // write double 
        { ADAPTER_WRITE_NUMERIC(buf, stackidx, err, double, -1.7E+308, 1.7E+308, double, ___pdr_luaL_checknumber) }; break;
        case 'b': // write singed char
        { ADAPTER_WRITE_NUMERIC(buf, stackidx, err, signed char, SCHAR_MIN, SCHAR_MAX, ___PDR_LUA_INTEGER, ___pdr_luaL_checkinteger) }; break;
        case 'B': // write char
        { ADAPTER_WRITE_NUMERIC(buf, stackidx, err, char, 0, UCHAR_MAX, ___PDR_LUA_INTEGER, ___pdr_luaL_checkinteger) }; break;
        case 's': // write short
        { ADAPTER_WRITE_NUMERIC(buf, stackidx, err, short, SHRT_MIN, SHRT_MAX, ___PDR_LUA_INTEGER, ___pdr_luaL_checkinteger) }; break;
        case 'S': // write unsigned short
        { ADAPTER_WRITE_NUMERIC(buf, stackidx, err, unsigned short, 0, USHRT_MAX, ___PDR_LUA_INTEGER, ___pdr_luaL_checkinteger) }; break;
        case 'i': // write int
        { ADAPTER_WRITE_NUMERIC(buf, stackidx, err, int, INT_MIN, INT_MAX, ___PDR_LUA_INTEGER, ___pdr_luaL_checkinteger) }; break;
        case 'I': // write unsigned int
        { ADAPTER_WRITE_NUMERIC(buf, stackidx, err, unsigned int, 0, UINT_MAX, ___PDR_LUA_INTEGER, ___pdr_luaL_checkinteger) }; break;
        case 'l': // write long long
        { ADAPTER_WRITE_NUMERIC(buf, stackidx, err, long long, LLONG_MIN, LLONG_MAX, ___PDR_LUA_INTEGER, ___pdr_luaL_checkinteger) }; break;
        case 'z': // write cstring
        {
            if (!___pdr_lua_isstring(L, stackidx))
            {
                err = ERR_TYPE_INVALID;
                break;
            }
            size_t cstrlen = 0;
            const char* str = ___pdr_luaL_checklstring(L, stackidx, &cstrlen);
            err = NS_CPPBUFFER::cb_write_string(buf, str, cstrlen);
            if (err != 0)
            {
                break;
            }
            if (str[cstrlen - 1] != '\0')
            {
                err = NS_CPPBUFFER::cb_write_convertable_data<char>(buf, '\0');
            }
        }; break;
        case 'c': // write string preceded by signed char
        { ADAPTER_WRITE_PRECEDED_STRING(buf, stackidx, err, scharlen, SCHAR_MAX, signed char) }; break;
        case 'C': // write string preceded by char
        { ADAPTER_WRITE_PRECEDED_STRING(buf, stackidx, err, charlen, UCHAR_MAX, char) }; break;
        case 'e': // write string preceded by short
        { ADAPTER_WRITE_PRECEDED_STRING(buf, stackidx, err, shortlen, SHRT_MAX, short) }; break;
        case 'E': // write string preceded by ushort
        { ADAPTER_WRITE_PRECEDED_STRING(buf, stackidx, err, ushortlen, USHRT_MAX, unsigned short) }; break;
        case 'a': // write string preceded by int
        { ADAPTER_WRITE_PRECEDED_STRING(buf, stackidx, err, intlen, INT_MAX, int) }; break;
        case 'A': // write string preceded by uint
        { ADAPTER_WRITE_PRECEDED_STRING(buf, stackidx, err, uintlen, UINT_MAX, unsigned int) }; break;
        case 'p': // write string preceded by long long
        { ADAPTER_WRITE_PRECEDED_STRING(buf, stackidx, err, lllen, LLONG_MAX, long long) }; break;
        default: // format invalid
            err = ERR_FORMAT_STRING_INVALID;
            break;
        }

        if (err != 0)
        {
            // failure recover
            buf->pwrite = buf->pread;
            buf->count = 0;
            NS_CPPBUFFER::cb_peekwrite(buf, count);
            if (err == ERR_TYPE_INVALID)
                ___pdr_luaL_argcheck(L, false, stackidx, "value type invalid");
            if (err == ERR_STRING_EXCEED_LIMIT)
                ___pdr_luaL_argcheck(L, false, stackidx, "string length exceed limit");
            break;
        }
    }
    ___pdr_lua_pushinteger(L, err);
    return 1;
}

static int lbuffer_method_read_string(NS_PDR_SLUA::___pdr_lua_State * L)
{
    NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(L, 1);
    if (!___pdr_lua_isnumber(L, 2))
        ___pdr_luaL_argcheck(L, false, 2, "'number' expected");

    ___PDR_LUA_INTEGER len = ___pdr_luaL_checkinteger(L, 2);

	char * str = static_cast<char*>(CB_MALLOC(len));
    int err = NS_CPPBUFFER::cb_read_string(buf, str, len);
    if (err != 0)
    {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushinteger(L, err);
		CB_FREE(str);
        return 2;
    }

    ___pdr_lua_pushlstring(L, str, len);
    ___pdr_lua_pushinteger(L, err);
	CB_FREE(str);
    return 2;
}

static int lbuffer_method_read_cstring(NS_PDR_SLUA::___pdr_lua_State* L)
{
    NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(L, 1);
    size_t len = 0;
    bool found = false;
    for (size_t i = 0; i < buf->count; ++i)
    {
        if (buf->buf[(i + buf->pread) % buf->capacity] != '\0')
            ++len;
        else
        {
            found = true;
            ++len;
            break;
        }
    }
    if (!found)
    {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushinteger(L, -1);
        return 2;
    }
    char* str = static_cast<char*>(CB_MALLOC(len));
    int err = NS_CPPBUFFER::cb_read_string(buf, str, len);
    if (err != 0)
    {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushinteger(L, err);
        CB_FREE(str);
        return 2;
    }

    ___pdr_lua_pushstring(L, str);
    ___pdr_lua_pushinteger(L, err);
    CB_FREE(str);
    return 2;
}

static int lbuffer_method_read_buffer(NS_PDR_SLUA::___pdr_lua_State * L)
{
    NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(L, 1);
    NS_CPPBUFFER::p_cppbuffer destbuf = NS_CB_LUAADAPTER::checkbuffer(L, 2);

    if (!___pdr_lua_isnumber(L, 3))
        ___pdr_luaL_argcheck(L, false, 3, "'number' expected");
    if (!___pdr_lua_isnumber(L, 4))
        ___pdr_luaL_argcheck(L, false, 4, "'number' expected");

    ___PDR_LUA_INTEGER offset = ___pdr_luaL_checkinteger(L, 3);
    ___PDR_LUA_INTEGER len = ___pdr_luaL_checkinteger(L, 4);
    int err = NS_CPPBUFFER::cb_buffer_cpy(buf, destbuf, offset, len);
    ___pdr_lua_pushinteger(L, err);
    return 1;
}


static int lbuffer_method_read_format(NS_PDR_SLUA::___pdr_lua_State * L)
{
    NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(L, 1);
    if (!___pdr_lua_isstring(L, 2))
        ___pdr_luaL_argcheck(L, false, 2, "'string' expected");
    size_t len;
    const char* format = ___pdr_luaL_checklstring(L, 2, &len);
    size_t count = buf->count;
    int err = 0;
    int top = ___pdr_lua_gettop(L);
    for (size_t i = 0; i < len; ++i)
    {
        char f = format[i];
        switch (f)
        {
        case 'f': // read float
        { ADAPTER_READ_NUMERIC(buf, err, float, ___pdr_lua_pushnumber) }; break;
        case 'd': // read double 
        { ADAPTER_READ_NUMERIC(buf, err, double, ___pdr_lua_pushnumber) }; break;
        case 'b': // read unsigned char
        { ADAPTER_READ_NUMERIC(buf, err, signed char, ___pdr_lua_pushinteger) }; break;
        case 'B': // read char
        { ADAPTER_READ_NUMERIC(buf, err, char, ___pdr_lua_pushinteger) }; break;
        case 's': // read short
        { ADAPTER_READ_NUMERIC(buf, err, short, ___pdr_lua_pushinteger) }; break;
        case 'S': // read unsigned short
        { ADAPTER_READ_NUMERIC(buf, err, unsigned short, ___pdr_lua_pushinteger) }; break;
        case 'i': // read int
        { ADAPTER_READ_NUMERIC(buf, err, int, ___pdr_lua_pushinteger) }; break;
        case 'I': // read unsigned int
        { ADAPTER_READ_NUMERIC(buf, err, unsigned int, ___pdr_lua_pushinteger) }; break;
        case 'l': // read long long
        { ADAPTER_READ_NUMERIC(buf, err, long long, ___pdr_lua_pushinteger) }; break;
        case 'z': // read cstring
        {
            size_t cstrlen = 0;
            bool found = false;
            for (size_t j = 0; j < buf->count; ++j)
            {
                if (buf->buf[j + buf->pread % buf->capacity] != '\0')
                    ++cstrlen;
                else
                {
                    found = true;
                    ++cstrlen;
                    break;
                }
            }
            if (found)
            {
                char* str = static_cast<char*>(CB_MALLOC(cstrlen));
                err = NS_CPPBUFFER::cb_read_string(buf, str, cstrlen);
                if (err == 0)
                    ___pdr_lua_pushstring(L, str);
                else
                    ___pdr_lua_pushnil(L);

                CB_FREE(str);
            }
            else
            {
                ___pdr_lua_pushnil(L);
                err = ERR_CSTRING_ENDING_NOT_FOUND;
            }
        }; break;
        case 'c': // read string preceded by signed char
        { ADAPTER_READ_PRECEDED_STRING(buf, err, scharlen, signed char) }; break;
        case 'C': // read string preceded by char
        { ADAPTER_READ_PRECEDED_STRING(buf, err, charlen, char) }; break;
        case 'e': // read string preceded by short
        { ADAPTER_READ_PRECEDED_STRING(buf, err, shortlen, short) }; break;
        case 'E': // read string preceded by unsigned short
        { ADAPTER_READ_PRECEDED_STRING(buf, err, ushortlen, unsigned short) }; break;
        case 'a': // read string preceded by int
        { ADAPTER_READ_PRECEDED_STRING(buf, err, intlen, int) }; break;
        case 'A': // read string preceded by unsigned int
        { ADAPTER_READ_PRECEDED_STRING(buf, err, uintlen, unsigned int) }; break;
        case 'p': // read string preceded by long long
        { ADAPTER_READ_PRECEDED_STRING(buf, err, lllen, long long) }; break;
        default: // invalid format
            err = ERR_FORMAT_STRING_INVALID;
            break;
        }

        if (err != 0)
        {
            // failure recover
            buf->pread = buf->pwrite;
            buf->count = 0;
            NS_CPPBUFFER::cb_peekread(buf, count, false);
            ___pdr_lua_settop(L, top);
            //___pdr_lua_pop(L, i + 1);
            break;
        }
    }
    ___pdr_lua_pushinteger(L, err);
    if (err == 0)
        return len + 1;
    else
        return 1;
}

static int lbuffer_method_skip(NS_PDR_SLUA::___pdr_lua_State * L)
{
    NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(L, 1);
    if (!___pdr_lua_isnumber(L, 2))
        ___pdr_luaL_argcheck(L, false, 2, "'number' expected");

    ___PDR_LUA_INTEGER skipcnt = ___pdr_luaL_checkinteger(L, 2);
    int err = NS_CPPBUFFER::cb_skip(buf, skipcnt);
    ___pdr_lua_pushinteger(L, err);
    return 1;
}

static int lbuffer_method_count(NS_PDR_SLUA::___pdr_lua_State * L)
{
    NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(L, 1);
    ___pdr_lua_pushinteger(L, buf->count);
    return 1;
}


static int lbuffer_method_peek_write(NS_PDR_SLUA::___pdr_lua_State* L)
{
    ___pdr_lua_settop(L, 3);
    NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(L, 1);
    if (!___pdr_lua_isnumber(L, 2))
        ___pdr_luaL_argcheck(L, false, 2, "'number' expected");

    ___PDR_LUA_INTEGER count = ___pdr_luaL_checkinteger(L, 2);
    bool forward = true;
    if (!___pdr_lua_isnil(L, 3))
    {
        if (!___pdr_lua_isboolean(L, 3))
            ___pdr_luaL_argcheck(L, false, 3, "'boolean' expected");
        forward = ___pdr_lua_toboolean(L, 3) == 1 ? true: false;
    }

    int err = NS_CPPBUFFER::cb_peekwrite(buf, count, forward);
    ___pdr_lua_pushinteger(L, err);
    return 1;
}

static int lbuffer_method_peek_read(NS_PDR_SLUA::___pdr_lua_State* L)
{
    ___pdr_lua_settop(L, 3);
    NS_CPPBUFFER::p_cppbuffer buf = NS_CB_LUAADAPTER::checkbuffer(L, 1);
    if (!___pdr_lua_isnumber(L, 2))
        ___pdr_luaL_argcheck(L, false, 2, "'number' expected");

    ___PDR_LUA_INTEGER count = ___pdr_luaL_checkinteger(L, 2);
    bool forward = true;
    if (!___pdr_lua_isnil(L, 3))
    {
        if (!___pdr_lua_isboolean(L, 3))
            ___pdr_luaL_argcheck(L, false, 3, "'boolean' expected");
        forward = ___pdr_lua_toboolean(L, 3) == 1 ? true: false;
    }

    int err = NS_CPPBUFFER::cb_peekread(buf, count, forward);
    ___pdr_lua_pushinteger(L, err);
    return 1;
}

static int lbuffer_global_create(NS_PDR_SLUA::___pdr_lua_State * L)
{
    using namespace NS_CPPBUFFER;
    ___pdr_lua_settop(L, 2);

    ___PDR_LUA_INTEGER buf_size = 4;
    ___PDR_LUA_INTEGER endian = 0;
    if (___pdr_lua_isnumber(L, 1))
        buf_size = ___pdr_luaL_checkinteger(L, 1);

    if (___pdr_lua_isnumber(L, 2))
        endian = ___pdr_luaL_checkinteger(L, 2);

    if (endian != 0 && endian != 1)
        ___pdr_luaL_argcheck(L, false, 2, "invalid value");

    // ___pdr_luaL_argcheck(L, buf_size >= 1, 1, "invalid buf size");
    if (buf_size <= 0)
        buf_size = 1;

    NS_CB_LUAADAPTER::newlbuffer(L, buf_size, (NS_CPPBUFFER::CB_EENDIAN)endian);
    return 1;
}

static void create_lbuffer_metatable(NS_PDR_SLUA::___pdr_lua_State * L, NS_PDR_SLUA::___pdr_luaL_Reg *funcs)
{
    ___pdr_luaL_newmetatable(L, "lbuffer{meta}");      // mt 
    ___pdr_lua_pushstring(L, "__index");               // mt, "__index"
    ___pdr_lua_newtable(L);                            // mt, "__index", indextable

    for (; funcs->name; ++funcs)
    {
        ___pdr_lua_pushstring(L, funcs->name);         // mt, "__index", indextable, funcname 
        ___pdr_lua_pushcfunction(L, funcs->func);      // mt, "__index", indextable, funcname, func
        ___pdr_lua_rawset(L, funcs->name[0] == '_' ? -5 : -3);
    }

    ___pdr_lua_rawset(L, -3);
    ___pdr_lua_pop(L, 1);
}

NS_CPPBUFFER::p_cppbuffer NS_CB_LUAADAPTER::checkbuffer(NS_PDR_SLUA::___pdr_lua_State * L, int index)
{
    void * ud = ___pdr_luaL_checkudata(L, index, "lbuffer{meta}");
    ___pdr_luaL_argcheck(L, ud != NULL, index, "'lbuffer' expected");
    return (NS_CPPBUFFER::t_cppbuffer *) ud;
}

static size_t lbuffer_id_inc = 0;
size_t lbuffer_gen_id()
{
	size_t id = lbuffer_id_inc;
	lbuffer_id_inc = lbuffer_id_inc + 1;
	return id;
}

NS_CPPBUFFER::p_cppbuffer NS_CB_LUAADAPTER::newlbuffer(NS_PDR_SLUA::___pdr_lua_State *L, size_t bufsize, NS_CPPBUFFER::CB_EENDIAN endian)
{
    size_t bytes = sizeof(NS_CPPBUFFER::t_cppbuffer);
    NS_CPPBUFFER::t_cppbuffer * buffer = (NS_CPPBUFFER::t_cppbuffer *)___pdr_lua_newuserdata(L, bytes);
	buffer->id = lbuffer_gen_id();
	buffer->buf = static_cast<char*>(CB_MALLOC(bufsize));

    ___pdr_luaL_getmetatable(L, "lbuffer{meta}");
    ___pdr_lua_setmetatable(L, -2);

    buffer->capacity = bufsize;
    buffer->count = 0;
    buffer->pread = buffer->pwrite = 0;
    buffer->endian = endian;
    return buffer;
}

bool NS_CB_LUAADAPTER::ismtcreated(NS_PDR_SLUA::___pdr_lua_State* L)
{
    ___pdr_luaL_getmetatable(L, "lbuffer{meta}");
    bool iscreated = !(___pdr_lua_type(L, -1) == ___PDR_LUA_TNIL);
    ___pdr_lua_pop(L, 1);
    return iscreated;
}

int NS_CB_LUAADAPTER::open(NS_PDR_SLUA::___pdr_lua_State *L)
{
    create_lbuffer_metatable(L, lbuffer_methods);

#if ___PDR_LUA_VERSION_NUM > 501 && !defined(___PDR_LUA_COMPAT_MODULE)
    ___pdr_lua_newtable(L);
    ___pdr_luaL_setfuncs(L, lbuffer_func, 0);
#else
    ___pdr_luaL_openlib(L, "luabuffer", lbuffer_func, 0);
#endif
    return 1;
}

int NS_CB_LUAADAPTER::init(NS_PDR_SLUA::___pdr_lua_State * L)
{
    ___pdr_luaL_getsubtable(L, ___PDR_LUA_REGISTRYINDEX, ___PDR_LUA_PRELOAD_TABLE);

    ___pdr_lua_pushcfunction(L, open);
    ___pdr_lua_setfield(L, -2, "luabuffer");
    
    ___pdr_lua_pop(L, 1);

    return 0;
}

#undef ERR_TYPE_INVALID
#undef ERR_STRING_EXCEED_LIMIT
#undef ERR_FORMAT_STRING_INVALID
#undef ERR_CSTRING_ENDING_NOT_FOUND 
