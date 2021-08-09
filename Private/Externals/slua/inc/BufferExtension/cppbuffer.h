//#ifndef ___CPPBUFFER_H___
//#define ___CPPBUFFER_H___
#pragma once

#define NS_CPPBUFFER cppbuffer
// #define CPPBUFFER_API __declspec(dllexport)
#define CPPBUFFER_API PANDORA_API

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif
#include <stdlib.h>
#include <string.h>
#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif
#include "HAL/UnrealMemory.h"

#define CB_MALLOC FMemory::Malloc
#define CB_MEMCPY FMemory::Memcpy
#define CB_FREE FMemory::Free

namespace NS_CPPBUFFER
{
    enum CB_EENDIAN
    {
        CB_BIG_ENDIAN = 0,
        CB_LITTLE_ENDIAN = 1,
    };

    typedef struct t_cppbuffer_
    {
        size_t id;
        size_t pread;
        size_t pwrite;
        size_t capacity;
        size_t count;
        CB_EENDIAN endian;
        char * buf;
    } t_cppbuffer;
    typedef t_cppbuffer * p_cppbuffer;

    CPPBUFFER_API size_t cb_get_bytes_count(p_cppbuffer buf);
    CPPBUFFER_API size_t cb_get_empty_size(p_cppbuffer buf);

    CPPBUFFER_API int cb_skip(p_cppbuffer buf, size_t count);

    CPPBUFFER_API int cb_peekwrite(p_cppbuffer buf, size_t count, bool peekforward=true);
    CPPBUFFER_API int cb_peekread(p_cppbuffer buf, size_t count, bool peekforward=true);

    CPPBUFFER_API int cb_clear(p_cppbuffer buf);
    CPPBUFFER_API int cb_write_raw(p_cppbuffer buf, const char * bytes, size_t len);
    CPPBUFFER_API int cb_read_raw(p_cppbuffer buf, char * bytes, size_t len, size_t offset = 0);

    CPPBUFFER_API int cb_write_string(p_cppbuffer buf, const char * str, size_t len);
    CPPBUFFER_API int cb_read_string(p_cppbuffer buf, char * str, size_t len);

    CPPBUFFER_API int cb_buffer_cpy(p_cppbuffer src, p_cppbuffer dest, size_t offset, size_t len);
    CPPBUFFER_API int cb_realloc(p_cppbuffer buf, size_t newsize);

    inline CB_EENDIAN get_localendian()
    {
        union {
            short val;
            char arr[2];
        } union_short;

        union_short.arr[0] = 0x00;
        union_short.arr[1] = 0x01;
        if (union_short.val == 1)
            return CB_EENDIAN::CB_BIG_ENDIAN;

        return CB_EENDIAN::CB_LITTLE_ENDIAN;
    }

    inline void convert_order(char * buf, size_t size, CB_EENDIAN endian)
    {
        if (get_localendian() == endian)
            return;

        size_t halfsize = size / 2;
        char t;
        for (size_t i = 0; i < halfsize; ++i)
        {
            t = buf[i];
            buf[i] = buf[size - 1 - i];
            buf[size - 1 - i] = t;
        }
    }

    template<typename T>
    int cb_write_convertable_data(p_cppbuffer buf, T val)
    {
        size_t size = sizeof(T);
        char tmp_buf[16];
        CB_MEMCPY(tmp_buf, &val, size);
        convert_order(tmp_buf, size, buf->endian);
        return cb_write_raw(buf, tmp_buf, size);
    }

    template<typename T>
    int cb_read_convertable_data(p_cppbuffer buf, T & val)
    {
        size_t size = sizeof(T);
        char *p = static_cast<char*>(static_cast<void*>(&val));
        int err = cb_read_raw(buf, p, size);
        if (err != 0)
            return err;
        convert_order(p, size, buf->endian);

        cb_skip(buf, size);
        return err;
    }

}


//#endif
