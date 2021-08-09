
#include "LuaSocketReadBufferCheckExt.h"
#include "BufferExtension/cppbuffer.h"
#include "BufferExtension/luaadapter.h"
#include "LuaSocketExtDefines.h"

#if PLATFORM_WINDOWS
#include "Runtime/Core/Public/Windows/AllowWindowsPlatformTypes.h"
#endif
#include "lauxlib.h"
#include "luasocket/socket.h"
#if PLATFORM_WINDOWS
#include "Runtime/Core/Public/Windows/HideWindowsPlatformTypes.h"
#endif

#if PLATFORM_WINDOWS
#include "Runtime/Core/Public/Windows/AllowWindowsPlatformTypes.h"
#include <WinSock2.h>
#include "Runtime/Core/Public/Windows/HideWindowsPlatformTypes.h"
#else
#include <sys/ioctl.h>
#include <sys/select.h>
#include <arpa/inet.h>
#endif

// check if socket is ready for data receive
// a more efficient implementation compare to luasocket-select for client
//int NS_LUASOCKET_EXT::IsSocketReadyForRead(___pdr_lua_State* L)
//{
//    if (!lua_isinteger(L, 1) && !lua_isnumber(L, 1))
//        luaL_argcheck(L, false, 1, "fd expected, fd should be an integer");
//    if (!lua_isnumber(L, 2))
//        luaL_argcheck(L, false, 1, "tm expected, tm should be a number");
//
//    t_socket fdnum = (t_socket)luaL_checkinteger(L, 1);
//    double t = luaL_checknumber(L, 2);
//
//    fd_set rset;
//    t_socket fd = (fdnum >= 0) ? fdnum : SOCKET_INVALID;
//    if (fd == SOCKET_INVALID)
//    {
//        lua_pushinteger(L, -1);
//        return 1;
//    }
//#if !PLATFORM_WINDOWS
//    if (fd >= FD_SETSIZE)
//    {
//        lua_pushinteger(L, -1);
//        return 1;
//    }
//#endif
//    FD_ZERO(&rset);
//    FD_SET(fd, &rset);
//
//    t_timeout tm;
//    timeout_init(&tm, t, -1);
//    timeout_markstart(&tm);
//    int ret = socket_select(fd + 1, &rset, NULL, NULL, &tm);
//    if (ret > 0)
//    {
//        if (FD_ISSET(fd, &rset))
//        {
//            // ready
//            lua_pushinteger(L, 0);
//            return 1;
//        }
//        else
//        {
//            // not ready
//            lua_pushinteger(L, 1);
//            return 1;
//        }
//    }
//    if (ret == 0)
//    {
//        // timeout
//        lua_pushinteger(L, 2);
//        return 1;
//    }
//
//    lua_pushinteger(L, 3);
//    return 1;
//}

int NS_LUASOCKET_EXT::GetErrno(NS_PDR_SLUA::___pdr_lua_State* L)
{
#if PLATFORM_WINDOWS
    ___pdr_lua_pushinteger(L, WSAGetLastError());
#else
    ___pdr_lua_pushinteger(L, errno);
#endif
    return 1;
}

int NS_LUASOCKET_EXT::IsSocketReadyForRead_SELECT(NS_PDR_SLUA::___pdr_lua_State* L)
{
    if (!___pdr_lua_isinteger(L, 1) && !___pdr_lua_isnumber(L, 1))
        ___pdr_luaL_argcheck(L, false, 1, "fd expected, fd should be an integer");

    ___pdr_t_socket fdnum = (___pdr_t_socket)___pdr_luaL_checkinteger(L, 1);
    //double t = luaL_checknumber(L, 2);

    fd_set rset;
    ___pdr_t_socket fd = (fdnum >= 0) ? fdnum : SOCKET_INVALID;

    if (fd == SOCKET_INVALID)
    {
        ___pdr_lua_pushinteger(L, -1);
        return 1;
    }

#if !PLATFORM_WINDOWS
    if (fd >= FD_SETSIZE)
    {
        ___pdr_lua_pushinteger(L, -1);
        return 1;
    }
#endif
    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    struct timeval tm;
    tm.tv_sec = 0;
    tm.tv_usec = 0;
    int ret = select(fd + 1, &rset, NULL, NULL, &tm);
    if (ret > 0)
    {
        if (FD_ISSET(fd, &rset))
        {
            // ready
            ___pdr_lua_pushinteger(L, 0);
            return 1;
        }
        else
        {
            // not ready
            ___pdr_lua_pushinteger(L, 1);
            return 1;
        }
    }
    if (ret == 0)
    {
        // timeout
        ___pdr_lua_pushinteger(L, 2);
        return 1;
    }

    ___pdr_lua_pushinteger(L, 3);
    return 1;
}

int NS_LUASOCKET_EXT::IsSocketReadyForRead_IOCTL(NS_PDR_SLUA::___pdr_lua_State* L)
{
    if (!___pdr_lua_isinteger(L, 1) && !___pdr_lua_isnumber(L, 1))
        ___pdr_luaL_argcheck(L, false, 1, "fd expected, fd should be an integer");

    ___pdr_t_socket fdnum = (___pdr_t_socket)___pdr_luaL_checkinteger(L, 1);
    ___pdr_t_socket fd = (fdnum >= 0) ? fdnum : SOCKET_INVALID;
    if (fd == SOCKET_INVALID)
    {
        ___pdr_lua_pushinteger(L, -1);
        return 1;
    }

#if !PLATFORM_WINDOWS
    if (fd >= FD_SETSIZE)
    {
        ___pdr_lua_pushinteger(L, -1);
        return 1;
    }
#endif

    u_long count = 0;
    int err;
#if PLATFORM_WINDOWS
    err = ioctlsocket(fd, FIONREAD, &count);
#else
    err = ioctl(fd, FIONREAD, &count);
#endif

    if (count > 0)
        ___pdr_lua_pushinteger(L, 0);
    else
        ___pdr_lua_pushinteger(L, 1);
    ___pdr_lua_pushinteger(L, err);
    return 2;
}

// return lua buffer
int NS_LUASOCKET_EXT::ByteOrderHostToNetwork32(NS_PDR_SLUA::___pdr_lua_State* L)
{
    if (!___pdr_lua_isinteger(L, 1))
        ___pdr_luaL_argcheck(L, false, 1, "integer expected");
    int val = (int)___pdr_luaL_checkinteger(L, 1);
    uint32 tmp = *((uint32*)(&val));

    uint32 result = htonl(tmp);
    NS_CPPBUFFER::p_cppbuffer buffer = NS_CB_LUAADAPTER::newlbuffer(L, 4, NS_CPPBUFFER::CB_EENDIAN::CB_BIG_ENDIAN);
    NS_CPPBUFFER::cb_write_raw(buffer, (char*)(&result), sizeof(uint32));
    return 1;
}

// return int32
int NS_LUASOCKET_EXT::ByteOrderNetworkToHost32(NS_PDR_SLUA::___pdr_lua_State* L)
{
    NS_CPPBUFFER::p_cppbuffer buffer = NS_CB_LUAADAPTER::checkbuffer(L, 1);
    uint32 tmp;
    NS_CPPBUFFER::cb_read_raw(buffer, (char*)(&tmp), sizeof(uint32), 0);
    NS_CPPBUFFER::cb_skip(buffer, sizeof(uint32));
    uint32 result = ntohl(tmp);
    int val = *((int*)(&result));
    ___pdr_lua_pushinteger(L, val);
    return 1;
}

