/*=========================================================================*\
* Input/Output interface for Lua programs
* LuaSocket toolkit
\*=========================================================================*/
#include "buffer.h"
#include "lua.h"
#include "lauxlib.h"


namespace NS_PDR_SLUA {

/*=========================================================================*\
* Internal function prototypes
\*=========================================================================*/
static int recvraw(___pdr_p_buffer buf, size_t wanted, ___pdr_luaL_Buffer *b);
static int recvline(___pdr_p_buffer buf, ___pdr_luaL_Buffer *b);
static int recvall(___pdr_p_buffer buf, ___pdr_luaL_Buffer *b);
static int buffer_get(___pdr_p_buffer buf, const char **data, size_t *count);
static void buffer_skip(___pdr_p_buffer buf, size_t count);
static int sendraw(___pdr_p_buffer buf, const char *data, size_t count, size_t *sent);

/* min and max macros */
#ifndef ___PDR_MIN
#define ___PDR_MIN(x, y) ((x) < (y) ? x : y)
#endif
#ifndef ___PDR_MAX
#define ___PDR_MAX(x, y) ((x) > (y) ? x : y)
#endif

/*=========================================================================*\
* Exported functions
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Initializes module
\*-------------------------------------------------------------------------*/
int ___pdr_buffer_open(___pdr_lua_State *L) {
    (void) L;
    return 0;
}

/*-------------------------------------------------------------------------*\
* Initializes C structure 
\*-------------------------------------------------------------------------*/
void ___pdr_buffer_init(___pdr_p_buffer buf, ___pdr_p_io io, ___pdr_p_timeout tm) {
    buf->first = buf->last = 0;
    buf->io = io;
    buf->tm = tm;
    buf->received = buf->sent = 0;
    buf->birthday = ___pdr_timeout_gettime();
}

/*-------------------------------------------------------------------------*\
* object:getstats() interface
\*-------------------------------------------------------------------------*/
int ___pdr_buffer_meth_getstats(___pdr_lua_State *L, ___pdr_p_buffer buf) {
    ___pdr_lua_pushnumber(L, (___pdr_lua_Number) buf->received);
    ___pdr_lua_pushnumber(L, (___pdr_lua_Number) buf->sent);
    ___pdr_lua_pushnumber(L, ___pdr_timeout_gettime() - buf->birthday);
    return 3;
}

/*-------------------------------------------------------------------------*\
* object:setstats() interface
\*-------------------------------------------------------------------------*/
int ___pdr_buffer_meth_setstats(___pdr_lua_State *L, ___pdr_p_buffer buf) {
    buf->received = (long) ___pdr_luaL_optnumber(L, 2, (___pdr_lua_Number) buf->received); 
    buf->sent = (long) ___pdr_luaL_optnumber(L, 3, (___pdr_lua_Number) buf->sent); 
    if (___pdr_lua_isnumber(L, 4)) buf->birthday = ___pdr_timeout_gettime() - ___pdr_lua_tonumber(L, 4);
    ___pdr_lua_pushnumber(L, 1);
    return 1;
}

/*-------------------------------------------------------------------------*\
* object:send() interface
\*-------------------------------------------------------------------------*/
int ___pdr_buffer_meth_send(___pdr_lua_State *L, ___pdr_p_buffer buf) {
    int top = ___pdr_lua_gettop(L);
    int err = PDR_IO_DONE;
    size_t size = 0, sent = 0;
    const char *data = ___pdr_luaL_checklstring(L, 2, &size);
    long start = (long) ___pdr_luaL_optnumber(L, 3, 1);
    long end = (long) ___pdr_luaL_optnumber(L, 4, -1);
#ifdef ___PDR_LUASOCKET_DEBUG
    ___pdr_p_timeout tm = ___pdr_timeout_markstart(buf->tm);
#endif
    if (start < 0) start = (long) (size+start+1);
    if (end < 0) end = (long) (size+end+1);
    if (start < 1) start = (long) 1;
    if (end > (long) size) end = (long) size;
    if (start <= end) err = sendraw(buf, data+start-1, end-start+1, &sent);
    /* check if there was an error */
    if (err != PDR_IO_DONE) {
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushstring(L, buf->io->error(buf->io->ctx, err)); 
        ___pdr_lua_pushnumber(L, (___pdr_lua_Number) (sent+start-1));
    } else {
        ___pdr_lua_pushnumber(L, (___pdr_lua_Number) (sent+start-1));
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushnil(L);
    }
#ifdef ___PDR_LUASOCKET_DEBUG
    /* push time elapsed during operation as the last return value */
    ___pdr_lua_pushnumber(L, ___pdr_timeout_gettime() - ___pdr_timeout_getstart(tm));
#endif
    return ___pdr_lua_gettop(L) - top;
}

/*-------------------------------------------------------------------------*\
* object:receive() interface
\*-------------------------------------------------------------------------*/
int ___pdr_buffer_meth_receive(___pdr_lua_State *L, ___pdr_p_buffer buf) {
    int err = PDR_IO_DONE, top = ___pdr_lua_gettop(L);
    ___pdr_luaL_Buffer b;
    size_t size;
    const char *part = ___pdr_luaL_optlstring(L, 3, "", &size);
#ifdef ___PDR_LUASOCKET_DEBUG
    ___pdr_p_timeout tm = ___pdr_timeout_markstart(buf->tm);
#endif
    /* initialize buffer with optional extra prefix 
     * (useful for concatenating previous partial results) */
    ___pdr_luaL_buffinit(L, &b);
    ___pdr_luaL_addlstring(&b, part, size);
    /* receive new patterns */
    if (!___pdr_lua_isnumber(L, 2)) {
        const char *p= ___pdr_luaL_optstring(L, 2, "*l");
        if (p[0] == '*' && p[1] == 'l') err = recvline(buf, &b);
        else if (p[0] == '*' && p[1] == 'a') err = recvall(buf, &b); 
        else ___pdr_luaL_argcheck(L, 0, 2, "invalid receive pattern");
    /* get a fixed number of bytes (minus what was already partially 
     * received) */
    } else {
        double n = ___pdr_lua_tonumber(L, 2); 
        size_t wanted = (size_t) n;
        ___pdr_luaL_argcheck(L, n >= 0, 2, "invalid receive pattern");
        if (size == 0 || wanted > size)
            err = recvraw(buf, wanted-size, &b);
    }
    /* check if there was an error */
    if (err != PDR_IO_DONE) {
        /* we can't push anyting in the stack before pushing the
         * contents of the buffer. this is the reason for the complication */
        ___pdr_luaL_pushresult(&b);
        ___pdr_lua_pushstring(L, buf->io->error(buf->io->ctx, err)); 
        ___pdr_lua_pushvalue(L, -2); 
        ___pdr_lua_pushnil(L);
        ___pdr_lua_replace(L, -4);
    } else {
        ___pdr_luaL_pushresult(&b);
        ___pdr_lua_pushnil(L);
        ___pdr_lua_pushnil(L);
    }
#ifdef ___PDR_LUASOCKET_DEBUG
    /* push time elapsed during operation as the last return value */
    ___pdr_lua_pushnumber(L, ___pdr_timeout_gettime() - ___pdr_timeout_getstart(tm));
#endif
    return ___pdr_lua_gettop(L) - top;
}

/*-------------------------------------------------------------------------*\
* Determines if there is any data in the read buffer
\*-------------------------------------------------------------------------*/
int ___pdr_buffer_isempty(___pdr_p_buffer buf) {
    return buf->first >= buf->last;
}

/*=========================================================================*\
* Internal functions
\*=========================================================================*/
/*-------------------------------------------------------------------------*\
* Sends a block of data (unbuffered)
\*-------------------------------------------------------------------------*/
#define ___PDR_STEPSIZE 8192
static int sendraw(___pdr_p_buffer buf, const char *data, size_t count, size_t *sent) {
    ___pdr_p_io io = buf->io;
    ___pdr_p_timeout tm = buf->tm;
    size_t total = 0;
    int err = PDR_IO_DONE;
    while (total < count && err == PDR_IO_DONE) {
        size_t done = 0;
        size_t step = (count-total <= ___PDR_STEPSIZE)? count-total: ___PDR_STEPSIZE;
        err = io->send(io->ctx, data+total, step, &done, tm);
        total += done;
    }
    *sent = total;
    buf->sent += total;
    return err;
}

/*-------------------------------------------------------------------------*\
* Reads a fixed number of bytes (buffered)
\*-------------------------------------------------------------------------*/
static int recvraw(___pdr_p_buffer buf, size_t wanted, ___pdr_luaL_Buffer *b) {
    int err = PDR_IO_DONE;
    size_t total = 0;
    while (err == PDR_IO_DONE) {
        size_t count; const char *data;
        err = buffer_get(buf, &data, &count);
        count = ___PDR_MIN(count, wanted - total);
        ___pdr_luaL_addlstring(b, data, count);
        buffer_skip(buf, count);
        total += count;
        if (total >= wanted) break;
    }
    return err;
}

/*-------------------------------------------------------------------------*\
* Reads everything until the connection is closed (buffered)
\*-------------------------------------------------------------------------*/
static int recvall(___pdr_p_buffer buf, ___pdr_luaL_Buffer *b) {
    int err = PDR_IO_DONE;
    size_t total = 0;
    while (err == PDR_IO_DONE) {
        const char *data; size_t count;
        err = buffer_get(buf, &data, &count);
        total += count;
        ___pdr_luaL_addlstring(b, data, count);
        buffer_skip(buf, count);
    }
    if (err == PDR_IO_CLOSED) {
        if (total > 0) return PDR_IO_DONE;
        else return PDR_IO_CLOSED;
    } else return err;
}

/*-------------------------------------------------------------------------*\
* Reads a line terminated by a CR LF pair or just by a LF. The CR and LF 
* are not returned by the function and are discarded from the buffer
\*-------------------------------------------------------------------------*/
static int recvline(___pdr_p_buffer buf, ___pdr_luaL_Buffer *b) {
    int err = PDR_IO_DONE;
    while (err == PDR_IO_DONE) {
        size_t count, pos; const char *data;
        err = buffer_get(buf, &data, &count);
        pos = 0;
        while (pos < count && data[pos] != '\n') {
            /* we ignore all \r's */
            if (data[pos] != '\r') ___pdr_luaL_addchar(b, data[pos]);
            pos++;
        }
        if (pos < count) { /* found '\n' */
            buffer_skip(buf, pos+1); /* skip '\n' too */
            break; /* we are done */
        } else /* reached the end of the buffer */
            buffer_skip(buf, pos);
    }
    return err;
}

/*-------------------------------------------------------------------------*\
* Skips a given number of bytes from read buffer. No data is read from the
* transport layer
\*-------------------------------------------------------------------------*/
static void buffer_skip(___pdr_p_buffer buf, size_t count) {
    buf->received += count;
    buf->first += count;
    if (___pdr_buffer_isempty(buf)) 
        buf->first = buf->last = 0;
}

/*-------------------------------------------------------------------------*\
* Return any data available in buffer, or get more data from transport layer
* if buffer is empty
\*-------------------------------------------------------------------------*/
static int buffer_get(___pdr_p_buffer buf, const char **data, size_t *count) {
    int err = PDR_IO_DONE;
    ___pdr_p_io io = buf->io;
    ___pdr_p_timeout tm = buf->tm;
    if (___pdr_buffer_isempty(buf)) {
        size_t got;
        err = io->recv(io->ctx, buf->data, ___PDR_BUF_SIZE, &got, tm);
        buf->first = 0;
        buf->last = got;
    }
    *count = buf->last - buf->first;
    *data = buf->data + buf->first;
    return err;
}

} // end NS_PDR_SLUA
