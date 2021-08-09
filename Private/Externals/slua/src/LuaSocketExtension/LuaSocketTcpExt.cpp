#include "LuaSocketTcpExt.h"
#if PLATFORM_WINDOWS
#include "Runtime/Core/Public/Windows/AllowWindowsPlatformTypes.h"
#endif

#include "LuaSocketExtDefines.h"
#include "luasocket/auxiliar.h"
#include "luasocket/tcp.h"
#include "LuaSocketTcpExt.h"
#include "luasocket/luasocket.h"


/* min and max macros */
#ifndef ___PDR_MIN
#define ___PDR_MIN(x, y) ((x) < (y) ? x : y)
#endif
#ifndef ___PDR_MAX
#define ___PDR_MAX(x, y) ((x) > (y) ? x : y)
#endif

typedef unsigned char byte;

#if PLATFORM_WINDOWS
#include "Runtime/Core/Public/Windows/HideWindowsPlatformTypes.h"
#endif

static int ExtendMetamethods(NS_PDR_SLUA::___pdr_lua_State* L, const char* name);

/*
 * read socket io data into luabuffer
 */
int NS_LUASOCKET_EXT::ReadIO(___pdr_p_buffer socketbuf, NS_CPPBUFFER::p_cppbuffer buffer, size_t wanted, size_t& readcount)
{
    ___pdr_p_io io = socketbuf->io;
    ___pdr_p_timeout tm = socketbuf->tm;
    int err = PDR_IO_DONE;
    size_t emptysize = NS_CPPBUFFER::cb_get_empty_size(buffer);
    if (emptysize <= 0)
        return -1;
    wanted = wanted > emptysize ? emptysize : wanted;
    readcount = 0;
    size_t io_readcount;
    size_t fin = buffer->pwrite + wanted;
    if (fin >= buffer->capacity)
    {
        fin = fin % buffer->capacity;
        size_t len1 = buffer->capacity - buffer->pwrite;
        size_t len2 = wanted - len1;
        err = io->recv(io->ctx, buffer->buf + buffer->pwrite, len1, &io_readcount, tm);
        buffer->count += io_readcount;
        readcount += io_readcount;
        buffer->pwrite += io_readcount;
        buffer->pwrite = buffer->pwrite % buffer->capacity;
        if (err == PDR_IO_DONE && len2 > 0)
        {
            err = io->recv(io->ctx, buffer->buf, len2, &io_readcount, tm);
            buffer->count += io_readcount;
            readcount += io_readcount;
            buffer->pwrite = io_readcount;
        }
    }
    else
    {
        err = io->recv(io->ctx, buffer->buf + buffer->pwrite, wanted, &io_readcount, tm);
        buffer->count += io_readcount;
        readcount += io_readcount;
        buffer->pwrite += io_readcount;
    }

    return err;
}

/*
 * write data from luabuffer into socket io
 */
#define ___PDR_STEPSIZE 8192
 //#define STEPSIZE 1
int NS_LUASOCKET_EXT::WriteIO(___pdr_p_buffer socketbuf, NS_CPPBUFFER::p_cppbuffer buffer)
{
    ___pdr_p_io io = socketbuf->io;
    ___pdr_p_timeout tm = socketbuf->tm;
    size_t count = buffer->count;
    size_t total = 0;
    int err = PDR_IO_DONE;
    while (total < count && err == PDR_IO_DONE)
    {
        size_t done = 0;
        size_t step = ___PDR_MIN(count - total, ___PDR_STEPSIZE);
        size_t pstart = buffer->pread + total;
        if (pstart > buffer->capacity)
            pstart = pstart % buffer->capacity;
        size_t pend = pstart + step;
        if (pend > buffer->capacity)
            step = ___PDR_MIN(buffer->capacity - pstart, step);
        err = io->send(io->ctx, buffer->buf + pstart, step, &done, tm);
        total += done;
    }

    NS_CPPBUFFER::cb_skip(buffer, total);
    return err;
}


static int TcpExtendMethod_ReadIO(NS_PDR_SLUA::___pdr_lua_State* L);
static int TcpExtendMethod_WriteIO(NS_PDR_SLUA::___pdr_lua_State* L);

static NS_PDR_SLUA::___pdr_luaL_Reg tcp_extend_methods[] = {
    {"readio",                  TcpExtendMethod_ReadIO},
    {"writeio",                 TcpExtendMethod_WriteIO},
    {NULL, NULL}
};

int NS_LUASOCKET_EXT::TcpExtender(NS_PDR_SLUA::___pdr_lua_State* L)
{
    int err = ExtendMetamethods(L, "tcp{master}");
    if (err != 0)
        return err;
    err = ExtendMetamethods(L, "tcp{client}");
    if (err != 0)
        return err;
    err = ExtendMetamethods(L, "tcp{server}");

    ___pdr_lua_pushinteger(L, err);
    return 1;
}

/*
return: errorcode, 0 means everything is ok, others means something wrong happened
 */
static int ExtendMetamethods(NS_PDR_SLUA::___pdr_lua_State* L, const char* name)
{
    ___pdr_luaL_getmetatable(L, name);         // mt,
    if (___pdr_lua_isnil(L, -1))
    {
        ___pdr_lua_pop(L, 1);
        return -1;
    }
    ___pdr_lua_pushstring(L, "__index");       // mt, "__index"
    ___pdr_lua_rawget(L, -2);                  // mt, it
    NS_PDR_SLUA::___pdr_luaL_Reg* funcs = tcp_extend_methods;
    for (; funcs->name; ++funcs)
    {
        ___pdr_lua_pushstring(L, funcs->name);     // mt, it, funcname
        ___pdr_lua_pushcfunction(L, funcs->func);  // mt, it, funcname, func
        ___pdr_lua_rawset(L, -3);                  // mt, it
    }
    ___pdr_lua_pop(L, 2);
    return 0;
}

#define LBUFFER_READ_LEN_PER_CALL 8192
static int TcpExtendMethod_ReadIO(NS_PDR_SLUA::___pdr_lua_State* L)
{
    NS_PDR_SLUA::___pdr_p_tcp tcp = (NS_PDR_SLUA::___pdr_p_tcp) ___pdr_auxiliar_checkclass(L, "tcp{client}", 1);
    NS_CPPBUFFER::p_cppbuffer buffer = NS_CB_LUAADAPTER::newlbuffer(L, LBUFFER_READ_LEN_PER_CALL);
    size_t readcount;
    int err = NS_LUASOCKET_EXT::ReadIO(&tcp->buf, buffer, LBUFFER_READ_LEN_PER_CALL, readcount);
    if (err == NS_PDR_SLUA::PDR_IO_CLOSED || err == NS_PDR_SLUA::PDR_IO_UNKNOWN)
        ___pdr_lua_pushinteger(L, err);
    else
        ___pdr_lua_pushinteger(L, readcount);
    return 2;
}

static int TcpExtendMethod_WriteIO(NS_PDR_SLUA::___pdr_lua_State* L)
{
    NS_PDR_SLUA::___pdr_p_tcp tcp = (NS_PDR_SLUA::___pdr_p_tcp) ___pdr_auxiliar_checkclass(L, "tcp{client}", 1);
    NS_CPPBUFFER::p_cppbuffer buffer = (NS_CPPBUFFER::p_cppbuffer) NS_CB_LUAADAPTER::checkbuffer(L, 2);
    int err = NS_LUASOCKET_EXT::WriteIO(&tcp->buf, buffer);
    ___pdr_lua_pushinteger(L, err);
    return 1;
}
