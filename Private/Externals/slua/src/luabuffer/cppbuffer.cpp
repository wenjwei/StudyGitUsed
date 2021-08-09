#include "cppbuffer.h"


size_t NS_CPPBUFFER::cb_get_bytes_count(p_cppbuffer buf)
{
    return buf->count;
}

size_t NS_CPPBUFFER::cb_get_empty_size(p_cppbuffer buf)
{
    return buf->capacity - buf->count;
}

int NS_CPPBUFFER::cb_skip(p_cppbuffer buf, size_t count)
{
    // skip all
    if (count >= buf->count)
    {
        count = buf->count;
    }

    buf->count -= count;
    size_t fin = buf->pread + count;
    if (fin >= buf->capacity)
        fin = fin % buf->capacity;

    buf->pread = fin;

    return 0;
}

int NS_CPPBUFFER::cb_peekwrite(p_cppbuffer buf, size_t count, bool peekforward)
{
    size_t empty = cb_get_empty_size(buf);
    if (peekforward && count > empty)
        return 1;
    if (!peekforward && count > buf->count)
        return 2;

    if (peekforward)
    {
        buf->pwrite += count;
        if (buf->pwrite >= buf->capacity)
            buf->pwrite = buf->pwrite % buf->capacity;
    }
    else
    {
        if (buf->pwrite >= count)
            buf->pwrite -= count;
        else
            buf->pwrite = buf->capacity - (count - buf->pwrite);
    }
    
    buf->count = peekforward ? buf->count + count: buf->count - count;
    return 0;
}

int NS_CPPBUFFER::cb_peekread(p_cppbuffer buf, size_t count, bool peekforward)
{
    size_t empty = cb_get_empty_size(buf);
    if (peekforward && count > buf->count)
        return 1;
    if (!peekforward && count > empty)
        return 2;
    
    if (peekforward)
    {
        buf->pread += count;
        if (buf->pread >= buf->capacity)
            buf->pread = buf->pread % buf->capacity;
    }
    else
    {
        if (buf->pread >= count)
            buf->pread -= count;
        else
            buf->pread = buf->capacity - (count - buf->pread);
    }
    
    buf->count = peekforward ? buf->count - count: buf->count + count;
    return 0;
}

int NS_CPPBUFFER::cb_clear(p_cppbuffer buf)
{
    buf->pread = buf->pwrite = 0;
    buf->count = 0;
    return 0;
}

// write data into buf
int NS_CPPBUFFER::cb_write_raw(p_cppbuffer buf, const char * bytes, size_t len)
{
    size_t empty = cb_get_empty_size(buf);
    if (len > empty)
    {
        // not enough space, expand buffer
        size_t newsize = buf->capacity;
        do 
        {
            newsize *= 2;
        } while (len + buf->count > newsize);

        cb_realloc(buf, newsize);
    }

    size_t fin = buf->pwrite + len;
    if (fin >= buf->capacity)
    {
        // segment write
        fin = fin % buf->capacity;
        size_t len1 = buf->capacity - buf->pwrite;
        size_t len2 = len - len1;
        CB_MEMCPY(buf->buf + buf->pwrite, bytes, len1);
        if (len2 > 0)
            CB_MEMCPY(buf->buf, bytes + len1, len2);
    }
    else
    {
        CB_MEMCPY(buf->buf + buf->pwrite, bytes, len);
    }

    buf->count += len;
    buf->pwrite = fin;
    return 0;
}

// read buf data into a char array, bytes which been read won't be skip
int NS_CPPBUFFER::cb_read_raw(p_cppbuffer buf, char * bytes, size_t len, size_t offset)
{
    // buffer doesn't have enough valid bytes to read, read failed
    if (len + offset > buf->count)
        return 1;

    size_t pread = buf->pread + offset;
    if (pread >= buf->capacity)
        pread = pread % buf->capacity;

    if (pread + len > buf->capacity)
    {
        size_t len1 = buf->capacity - pread;
        size_t len2 = len - len1;
        CB_MEMCPY(bytes, buf->buf + pread, len1);
        CB_MEMCPY(bytes + len1, buf->buf, len2);
    }
    else
    {
        CB_MEMCPY(bytes, buf->buf + pread, len);
    }

    return 0;
}

int NS_CPPBUFFER::cb_write_string(p_cppbuffer buf, const char * str, size_t len)
{
    return cb_write_raw(buf, str, len);
}

int NS_CPPBUFFER::cb_read_string(p_cppbuffer buf, char * str, size_t len)
{
    int err = cb_read_raw(buf, str, len);
    if (err == 0)
        cb_skip(buf, len);
    return err;
}

int NS_CPPBUFFER::cb_buffer_cpy(p_cppbuffer src, p_cppbuffer dest, size_t offset, size_t len)
{
    int err = 0;
	char * tempbuf = static_cast<char*>(CB_MALLOC(len));
    err = cb_read_raw(src, tempbuf, len, offset);
    if (err != 0)
    {
		CB_FREE(tempbuf);
        return err;
    }
    err = cb_write_raw(dest, tempbuf, len);
	CB_FREE(tempbuf);
    return err;
}

int NS_CPPBUFFER::cb_realloc(p_cppbuffer buf, size_t newsize)
{
    if (newsize < buf->count)
        return -1;
	char * newmemory = static_cast<char*>(CB_MALLOC(newsize));
    cb_read_raw(buf, newmemory, buf->count, 0);
	if (buf->buf != nullptr)
		CB_FREE(buf->buf);
    buf->buf = newmemory;
    buf->capacity = newsize;
    buf->pread = 0;
    buf->pwrite = buf->count;
    buf->pwrite = buf->pwrite % buf->capacity;
    return 0;
}
