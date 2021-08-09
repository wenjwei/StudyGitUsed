/* miniz.c v1.15 - public domain deflate/inflate, zlib-subset, ZIP reading/writing/appending, PNG writing
   See "unlicense" statement at the end of this file.
   Rich Geldreich <richgel99@gmail.com>, last updated Oct. 13, 2013
   Implements RFC 1950: http://www.ietf.org/rfc/rfc1950.txt and RFC 1951: http://www.ietf.org/rfc/rfc1951.txt

   Most API's defined in miniz.c are optional. For example, to disable the archive related functions just define
   __MINIZ_NO_ARCHIVE_APIS, or to get rid of all stdio usage define __MINIZ_NO_STDIO (see the list below for more macros).

   * Change History
     10/13/13 v1.15 r4 - Interim bugfix release while I work on the next major release with Zip64 support (almost there!):
       - Critical fix for the MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY bug (thanks kahmyong.moon@hp.com) which could cause locate files to not find files. This bug
        would only have occured in earlier versions if you explicitly used this flag, OR if you used mz_zip_extract_archive_file_to_heap() or mz_zip_add_mem_to_archive_file_in_place()
        (which used this flag). If you can't switch to v1.15 but want to fix this bug, just remove the uses of this flag from both helper funcs (and of course don't use the flag).
       - Bugfix in mz_zip_reader_extract_to_mem_no_alloc() from kymoon when pUser_read_buf is not NULL and compressed size is > uncompressed size
       - Fixing mz_zip_reader_extract_*() funcs so they don't try to extract compressed data from directory entries, to account for weird zipfiles which contain zero-size compressed data on dir entries.
         Hopefully this fix won't cause any issues on weird zip archives, because it assumes the low 16-bits of zip external attributes are DOS attributes (which I believe they always are in practice).
       - Fixing mz_zip_reader_is_file_a_directory() so it doesn't check the internal attributes, just the filename and external attributes
       - mz_zip_reader_init_file() - missing __MZ_FCLOSE() call if the seek failed
       - Added cmake support for Linux builds which builds all the examples, tested with clang v3.3 and gcc v4.6.
       - Clang fix for tdefl_write_image_to_png_file_in_memory() from toffaletti
       - Merged __MZ_FORCEINLINE fix from hdeanclark
       - Fix <time.h> include before config #ifdef, thanks emil.brink
       - Added tdefl_write_image_to_png_file_in_memory_ex(): supports Y flipping (super useful for OpenGL apps), and explicit control over the compression level (so you can
        set it to 1 for real-time compression).
       - Merged in some compiler fixes from paulharris's github repro.
       - Retested this build under Windows (VS 2010, including static analysis), tcc  0.9.26, gcc v4.6 and clang v3.3.
       - Added example6.c, which dumps an image of the mandelbrot set to a PNG file.
       - Modified example2 to help test the MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY flag more.
       - In r3: Bugfix to mz_zip_writer_add_file() found during merge: Fix possible src file fclose() leak if alignment bytes+local header file write faiiled
       - In r4: Minor bugfix to mz_zip_writer_add_from_zip_reader(): Was pushing the wrong central dir header offset, appears harmless in this release, but it became a problem in the zip64 branch
     5/20/12 v1.14 - MinGW32/64 GCC 4.6.1 compiler fixes: added __MZ_FORCEINLINE, #include <time.h> (thanks fermtect).
     5/19/12 v1.13 - From jason@cornsyrup.org and kelwert@mtu.edu - Fix mz_crc32() so it doesn't compute the wrong CRC-32's when mz_ulong is 64-bit.
       - Temporarily/locally slammed in "typedef unsigned long mz_ulong" and re-ran a randomized regression test on ~500k files.
       - Eliminated a bunch of warnings when compiling with GCC 32-bit/64.
       - Ran all examples, miniz.c, and tinfl.c through MSVC 2008's /analyze (static analysis) option and fixed all warnings (except for the silly
        "Use of the comma-operator in a tested expression.." analysis warning, which I purposely use to work around a MSVC compiler warning).
       - Created 32-bit and 64-bit Codeblocks projects/workspace. Built and tested Linux executables. The codeblocks workspace is compatible with Linux+Win32/x64.
       - Added miniz_tester solution/project, which is a useful little app derived from LZHAM's tester app that I use as part of the regression test.
       - Ran miniz.c and tinfl.c through another series of regression testing on ~500,000 files and archives.
       - Modified example5.c so it purposely disables a bunch of high-level functionality (__MINIZ_NO_STDIO, etc.). (Thanks to corysama for the __MINIZ_NO_STDIO bug report.)
       - Fix ftell() usage in examples so they exit with an error on files which are too large (a limitation of the examples, not miniz itself).
     4/12/12 v1.12 - More comments, added low-level example5.c, fixed a couple minor level_and_flags issues in the archive API's.
      level_and_flags can now be set to __MZ_DEFAULT_COMPRESSION. Thanks to Bruce Dawson <bruced@valvesoftware.com> for the feedback/bug report.
     5/28/11 v1.11 - Added statement from unlicense.org
     5/27/11 v1.10 - Substantial compressor optimizations:
      - Level 1 is now ~4x faster than before. The L1 compressor's throughput now varies between 70-110MB/sec. on a
      - Core i7 (actual throughput varies depending on the type of data, and x64 vs. x86).
      - Improved baseline L2-L9 compression perf. Also, greatly improved compression perf. issues on some file types.
      - Refactored the compression code for better readability and maintainability.
      - Added level 10 compression level (L10 has slightly better ratio than level 9, but could have a potentially large
       drop in throughput on some files).
     5/15/11 v1.09 - Initial stable release.

   * Low-level Deflate/Inflate implementation notes:

     Compression: Use the "tdefl" API's. The compressor supports raw, static, and dynamic blocks, lazy or
     greedy parsing, match length filtering, RLE-only, and Huffman-only streams. It performs and compresses
     approximately as well as zlib.

     Decompression: Use the "tinfl" API's. The entire decompressor is implemented as a single function
     coroutine: see tinfl_decompress(). It supports decompression into a 32KB (or larger power of 2) wrapping buffer, or into a memory
     block large enough to hold the entire file.

     The low-level tdefl/tinfl API's do not make any use of dynamic memory allocation.

   * zlib-style API notes:

     miniz.c implements a fairly large subset of zlib. There's enough functionality present for it to be a drop-in
     zlib replacement in many apps:
        The z_stream struct, optional memory allocation callbacks
        deflateInit/deflateInit2/deflate/deflateReset/deflateEnd/deflateBound
        inflateInit/inflateInit2/inflate/inflateEnd
        compress, compress2, compressBound, uncompress
        CRC-32, Adler-32 - Using modern, minimal code size, CPU cache friendly routines.
        Supports raw deflate streams or standard zlib streams with adler-32 checking.

     Limitations:
      The callback API's are not implemented yet. No support for gzip headers or zlib static dictionaries.
      I've tried to closely emulate zlib's various flavors of stream flushing and return status codes, but
      there are no guarantees that miniz.c pulls this off perfectly.

   * PNG writing: See the tdefl_write_image_to_png_file_in_memory() function, originally written by
     Alex Evans. Supports 1-4 bytes/pixel images.

   * ZIP archive API notes:

     The ZIP archive API's where designed with simplicity and efficiency in mind, with just enough abstraction to
     get the job done with minimal fuss. There are simple API's to retrieve file information, read files from
     existing archives, create new archives, append new files to existing archives, or clone archive data from
     one archive to another. It supports archives located in memory or the heap, on disk (using stdio.h),
     or you can specify custom file read/write callbacks.

     - Archive reading: Just call this function to read a single file from a disk archive:

      void *mz_zip_extract_archive_file_to_heap(const char *pZip_filename, const char *pArchive_name,
        size_t *pSize, __mz_uint zip_flags);

     For more complex cases, use the "mz_zip_reader" functions. Upon opening an archive, the entire central
     directory is located and read as-is into memory, and subsequent file access only occurs when reading individual files.

     - Archives file scanning: The simple way is to use this function to scan a loaded archive for a specific file:

     int mz_zip_reader_locate_file(mz_zip_archive *pZip, const char *pName, const char *pComment, __mz_uint flags);

     The locate operation can optionally check file comments too, which (as one example) can be used to identify
     multiple versions of the same file in an archive. This function uses a simple linear search through the central
     directory, so it's not very fast.

     Alternately, you can iterate through all the files in an archive (using mz_zip_reader_get_num_files()) and
     retrieve detailed info on each file by calling mz_zip_reader_file_stat().

     - Archive creation: Use the "mz_zip_writer" functions. The ZIP writer immediately writes compressed file data
     to disk and builds an exact image of the central directory in memory. The central directory image is written
     all at once at the end of the archive file when the archive is finalized.

     The archive writer can optionally align each file's local header and file data to any power of 2 alignment,
     which can be useful when the archive will be read from optical media. Also, the writer supports placing
     arbitrary data blobs at the very beginning of ZIP archives. Archives written using either feature are still
     readable by any ZIP tool.

     - Archive appending: The simple way to add a single file to an archive is to call this function:

      mz_bool mz_zip_add_mem_to_archive_file_in_place(const char *pZip_filename, const char *pArchive_name,
        const void *pBuf, size_t buf_size, const void *pComment, __mz_uint16 comment_size, __mz_uint level_and_flags);

     The archive will be created if it doesn't already exist, otherwise it'll be appended to.
     Note the appending is done in-place and is not an atomic operation, so if something goes wrong
     during the operation it's possible the archive could be left without a central directory (although the local
     file headers and file data will be fine, so the archive will be recoverable).

     For more complex archive modification scenarios:
     1. The safest way is to use a mz_zip_reader to read the existing archive, cloning only those bits you want to
     preserve into a new archive using using the mz_zip_writer_add_from_zip_reader() function (which compiles the
     compressed file data as-is). When you're done, delete the old archive and rename the newly written archive, and
     you're done. This is safe but requires a bunch of temporary disk space or heap memory.

     2. Or, you can convert an mz_zip_reader in-place to an mz_zip_writer using mz_zip_writer_init_from_reader(),
     append new files as needed, then finalize the archive which will write an updated central directory to the
     original archive. (This is basically what mz_zip_add_mem_to_archive_file_in_place() does.) There's a
     possibility that the archive's central directory could be lost with this method if anything goes wrong, though.

     - ZIP archive support limitations:
     No zip64 or spanning support. Extraction functions can only handle unencrypted, stored or deflated files.
     Requires streams capable of seeking.

   * This is a header file library, like stb_image.c. To get only a header file, either cut and paste the
     below header, or create miniz.h, #define MINIZ_HEADER_FILE_ONLY, and then include miniz.c from it.

   * Important: For best perf. be sure to customize the below macros for your target platform:
     #define __MINIZ_USE_UNALIGNED_LOADS_AND_STORES 1
     #define __MINIZ_LITTLE_ENDIAN 1
     #define __MINIZ_HAS_64BIT_REGISTERS 1

   * On platforms using glibc, Be sure to "#define _LARGEFILE64_SOURCE 1" before including miniz.c to ensure miniz
     uses the 64-bit variants: fopen64(), stat64(), etc. Otherwise you won't be able to process large files
     (i.e. 32-bit stat() fails for me on files > 0x7FFFFFFF bytes).
*/

#include "__miniz.h"
#ifndef MINIZ_HEADER_FILE_ONLY

typedef unsigned char __mz_validate_uint16[sizeof(__mz_uint16)==2 ? 1 : -1];
typedef unsigned char __mz_validate_uint32[sizeof(__mz_uint32)==4 ? 1 : -1];
typedef unsigned char __mz_validate_uint64[sizeof(__mz_uint64)==8 ? 1 : -1];


#include <string.h>
#include <assert.h>


#define __MZ_ASSERT(x) assert(x)

#ifdef __MINIZ_NO_MALLOC
  #define __MZ_MALLOC(x) NULL
  #define __MZ_FREE(x) (void)x, ((void)0)
  #define __MZ_REALLOC(p, x) NULL
#else
  #define __MZ_MALLOC(x) malloc(x)
  #define __MZ_FREE(x) free(x)
  #define __MZ_REALLOC(p, x) realloc(p, x)
#endif

#define __MZ_MAX(a,b) (((a)>(b))?(a):(b))
#define __MZ_MIN(a,b) (((a)<(b))?(a):(b))
#define __MZ_CLEAR_OBJ(obj) memset(&(obj), 0, sizeof(obj))

#if __MINIZ_USE_UNALIGNED_LOADS_AND_STORES && __MINIZ_LITTLE_ENDIAN
  #define __MZ_READ_LE16(p) *((const __mz_uint16 *)(p))
  #define __MZ_READ_LE32(p) *((const __mz_uint32 *)(p))
#else
  #define __MZ_READ_LE16(p) ((__mz_uint32)(((const __mz_uint8 *)(p))[0]) | ((__mz_uint32)(((const __mz_uint8 *)(p))[1]) << 8U))
  #define __MZ_READ_LE32(p) ((__mz_uint32)(((const __mz_uint8 *)(p))[0]) | ((__mz_uint32)(((const __mz_uint8 *)(p))[1]) << 8U) | ((__mz_uint32)(((const __mz_uint8 *)(p))[2]) << 16U) | ((__mz_uint32)(((const __mz_uint8 *)(p))[3]) << 24U))
#endif

#ifdef _MSC_VER
  #define __MZ_FORCEINLINE __forceinline
#elif defined(__GNUC__)
  #define __MZ_FORCEINLINE inline __attribute__((__always_inline__))
#else
  #define __MZ_FORCEINLINE inline
#endif

#ifdef __cplusplus
  extern "C" {
#endif

// ------------------- zlib-style API's

__mz_ulong __mz_adler32(__mz_ulong adler, const unsigned char *ptr, size_t buf_len)
{
  __mz_uint32 i, s1 = (__mz_uint32)(adler & 0xffff), s2 = (__mz_uint32)(adler >> 16); size_t block_len = buf_len % 5552;
  if (!ptr) return __MZ_ADLER32_INIT;
  while (buf_len) {
    for (i = 0; i + 7 < block_len; i += 8, ptr += 8) {
      s1 += ptr[0], s2 += s1; s1 += ptr[1], s2 += s1; s1 += ptr[2], s2 += s1; s1 += ptr[3], s2 += s1;
      s1 += ptr[4], s2 += s1; s1 += ptr[5], s2 += s1; s1 += ptr[6], s2 += s1; s1 += ptr[7], s2 += s1;
    }
    for ( ; i < block_len; ++i) s1 += *ptr++, s2 += s1;
    s1 %= 65521U, s2 %= 65521U; buf_len -= block_len; block_len = 5552;
  }
  return (s2 << 16) + s1;
}

// Karl Malbrain's compact CRC-32. See "A compact CCITT crc16 and crc32 C implementation that balances processor cache usage against speed": http://www.geocities.com/malbrain/
__mz_ulong __mz_crc32(__mz_ulong crc, const __mz_uint8 *ptr, size_t buf_len)
{
  static const __mz_uint32 s_crc32[16] = { 0, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c };
  __mz_uint32 crcu32 = (__mz_uint32)crc;
  if (!ptr) return __MZ_CRC32_INIT;
  crcu32 = ~crcu32; while (buf_len--) { __mz_uint8 b = *ptr++; crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b & 0xF)]; crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b >> 4)]; }
  return ~crcu32;
}

void __mz_free(void *p)
{
  __MZ_FREE(p);
}

#ifndef __MINIZ_NO_ZLIB_APIS

static void *__def_alloc_func(void *opaque, size_t items, size_t size) { (void)opaque, (void)items, (void)size; return __MZ_MALLOC(items * size); }
static void __def_free_func(void *opaque, void *address) { (void)opaque, (void)address; __MZ_FREE(address); }
static void *__def_realloc_func(void *opaque, void *address, size_t items, size_t size) { (void)opaque, (void)address, (void)items, (void)size; return __MZ_REALLOC(address, items * size); }

const char *__mz_version(void)
{
  return __MZ_VERSION;
}

int __mz_deflateInit(__mz_streamp pStream, int level)
{
  return __mz_deflateInit2(pStream, level, __MZ_DEFLATED, __MZ_DEFAULT_WINDOW_BITS, 9, __MZ_DEFAULT_STRATEGY);
}

int __mz_deflateInit2(__mz_streamp pStream, int level, int method, int window_bits, int mem_level, int strategy)
{
  __tdefl_compressor *pComp;
  __mz_uint comp_flags = __TDEFL_COMPUTE_ADLER32 | __tdefl_create_comp_flags_from_zip_params(level, window_bits, strategy);

  if (!pStream) return __MZ_STREAM_ERROR;
  if ((method != __MZ_DEFLATED) || ((mem_level < 1) || (mem_level > 9)) || ((window_bits != __MZ_DEFAULT_WINDOW_BITS) && (-window_bits != __MZ_DEFAULT_WINDOW_BITS))) return __MZ_PARAM_ERROR;

  pStream->data_type = 0;
  pStream->adler = __MZ_ADLER32_INIT;
  pStream->msg = NULL;
  pStream->reserved = 0;
  pStream->total_in = 0;
  pStream->total_out = 0;
  if (!pStream->zalloc) pStream->zalloc = __def_alloc_func;
  if (!pStream->zfree) pStream->zfree = __def_free_func;

  pComp = (__tdefl_compressor *)pStream->zalloc(pStream->opaque, 1, sizeof(__tdefl_compressor));
  if (!pComp)
    return __MZ_MEM_ERROR;

  pStream->state = (struct __mz_internal_state *)pComp;

  if (__tdefl_init(pComp, NULL, NULL, comp_flags) != __TDEFL_STATUS_OKAY)
  {
    __mz_deflateEnd(pStream);
    return __MZ_PARAM_ERROR;
  }

  return __MZ_OK;
}

int __mz_deflateReset(__mz_streamp pStream)
{
  if ((!pStream) || (!pStream->state) || (!pStream->zalloc) || (!pStream->zfree)) return __MZ_STREAM_ERROR;
  pStream->total_in = pStream->total_out = 0;
  __tdefl_init((__tdefl_compressor*)pStream->state, NULL, NULL, ((__tdefl_compressor*)pStream->state)->m_flags);
  return __MZ_OK;
}

int __mz_deflate(__mz_streamp pStream, int flush)
{
  size_t in_bytes, out_bytes;
  __mz_ulong orig_total_in, orig_total_out;
  int mz_status = __MZ_OK;

  if ((!pStream) || (!pStream->state) || (flush < 0) || (flush > __MZ_FINISH) || (!pStream->next_out)) return __MZ_STREAM_ERROR;
  if (!pStream->avail_out) return __MZ_BUF_ERROR;

  if (flush == __MZ_PARTIAL_FLUSH) flush = __MZ_SYNC_FLUSH;

  if (((__tdefl_compressor*)pStream->state)->m_prev_return_status == __TDEFL_STATUS_DONE)
    return (flush == __MZ_FINISH) ? __MZ_STREAM_END : __MZ_BUF_ERROR;

  orig_total_in = pStream->total_in; orig_total_out = pStream->total_out;
  for ( ; ; )
  {
    __tdefl_status defl_status;
    in_bytes = pStream->avail_in; out_bytes = pStream->avail_out;

    defl_status = __tdefl_compress((__tdefl_compressor*)pStream->state, pStream->next_in, &in_bytes, pStream->next_out, &out_bytes, (__tdefl_flush)flush);
    pStream->next_in += (__mz_uint)in_bytes; pStream->avail_in -= (__mz_uint)in_bytes;
    pStream->total_in += (__mz_uint)in_bytes; pStream->adler = __tdefl_get_adler32((__tdefl_compressor*)pStream->state);

    pStream->next_out += (__mz_uint)out_bytes; pStream->avail_out -= (__mz_uint)out_bytes;
    pStream->total_out += (__mz_uint)out_bytes;

    if (defl_status < 0)
    {
      mz_status = __MZ_STREAM_ERROR;
      break;
    }
    else if (defl_status == __TDEFL_STATUS_DONE)
    {
      mz_status = __MZ_STREAM_END;
      break;
    }
    else if (!pStream->avail_out)
      break;
    else if ((!pStream->avail_in) && (flush != __MZ_FINISH))
    {
      if ((flush) || (pStream->total_in != orig_total_in) || (pStream->total_out != orig_total_out))
        break;
      return __MZ_BUF_ERROR; // Can't make forward progress without some input.
    }
  }
  return mz_status;
}

int __mz_deflateEnd(__mz_streamp pStream)
{
  if (!pStream) return __MZ_STREAM_ERROR;
  if (pStream->state)
  {
    pStream->zfree(pStream->opaque, pStream->state);
    pStream->state = NULL;
  }
  return __MZ_OK;
}

__mz_ulong __mz_deflateBound(__mz_streamp pStream, __mz_ulong source_len)
{
  (void)pStream;
  // This is really over conservative. (And lame, but it's actually pretty tricky to compute a true upper bound given the way tdefl's blocking works.)
  return __MZ_MAX(128 + (source_len * 110) / 100, 128 + source_len + ((source_len / (31 * 1024)) + 1) * 5);
}

int __mz_compress2(unsigned char *pDest, __mz_ulong *pDest_len, const unsigned char *pSource, __mz_ulong source_len, int level)
{
  int status;
  __mz_stream stream;
  memset(&stream, 0, sizeof(stream));

  // In case mz_ulong is 64-bits (argh I hate longs).
  if ((source_len | *pDest_len) > 0xFFFFFFFFU) return __MZ_PARAM_ERROR;

  stream.next_in = pSource;
  stream.avail_in = (__mz_uint32)source_len;
  stream.next_out = pDest;
  stream.avail_out = (__mz_uint32)*pDest_len;

  status = __mz_deflateInit(&stream, level);
  if (status != __MZ_OK) return status;

  status = __mz_deflate(&stream, __MZ_FINISH);
  if (status != __MZ_STREAM_END)
  {
    __mz_deflateEnd(&stream);
    return (status == __MZ_OK) ? __MZ_BUF_ERROR : status;
  }

  *pDest_len = stream.total_out;
  return __mz_deflateEnd(&stream);
}

int __mz_compress(unsigned char *pDest, __mz_ulong *pDest_len, const unsigned char *pSource, __mz_ulong source_len)
{
  return __mz_compress2(pDest, pDest_len, pSource, source_len, __MZ_DEFAULT_COMPRESSION);
}

__mz_ulong __mz_compressBound(__mz_ulong source_len)
{
  return __mz_deflateBound(NULL, source_len);
}

typedef struct
{
  __tinfl_decompressor m_decomp;
  __mz_uint m_dict_ofs, m_dict_avail, m_first_call, m_has_flushed; int m_window_bits;
  __mz_uint8 m_dict[__TINFL_LZ_DICT_SIZE];
  __tinfl_status m_last_status;
} __inflate_state;

int __mz_inflateInit2(__mz_streamp pStream, int window_bits)
{
  __inflate_state *pDecomp;
  if (!pStream) return __MZ_STREAM_ERROR;
  if ((window_bits != __MZ_DEFAULT_WINDOW_BITS) && (-window_bits != __MZ_DEFAULT_WINDOW_BITS)) return __MZ_PARAM_ERROR;

  pStream->data_type = 0;
  pStream->adler = 0;
  pStream->msg = NULL;
  pStream->total_in = 0;
  pStream->total_out = 0;
  pStream->reserved = 0;
  if (!pStream->zalloc) pStream->zalloc = __def_alloc_func;
  if (!pStream->zfree) pStream->zfree = __def_free_func;

  pDecomp = (__inflate_state*)pStream->zalloc(pStream->opaque, 1, sizeof(__inflate_state));
  if (!pDecomp) return __MZ_MEM_ERROR;

  pStream->state = (struct __mz_internal_state *)pDecomp;

  __tinfl_init(&pDecomp->m_decomp);
  pDecomp->m_dict_ofs = 0;
  pDecomp->m_dict_avail = 0;
  pDecomp->m_last_status = __TINFL_STATUS_NEEDS_MORE_INPUT;
  pDecomp->m_first_call = 1;
  pDecomp->m_has_flushed = 0;
  pDecomp->m_window_bits = window_bits;

  return __MZ_OK;
}

int __mz_inflateInit(__mz_streamp pStream)
{
   return __mz_inflateInit2(pStream, __MZ_DEFAULT_WINDOW_BITS);
}

int __mz_inflate(__mz_streamp pStream, int flush)
{
  __inflate_state* pState;
  __mz_uint n, first_call, decomp_flags = __TINFL_FLAG_COMPUTE_ADLER32;
  size_t in_bytes, out_bytes, orig_avail_in;
  __tinfl_status status;

  if ((!pStream) || (!pStream->state)) return __MZ_STREAM_ERROR;
  if (flush == __MZ_PARTIAL_FLUSH) flush = __MZ_SYNC_FLUSH;
  if ((flush) && (flush != __MZ_SYNC_FLUSH) && (flush != __MZ_FINISH)) return __MZ_STREAM_ERROR;

  pState = (__inflate_state*)pStream->state;
  if (pState->m_window_bits > 0) decomp_flags |= __TINFL_FLAG_PARSE_ZLIB_HEADER;
  orig_avail_in = pStream->avail_in;

  first_call = pState->m_first_call; pState->m_first_call = 0;
  if (pState->m_last_status < 0) return __MZ_DATA_ERROR;

  if (pState->m_has_flushed && (flush != __MZ_FINISH)) return __MZ_STREAM_ERROR;
  pState->m_has_flushed |= (flush == __MZ_FINISH);

  if ((flush == __MZ_FINISH) && (first_call))
  {
    // __MZ_FINISH on the first call implies that the input and output buffers are large enough to hold the entire compressed/decompressed file.
    decomp_flags |= __TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF;
    in_bytes = pStream->avail_in; out_bytes = pStream->avail_out;
    status = __tinfl_decompress(&pState->m_decomp, pStream->next_in, &in_bytes, pStream->next_out, pStream->next_out, &out_bytes, decomp_flags);
    pState->m_last_status = status;
    pStream->next_in += (__mz_uint)in_bytes; pStream->avail_in -= (__mz_uint)in_bytes; pStream->total_in += (__mz_uint)in_bytes;
    pStream->adler = __tinfl_get_adler32(&pState->m_decomp);
    pStream->next_out += (__mz_uint)out_bytes; pStream->avail_out -= (__mz_uint)out_bytes; pStream->total_out += (__mz_uint)out_bytes;

    if (status < 0)
      return __MZ_DATA_ERROR;
    else if (status != __TINFL_STATUS_DONE)
    {
      pState->m_last_status = __TINFL_STATUS_FAILED;
      return __MZ_BUF_ERROR;
    }
    return __MZ_STREAM_END;
  }
  // flush != __MZ_FINISH then we must assume there's more input.
  if (flush != __MZ_FINISH) decomp_flags |= __TINFL_FLAG_HAS_MORE_INPUT;

  if (pState->m_dict_avail)
  {
    n = __MZ_MIN(pState->m_dict_avail, pStream->avail_out);
    memcpy(pStream->next_out, pState->m_dict + pState->m_dict_ofs, n);
    pStream->next_out += n; pStream->avail_out -= n; pStream->total_out += n;
    pState->m_dict_avail -= n; pState->m_dict_ofs = (pState->m_dict_ofs + n) & (__TINFL_LZ_DICT_SIZE - 1);
    return ((pState->m_last_status == __TINFL_STATUS_DONE) && (!pState->m_dict_avail)) ? __MZ_STREAM_END : __MZ_OK;
  }

  for ( ; ; )
  {
    in_bytes = pStream->avail_in;
    out_bytes = __TINFL_LZ_DICT_SIZE - pState->m_dict_ofs;

    status = __tinfl_decompress(&pState->m_decomp, pStream->next_in, &in_bytes, pState->m_dict, pState->m_dict + pState->m_dict_ofs, &out_bytes, decomp_flags);
    pState->m_last_status = status;

    pStream->next_in += (__mz_uint)in_bytes; pStream->avail_in -= (__mz_uint)in_bytes;
    pStream->total_in += (__mz_uint)in_bytes; pStream->adler = __tinfl_get_adler32(&pState->m_decomp);

    pState->m_dict_avail = (__mz_uint)out_bytes;

    n = __MZ_MIN(pState->m_dict_avail, pStream->avail_out);
    memcpy(pStream->next_out, pState->m_dict + pState->m_dict_ofs, n);
    pStream->next_out += n; pStream->avail_out -= n; pStream->total_out += n;
    pState->m_dict_avail -= n; pState->m_dict_ofs = (pState->m_dict_ofs + n) & (__TINFL_LZ_DICT_SIZE - 1);

    if (status < 0)
       return __MZ_DATA_ERROR; // Stream is corrupted (there could be some uncompressed data left in the output dictionary - oh well).
    else if ((status == __TINFL_STATUS_NEEDS_MORE_INPUT) && (!orig_avail_in))
      return __MZ_BUF_ERROR; // Signal caller that we can't make forward progress without supplying more input or by setting flush to __MZ_FINISH.
    else if (flush == __MZ_FINISH)
    {
       // The output buffer MUST be large to hold the remaining uncompressed data when flush==__MZ_FINISH.
       if (status == __TINFL_STATUS_DONE)
          return pState->m_dict_avail ? __MZ_BUF_ERROR : __MZ_STREAM_END;
       // status here must be TINFL_STATUS_HAS_MORE_OUTPUT, which means there's at least 1 more byte on the way. If there's no more room left in the output buffer then something is wrong.
       else if (!pStream->avail_out)
          return __MZ_BUF_ERROR;
    }
    else if ((status == __TINFL_STATUS_DONE) || (!pStream->avail_in) || (!pStream->avail_out) || (pState->m_dict_avail))
      break;
  }

  return ((status == __TINFL_STATUS_DONE) && (!pState->m_dict_avail)) ? __MZ_STREAM_END : __MZ_OK;
}

int __mz_inflateEnd(__mz_streamp pStream)
{
  if (!pStream)
    return __MZ_STREAM_ERROR;
  if (pStream->state)
  {
    pStream->zfree(pStream->opaque, pStream->state);
    pStream->state = NULL;
  }
  return __MZ_OK;
}

int __mz_uncompress(unsigned char *pDest, __mz_ulong *pDest_len, const unsigned char *pSource, __mz_ulong source_len)
{
  __mz_stream stream;
  int status;
  memset(&stream, 0, sizeof(stream));

  // In case mz_ulong is 64-bits (argh I hate longs).
  if ((source_len | *pDest_len) > 0xFFFFFFFFU) return __MZ_PARAM_ERROR;

  stream.next_in = pSource;
  stream.avail_in = (__mz_uint32)source_len;
  stream.next_out = pDest;
  stream.avail_out = (__mz_uint32)*pDest_len;

  status = __mz_inflateInit(&stream);
  if (status != __MZ_OK)
    return status;

  status = __mz_inflate(&stream, __MZ_FINISH);
  if (status != __MZ_STREAM_END)
  {
    __mz_inflateEnd(&stream);
    return ((status == __MZ_BUF_ERROR) && (!stream.avail_in)) ? __MZ_DATA_ERROR : status;
  }
  *pDest_len = stream.total_out;

  return __mz_inflateEnd(&stream);
}

const char *__mz_error(int err)
{
  static struct { int m_err; const char *m_pDesc; } s_error_descs[] =
  {
    { __MZ_OK, "" }, { __MZ_STREAM_END, "stream end" }, { __MZ_NEED_DICT, "need dictionary" }, { __MZ_ERRNO, "file error" }, { __MZ_STREAM_ERROR, "stream error" },
    { __MZ_DATA_ERROR, "data error" }, { __MZ_MEM_ERROR, "out of memory" }, { __MZ_BUF_ERROR, "buf error" }, { __MZ_VERSION_ERROR, "version error" }, { __MZ_PARAM_ERROR, "parameter error" }
  };
  __mz_uint i; for (i = 0; i < sizeof(s_error_descs) / sizeof(s_error_descs[0]); ++i) if (s_error_descs[i].m_err == err) return s_error_descs[i].m_pDesc;
  return NULL;
}

#endif //__MINIZ_NO_ZLIB_APIS

// ------------------- Low-level Decompression (completely independent from all compression API's)

#define __TINFL_MEMCPY(d, s, l) memcpy(d, s, l)
#define __TINFL_MEMSET(p, c, l) memset(p, c, l)

#define __TINFL_CR_BEGIN switch(r->m_state) { case 0:
#define __TINFL_CR_RETURN(state_index, result) do { status = result; r->m_state = state_index; goto common_exit; case state_index:; } __MZ_MACRO_END
#define __TINFL_CR_RETURN_FOREVER(state_index, result) do { for ( ; ; ) { __TINFL_CR_RETURN(state_index, result); } } __MZ_MACRO_END
#define __TINFL_CR_FINISH }

// TODO: If the caller has indicated that there's no more input, and we attempt to read beyond the input buf, then something is wrong with the input because the inflator never
// reads ahead more than it needs to. Currently __TINFL_GET_BYTE() pads the end of the stream with 0's in this scenario.
#define __TINFL_GET_BYTE(state_index, c) do { \
  if (pIn_buf_cur >= pIn_buf_end) { \
    for ( ; ; ) { \
      if (decomp_flags & __TINFL_FLAG_HAS_MORE_INPUT) { \
        __TINFL_CR_RETURN(state_index, __TINFL_STATUS_NEEDS_MORE_INPUT); \
        if (pIn_buf_cur < pIn_buf_end) { \
          c = *pIn_buf_cur++; \
          break; \
        } \
      } else { \
        c = 0; \
        break; \
      } \
    } \
  } else c = *pIn_buf_cur++; } __MZ_MACRO_END

#define __TINFL_NEED_BITS(state_index, n) do { __mz_uint c; __TINFL_GET_BYTE(state_index, c); bit_buf |= (((__tinfl_bit_buf_t)c) << num_bits); num_bits += 8; } while (num_bits < (__mz_uint)(n))
#define __TINFL_SKIP_BITS(state_index, n) do { if (num_bits < (__mz_uint)(n)) { __TINFL_NEED_BITS(state_index, n); } bit_buf >>= (n); num_bits -= (n); } __MZ_MACRO_END
#define __TINFL_GET_BITS(state_index, b, n) do { if (num_bits < (__mz_uint)(n)) { __TINFL_NEED_BITS(state_index, n); } b = bit_buf & ((1 << (n)) - 1); bit_buf >>= (n); num_bits -= (n); } __MZ_MACRO_END

// __TINFL_HUFF_BITBUF_FILL() is only used rarely, when the number of bytes remaining in the input buffer falls below 2.
// It reads just enough bytes from the input stream that are needed to decode the next Huffman code (and absolutely no more). It works by trying to fully decode a
// Huffman code by using whatever bits are currently present in the bit buffer. If this fails, it reads another byte, and tries again until it succeeds or until the
// bit buffer contains >=15 bits (deflate's max. Huffman code size).
#define __TINFL_HUFF_BITBUF_FILL(state_index, pHuff) \
  do { \
    temp = (pHuff)->m_look_up[bit_buf & (__TINFL_FAST_LOOKUP_SIZE - 1)]; \
    if (temp >= 0) { \
      code_len = temp >> 9; \
      if ((code_len) && (num_bits >= code_len)) \
      break; \
    } else if (num_bits > __TINFL_FAST_LOOKUP_BITS) { \
       code_len = __TINFL_FAST_LOOKUP_BITS; \
       do { \
          temp = (pHuff)->m_tree[~temp + ((bit_buf >> code_len++) & 1)]; \
       } while ((temp < 0) && (num_bits >= (code_len + 1))); if (temp >= 0) break; \
    } __TINFL_GET_BYTE(state_index, c); bit_buf |= (((__tinfl_bit_buf_t)c) << num_bits); num_bits += 8; \
  } while (num_bits < 15);

// __TINFL_HUFF_DECODE() decodes the next Huffman coded symbol. It's more complex than you would initially expect because the zlib API expects the decompressor to never read
// beyond the final byte of the deflate stream. (In other words, when this macro wants to read another byte from the input, it REALLY needs another byte in order to fully
// decode the next Huffman code.) Handling this properly is particularly important on raw deflate (non-zlib) streams, which aren't followed by a byte aligned adler-32.
// The slow path is only executed at the very end of the input buffer.
#define __TINFL_HUFF_DECODE(state_index, sym, pHuff) do { \
  int temp; __mz_uint code_len, c; \
  if (num_bits < 15) { \
    if ((pIn_buf_end - pIn_buf_cur) < 2) { \
       __TINFL_HUFF_BITBUF_FILL(state_index, pHuff); \
    } else { \
       bit_buf |= (((__tinfl_bit_buf_t)pIn_buf_cur[0]) << num_bits) | (((__tinfl_bit_buf_t)pIn_buf_cur[1]) << (num_bits + 8)); pIn_buf_cur += 2; num_bits += 16; \
    } \
  } \
  if ((temp = (pHuff)->m_look_up[bit_buf & (__TINFL_FAST_LOOKUP_SIZE - 1)]) >= 0) \
    code_len = temp >> 9, temp &= 511; \
  else { \
    code_len = __TINFL_FAST_LOOKUP_BITS; do { temp = (pHuff)->m_tree[~temp + ((bit_buf >> code_len++) & 1)]; } while (temp < 0); \
  } sym = temp; bit_buf >>= code_len; num_bits -= code_len; } __MZ_MACRO_END

__tinfl_status __tinfl_decompress(__tinfl_decompressor *r, const __mz_uint8 *pIn_buf_next, size_t *pIn_buf_size, __mz_uint8 *pOut_buf_start, __mz_uint8 *pOut_buf_next, size_t *pOut_buf_size, const __mz_uint32 decomp_flags)
{
  static const int s_length_base[31] = { 3,4,5,6,7,8,9,10,11,13, 15,17,19,23,27,31,35,43,51,59, 67,83,99,115,131,163,195,227,258,0,0 };
  static const int s_length_extra[31]= { 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0 };
  static const int s_dist_base[32] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193, 257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0};
  static const int s_dist_extra[32] = { 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
  static const __mz_uint8 s_length_dezigzag[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
  static const int s_min_table_sizes[3] = { 257, 1, 4 };

  __tinfl_status status = __TINFL_STATUS_FAILED; __mz_uint32 num_bits, dist, counter, num_extra; __tinfl_bit_buf_t bit_buf;
  const __mz_uint8 *pIn_buf_cur = pIn_buf_next, *const pIn_buf_end = pIn_buf_next + *pIn_buf_size;
  __mz_uint8 *pOut_buf_cur = pOut_buf_next, *const pOut_buf_end = pOut_buf_next + *pOut_buf_size;
  size_t out_buf_size_mask = (decomp_flags & __TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF) ? (size_t)-1 : ((pOut_buf_next - pOut_buf_start) + *pOut_buf_size) - 1, dist_from_out_buf_start;

  // Ensure the output buffer's size is a power of 2, unless the output buffer is large enough to hold the entire output file (in which case it doesn't matter).
  if (((out_buf_size_mask + 1) & out_buf_size_mask) || (pOut_buf_next < pOut_buf_start)) { *pIn_buf_size = *pOut_buf_size = 0; return __TINFL_STATUS_BAD_PARAM; }

  num_bits = r->m_num_bits; bit_buf = r->m_bit_buf; dist = r->m_dist; counter = r->m_counter; num_extra = r->m_num_extra; dist_from_out_buf_start = r->m_dist_from_out_buf_start;
  __TINFL_CR_BEGIN

  bit_buf = num_bits = dist = counter = num_extra = r->m_zhdr0 = r->m_zhdr1 = 0; r->m_z_adler32 = r->m_check_adler32 = 1;
  if (decomp_flags & __TINFL_FLAG_PARSE_ZLIB_HEADER)
  {
    __TINFL_GET_BYTE(1, r->m_zhdr0); __TINFL_GET_BYTE(2, r->m_zhdr1);
    counter = (((r->m_zhdr0 * 256 + r->m_zhdr1) % 31 != 0) || (r->m_zhdr1 & 32) || ((r->m_zhdr0 & 15) != 8));
    if (!(decomp_flags & __TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF)) counter |= (((1ULL << (8ULL + (r->m_zhdr0 >> 4))) > 32768U) || ((out_buf_size_mask + 1) < (size_t)(1ULL << (8ULL + (r->m_zhdr0 >> 4)))));
    if (counter) { __TINFL_CR_RETURN_FOREVER(36, __TINFL_STATUS_FAILED); }
  }

  do
  {
    __TINFL_GET_BITS(3, r->m_final, 3); r->m_type = r->m_final >> 1;
    if (r->m_type == 0)
    {
      __TINFL_SKIP_BITS(5, num_bits & 7);
      for (counter = 0; counter < 4; ++counter) { if (num_bits) __TINFL_GET_BITS(6, r->m_raw_header[counter], 8); else __TINFL_GET_BYTE(7, r->m_raw_header[counter]); }
      if ((counter = (r->m_raw_header[0] | (r->m_raw_header[1] << 8))) != (__mz_uint)(0xFFFF ^ (r->m_raw_header[2] | (r->m_raw_header[3] << 8)))) { __TINFL_CR_RETURN_FOREVER(39, __TINFL_STATUS_FAILED); }
      while ((counter) && (num_bits))
      {
        __TINFL_GET_BITS(51, dist, 8);
        while (pOut_buf_cur >= pOut_buf_end) { __TINFL_CR_RETURN(52, __TINFL_STATUS_HAS_MORE_OUTPUT); }
        *pOut_buf_cur++ = (__mz_uint8)dist;
        counter--;
      }
      while (counter)
      {
        size_t n; while (pOut_buf_cur >= pOut_buf_end) { __TINFL_CR_RETURN(9, __TINFL_STATUS_HAS_MORE_OUTPUT); }
        while (pIn_buf_cur >= pIn_buf_end)
        {
          if (decomp_flags & __TINFL_FLAG_HAS_MORE_INPUT)
          {
            __TINFL_CR_RETURN(38, __TINFL_STATUS_NEEDS_MORE_INPUT);
          }
          else
          {
            __TINFL_CR_RETURN_FOREVER(40, __TINFL_STATUS_FAILED);
          }
        }
        n = __MZ_MIN(__MZ_MIN((size_t)(pOut_buf_end - pOut_buf_cur), (size_t)(pIn_buf_end - pIn_buf_cur)), counter);
        __TINFL_MEMCPY(pOut_buf_cur, pIn_buf_cur, n); pIn_buf_cur += n; pOut_buf_cur += n; counter -= (__mz_uint)n;
      }
    }
    else if (r->m_type == 3)
    {
      __TINFL_CR_RETURN_FOREVER(10, __TINFL_STATUS_FAILED);
    }
    else
    {
      if (r->m_type == 1)
      {
        __mz_uint8 *p = r->m_tables[0].m_code_size; __mz_uint i;
        r->m_table_sizes[0] = 288; r->m_table_sizes[1] = 32; __TINFL_MEMSET(r->m_tables[1].m_code_size, 5, 32);
        for ( i = 0; i <= 143; ++i) *p++ = 8; for ( ; i <= 255; ++i) *p++ = 9; for ( ; i <= 279; ++i) *p++ = 7; for ( ; i <= 287; ++i) *p++ = 8;
      }
      else
      {
        for (counter = 0; counter < 3; counter++) { __TINFL_GET_BITS(11, r->m_table_sizes[counter], "\05\05\04"[counter]); r->m_table_sizes[counter] += s_min_table_sizes[counter]; }
        __MZ_CLEAR_OBJ(r->m_tables[2].m_code_size); for (counter = 0; counter < r->m_table_sizes[2]; counter++) { __mz_uint s; __TINFL_GET_BITS(14, s, 3); r->m_tables[2].m_code_size[s_length_dezigzag[counter]] = (__mz_uint8)s; }
        r->m_table_sizes[2] = 19;
      }
      for ( ; (int)r->m_type >= 0; r->m_type--)
      {
        int tree_next, tree_cur; __tinfl_huff_table *pTable;
        __mz_uint i, j, used_syms, total, sym_index, next_code[17], total_syms[16]; pTable = &r->m_tables[r->m_type]; __MZ_CLEAR_OBJ(total_syms); __MZ_CLEAR_OBJ(pTable->m_look_up); __MZ_CLEAR_OBJ(pTable->m_tree);
        for (i = 0; i < r->m_table_sizes[r->m_type]; ++i) total_syms[pTable->m_code_size[i]]++;
        used_syms = 0, total = 0; next_code[0] = next_code[1] = 0;
        for (i = 1; i <= 15; ++i) { used_syms += total_syms[i]; next_code[i + 1] = (total = ((total + total_syms[i]) << 1)); }
        if ((65536 != total) && (used_syms > 1))
        {
          __TINFL_CR_RETURN_FOREVER(35, __TINFL_STATUS_FAILED);
        }
        for (tree_next = -1, sym_index = 0; sym_index < r->m_table_sizes[r->m_type]; ++sym_index)
        {
          __mz_uint rev_code = 0, l, cur_code, code_size = pTable->m_code_size[sym_index]; if (!code_size) continue;
          cur_code = next_code[code_size]++; for (l = code_size; l > 0; l--, cur_code >>= 1) rev_code = (rev_code << 1) | (cur_code & 1);
          if (code_size <= __TINFL_FAST_LOOKUP_BITS) { __mz_int16 k = (__mz_int16)((code_size << 9) | sym_index); while (rev_code < __TINFL_FAST_LOOKUP_SIZE) { pTable->m_look_up[rev_code] = k; rev_code += (1 << code_size); } continue; }
          if (0 == (tree_cur = pTable->m_look_up[rev_code & (__TINFL_FAST_LOOKUP_SIZE - 1)])) { pTable->m_look_up[rev_code & (__TINFL_FAST_LOOKUP_SIZE - 1)] = (__mz_int16)tree_next; tree_cur = tree_next; tree_next -= 2; }
          rev_code >>= (__TINFL_FAST_LOOKUP_BITS - 1);
          for (j = code_size; j > (__TINFL_FAST_LOOKUP_BITS + 1); j--)
          {
            tree_cur -= ((rev_code >>= 1) & 1);
            if (!pTable->m_tree[-tree_cur - 1]) { pTable->m_tree[-tree_cur - 1] = (__mz_int16)tree_next; tree_cur = tree_next; tree_next -= 2; } else tree_cur = pTable->m_tree[-tree_cur - 1];
          }
          tree_cur -= ((rev_code >>= 1) & 1); pTable->m_tree[-tree_cur - 1] = (__mz_int16)sym_index;
        }
        if (r->m_type == 2)
        {
          for (counter = 0; counter < (r->m_table_sizes[0] + r->m_table_sizes[1]); )
          {
            __mz_uint s; __TINFL_HUFF_DECODE(16, dist, &r->m_tables[2]); if (dist < 16) { r->m_len_codes[counter++] = (__mz_uint8)dist; continue; }
            if ((dist == 16) && (!counter))
            {
              __TINFL_CR_RETURN_FOREVER(17, __TINFL_STATUS_FAILED);
            }
            num_extra = "\02\03\07"[dist - 16]; __TINFL_GET_BITS(18, s, num_extra); s += "\03\03\013"[dist - 16];
            __TINFL_MEMSET(r->m_len_codes + counter, (dist == 16) ? r->m_len_codes[counter - 1] : 0, s); counter += s;
          }
          if ((r->m_table_sizes[0] + r->m_table_sizes[1]) != counter)
          {
            __TINFL_CR_RETURN_FOREVER(21, __TINFL_STATUS_FAILED);
          }
          __TINFL_MEMCPY(r->m_tables[0].m_code_size, r->m_len_codes, r->m_table_sizes[0]); __TINFL_MEMCPY(r->m_tables[1].m_code_size, r->m_len_codes + r->m_table_sizes[0], r->m_table_sizes[1]);
        }
      }
      for ( ; ; )
      {
        __mz_uint8 *pSrc;
        for ( ; ; )
        {
          if (((pIn_buf_end - pIn_buf_cur) < 4) || ((pOut_buf_end - pOut_buf_cur) < 2))
          {
            __TINFL_HUFF_DECODE(23, counter, &r->m_tables[0]);
            if (counter >= 256)
              break;
            while (pOut_buf_cur >= pOut_buf_end) { __TINFL_CR_RETURN(24, __TINFL_STATUS_HAS_MORE_OUTPUT); }
            *pOut_buf_cur++ = (__mz_uint8)counter;
          }
          else
          {
            int sym2; __mz_uint code_len;
#if __TINFL_USE_64BIT_BITBUF
            if (num_bits < 30) { bit_buf |= (((__tinfl_bit_buf_t)__MZ_READ_LE32(pIn_buf_cur)) << num_bits); pIn_buf_cur += 4; num_bits += 32; }
#else
            if (num_bits < 15) { bit_buf |= (((__tinfl_bit_buf_t)__MZ_READ_LE16(pIn_buf_cur)) << num_bits); pIn_buf_cur += 2; num_bits += 16; }
#endif
            if ((sym2 = r->m_tables[0].m_look_up[bit_buf & (__TINFL_FAST_LOOKUP_SIZE - 1)]) >= 0)
              code_len = sym2 >> 9;
            else
            {
              code_len = __TINFL_FAST_LOOKUP_BITS; do { sym2 = r->m_tables[0].m_tree[~sym2 + ((bit_buf >> code_len++) & 1)]; } while (sym2 < 0);
            }
            counter = sym2; bit_buf >>= code_len; num_bits -= code_len;
            if (counter & 256)
              break;

#if !__TINFL_USE_64BIT_BITBUF
            if (num_bits < 15) { bit_buf |= (((__tinfl_bit_buf_t)__MZ_READ_LE16(pIn_buf_cur)) << num_bits); pIn_buf_cur += 2; num_bits += 16; }
#endif
            if ((sym2 = r->m_tables[0].m_look_up[bit_buf & (__TINFL_FAST_LOOKUP_SIZE - 1)]) >= 0)
              code_len = sym2 >> 9;
            else
            {
              code_len = __TINFL_FAST_LOOKUP_BITS; do { sym2 = r->m_tables[0].m_tree[~sym2 + ((bit_buf >> code_len++) & 1)]; } while (sym2 < 0);
            }
            bit_buf >>= code_len; num_bits -= code_len;

            pOut_buf_cur[0] = (__mz_uint8)counter;
            if (sym2 & 256)
            {
              pOut_buf_cur++;
              counter = sym2;
              break;
            }
            pOut_buf_cur[1] = (__mz_uint8)sym2;
            pOut_buf_cur += 2;
          }
        }
        if ((counter &= 511) == 256) break;

        num_extra = s_length_extra[counter - 257]; counter = s_length_base[counter - 257];
        if (num_extra) { __mz_uint extra_bits; __TINFL_GET_BITS(25, extra_bits, num_extra); counter += extra_bits; }

        __TINFL_HUFF_DECODE(26, dist, &r->m_tables[1]);
        num_extra = s_dist_extra[dist]; dist = s_dist_base[dist];
        if (num_extra) { __mz_uint extra_bits; __TINFL_GET_BITS(27, extra_bits, num_extra); dist += extra_bits; }

        dist_from_out_buf_start = pOut_buf_cur - pOut_buf_start;
        if ((dist > dist_from_out_buf_start) && (decomp_flags & __TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF))
        {
          __TINFL_CR_RETURN_FOREVER(37, __TINFL_STATUS_FAILED);
        }

        pSrc = pOut_buf_start + ((dist_from_out_buf_start - dist) & out_buf_size_mask);

        if ((__MZ_MAX(pOut_buf_cur, pSrc) + counter) > pOut_buf_end)
        {
          while (counter--)
          {
            while (pOut_buf_cur >= pOut_buf_end) { __TINFL_CR_RETURN(53, __TINFL_STATUS_HAS_MORE_OUTPUT); }
            *pOut_buf_cur++ = pOut_buf_start[(dist_from_out_buf_start++ - dist) & out_buf_size_mask];
          }
          continue;
        }
#if __MINIZ_USE_UNALIGNED_LOADS_AND_STORES
        else if ((counter >= 9) && (counter <= dist))
        {
          const __mz_uint8 *pSrc_end = pSrc + (counter & ~7);
          do
          {
            ((__mz_uint32 *)pOut_buf_cur)[0] = ((const __mz_uint32 *)pSrc)[0];
            ((__mz_uint32 *)pOut_buf_cur)[1] = ((const __mz_uint32 *)pSrc)[1];
            pOut_buf_cur += 8;
          } while ((pSrc += 8) < pSrc_end);
          if ((counter &= 7) < 3)
          {
            if (counter)
            {
              pOut_buf_cur[0] = pSrc[0];
              if (counter > 1)
                pOut_buf_cur[1] = pSrc[1];
              pOut_buf_cur += counter;
            }
            continue;
          }
        }
#endif
        do
        {
          pOut_buf_cur[0] = pSrc[0];
          pOut_buf_cur[1] = pSrc[1];
          pOut_buf_cur[2] = pSrc[2];
          pOut_buf_cur += 3; pSrc += 3;
        } while ((int)(counter -= 3) > 2);
        if ((int)counter > 0)
        {
          pOut_buf_cur[0] = pSrc[0];
          if ((int)counter > 1)
            pOut_buf_cur[1] = pSrc[1];
          pOut_buf_cur += counter;
        }
      }
    }
  } while (!(r->m_final & 1));
  if (decomp_flags & __TINFL_FLAG_PARSE_ZLIB_HEADER)
  {
    __TINFL_SKIP_BITS(32, num_bits & 7); for (counter = 0; counter < 4; ++counter) { __mz_uint s; if (num_bits) __TINFL_GET_BITS(41, s, 8); else __TINFL_GET_BYTE(42, s); r->m_z_adler32 = (r->m_z_adler32 << 8) | s; }
  }
  __TINFL_CR_RETURN_FOREVER(34, __TINFL_STATUS_DONE);
  __TINFL_CR_FINISH

common_exit:
  r->m_num_bits = num_bits; r->m_bit_buf = bit_buf; r->m_dist = dist; r->m_counter = counter; r->m_num_extra = num_extra; r->m_dist_from_out_buf_start = dist_from_out_buf_start;
  *pIn_buf_size = pIn_buf_cur - pIn_buf_next; *pOut_buf_size = pOut_buf_cur - pOut_buf_next;
  if ((decomp_flags & (__TINFL_FLAG_PARSE_ZLIB_HEADER | __TINFL_FLAG_COMPUTE_ADLER32)) && (status >= 0))
  {
    const __mz_uint8 *ptr = pOut_buf_next; size_t buf_len = *pOut_buf_size;
    __mz_uint32 i, s1 = r->m_check_adler32 & 0xffff, s2 = r->m_check_adler32 >> 16; size_t block_len = buf_len % 5552;
    while (buf_len)
    {
      for (i = 0; i + 7 < block_len; i += 8, ptr += 8)
      {
        s1 += ptr[0], s2 += s1; s1 += ptr[1], s2 += s1; s1 += ptr[2], s2 += s1; s1 += ptr[3], s2 += s1;
        s1 += ptr[4], s2 += s1; s1 += ptr[5], s2 += s1; s1 += ptr[6], s2 += s1; s1 += ptr[7], s2 += s1;
      }
      for ( ; i < block_len; ++i) s1 += *ptr++, s2 += s1;
      s1 %= 65521U, s2 %= 65521U; buf_len -= block_len; block_len = 5552;
    }
    r->m_check_adler32 = (s2 << 16) + s1; if ((status == __TINFL_STATUS_DONE) && (decomp_flags & __TINFL_FLAG_PARSE_ZLIB_HEADER) && (r->m_check_adler32 != r->m_z_adler32)) status = __TINFL_STATUS_ADLER32_MISMATCH;
  }
  return status;
}

// Higher level helper functions.
void *__tinfl_decompress_mem_to_heap(const void *pSrc_buf, size_t src_buf_len, size_t *pOut_len, int flags)
{
  __tinfl_decompressor decomp; void *pBuf = NULL, *pNew_buf; size_t src_buf_ofs = 0, out_buf_capacity = 0;
  *pOut_len = 0;
  __tinfl_init(&decomp);
  for ( ; ; )
  {
    size_t src_buf_size = src_buf_len - src_buf_ofs, dst_buf_size = out_buf_capacity - *pOut_len, new_out_buf_capacity;
    __tinfl_status status = __tinfl_decompress(&decomp, (const __mz_uint8*)pSrc_buf + src_buf_ofs, &src_buf_size, (__mz_uint8*)pBuf, pBuf ? (__mz_uint8*)pBuf + *pOut_len : NULL, &dst_buf_size,
      (flags & ~__TINFL_FLAG_HAS_MORE_INPUT) | __TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    if ((status < 0) || (status == __TINFL_STATUS_NEEDS_MORE_INPUT))
    {
      __MZ_FREE(pBuf); *pOut_len = 0; return NULL;
    }
    src_buf_ofs += src_buf_size;
    *pOut_len += dst_buf_size;
    if (status == __TINFL_STATUS_DONE) break;
    new_out_buf_capacity = out_buf_capacity * 2; if (new_out_buf_capacity < 128) new_out_buf_capacity = 128;
    pNew_buf = __MZ_REALLOC(pBuf, new_out_buf_capacity);
    if (!pNew_buf)
    {
      __MZ_FREE(pBuf); *pOut_len = 0; return NULL;
    }
    pBuf = pNew_buf; out_buf_capacity = new_out_buf_capacity;
  }
  return pBuf;
}

size_t __tinfl_decompress_mem_to_mem(void *pOut_buf, size_t out_buf_len, const void *pSrc_buf, size_t src_buf_len, int flags)
{
  __tinfl_decompressor decomp; __tinfl_status status; __tinfl_init(&decomp);
  status = __tinfl_decompress(&decomp, (const __mz_uint8*)pSrc_buf, &src_buf_len, (__mz_uint8*)pOut_buf, (__mz_uint8*)pOut_buf, &out_buf_len, (flags & ~__TINFL_FLAG_HAS_MORE_INPUT) | __TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
  return (status != __TINFL_STATUS_DONE) ? __TINFL_DECOMPRESS_MEM_TO_MEM_FAILED : out_buf_len;
}

int __tinfl_decompress_mem_to_callback(const void *pIn_buf, size_t *pIn_buf_size, __tinfl_put_buf_func_ptr pPut_buf_func, void *pPut_buf_user, int flags)
{
  int result = 0;
  __tinfl_decompressor decomp;
  __mz_uint8 *pDict = (__mz_uint8*)__MZ_MALLOC(__TINFL_LZ_DICT_SIZE); size_t in_buf_ofs = 0, dict_ofs = 0;
  if (!pDict)
    return __TINFL_STATUS_FAILED;
  __tinfl_init(&decomp);
  for ( ; ; )
  {
    size_t in_buf_size = *pIn_buf_size - in_buf_ofs, dst_buf_size = __TINFL_LZ_DICT_SIZE - dict_ofs;
    __tinfl_status status = __tinfl_decompress(&decomp, (const __mz_uint8*)pIn_buf + in_buf_ofs, &in_buf_size, pDict, pDict + dict_ofs, &dst_buf_size,
      (flags & ~(__TINFL_FLAG_HAS_MORE_INPUT | __TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF)));
    in_buf_ofs += in_buf_size;
    if ((dst_buf_size) && (!(*pPut_buf_func)(pDict + dict_ofs, (int)dst_buf_size, pPut_buf_user)))
      break;
    if (status != __TINFL_STATUS_HAS_MORE_OUTPUT)
    {
      result = (status == __TINFL_STATUS_DONE);
      break;
    }
    dict_ofs = (dict_ofs + dst_buf_size) & (__TINFL_LZ_DICT_SIZE - 1);
  }
  __MZ_FREE(pDict);
  *pIn_buf_size = in_buf_ofs;
  return result;
}

// ------------------- Low-level Compression (independent from all decompression API's)

// Purposely making these tables static for faster init and thread safety.
static const __mz_uint16 s_tdefl_len_sym[256] = {
  257,258,259,260,261,262,263,264,265,265,266,266,267,267,268,268,269,269,269,269,270,270,270,270,271,271,271,271,272,272,272,272,
  273,273,273,273,273,273,273,273,274,274,274,274,274,274,274,274,275,275,275,275,275,275,275,275,276,276,276,276,276,276,276,276,
  277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,277,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,
  279,279,279,279,279,279,279,279,279,279,279,279,279,279,279,279,280,280,280,280,280,280,280,280,280,280,280,280,280,280,280,280,
  281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,281,
  282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,
  283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,283,
  284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,284,285 };

static const __mz_uint8 s_tdefl_len_extra[256] = {
  0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,0 };

static const __mz_uint8 s_tdefl_small_dist_sym[512] = {
  0,1,2,3,4,4,5,5,6,6,6,6,7,7,7,7,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,11,11,11,11,11,11,
  11,11,11,11,11,11,11,11,11,11,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,13,
  13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,14,14,14,14,14,14,14,14,14,14,14,14,
  14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
  14,14,14,14,14,14,14,14,14,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
  15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,16,16,16,16,16,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
  16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
  17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
  17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,
  17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17 };

static const __mz_uint8 s_tdefl_small_dist_extra[512] = {
  0,0,0,0,1,1,1,1,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7 };

static const __mz_uint8 s_tdefl_large_dist_sym[128] = {
  0,0,18,19,20,20,21,21,22,22,22,22,23,23,23,23,24,24,24,24,24,24,24,24,25,25,25,25,25,25,25,25,26,26,26,26,26,26,26,26,26,26,26,26,
  26,26,26,26,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,27,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,28,
  28,28,28,28,28,28,28,28,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29 };

static const __mz_uint8 s_tdefl_large_dist_extra[128] = {
  0,0,8,8,9,9,9,9,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
  12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
  13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13 };

// Radix sorts tdefl_sym_freq[] array by 16-bit key m_key. Returns ptr to sorted values.
typedef struct { __mz_uint16 m_key, m_sym_index; } __tdefl_sym_freq;
static __tdefl_sym_freq* __tdefl_radix_sort_syms(__mz_uint num_syms, __tdefl_sym_freq* pSyms0, __tdefl_sym_freq* pSyms1)
{
  __mz_uint32 total_passes = 2, pass_shift, pass, i, hist[256 * 2]; __tdefl_sym_freq* pCur_syms = pSyms0, *pNew_syms = pSyms1; __MZ_CLEAR_OBJ(hist);
  for (i = 0; i < num_syms; i++) { __mz_uint freq = pSyms0[i].m_key; hist[freq & 0xFF]++; hist[256 + ((freq >> 8) & 0xFF)]++; }
  while ((total_passes > 1) && (num_syms == hist[(total_passes - 1) * 256])) total_passes--;
  for (pass_shift = 0, pass = 0; pass < total_passes; pass++, pass_shift += 8)
  {
    const __mz_uint32* pHist = &hist[pass << 8];
    __mz_uint offsets[256], cur_ofs = 0;
    for (i = 0; i < 256; i++) { offsets[i] = cur_ofs; cur_ofs += pHist[i]; }
    for (i = 0; i < num_syms; i++) pNew_syms[offsets[(pCur_syms[i].m_key >> pass_shift) & 0xFF]++] = pCur_syms[i];
    { __tdefl_sym_freq* t = pCur_syms; pCur_syms = pNew_syms; pNew_syms = t; }
  }
  return pCur_syms;
}

// tdefl_calculate_minimum_redundancy() originally written by: Alistair Moffat, alistair@cs.mu.oz.au, Jyrki Katajainen, jyrki@diku.dk, November 1996.
static void __tdefl_calculate_minimum_redundancy(__tdefl_sym_freq *A, int n)
{
  int root, leaf, next, avbl, used, dpth;
  if (n==0) return; else if (n==1) { A[0].m_key = 1; return; }
  A[0].m_key += A[1].m_key; root = 0; leaf = 2;
  for (next=1; next < n-1; next++)
  {
    if (leaf>=n || A[root].m_key<A[leaf].m_key) { A[next].m_key = A[root].m_key; A[root++].m_key = (__mz_uint16)next; } else A[next].m_key = A[leaf++].m_key;
    if (leaf>=n || (root<next && A[root].m_key<A[leaf].m_key)) { A[next].m_key = (__mz_uint16)(A[next].m_key + A[root].m_key); A[root++].m_key = (__mz_uint16)next; } else A[next].m_key = (__mz_uint16)(A[next].m_key + A[leaf++].m_key);
  }
  A[n-2].m_key = 0; for (next=n-3; next>=0; next--) A[next].m_key = A[A[next].m_key].m_key+1;
  avbl = 1; used = dpth = 0; root = n-2; next = n-1;
  while (avbl>0)
  {
    while (root>=0 && (int)A[root].m_key==dpth) { used++; root--; }
    while (avbl>used) { A[next--].m_key = (__mz_uint16)(dpth); avbl--; }
    avbl = 2*used; dpth++; used = 0;
  }
}

// Limits canonical Huffman code table's max code size.
enum { __TDEFL_MAX_SUPPORTED_HUFF_CODESIZE = 32 };
static void __tdefl_huffman_enforce_max_code_size(int *pNum_codes, int code_list_len, int max_code_size)
{
  int i; __mz_uint32 total = 0; if (code_list_len <= 1) return;
  for (i = max_code_size + 1; i <= __TDEFL_MAX_SUPPORTED_HUFF_CODESIZE; i++) pNum_codes[max_code_size] += pNum_codes[i];
  for (i = max_code_size; i > 0; i--) total += (((__mz_uint32)pNum_codes[i]) << (max_code_size - i));
  while (total != (1UL << max_code_size))
  {
    pNum_codes[max_code_size]--;
    for (i = max_code_size - 1; i > 0; i--) if (pNum_codes[i]) { pNum_codes[i]--; pNum_codes[i + 1] += 2; break; }
    total--;
  }
}

static void __tdefl_optimize_huffman_table(__tdefl_compressor *d, int table_num, int table_len, int code_size_limit, int static_table)
{
  int i, j, l, num_codes[1 + __TDEFL_MAX_SUPPORTED_HUFF_CODESIZE]; __mz_uint next_code[__TDEFL_MAX_SUPPORTED_HUFF_CODESIZE + 1]; __MZ_CLEAR_OBJ(num_codes);
  if (static_table)
  {
    for (i = 0; i < table_len; i++) num_codes[d->m_huff_code_sizes[table_num][i]]++;
  }
  else
  {
    __tdefl_sym_freq syms0[__TDEFL_MAX_HUFF_SYMBOLS], syms1[__TDEFL_MAX_HUFF_SYMBOLS], *pSyms;
    int num_used_syms = 0;
    const __mz_uint16 *pSym_count = &d->m_huff_count[table_num][0];
    for (i = 0; i < table_len; i++) if (pSym_count[i]) { syms0[num_used_syms].m_key = (__mz_uint16)pSym_count[i]; syms0[num_used_syms++].m_sym_index = (__mz_uint16)i; }

    pSyms = __tdefl_radix_sort_syms(num_used_syms, syms0, syms1); __tdefl_calculate_minimum_redundancy(pSyms, num_used_syms);

    for (i = 0; i < num_used_syms; i++) num_codes[pSyms[i].m_key]++;

    __tdefl_huffman_enforce_max_code_size(num_codes, num_used_syms, code_size_limit);

    __MZ_CLEAR_OBJ(d->m_huff_code_sizes[table_num]); __MZ_CLEAR_OBJ(d->m_huff_codes[table_num]);
    for (i = 1, j = num_used_syms; i <= code_size_limit; i++)
      for (l = num_codes[i]; l > 0; l--) d->m_huff_code_sizes[table_num][pSyms[--j].m_sym_index] = (__mz_uint8)(i);
  }

  next_code[1] = 0; for (j = 0, i = 2; i <= code_size_limit; i++) next_code[i] = j = ((j + num_codes[i - 1]) << 1);

  for (i = 0; i < table_len; i++)
  {
    __mz_uint rev_code = 0, code, code_size; if ((code_size = d->m_huff_code_sizes[table_num][i]) == 0) continue;
    code = next_code[code_size]++; for (l = code_size; l > 0; l--, code >>= 1) rev_code = (rev_code << 1) | (code & 1);
    d->m_huff_codes[table_num][i] = (__mz_uint16)rev_code;
  }
}

#define __TDEFL_PUT_BITS(b, l) do { \
  __mz_uint bits = b; __mz_uint len = l; __MZ_ASSERT(bits <= ((1U << len) - 1U)); \
  d->m_bit_buffer |= (bits << d->m_bits_in); d->m_bits_in += len; \
  while (d->m_bits_in >= 8) { \
    if (d->m_pOutput_buf < d->m_pOutput_buf_end) \
      *d->m_pOutput_buf++ = (__mz_uint8)(d->m_bit_buffer); \
      d->m_bit_buffer >>= 8; \
      d->m_bits_in -= 8; \
  } \
} __MZ_MACRO_END

#define __TDEFL_RLE_PREV_CODE_SIZE() { if (rle_repeat_count) { \
  if (rle_repeat_count < 3) { \
    d->m_huff_count[2][prev_code_size] = (__mz_uint16)(d->m_huff_count[2][prev_code_size] + rle_repeat_count); \
    while (rle_repeat_count--) packed_code_sizes[num_packed_code_sizes++] = prev_code_size; \
  } else { \
    d->m_huff_count[2][16] = (__mz_uint16)(d->m_huff_count[2][16] + 1); packed_code_sizes[num_packed_code_sizes++] = 16; packed_code_sizes[num_packed_code_sizes++] = (__mz_uint8)(rle_repeat_count - 3); \
} rle_repeat_count = 0; } }

#define __TDEFL_RLE_ZERO_CODE_SIZE() { if (rle_z_count) { \
  if (rle_z_count < 3) { \
    d->m_huff_count[2][0] = (__mz_uint16)(d->m_huff_count[2][0] + rle_z_count); while (rle_z_count--) packed_code_sizes[num_packed_code_sizes++] = 0; \
  } else if (rle_z_count <= 10) { \
    d->m_huff_count[2][17] = (__mz_uint16)(d->m_huff_count[2][17] + 1); packed_code_sizes[num_packed_code_sizes++] = 17; packed_code_sizes[num_packed_code_sizes++] = (__mz_uint8)(rle_z_count - 3); \
  } else { \
    d->m_huff_count[2][18] = (__mz_uint16)(d->m_huff_count[2][18] + 1); packed_code_sizes[num_packed_code_sizes++] = 18; packed_code_sizes[num_packed_code_sizes++] = (__mz_uint8)(rle_z_count - 11); \
} rle_z_count = 0; } }

static __mz_uint8 s_tdefl_packed_code_size_syms_swizzle[] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

static void __tdefl_start_dynamic_block(__tdefl_compressor *d)
{
  int num_lit_codes, num_dist_codes, num_bit_lengths; __mz_uint i, total_code_sizes_to_pack, num_packed_code_sizes, rle_z_count, rle_repeat_count, packed_code_sizes_index;
  __mz_uint8 code_sizes_to_pack[__TDEFL_MAX_HUFF_SYMBOLS_0 + __TDEFL_MAX_HUFF_SYMBOLS_1], packed_code_sizes[__TDEFL_MAX_HUFF_SYMBOLS_0 + __TDEFL_MAX_HUFF_SYMBOLS_1], prev_code_size = 0xFF;

  d->m_huff_count[0][256] = 1;

  __tdefl_optimize_huffman_table(d, 0, __TDEFL_MAX_HUFF_SYMBOLS_0, 15, __MZ_FALSE);
  __tdefl_optimize_huffman_table(d, 1, __TDEFL_MAX_HUFF_SYMBOLS_1, 15, __MZ_FALSE);

  for (num_lit_codes = 286; num_lit_codes > 257; num_lit_codes--) if (d->m_huff_code_sizes[0][num_lit_codes - 1]) break;
  for (num_dist_codes = 30; num_dist_codes > 1; num_dist_codes--) if (d->m_huff_code_sizes[1][num_dist_codes - 1]) break;

  memcpy(code_sizes_to_pack, &d->m_huff_code_sizes[0][0], num_lit_codes);
  memcpy(code_sizes_to_pack + num_lit_codes, &d->m_huff_code_sizes[1][0], num_dist_codes);
  total_code_sizes_to_pack = num_lit_codes + num_dist_codes; num_packed_code_sizes = 0; rle_z_count = 0; rle_repeat_count = 0;

  memset(&d->m_huff_count[2][0], 0, sizeof(d->m_huff_count[2][0]) * __TDEFL_MAX_HUFF_SYMBOLS_2);
  for (i = 0; i < total_code_sizes_to_pack; i++)
  {
    __mz_uint8 code_size = code_sizes_to_pack[i];
    if (!code_size)
    {
      __TDEFL_RLE_PREV_CODE_SIZE();
      if (++rle_z_count == 138) { __TDEFL_RLE_ZERO_CODE_SIZE(); }
    }
    else
    {
      __TDEFL_RLE_ZERO_CODE_SIZE();
      if (code_size != prev_code_size)
      {
        __TDEFL_RLE_PREV_CODE_SIZE();
        d->m_huff_count[2][code_size] = (__mz_uint16)(d->m_huff_count[2][code_size] + 1); packed_code_sizes[num_packed_code_sizes++] = code_size;
      }
      else if (++rle_repeat_count == 6)
      {
        __TDEFL_RLE_PREV_CODE_SIZE();
      }
    }
    prev_code_size = code_size;
  }
  if (rle_repeat_count) { __TDEFL_RLE_PREV_CODE_SIZE(); } else { __TDEFL_RLE_ZERO_CODE_SIZE(); }

  __tdefl_optimize_huffman_table(d, 2, __TDEFL_MAX_HUFF_SYMBOLS_2, 7, __MZ_FALSE);

  __TDEFL_PUT_BITS(2, 2);

  __TDEFL_PUT_BITS(num_lit_codes - 257, 5);
  __TDEFL_PUT_BITS(num_dist_codes - 1, 5);

  for (num_bit_lengths = 18; num_bit_lengths >= 0; num_bit_lengths--) if (d->m_huff_code_sizes[2][s_tdefl_packed_code_size_syms_swizzle[num_bit_lengths]]) break;
  num_bit_lengths = __MZ_MAX(4, (num_bit_lengths + 1)); __TDEFL_PUT_BITS(num_bit_lengths - 4, 4);
  for (i = 0; (int)i < num_bit_lengths; i++) __TDEFL_PUT_BITS(d->m_huff_code_sizes[2][s_tdefl_packed_code_size_syms_swizzle[i]], 3);

  for (packed_code_sizes_index = 0; packed_code_sizes_index < num_packed_code_sizes; )
  {
    __mz_uint code = packed_code_sizes[packed_code_sizes_index++]; __MZ_ASSERT(code < __TDEFL_MAX_HUFF_SYMBOLS_2);
    __TDEFL_PUT_BITS(d->m_huff_codes[2][code], d->m_huff_code_sizes[2][code]);
    if (code >= 16) __TDEFL_PUT_BITS(packed_code_sizes[packed_code_sizes_index++], "\02\03\07"[code - 16]);
  }
}

static void __tdefl_start_static_block(__tdefl_compressor *d)
{
  __mz_uint i;
  __mz_uint8 *p = &d->m_huff_code_sizes[0][0];

  for (i = 0; i <= 143; ++i) *p++ = 8;
  for ( ; i <= 255; ++i) *p++ = 9;
  for ( ; i <= 279; ++i) *p++ = 7;
  for ( ; i <= 287; ++i) *p++ = 8;

  memset(d->m_huff_code_sizes[1], 5, 32);

  __tdefl_optimize_huffman_table(d, 0, 288, 15, __MZ_TRUE);
  __tdefl_optimize_huffman_table(d, 1, 32, 15, __MZ_TRUE);

  __TDEFL_PUT_BITS(1, 2);
}

static const __mz_uint mz_bitmasks[17] = { 0x0000, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F, 0x00FF, 0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };

#if __MINIZ_USE_UNALIGNED_LOADS_AND_STORES && __MINIZ_LITTLE_ENDIAN && __MINIZ_HAS_64BIT_REGISTERS
static __mz_bool __tdefl_compress_lz_codes(__tdefl_compressor *d)
{
  __mz_uint flags;
  __mz_uint8 *pLZ_codes;
  __mz_uint8 *pOutput_buf = d->m_pOutput_buf;
  __mz_uint8 *pLZ_code_buf_end = d->m_pLZ_code_buf;
  __mz_uint64 bit_buffer = d->m_bit_buffer;
  __mz_uint bits_in = d->m_bits_in;

#define __TDEFL_PUT_BITS_FAST(b, l) { bit_buffer |= (((__mz_uint64)(b)) << bits_in); bits_in += (l); }

  flags = 1;
  for (pLZ_codes = d->m_lz_code_buf; pLZ_codes < pLZ_code_buf_end; flags >>= 1)
  {
    if (flags == 1)
      flags = *pLZ_codes++ | 0x100;

    if (flags & 1)
    {
      __mz_uint s0, s1, n0, n1, sym, num_extra_bits;
      __mz_uint match_len = pLZ_codes[0], match_dist = *(const __mz_uint16 *)(pLZ_codes + 1); pLZ_codes += 3;

      __MZ_ASSERT(d->m_huff_code_sizes[0][s_tdefl_len_sym[match_len]]);
      __TDEFL_PUT_BITS_FAST(d->m_huff_codes[0][s_tdefl_len_sym[match_len]], d->m_huff_code_sizes[0][s_tdefl_len_sym[match_len]]);
      __TDEFL_PUT_BITS_FAST(match_len & mz_bitmasks[s_tdefl_len_extra[match_len]], s_tdefl_len_extra[match_len]);

      // This sequence coaxes MSVC into using cmov's vs. jmp's.
      s0 = s_tdefl_small_dist_sym[match_dist & 511];
      n0 = s_tdefl_small_dist_extra[match_dist & 511];
      s1 = s_tdefl_large_dist_sym[match_dist >> 8];
      n1 = s_tdefl_large_dist_extra[match_dist >> 8];
      sym = (match_dist < 512) ? s0 : s1;
      num_extra_bits = (match_dist < 512) ? n0 : n1;

      __MZ_ASSERT(d->m_huff_code_sizes[1][sym]);
      __TDEFL_PUT_BITS_FAST(d->m_huff_codes[1][sym], d->m_huff_code_sizes[1][sym]);
      __TDEFL_PUT_BITS_FAST(match_dist & mz_bitmasks[num_extra_bits], num_extra_bits);
    }
    else
    {
      __mz_uint lit = *pLZ_codes++;
      __MZ_ASSERT(d->m_huff_code_sizes[0][lit]);
      __TDEFL_PUT_BITS_FAST(d->m_huff_codes[0][lit], d->m_huff_code_sizes[0][lit]);

      if (((flags & 2) == 0) && (pLZ_codes < pLZ_code_buf_end))
      {
        flags >>= 1;
        lit = *pLZ_codes++;
        __MZ_ASSERT(d->m_huff_code_sizes[0][lit]);
        __TDEFL_PUT_BITS_FAST(d->m_huff_codes[0][lit], d->m_huff_code_sizes[0][lit]);

        if (((flags & 2) == 0) && (pLZ_codes < pLZ_code_buf_end))
        {
          flags >>= 1;
          lit = *pLZ_codes++;
          __MZ_ASSERT(d->m_huff_code_sizes[0][lit]);
          __TDEFL_PUT_BITS_FAST(d->m_huff_codes[0][lit], d->m_huff_code_sizes[0][lit]);
        }
      }
    }

    if (pOutput_buf >= d->m_pOutput_buf_end)
      return __MZ_FALSE;

    *(__mz_uint64*)pOutput_buf = bit_buffer;
    pOutput_buf += (bits_in >> 3);
    bit_buffer >>= (bits_in & ~7);
    bits_in &= 7;
  }

#undef __TDEFL_PUT_BITS_FAST

  d->m_pOutput_buf = pOutput_buf;
  d->m_bits_in = 0;
  d->m_bit_buffer = 0;

  while (bits_in)
  {
    __mz_uint32 n = __MZ_MIN(bits_in, 16);
    __TDEFL_PUT_BITS((__mz_uint)bit_buffer & mz_bitmasks[n], n);
    bit_buffer >>= n;
    bits_in -= n;
  }

  __TDEFL_PUT_BITS(d->m_huff_codes[0][256], d->m_huff_code_sizes[0][256]);

  return (d->m_pOutput_buf < d->m_pOutput_buf_end);
}
#else
static __mz_bool __tdefl_compress_lz_codes(__tdefl_compressor *d)
{
  __mz_uint flags;
  __mz_uint8 *pLZ_codes;

  flags = 1;
  for (pLZ_codes = d->m_lz_code_buf; pLZ_codes < d->m_pLZ_code_buf; flags >>= 1)
  {
    if (flags == 1)
      flags = *pLZ_codes++ | 0x100;
    if (flags & 1)
    {
      __mz_uint sym, num_extra_bits;
      __mz_uint match_len = pLZ_codes[0], match_dist = (pLZ_codes[1] | (pLZ_codes[2] << 8)); pLZ_codes += 3;

      __MZ_ASSERT(d->m_huff_code_sizes[0][s_tdefl_len_sym[match_len]]);
      __TDEFL_PUT_BITS(d->m_huff_codes[0][s_tdefl_len_sym[match_len]], d->m_huff_code_sizes[0][s_tdefl_len_sym[match_len]]);
      __TDEFL_PUT_BITS(match_len & mz_bitmasks[s_tdefl_len_extra[match_len]], s_tdefl_len_extra[match_len]);

      if (match_dist < 512)
      {
        sym = s_tdefl_small_dist_sym[match_dist]; num_extra_bits = s_tdefl_small_dist_extra[match_dist];
      }
      else
      {
        sym = s_tdefl_large_dist_sym[match_dist >> 8]; num_extra_bits = s_tdefl_large_dist_extra[match_dist >> 8];
      }
      __MZ_ASSERT(d->m_huff_code_sizes[1][sym]);
      __TDEFL_PUT_BITS(d->m_huff_codes[1][sym], d->m_huff_code_sizes[1][sym]);
      __TDEFL_PUT_BITS(match_dist & mz_bitmasks[num_extra_bits], num_extra_bits);
    }
    else
    {
      __mz_uint lit = *pLZ_codes++;
      __MZ_ASSERT(d->m_huff_code_sizes[0][lit]);
      __TDEFL_PUT_BITS(d->m_huff_codes[0][lit], d->m_huff_code_sizes[0][lit]);
    }
  }

  __TDEFL_PUT_BITS(d->m_huff_codes[0][256], d->m_huff_code_sizes[0][256]);

  return (d->m_pOutput_buf < d->m_pOutput_buf_end);
}
#endif // __MINIZ_USE_UNALIGNED_LOADS_AND_STORES && __MINIZ_LITTLE_ENDIAN && __MINIZ_HAS_64BIT_REGISTERS

static __mz_bool __tdefl_compress_block(__tdefl_compressor *d, __mz_bool static_block)
{
  if (static_block)
    __tdefl_start_static_block(d);
  else
    __tdefl_start_dynamic_block(d);
  return __tdefl_compress_lz_codes(d);
}

static int __tdefl_flush_block(__tdefl_compressor *d, int flush)
{
  __mz_uint saved_bit_buf, saved_bits_in;
  __mz_uint8 *pSaved_output_buf;
  __mz_bool comp_block_succeeded = __MZ_FALSE;
  int n, use_raw_block = ((d->m_flags & __TDEFL_FORCE_ALL_RAW_BLOCKS) != 0) && (d->m_lookahead_pos - d->m_lz_code_buf_dict_pos) <= d->m_dict_size;
  __mz_uint8 *pOutput_buf_start = ((d->m_pPut_buf_func == NULL) && ((*d->m_pOut_buf_size - d->m_out_buf_ofs) >= __TDEFL_OUT_BUF_SIZE)) ? ((__mz_uint8 *)d->m_pOut_buf + d->m_out_buf_ofs) : d->m_output_buf;

  d->m_pOutput_buf = pOutput_buf_start;
  d->m_pOutput_buf_end = d->m_pOutput_buf + __TDEFL_OUT_BUF_SIZE - 16;

  __MZ_ASSERT(!d->m_output_flush_remaining);
  d->m_output_flush_ofs = 0;
  d->m_output_flush_remaining = 0;

  *d->m_pLZ_flags = (__mz_uint8)(*d->m_pLZ_flags >> d->m_num_flags_left);
  d->m_pLZ_code_buf -= (d->m_num_flags_left == 8);

  if ((d->m_flags & __TDEFL_WRITE_ZLIB_HEADER) && (!d->m_block_index))
  {
    __TDEFL_PUT_BITS(0x78, 8); __TDEFL_PUT_BITS(0x01, 8);
  }

  __TDEFL_PUT_BITS(flush == __TDEFL_FINISH, 1);

  pSaved_output_buf = d->m_pOutput_buf; saved_bit_buf = d->m_bit_buffer; saved_bits_in = d->m_bits_in;

  if (!use_raw_block)
    comp_block_succeeded = __tdefl_compress_block(d, (d->m_flags & __TDEFL_FORCE_ALL_STATIC_BLOCKS) || (d->m_total_lz_bytes < 48));

  // If the block gets expanded, forget the current contents of the output buffer and send a raw block instead.
  if ( ((use_raw_block) || ((d->m_total_lz_bytes) && ((d->m_pOutput_buf - pSaved_output_buf + 1U) >= d->m_total_lz_bytes))) &&
       ((d->m_lookahead_pos - d->m_lz_code_buf_dict_pos) <= d->m_dict_size) )
  {
    __mz_uint i; d->m_pOutput_buf = pSaved_output_buf; d->m_bit_buffer = saved_bit_buf, d->m_bits_in = saved_bits_in;
    __TDEFL_PUT_BITS(0, 2);
    if (d->m_bits_in) { __TDEFL_PUT_BITS(0, 8 - d->m_bits_in); }
    for (i = 2; i; --i, d->m_total_lz_bytes ^= 0xFFFF)
    {
      __TDEFL_PUT_BITS(d->m_total_lz_bytes & 0xFFFF, 16);
    }
    for (i = 0; i < d->m_total_lz_bytes; ++i)
    {
      __TDEFL_PUT_BITS(d->m_dict[(d->m_lz_code_buf_dict_pos + i) & __TDEFL_LZ_DICT_SIZE_MASK], 8);
    }
  }
  // Check for the extremely unlikely (if not impossible) case of the compressed block not fitting into the output buffer when using dynamic codes.
  else if (!comp_block_succeeded)
  {
    d->m_pOutput_buf = pSaved_output_buf; d->m_bit_buffer = saved_bit_buf, d->m_bits_in = saved_bits_in;
    __tdefl_compress_block(d, __MZ_TRUE);
  }

  if (flush)
  {
    if (flush == __TDEFL_FINISH)
    {
      if (d->m_bits_in) { __TDEFL_PUT_BITS(0, 8 - d->m_bits_in); }
      if (d->m_flags & __TDEFL_WRITE_ZLIB_HEADER) { __mz_uint i, a = d->m_adler32; for (i = 0; i < 4; i++) { __TDEFL_PUT_BITS((a >> 24) & 0xFF, 8); a <<= 8; } }
    }
    else
    {
      __mz_uint i, z = 0; __TDEFL_PUT_BITS(0, 3); if (d->m_bits_in) { __TDEFL_PUT_BITS(0, 8 - d->m_bits_in); } for (i = 2; i; --i, z ^= 0xFFFF) { __TDEFL_PUT_BITS(z & 0xFFFF, 16); }
    }
  }

  __MZ_ASSERT(d->m_pOutput_buf < d->m_pOutput_buf_end);

  memset(&d->m_huff_count[0][0], 0, sizeof(d->m_huff_count[0][0]) * __TDEFL_MAX_HUFF_SYMBOLS_0);
  memset(&d->m_huff_count[1][0], 0, sizeof(d->m_huff_count[1][0]) * __TDEFL_MAX_HUFF_SYMBOLS_1);

  d->m_pLZ_code_buf = d->m_lz_code_buf + 1; d->m_pLZ_flags = d->m_lz_code_buf; d->m_num_flags_left = 8; d->m_lz_code_buf_dict_pos += d->m_total_lz_bytes; d->m_total_lz_bytes = 0; d->m_block_index++;

  if ((n = (int)(d->m_pOutput_buf - pOutput_buf_start)) != 0)
  {
    if (d->m_pPut_buf_func)
    {
      *d->m_pIn_buf_size = d->m_pSrc - (const __mz_uint8 *)d->m_pIn_buf;
      if (!(*d->m_pPut_buf_func)(d->m_output_buf, n, d->m_pPut_buf_user))
        return (d->m_prev_return_status = __TDEFL_STATUS_PUT_BUF_FAILED);
    }
    else if (pOutput_buf_start == d->m_output_buf)
    {
      int bytes_to_copy = (int)__MZ_MIN((size_t)n, (size_t)(*d->m_pOut_buf_size - d->m_out_buf_ofs));
      memcpy((__mz_uint8 *)d->m_pOut_buf + d->m_out_buf_ofs, d->m_output_buf, bytes_to_copy);
      d->m_out_buf_ofs += bytes_to_copy;
      if ((n -= bytes_to_copy) != 0)
      {
        d->m_output_flush_ofs = bytes_to_copy;
        d->m_output_flush_remaining = n;
      }
    }
    else
    {
      d->m_out_buf_ofs += n;
    }
  }

  return d->m_output_flush_remaining;
}

#if __MINIZ_USE_UNALIGNED_LOADS_AND_STORES
#define __TDEFL_READ_UNALIGNED_WORD(p) *(const __mz_uint16*)(p)
static __MZ_FORCEINLINE void __tdefl_find_match(__tdefl_compressor *d, __mz_uint lookahead_pos, __mz_uint max_dist, __mz_uint max_match_len, __mz_uint *pMatch_dist, __mz_uint *pMatch_len)
{
  __mz_uint dist, pos = lookahead_pos & __TDEFL_LZ_DICT_SIZE_MASK, match_len = *pMatch_len, probe_pos = pos, next_probe_pos, probe_len;
  __mz_uint num_probes_left = d->m_max_probes[match_len >= 32];
  const __mz_uint16 *s = (const __mz_uint16*)(d->m_dict + pos), *p, *q;
  __mz_uint16 c01 = __TDEFL_READ_UNALIGNED_WORD(&d->m_dict[pos + match_len - 1]), s01 = __TDEFL_READ_UNALIGNED_WORD(s);
  __MZ_ASSERT(max_match_len <= __TDEFL_MAX_MATCH_LEN); if (max_match_len <= match_len) return;
  for ( ; ; )
  {
    for ( ; ; )
    {
      if (--num_probes_left == 0) return;
      #define __TDEFL_PROBE \
        next_probe_pos = d->m_next[probe_pos]; \
        if ((!next_probe_pos) || ((dist = (__mz_uint16)(lookahead_pos - next_probe_pos)) > max_dist)) return; \
        probe_pos = next_probe_pos & __TDEFL_LZ_DICT_SIZE_MASK; \
        if (__TDEFL_READ_UNALIGNED_WORD(&d->m_dict[probe_pos + match_len - 1]) == c01) break;
      __TDEFL_PROBE; __TDEFL_PROBE; __TDEFL_PROBE;
    }
    if (!dist) break; q = (const __mz_uint16*)(d->m_dict + probe_pos); if (__TDEFL_READ_UNALIGNED_WORD(q) != s01) continue; p = s; probe_len = 32;
    do { } while ( (__TDEFL_READ_UNALIGNED_WORD(++p) == __TDEFL_READ_UNALIGNED_WORD(++q)) && (__TDEFL_READ_UNALIGNED_WORD(++p) == __TDEFL_READ_UNALIGNED_WORD(++q)) &&
                   (__TDEFL_READ_UNALIGNED_WORD(++p) == __TDEFL_READ_UNALIGNED_WORD(++q)) && (__TDEFL_READ_UNALIGNED_WORD(++p) == __TDEFL_READ_UNALIGNED_WORD(++q)) && (--probe_len > 0) );
    if (!probe_len)
    {
      *pMatch_dist = dist; *pMatch_len = __MZ_MIN(max_match_len, __TDEFL_MAX_MATCH_LEN); break;
    }
    else if ((probe_len = ((__mz_uint)(p - s) * 2) + (__mz_uint)(*(const __mz_uint8*)p == *(const __mz_uint8*)q)) > match_len)
    {
      *pMatch_dist = dist; if ((*pMatch_len = match_len = __MZ_MIN(max_match_len, probe_len)) == max_match_len) break;
      c01 = __TDEFL_READ_UNALIGNED_WORD(&d->m_dict[pos + match_len - 1]);
    }
  }
}
#else
static __MZ_FORCEINLINE void __tdefl_find_match(__tdefl_compressor *d, __mz_uint lookahead_pos, __mz_uint max_dist, __mz_uint max_match_len, __mz_uint *pMatch_dist, __mz_uint *pMatch_len)
{
  __mz_uint dist, pos = lookahead_pos & __TDEFL_LZ_DICT_SIZE_MASK, match_len = *pMatch_len, probe_pos = pos, next_probe_pos, probe_len;
  __mz_uint num_probes_left = d->m_max_probes[match_len >= 32];
  const __mz_uint8 *s = d->m_dict + pos, *p, *q;
  __mz_uint8 c0 = d->m_dict[pos + match_len], c1 = d->m_dict[pos + match_len - 1];
  __MZ_ASSERT(max_match_len <= __TDEFL_MAX_MATCH_LEN); if (max_match_len <= match_len) return;
  for ( ; ; )
  {
    for ( ; ; )
    {
      if (--num_probes_left == 0) return;
      #define __TDEFL_PROBE \
        next_probe_pos = d->m_next[probe_pos]; \
        if ((!next_probe_pos) || ((dist = (__mz_uint16)(lookahead_pos - next_probe_pos)) > max_dist)) return; \
        probe_pos = next_probe_pos & __TDEFL_LZ_DICT_SIZE_MASK; \
        if ((d->m_dict[probe_pos + match_len] == c0) && (d->m_dict[probe_pos + match_len - 1] == c1)) break;
      __TDEFL_PROBE; __TDEFL_PROBE; __TDEFL_PROBE;
    }
    if (!dist) break; p = s; q = d->m_dict + probe_pos; for (probe_len = 0; probe_len < max_match_len; probe_len++) if (*p++ != *q++) break;
    if (probe_len > match_len)
    {
      *pMatch_dist = dist; if ((*pMatch_len = match_len = probe_len) == max_match_len) return;
      c0 = d->m_dict[pos + match_len]; c1 = d->m_dict[pos + match_len - 1];
    }
  }
}
#endif // #if __MINIZ_USE_UNALIGNED_LOADS_AND_STORES

#if __MINIZ_USE_UNALIGNED_LOADS_AND_STORES && __MINIZ_LITTLE_ENDIAN
static __mz_bool __tdefl_compress_fast(__tdefl_compressor *d)
{
  // Faster, minimally featured LZRW1-style match+parse loop with better register utilization. Intended for applications where raw throughput is valued more highly than ratio.
  __mz_uint lookahead_pos = d->m_lookahead_pos, lookahead_size = d->m_lookahead_size, dict_size = d->m_dict_size, total_lz_bytes = d->m_total_lz_bytes, num_flags_left = d->m_num_flags_left;
  __mz_uint8 *pLZ_code_buf = d->m_pLZ_code_buf, *pLZ_flags = d->m_pLZ_flags;
  __mz_uint cur_pos = lookahead_pos & __TDEFL_LZ_DICT_SIZE_MASK;

  while ((d->m_src_buf_left) || ((d->m_flush) && (lookahead_size)))
  {
    const __mz_uint TDEFL_COMP_FAST_LOOKAHEAD_SIZE = 4096;
    __mz_uint dst_pos = (lookahead_pos + lookahead_size) & __TDEFL_LZ_DICT_SIZE_MASK;
    __mz_uint num_bytes_to_process = (__mz_uint)__MZ_MIN(d->m_src_buf_left, TDEFL_COMP_FAST_LOOKAHEAD_SIZE - lookahead_size);
    d->m_src_buf_left -= num_bytes_to_process;
    lookahead_size += num_bytes_to_process;

    while (num_bytes_to_process)
    {
      __mz_uint32 n = __MZ_MIN(__TDEFL_LZ_DICT_SIZE - dst_pos, num_bytes_to_process);
      memcpy(d->m_dict + dst_pos, d->m_pSrc, n);
      if (dst_pos < (__TDEFL_MAX_MATCH_LEN - 1))
        memcpy(d->m_dict + __TDEFL_LZ_DICT_SIZE + dst_pos, d->m_pSrc, __MZ_MIN(n, (__TDEFL_MAX_MATCH_LEN - 1) - dst_pos));
      d->m_pSrc += n;
      dst_pos = (dst_pos + n) & __TDEFL_LZ_DICT_SIZE_MASK;
      num_bytes_to_process -= n;
    }

    dict_size = __MZ_MIN(__TDEFL_LZ_DICT_SIZE - lookahead_size, dict_size);
    if ((!d->m_flush) && (lookahead_size < TDEFL_COMP_FAST_LOOKAHEAD_SIZE)) break;

    while (lookahead_size >= 4)
    {
      __mz_uint cur_match_dist, cur_match_len = 1;
      __mz_uint8 *pCur_dict = d->m_dict + cur_pos;
      __mz_uint first_trigram = (*(const __mz_uint32 *)pCur_dict) & 0xFFFFFF;
      __mz_uint hash = (first_trigram ^ (first_trigram >> (24 - (__TDEFL_LZ_HASH_BITS - 8)))) & __TDEFL_LEVEL1_HASH_SIZE_MASK;
      __mz_uint probe_pos = d->m_hash[hash];
      d->m_hash[hash] = (__mz_uint16)lookahead_pos;

      if (((cur_match_dist = (__mz_uint16)(lookahead_pos - probe_pos)) <= dict_size) && ((*(const __mz_uint32 *)(d->m_dict + (probe_pos &= __TDEFL_LZ_DICT_SIZE_MASK)) & 0xFFFFFF) == first_trigram))
      {
        const __mz_uint16 *p = (const __mz_uint16 *)pCur_dict;
        const __mz_uint16 *q = (const __mz_uint16 *)(d->m_dict + probe_pos);
        __mz_uint32 probe_len = 32;
        do { } while ( (__TDEFL_READ_UNALIGNED_WORD(++p) == __TDEFL_READ_UNALIGNED_WORD(++q)) && (__TDEFL_READ_UNALIGNED_WORD(++p) == __TDEFL_READ_UNALIGNED_WORD(++q)) &&
          (__TDEFL_READ_UNALIGNED_WORD(++p) == __TDEFL_READ_UNALIGNED_WORD(++q)) && (__TDEFL_READ_UNALIGNED_WORD(++p) == __TDEFL_READ_UNALIGNED_WORD(++q)) && (--probe_len > 0) );
        cur_match_len = ((__mz_uint)(p - (const __mz_uint16 *)pCur_dict) * 2) + (__mz_uint)(*(const __mz_uint8 *)p == *(const __mz_uint8 *)q);
        if (!probe_len)
          cur_match_len = cur_match_dist ? __TDEFL_MAX_MATCH_LEN : 0;

        if ((cur_match_len < __TDEFL_MIN_MATCH_LEN) || ((cur_match_len == __TDEFL_MIN_MATCH_LEN) && (cur_match_dist >= 8U*1024U)))
        {
          cur_match_len = 1;
          *pLZ_code_buf++ = (__mz_uint8)first_trigram;
          *pLZ_flags = (__mz_uint8)(*pLZ_flags >> 1);
          d->m_huff_count[0][(__mz_uint8)first_trigram]++;
        }
        else
        {
          __mz_uint32 s0, s1;
          cur_match_len = __MZ_MIN(cur_match_len, lookahead_size);

          __MZ_ASSERT((cur_match_len >= __TDEFL_MIN_MATCH_LEN) && (cur_match_dist >= 1) && (cur_match_dist <= __TDEFL_LZ_DICT_SIZE));

          cur_match_dist--;

          pLZ_code_buf[0] = (__mz_uint8)(cur_match_len - __TDEFL_MIN_MATCH_LEN);
          *(__mz_uint16 *)(&pLZ_code_buf[1]) = (__mz_uint16)cur_match_dist;
          pLZ_code_buf += 3;
          *pLZ_flags = (__mz_uint8)((*pLZ_flags >> 1) | 0x80);

          s0 = s_tdefl_small_dist_sym[cur_match_dist & 511];
          s1 = s_tdefl_large_dist_sym[cur_match_dist >> 8];
          d->m_huff_count[1][(cur_match_dist < 512) ? s0 : s1]++;

          d->m_huff_count[0][s_tdefl_len_sym[cur_match_len - __TDEFL_MIN_MATCH_LEN]]++;
        }
      }
      else
      {
        *pLZ_code_buf++ = (__mz_uint8)first_trigram;
        *pLZ_flags = (__mz_uint8)(*pLZ_flags >> 1);
        d->m_huff_count[0][(__mz_uint8)first_trigram]++;
      }

      if (--num_flags_left == 0) { num_flags_left = 8; pLZ_flags = pLZ_code_buf++; }

      total_lz_bytes += cur_match_len;
      lookahead_pos += cur_match_len;
      dict_size = __MZ_MIN(dict_size + cur_match_len, __TDEFL_LZ_DICT_SIZE);
      cur_pos = (cur_pos + cur_match_len) & __TDEFL_LZ_DICT_SIZE_MASK;
      __MZ_ASSERT(lookahead_size >= cur_match_len);
      lookahead_size -= cur_match_len;

      if (pLZ_code_buf > &d->m_lz_code_buf[__TDEFL_LZ_CODE_BUF_SIZE - 8])
      {
        int n;
        d->m_lookahead_pos = lookahead_pos; d->m_lookahead_size = lookahead_size; d->m_dict_size = dict_size;
        d->m_total_lz_bytes = total_lz_bytes; d->m_pLZ_code_buf = pLZ_code_buf; d->m_pLZ_flags = pLZ_flags; d->m_num_flags_left = num_flags_left;
        if ((n = __tdefl_flush_block(d, 0)) != 0)
          return (n < 0) ? __MZ_FALSE : __MZ_TRUE;
        total_lz_bytes = d->m_total_lz_bytes; pLZ_code_buf = d->m_pLZ_code_buf; pLZ_flags = d->m_pLZ_flags; num_flags_left = d->m_num_flags_left;
      }
    }

    while (lookahead_size)
    {
      __mz_uint8 lit = d->m_dict[cur_pos];

      total_lz_bytes++;
      *pLZ_code_buf++ = lit;
      *pLZ_flags = (__mz_uint8)(*pLZ_flags >> 1);
      if (--num_flags_left == 0) { num_flags_left = 8; pLZ_flags = pLZ_code_buf++; }

      d->m_huff_count[0][lit]++;

      lookahead_pos++;
      dict_size = __MZ_MIN(dict_size + 1, __TDEFL_LZ_DICT_SIZE);
      cur_pos = (cur_pos + 1) & __TDEFL_LZ_DICT_SIZE_MASK;
      lookahead_size--;

      if (pLZ_code_buf > &d->m_lz_code_buf[__TDEFL_LZ_CODE_BUF_SIZE - 8])
      {
        int n;
        d->m_lookahead_pos = lookahead_pos; d->m_lookahead_size = lookahead_size; d->m_dict_size = dict_size;
        d->m_total_lz_bytes = total_lz_bytes; d->m_pLZ_code_buf = pLZ_code_buf; d->m_pLZ_flags = pLZ_flags; d->m_num_flags_left = num_flags_left;
        if ((n = __tdefl_flush_block(d, 0)) != 0)
          return (n < 0) ? __MZ_FALSE : __MZ_TRUE;
        total_lz_bytes = d->m_total_lz_bytes; pLZ_code_buf = d->m_pLZ_code_buf; pLZ_flags = d->m_pLZ_flags; num_flags_left = d->m_num_flags_left;
      }
    }
  }

  d->m_lookahead_pos = lookahead_pos; d->m_lookahead_size = lookahead_size; d->m_dict_size = dict_size;
  d->m_total_lz_bytes = total_lz_bytes; d->m_pLZ_code_buf = pLZ_code_buf; d->m_pLZ_flags = pLZ_flags; d->m_num_flags_left = num_flags_left;
  return __MZ_TRUE;
}
#endif // __MINIZ_USE_UNALIGNED_LOADS_AND_STORES && __MINIZ_LITTLE_ENDIAN

static __MZ_FORCEINLINE void __tdefl_record_literal(__tdefl_compressor *d, __mz_uint8 lit)
{
  d->m_total_lz_bytes++;
  *d->m_pLZ_code_buf++ = lit;
  *d->m_pLZ_flags = (__mz_uint8)(*d->m_pLZ_flags >> 1); if (--d->m_num_flags_left == 0) { d->m_num_flags_left = 8; d->m_pLZ_flags = d->m_pLZ_code_buf++; }
  d->m_huff_count[0][lit]++;
}

static __MZ_FORCEINLINE void __tdefl_record_match(__tdefl_compressor *d, __mz_uint match_len, __mz_uint match_dist)
{
  __mz_uint32 s0, s1;

  __MZ_ASSERT((match_len >= __TDEFL_MIN_MATCH_LEN) && (match_dist >= 1) && (match_dist <= __TDEFL_LZ_DICT_SIZE));

  d->m_total_lz_bytes += match_len;

  d->m_pLZ_code_buf[0] = (__mz_uint8)(match_len - __TDEFL_MIN_MATCH_LEN);

  match_dist -= 1;
  d->m_pLZ_code_buf[1] = (__mz_uint8)(match_dist & 0xFF);
  d->m_pLZ_code_buf[2] = (__mz_uint8)(match_dist >> 8); d->m_pLZ_code_buf += 3;

  *d->m_pLZ_flags = (__mz_uint8)((*d->m_pLZ_flags >> 1) | 0x80); if (--d->m_num_flags_left == 0) { d->m_num_flags_left = 8; d->m_pLZ_flags = d->m_pLZ_code_buf++; }

  s0 = s_tdefl_small_dist_sym[match_dist & 511]; s1 = s_tdefl_large_dist_sym[(match_dist >> 8) & 127];
  d->m_huff_count[1][(match_dist < 512) ? s0 : s1]++;

  if (match_len >= __TDEFL_MIN_MATCH_LEN) d->m_huff_count[0][s_tdefl_len_sym[match_len - __TDEFL_MIN_MATCH_LEN]]++;
}

static __mz_bool __tdefl_compress_normal(__tdefl_compressor *d)
{
  const __mz_uint8 *pSrc = d->m_pSrc; size_t src_buf_left = d->m_src_buf_left;
  __tdefl_flush flush = d->m_flush;

  while ((src_buf_left) || ((flush) && (d->m_lookahead_size)))
  {
    __mz_uint len_to_move, cur_match_dist, cur_match_len, cur_pos;
    // Update dictionary and hash chains. Keeps the lookahead size equal to TDEFL_MAX_MATCH_LEN.
    if ((d->m_lookahead_size + d->m_dict_size) >= (__TDEFL_MIN_MATCH_LEN - 1))
    {
      __mz_uint dst_pos = (d->m_lookahead_pos + d->m_lookahead_size) & __TDEFL_LZ_DICT_SIZE_MASK, ins_pos = d->m_lookahead_pos + d->m_lookahead_size - 2;
      __mz_uint hash = (d->m_dict[ins_pos & __TDEFL_LZ_DICT_SIZE_MASK] << __TDEFL_LZ_HASH_SHIFT) ^ d->m_dict[(ins_pos + 1) & __TDEFL_LZ_DICT_SIZE_MASK];
      __mz_uint num_bytes_to_process = (__mz_uint)__MZ_MIN(src_buf_left, __TDEFL_MAX_MATCH_LEN - d->m_lookahead_size);
      const __mz_uint8 *pSrc_end = pSrc + num_bytes_to_process;
      src_buf_left -= num_bytes_to_process;
      d->m_lookahead_size += num_bytes_to_process;
      while (pSrc != pSrc_end)
      {
        __mz_uint8 c = *pSrc++; d->m_dict[dst_pos] = c; if (dst_pos < (__TDEFL_MAX_MATCH_LEN - 1)) d->m_dict[__TDEFL_LZ_DICT_SIZE + dst_pos] = c;
        hash = ((hash << __TDEFL_LZ_HASH_SHIFT) ^ c) & (__TDEFL_LZ_HASH_SIZE - 1);
        d->m_next[ins_pos & __TDEFL_LZ_DICT_SIZE_MASK] = d->m_hash[hash]; d->m_hash[hash] = (__mz_uint16)(ins_pos);
        dst_pos = (dst_pos + 1) & __TDEFL_LZ_DICT_SIZE_MASK; ins_pos++;
      }
    }
    else
    {
      while ((src_buf_left) && (d->m_lookahead_size < __TDEFL_MAX_MATCH_LEN))
      {
        __mz_uint8 c = *pSrc++;
        __mz_uint dst_pos = (d->m_lookahead_pos + d->m_lookahead_size) & __TDEFL_LZ_DICT_SIZE_MASK;
        src_buf_left--;
        d->m_dict[dst_pos] = c;
        if (dst_pos < (__TDEFL_MAX_MATCH_LEN - 1))
          d->m_dict[__TDEFL_LZ_DICT_SIZE + dst_pos] = c;
        if ((++d->m_lookahead_size + d->m_dict_size) >= __TDEFL_MIN_MATCH_LEN)
        {
          __mz_uint ins_pos = d->m_lookahead_pos + (d->m_lookahead_size - 1) - 2;
          __mz_uint hash = ((d->m_dict[ins_pos & __TDEFL_LZ_DICT_SIZE_MASK] << (__TDEFL_LZ_HASH_SHIFT * 2)) ^ (d->m_dict[(ins_pos + 1) & __TDEFL_LZ_DICT_SIZE_MASK] << __TDEFL_LZ_HASH_SHIFT) ^ c) & (__TDEFL_LZ_HASH_SIZE - 1);
          d->m_next[ins_pos & __TDEFL_LZ_DICT_SIZE_MASK] = d->m_hash[hash]; d->m_hash[hash] = (__mz_uint16)(ins_pos);
        }
      }
    }
    d->m_dict_size = __MZ_MIN(__TDEFL_LZ_DICT_SIZE - d->m_lookahead_size, d->m_dict_size);
    if ((!flush) && (d->m_lookahead_size < __TDEFL_MAX_MATCH_LEN))
      break;

    // Simple lazy/greedy parsing state machine.
    len_to_move = 1; cur_match_dist = 0; cur_match_len = d->m_saved_match_len ? d->m_saved_match_len : (__TDEFL_MIN_MATCH_LEN - 1); cur_pos = d->m_lookahead_pos & __TDEFL_LZ_DICT_SIZE_MASK;
    if (d->m_flags & (__TDEFL_RLE_MATCHES | __TDEFL_FORCE_ALL_RAW_BLOCKS))
    {
      if ((d->m_dict_size) && (!(d->m_flags & __TDEFL_FORCE_ALL_RAW_BLOCKS)))
      {
        __mz_uint8 c = d->m_dict[(cur_pos - 1) & __TDEFL_LZ_DICT_SIZE_MASK];
        cur_match_len = 0; while (cur_match_len < d->m_lookahead_size) { if (d->m_dict[cur_pos + cur_match_len] != c) break; cur_match_len++; }
        if (cur_match_len < __TDEFL_MIN_MATCH_LEN) cur_match_len = 0; else cur_match_dist = 1;
      }
    }
    else
    {
      __tdefl_find_match(d, d->m_lookahead_pos, d->m_dict_size, d->m_lookahead_size, &cur_match_dist, &cur_match_len);
    }
    if (((cur_match_len == __TDEFL_MIN_MATCH_LEN) && (cur_match_dist >= 8U*1024U)) || (cur_pos == cur_match_dist) || ((d->m_flags & __TDEFL_FILTER_MATCHES) && (cur_match_len <= 5)))
    {
      cur_match_dist = cur_match_len = 0;
    }
    if (d->m_saved_match_len)
    {
      if (cur_match_len > d->m_saved_match_len)
      {
        __tdefl_record_literal(d, (__mz_uint8)d->m_saved_lit);
        if (cur_match_len >= 128)
        {
          __tdefl_record_match(d, cur_match_len, cur_match_dist);
          d->m_saved_match_len = 0; len_to_move = cur_match_len;
        }
        else
        {
          d->m_saved_lit = d->m_dict[cur_pos]; d->m_saved_match_dist = cur_match_dist; d->m_saved_match_len = cur_match_len;
        }
      }
      else
      {
        __tdefl_record_match(d, d->m_saved_match_len, d->m_saved_match_dist);
        len_to_move = d->m_saved_match_len - 1; d->m_saved_match_len = 0;
      }
    }
    else if (!cur_match_dist)
      __tdefl_record_literal(d, d->m_dict[__MZ_MIN(cur_pos, sizeof(d->m_dict) - 1)]);
    else if ((d->m_greedy_parsing) || (d->m_flags & __TDEFL_RLE_MATCHES) || (cur_match_len >= 128))
    {
      __tdefl_record_match(d, cur_match_len, cur_match_dist);
      len_to_move = cur_match_len;
    }
    else
    {
      d->m_saved_lit = d->m_dict[__MZ_MIN(cur_pos, sizeof(d->m_dict) - 1)]; d->m_saved_match_dist = cur_match_dist; d->m_saved_match_len = cur_match_len;
    }
    // Move the lookahead forward by len_to_move bytes.
    d->m_lookahead_pos += len_to_move;
    __MZ_ASSERT(d->m_lookahead_size >= len_to_move);
    d->m_lookahead_size -= len_to_move;
    d->m_dict_size = __MZ_MIN(d->m_dict_size + len_to_move, __TDEFL_LZ_DICT_SIZE);
    // Check if it's time to flush the current LZ codes to the internal output buffer.
    if ( (d->m_pLZ_code_buf > &d->m_lz_code_buf[__TDEFL_LZ_CODE_BUF_SIZE - 8]) ||
         ( (d->m_total_lz_bytes > 31*1024) && (((((__mz_uint)(d->m_pLZ_code_buf - d->m_lz_code_buf) * 115) >> 7) >= d->m_total_lz_bytes) || (d->m_flags & __TDEFL_FORCE_ALL_RAW_BLOCKS))) )
    {
      int n;
      d->m_pSrc = pSrc; d->m_src_buf_left = src_buf_left;
      if ((n = __tdefl_flush_block(d, 0)) != 0)
        return (n < 0) ? __MZ_FALSE : __MZ_TRUE;
    }
  }

  d->m_pSrc = pSrc; d->m_src_buf_left = src_buf_left;
  return __MZ_TRUE;
}

static __tdefl_status __tdefl_flush_output_buffer(__tdefl_compressor *d)
{
  if (d->m_pIn_buf_size)
  {
    *d->m_pIn_buf_size = d->m_pSrc - (const __mz_uint8 *)d->m_pIn_buf;
  }

  if (d->m_pOut_buf_size)
  {
    size_t n = __MZ_MIN(*d->m_pOut_buf_size - d->m_out_buf_ofs, d->m_output_flush_remaining);
    memcpy((__mz_uint8 *)d->m_pOut_buf + d->m_out_buf_ofs, d->m_output_buf + d->m_output_flush_ofs, n);
    d->m_output_flush_ofs += (__mz_uint)n;
    d->m_output_flush_remaining -= (__mz_uint)n;
    d->m_out_buf_ofs += n;

    *d->m_pOut_buf_size = d->m_out_buf_ofs;
  }

  return (d->m_finished && !d->m_output_flush_remaining) ? __TDEFL_STATUS_DONE : __TDEFL_STATUS_OKAY;
}

__tdefl_status __tdefl_compress(__tdefl_compressor *d, const void *pIn_buf, size_t *pIn_buf_size, void *pOut_buf, size_t *pOut_buf_size, __tdefl_flush flush)
{
  if (!d)
  {
    if (pIn_buf_size) *pIn_buf_size = 0;
    if (pOut_buf_size) *pOut_buf_size = 0;
    return __TDEFL_STATUS_BAD_PARAM;
  }

  d->m_pIn_buf = pIn_buf; d->m_pIn_buf_size = pIn_buf_size;
  d->m_pOut_buf = pOut_buf; d->m_pOut_buf_size = pOut_buf_size;
  d->m_pSrc = (const __mz_uint8 *)(pIn_buf); d->m_src_buf_left = pIn_buf_size ? *pIn_buf_size : 0;
  d->m_out_buf_ofs = 0;
  d->m_flush = flush;

  if ( ((d->m_pPut_buf_func != NULL) == ((pOut_buf != NULL) || (pOut_buf_size != NULL))) || (d->m_prev_return_status != __TDEFL_STATUS_OKAY) ||
        (d->m_wants_to_finish && (flush != __TDEFL_FINISH)) || (pIn_buf_size && *pIn_buf_size && !pIn_buf) || (pOut_buf_size && *pOut_buf_size && !pOut_buf) )
  {
    if (pIn_buf_size) *pIn_buf_size = 0;
    if (pOut_buf_size) *pOut_buf_size = 0;
    return (d->m_prev_return_status = __TDEFL_STATUS_BAD_PARAM);
  }
  d->m_wants_to_finish |= (flush == __TDEFL_FINISH);

  if ((d->m_output_flush_remaining) || (d->m_finished))
    return (d->m_prev_return_status = __tdefl_flush_output_buffer(d));

#if __MINIZ_USE_UNALIGNED_LOADS_AND_STORES && __MINIZ_LITTLE_ENDIAN
  if (((d->m_flags & __TDEFL_MAX_PROBES_MASK) == 1) &&
      ((d->m_flags & __TDEFL_GREEDY_PARSING_FLAG) != 0) &&
      ((d->m_flags & (__TDEFL_FILTER_MATCHES | __TDEFL_FORCE_ALL_RAW_BLOCKS | __TDEFL_RLE_MATCHES)) == 0))
  {
    if (!__tdefl_compress_fast(d))
      return d->m_prev_return_status;
  }
  else
#endif // #if __MINIZ_USE_UNALIGNED_LOADS_AND_STORES && __MINIZ_LITTLE_ENDIAN
  {
    if (!__tdefl_compress_normal(d))
      return d->m_prev_return_status;
  }

  if ((d->m_flags & (__TDEFL_WRITE_ZLIB_HEADER | __TDEFL_COMPUTE_ADLER32)) && (pIn_buf))
    d->m_adler32 = (__mz_uint32)__mz_adler32(d->m_adler32, (const __mz_uint8 *)pIn_buf, d->m_pSrc - (const __mz_uint8 *)pIn_buf);

  if ((flush) && (!d->m_lookahead_size) && (!d->m_src_buf_left) && (!d->m_output_flush_remaining))
  {
    if (__tdefl_flush_block(d, flush) < 0)
      return d->m_prev_return_status;
    d->m_finished = (flush == __TDEFL_FINISH);
    if (flush == __TDEFL_FULL_FLUSH) { __MZ_CLEAR_OBJ(d->m_hash); __MZ_CLEAR_OBJ(d->m_next); d->m_dict_size = 0; }
  }

  return (d->m_prev_return_status = __tdefl_flush_output_buffer(d));
}

__tdefl_status __tdefl_compress_buffer(__tdefl_compressor *d, const void *pIn_buf, size_t in_buf_size, __tdefl_flush flush)
{
  __MZ_ASSERT(d->m_pPut_buf_func); return __tdefl_compress(d, pIn_buf, &in_buf_size, NULL, NULL, flush);
}

__tdefl_status __tdefl_init(__tdefl_compressor *d, __tdefl_put_buf_func_ptr pPut_buf_func, void *pPut_buf_user, int flags)
{
  d->m_pPut_buf_func = pPut_buf_func; d->m_pPut_buf_user = pPut_buf_user;
  d->m_flags = (__mz_uint)(flags); d->m_max_probes[0] = 1 + ((flags & 0xFFF) + 2) / 3; d->m_greedy_parsing = (flags & __TDEFL_GREEDY_PARSING_FLAG) != 0;
  d->m_max_probes[1] = 1 + (((flags & 0xFFF) >> 2) + 2) / 3;
  if (!(flags & __TDEFL_NONDETERMINISTIC_PARSING_FLAG)) __MZ_CLEAR_OBJ(d->m_hash);
  d->m_lookahead_pos = d->m_lookahead_size = d->m_dict_size = d->m_total_lz_bytes = d->m_lz_code_buf_dict_pos = d->m_bits_in = 0;
  d->m_output_flush_ofs = d->m_output_flush_remaining = d->m_finished = d->m_block_index = d->m_bit_buffer = d->m_wants_to_finish = 0;
  d->m_pLZ_code_buf = d->m_lz_code_buf + 1; d->m_pLZ_flags = d->m_lz_code_buf; d->m_num_flags_left = 8;
  d->m_pOutput_buf = d->m_output_buf; d->m_pOutput_buf_end = d->m_output_buf; d->m_prev_return_status = __TDEFL_STATUS_OKAY;
  d->m_saved_match_dist = d->m_saved_match_len = d->m_saved_lit = 0; d->m_adler32 = 1;
  d->m_pIn_buf = NULL; d->m_pOut_buf = NULL;
  d->m_pIn_buf_size = NULL; d->m_pOut_buf_size = NULL;
  d->m_flush = __TDEFL_NO_FLUSH; d->m_pSrc = NULL; d->m_src_buf_left = 0; d->m_out_buf_ofs = 0;
  memset(&d->m_huff_count[0][0], 0, sizeof(d->m_huff_count[0][0]) * __TDEFL_MAX_HUFF_SYMBOLS_0);
  memset(&d->m_huff_count[1][0], 0, sizeof(d->m_huff_count[1][0]) * __TDEFL_MAX_HUFF_SYMBOLS_1);
  return __TDEFL_STATUS_OKAY;
}

__tdefl_status __tdefl_get_prev_return_status(__tdefl_compressor *d)
{
  return d->m_prev_return_status;
}

__mz_uint32 __tdefl_get_adler32(__tdefl_compressor *d)
{
  return d->m_adler32;
}

__mz_bool __tdefl_compress_mem_to_output(const void *pBuf, size_t buf_len, __tdefl_put_buf_func_ptr pPut_buf_func, void *pPut_buf_user, int flags)
{
  __tdefl_compressor *pComp; __mz_bool succeeded; if (((buf_len) && (!pBuf)) || (!pPut_buf_func)) return __MZ_FALSE;
  pComp = (__tdefl_compressor*)__MZ_MALLOC(sizeof(__tdefl_compressor)); if (!pComp) return __MZ_FALSE;
  succeeded = (__tdefl_init(pComp, pPut_buf_func, pPut_buf_user, flags) == __TDEFL_STATUS_OKAY);
  succeeded = succeeded && (__tdefl_compress_buffer(pComp, pBuf, buf_len, __TDEFL_FINISH) == __TDEFL_STATUS_DONE);
  __MZ_FREE(pComp); return succeeded;
}

typedef struct
{
  size_t m_size, m_capacity;
  __mz_uint8 *m_pBuf;
  __mz_bool m_expandable;
} __tdefl_output_buffer;

static __mz_bool __tdefl_output_buffer_putter(const void *pBuf, int len, void *pUser)
{
  __tdefl_output_buffer *p = (__tdefl_output_buffer *)pUser;
  size_t new_size = p->m_size + len;
  if (new_size > p->m_capacity)
  {
    size_t new_capacity = p->m_capacity; __mz_uint8 *pNew_buf; if (!p->m_expandable) return __MZ_FALSE;
    do { new_capacity = __MZ_MAX(128U, new_capacity << 1U); } while (new_size > new_capacity);
    pNew_buf = (__mz_uint8*)__MZ_REALLOC(p->m_pBuf, new_capacity); if (!pNew_buf) return __MZ_FALSE;
    p->m_pBuf = pNew_buf; p->m_capacity = new_capacity;
  }
  memcpy((__mz_uint8*)p->m_pBuf + p->m_size, pBuf, len); p->m_size = new_size;
  return __MZ_TRUE;
}

void *__tdefl_compress_mem_to_heap(const void *pSrc_buf, size_t src_buf_len, size_t *pOut_len, int flags)
{
  __tdefl_output_buffer out_buf; __MZ_CLEAR_OBJ(out_buf);
  if (!pOut_len) return __MZ_FALSE; else *pOut_len = 0;
  out_buf.m_expandable = __MZ_TRUE;
  if (!__tdefl_compress_mem_to_output(pSrc_buf, src_buf_len, __tdefl_output_buffer_putter, &out_buf, flags)) return NULL;
  *pOut_len = out_buf.m_size; return out_buf.m_pBuf;
}

size_t __tdefl_compress_mem_to_mem(void *pOut_buf, size_t out_buf_len, const void *pSrc_buf, size_t src_buf_len, int flags)
{
  __tdefl_output_buffer out_buf; __MZ_CLEAR_OBJ(out_buf);
  if (!pOut_buf) return 0;
  out_buf.m_pBuf = (__mz_uint8*)pOut_buf; out_buf.m_capacity = out_buf_len;
  if (!__tdefl_compress_mem_to_output(pSrc_buf, src_buf_len, __tdefl_output_buffer_putter, &out_buf, flags)) return 0;
  return out_buf.m_size;
}

#ifndef __MINIZ_NO_ZLIB_APIS
static const __mz_uint s_tdefl_num_probes[11] = { 0, 1, 6, 32,  16, 32, 128, 256,  512, 768, 1500 };

// level may actually range from [0,10] (10 is a "hidden" max level, where we want a bit more compression and it's fine if throughput to fall off a cliff on some files).
__mz_uint __tdefl_create_comp_flags_from_zip_params(int level, int window_bits, int strategy)
{
  __mz_uint comp_flags = s_tdefl_num_probes[(level >= 0) ? __MZ_MIN(10, level) : __MZ_DEFAULT_LEVEL] | ((level <= 3) ? __TDEFL_GREEDY_PARSING_FLAG : 0);
  if (window_bits > 0) comp_flags |= __TDEFL_WRITE_ZLIB_HEADER;

  if (!level) comp_flags |= __TDEFL_FORCE_ALL_RAW_BLOCKS;
  else if (strategy == __MZ_FILTERED) comp_flags |= __TDEFL_FILTER_MATCHES;
  else if (strategy == __MZ_HUFFMAN_ONLY) comp_flags &= ~__TDEFL_MAX_PROBES_MASK;
  else if (strategy == __MZ_FIXED) comp_flags |= __TDEFL_FORCE_ALL_STATIC_BLOCKS;
  else if (strategy == __MZ_RLE) comp_flags |= __TDEFL_RLE_MATCHES;

  return comp_flags;
}
#endif //__MINIZ_NO_ZLIB_APIS

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable:4204) // nonstandard extension used : non-constant aggregate initializer (also supported by GNU C and C99, so no big deal)
#endif

// Simple PNG writer function by Alex Evans, 2011. Released into the public domain: https://gist.github.com/908299, more context at
// http://altdevblogaday.org/2011/04/06/a-smaller-jpg-encoder/.
// This is actually a modification of Alex's original code so PNG files generated by this function pass pngcheck.
void *__tdefl_write_image_to_png_file_in_memory_ex(const void *pImage, int w, int h, int num_chans, size_t *pLen_out, __mz_uint level, __mz_bool flip)
{
  // Using a local copy of this array here in case __MINIZ_NO_ZLIB_APIS was defined.
  static const __mz_uint s_tdefl_png_num_probes[11] = { 0, 1, 6, 32,  16, 32, 128, 256,  512, 768, 1500 };
  __tdefl_compressor *pComp = (__tdefl_compressor *)__MZ_MALLOC(sizeof(__tdefl_compressor)); __tdefl_output_buffer out_buf; int i, bpl = w * num_chans, y, z; __mz_uint32 c; *pLen_out = 0;
  if (!pComp) return NULL;
  __MZ_CLEAR_OBJ(out_buf); out_buf.m_expandable = __MZ_TRUE; out_buf.m_capacity = 57+__MZ_MAX(64, (1+bpl)*h); if (NULL == (out_buf.m_pBuf = (__mz_uint8*)__MZ_MALLOC(out_buf.m_capacity))) { __MZ_FREE(pComp); return NULL; }
  // write dummy header
  for (z = 41; z; --z) __tdefl_output_buffer_putter(&z, 1, &out_buf);
  // compress image data
  __tdefl_init(pComp, __tdefl_output_buffer_putter, &out_buf, s_tdefl_png_num_probes[__MZ_MIN(10, level)] | __TDEFL_WRITE_ZLIB_HEADER);
  for (y = 0; y < h; ++y) { __tdefl_compress_buffer(pComp, &z, 1, __TDEFL_NO_FLUSH); __tdefl_compress_buffer(pComp, (__mz_uint8*)pImage + (flip ? (h - 1 - y) : y) * bpl, bpl, __TDEFL_NO_FLUSH); }
  if (__tdefl_compress_buffer(pComp, NULL, 0, __TDEFL_FINISH) != __TDEFL_STATUS_DONE) { __MZ_FREE(pComp); __MZ_FREE(out_buf.m_pBuf); return NULL; }
  // write real header
  *pLen_out = out_buf.m_size-41;
  {
    static const __mz_uint8 chans[] = {0x00, 0x00, 0x04, 0x02, 0x06};
    __mz_uint8 pnghdr[41]={0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
      0,0,(__mz_uint8)(w>>8),(__mz_uint8)w,0,0,(__mz_uint8)(h>>8),(__mz_uint8)h,8,chans[num_chans],0,0,0,0,0,0,0,
      (__mz_uint8)(*pLen_out>>24),(__mz_uint8)(*pLen_out>>16),(__mz_uint8)(*pLen_out>>8),(__mz_uint8)*pLen_out,0x49,0x44,0x41,0x54};
    c=(__mz_uint32)__mz_crc32(__MZ_CRC32_INIT,pnghdr+12,17); for (i=0; i<4; ++i, c<<=8) ((__mz_uint8*)(pnghdr+29))[i]=(__mz_uint8)(c>>24);
    memcpy(out_buf.m_pBuf, pnghdr, 41);
  }
  // write footer (IDAT CRC-32, followed by IEND chunk)
  if (!__tdefl_output_buffer_putter("\0\0\0\0\0\0\0\0\x49\x45\x4e\x44\xae\x42\x60\x82", 16, &out_buf)) { *pLen_out = 0; __MZ_FREE(pComp); __MZ_FREE(out_buf.m_pBuf); return NULL; }
  c = (__mz_uint32)__mz_crc32(__MZ_CRC32_INIT,out_buf.m_pBuf+41-4, *pLen_out+4); for (i=0; i<4; ++i, c<<=8) (out_buf.m_pBuf+out_buf.m_size-16)[i] = (__mz_uint8)(c >> 24);
  // compute final size of file, grab compressed data buffer and return
  *pLen_out += 57; __MZ_FREE(pComp); return out_buf.m_pBuf;
}
void *__tdefl_write_image_to_png_file_in_memory(const void *pImage, int w, int h, int num_chans, size_t *pLen_out)
{
  // Level 6 corresponds to TDEFL_DEFAULT_MAX_PROBES or MZ_DEFAULT_LEVEL (but we can't depend on MZ_DEFAULT_LEVEL being available in case the zlib API's where #defined out)
  return __tdefl_write_image_to_png_file_in_memory_ex(pImage, w, h, num_chans, pLen_out, 6, __MZ_FALSE);
}

#ifdef _MSC_VER
#pragma warning (pop)
#endif

// ------------------- .ZIP archive reading

#ifndef __MINIZ_NO_ARCHIVE_APIS

#ifdef __MINIZ_NO_STDIO
  #define __MZ_FILE void *
#else
  #include <stdio.h>
  #include <sys/stat.h>

  #if defined(_MSC_VER) || defined(__MINGW64__)
    static FILE *__mz_fopen(const char *pFilename, const char *pMode)
    {
      FILE* pFile = NULL;
      fopen_s(&pFile, pFilename, pMode);
      return pFile;
    }
    static FILE *__mz_freopen(const char *pPath, const char *pMode, FILE *pStream)
    {
      FILE* pFile = NULL;
      if (freopen_s(&pFile, pPath, pMode, pStream))
        return NULL;
      return pFile;
    }
    #ifndef __MINIZ_NO_TIME
      #include <sys/utime.h>
    #endif
    #define __MZ_FILE FILE
    #define __MZ_FOPEN __mz_fopen
    #define __MZ_FCLOSE fclose
    #define __MZ_FREAD fread
    #define __MZ_FWRITE fwrite
    #define __MZ_FTELL64 _ftelli64
    #define __MZ_FSEEK64 _fseeki64
    #define __MZ_FILE_STAT_STRUCT _stat
    #define __MZ_FILE_STAT _stat
    #define __MZ_FFLUSH fflush
    #define __MZ_FREOPEN __mz_freopen
    #define __MZ_DELETE_FILE remove
  #elif defined(__MINGW32__)
    #ifndef __MINIZ_NO_TIME
      #include <sys/utime.h>
    #endif
    #define __MZ_FILE FILE
    #define __MZ_FOPEN(f, m) fopen(f, m)
    #define __MZ_FCLOSE fclose
    #define __MZ_FREAD fread
    #define __MZ_FWRITE fwrite
    #define __MZ_FTELL64 ftello64
    #define __MZ_FSEEK64 fseeko64
    #define __MZ_FILE_STAT_STRUCT _stat
    #define __MZ_FILE_STAT _stat
    #define __MZ_FFLUSH fflush
    #define __MZ_FREOPEN(f, m, s) freopen(f, m, s)
    #define __MZ_DELETE_FILE remove
  #elif defined(__TINYC__)
    #ifndef __MINIZ_NO_TIME
      #include <sys/utime.h>
    #endif
    #define __MZ_FILE FILE
    #define __MZ_FOPEN(f, m) fopen(f, m)
    #define __MZ_FCLOSE fclose
    #define __MZ_FREAD fread
    #define __MZ_FWRITE fwrite
    #define __MZ_FTELL64 ftell
    #define __MZ_FSEEK64 fseek
    #define __MZ_FILE_STAT_STRUCT stat
    #define __MZ_FILE_STAT stat
    #define __MZ_FFLUSH fflush
    #define __MZ_FREOPEN(f, m, s) freopen(f, m, s)
    #define __MZ_DELETE_FILE remove
  #elif defined(__GNUC__) && defined(_LARGEFILE64_SOURCE)
    #ifndef __MINIZ_NO_TIME
      #include <utime.h>
    #endif
    #define __MZ_FILE FILE
    #define __MZ_FOPEN(f, m) fopen64(f, m)
    #define __MZ_FCLOSE fclose
    #define __MZ_FREAD fread
    #define __MZ_FWRITE fwrite
    #define __MZ_FTELL64 ftello64
    #define __MZ_FSEEK64 fseeko64
    #define __MZ_FILE_STAT_STRUCT stat64
    #define __MZ_FILE_STAT stat64
    #define __MZ_FFLUSH fflush
    #define __MZ_FREOPEN(p, m, s) freopen64(p, m, s)
    #define __MZ_DELETE_FILE remove
  #else
    #ifndef __MINIZ_NO_TIME
      #include <utime.h>
    #endif
    #define __MZ_FILE FILE
    #define __MZ_FOPEN(f, m) fopen(f, m)
    #define __MZ_FCLOSE fclose
    #define __MZ_FREAD fread
    #define __MZ_FWRITE fwrite
    #define __MZ_FTELL64 ftello
    #define __MZ_FSEEK64 fseeko
    #define __MZ_FILE_STAT_STRUCT stat
    #define __MZ_FILE_STAT stat
    #define __MZ_FFLUSH fflush
    #define __MZ_FREOPEN(f, m, s) freopen(f, m, s)
    #define __MZ_DELETE_FILE remove
  #endif // #ifdef _MSC_VER
#endif // #ifdef __MINIZ_NO_STDIO

#define __MZ_TOLOWER(c) ((((c) >= 'A') && ((c) <= 'Z')) ? ((c) - 'A' + 'a') : (c))

// Various ZIP archive enums. To completely avoid cross platform compiler alignment and platform endian issues, miniz.c doesn't use structs for any of this stuff.
enum
{
  // ZIP archive identifiers and record sizes
  __MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIG = 0x06054b50, __MZ_ZIP_CENTRAL_DIR_HEADER_SIG = 0x02014b50, __MZ_ZIP_LOCAL_DIR_HEADER_SIG = 0x04034b50,
  __MZ_ZIP_LOCAL_DIR_HEADER_SIZE = 30, __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE = 46, __MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE = 22,
  // Central directory header record offsets
  __MZ_ZIP_CDH_SIG_OFS = 0, __MZ_ZIP_CDH_VERSION_MADE_BY_OFS = 4, __MZ_ZIP_CDH_VERSION_NEEDED_OFS = 6, __MZ_ZIP_CDH_BIT_FLAG_OFS = 8,
  __MZ_ZIP_CDH_METHOD_OFS = 10, __MZ_ZIP_CDH_FILE_TIME_OFS = 12, __MZ_ZIP_CDH_FILE_DATE_OFS = 14, __MZ_ZIP_CDH_CRC32_OFS = 16,
  __MZ_ZIP_CDH_COMPRESSED_SIZE_OFS = 20, __MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS = 24, __MZ_ZIP_CDH_FILENAME_LEN_OFS = 28, __MZ_ZIP_CDH_EXTRA_LEN_OFS = 30,
  __MZ_ZIP_CDH_COMMENT_LEN_OFS = 32, __MZ_ZIP_CDH_DISK_START_OFS = 34, __MZ_ZIP_CDH_INTERNAL_ATTR_OFS = 36, __MZ_ZIP_CDH_EXTERNAL_ATTR_OFS = 38, __MZ_ZIP_CDH_LOCAL_HEADER_OFS = 42,
  // Local directory header offsets
  __MZ_ZIP_LDH_SIG_OFS = 0, __MZ_ZIP_LDH_VERSION_NEEDED_OFS = 4, __MZ_ZIP_LDH_BIT_FLAG_OFS = 6, __MZ_ZIP_LDH_METHOD_OFS = 8, __MZ_ZIP_LDH_FILE_TIME_OFS = 10,
  __MZ_ZIP_LDH_FILE_DATE_OFS = 12, __MZ_ZIP_LDH_CRC32_OFS = 14, __MZ_ZIP_LDH_COMPRESSED_SIZE_OFS = 18, __MZ_ZIP_LDH_DECOMPRESSED_SIZE_OFS = 22,
  __MZ_ZIP_LDH_FILENAME_LEN_OFS = 26, __MZ_ZIP_LDH_EXTRA_LEN_OFS = 28,
  // End of central directory offsets
  __MZ_ZIP_ECDH_SIG_OFS = 0, __MZ_ZIP_ECDH_NUM_THIS_DISK_OFS = 4, __MZ_ZIP_ECDH_NUM_DISK_CDIR_OFS = 6, __MZ_ZIP_ECDH_CDIR_NUM_ENTRIES_ON_DISK_OFS = 8,
  __MZ_ZIP_ECDH_CDIR_TOTAL_ENTRIES_OFS = 10, __MZ_ZIP_ECDH_CDIR_SIZE_OFS = 12, __MZ_ZIP_ECDH_CDIR_OFS_OFS = 16, __MZ_ZIP_ECDH_COMMENT_SIZE_OFS = 20,
};

typedef struct
{
  void *m_p;
  size_t m_size, m_capacity;
  __mz_uint m_element_size;
} __mz_zip_array;

struct __mz_zip_internal_state_tag
{
  __mz_zip_array m_central_dir;
  __mz_zip_array m_central_dir_offsets;
  __mz_zip_array m_sorted_central_dir_offsets;
  __MZ_FILE *m_pFile;
  void *m_pMem;
  size_t m_mem_size;
  size_t m_mem_capacity;
};

#define __MZ_ZIP_ARRAY_SET_ELEMENT_SIZE(array_ptr, element_size) (array_ptr)->m_element_size = element_size
#define __MZ_ZIP_ARRAY_ELEMENT(array_ptr, element_type, index) ((element_type *)((array_ptr)->m_p))[index]

static __MZ_FORCEINLINE void __mz_zip_array_clear(__mz_zip_archive *pZip, __mz_zip_array *pArray)
{
  pZip->m_pFree(pZip->m_pAlloc_opaque, pArray->m_p);
  memset(pArray, 0, sizeof(__mz_zip_array));
}

static __mz_bool __mz_zip_array_ensure_capacity(__mz_zip_archive *pZip, __mz_zip_array *pArray, size_t min_new_capacity, __mz_uint growing)
{
  void *pNew_p; size_t new_capacity = min_new_capacity; __MZ_ASSERT(pArray->m_element_size); if (pArray->m_capacity >= min_new_capacity) return __MZ_TRUE;
  if (growing) { new_capacity = __MZ_MAX(1, pArray->m_capacity); while (new_capacity < min_new_capacity) new_capacity *= 2; }
  if (NULL == (pNew_p = pZip->m_pRealloc(pZip->m_pAlloc_opaque, pArray->m_p, pArray->m_element_size, new_capacity))) return __MZ_FALSE;
  pArray->m_p = pNew_p; pArray->m_capacity = new_capacity;
  return __MZ_TRUE;
}

static __MZ_FORCEINLINE __mz_bool __mz_zip_array_reserve(__mz_zip_archive *pZip, __mz_zip_array *pArray, size_t new_capacity, __mz_uint growing)
{
  if (new_capacity > pArray->m_capacity) { if (!__mz_zip_array_ensure_capacity(pZip, pArray, new_capacity, growing)) return __MZ_FALSE; }
  return __MZ_TRUE;
}

static __MZ_FORCEINLINE __mz_bool __mz_zip_array_resize(__mz_zip_archive *pZip, __mz_zip_array *pArray, size_t new_size, __mz_uint growing)
{
  if (new_size > pArray->m_capacity) { if (!__mz_zip_array_ensure_capacity(pZip, pArray, new_size, growing)) return __MZ_FALSE; }
  pArray->m_size = new_size;
  return __MZ_TRUE;
}

static __MZ_FORCEINLINE __mz_bool __mz_zip_array_ensure_room(__mz_zip_archive *pZip, __mz_zip_array *pArray, size_t n)
{
  return __mz_zip_array_reserve(pZip, pArray, pArray->m_size + n, __MZ_TRUE);
}

static __MZ_FORCEINLINE __mz_bool __mz_zip_array_push_back(__mz_zip_archive *pZip, __mz_zip_array *pArray, const void *pElements, size_t n)
{
  size_t orig_size = pArray->m_size; if (!__mz_zip_array_resize(pZip, pArray, orig_size + n, __MZ_TRUE)) return __MZ_FALSE;
  memcpy((__mz_uint8*)pArray->m_p + orig_size * pArray->m_element_size, pElements, n * pArray->m_element_size);
  return __MZ_TRUE;
}

#ifndef __MINIZ_NO_TIME
static time_t __mz_zip_dos_to_time_t(int dos_time, int dos_date)
{
  struct tm tm;
  memset(&tm, 0, sizeof(tm)); tm.tm_isdst = -1;
  tm.tm_year = ((dos_date >> 9) & 127) + 1980 - 1900; tm.tm_mon = ((dos_date >> 5) & 15) - 1; tm.tm_mday = dos_date & 31;
  tm.tm_hour = (dos_time >> 11) & 31; tm.tm_min = (dos_time >> 5) & 63; tm.tm_sec = (dos_time << 1) & 62;
  return mktime(&tm);
}

static void __mz_zip_time_to_dos_time(time_t time, __mz_uint16 *pDOS_time, __mz_uint16 *pDOS_date)
{
#ifdef _MSC_VER
  struct tm tm_struct;
  struct tm *tm = &tm_struct;
  errno_t err = localtime_s(tm, &time);
  if (err)
  {
    *pDOS_date = 0; *pDOS_time = 0;
    return;
  }
#else
  struct tm *tm = localtime(&time);
#endif
  *pDOS_time = (__mz_uint16)(((tm->tm_hour) << 11) + ((tm->tm_min) << 5) + ((tm->tm_sec) >> 1));
  *pDOS_date = (__mz_uint16)(((tm->tm_year + 1900 - 1980) << 9) + ((tm->tm_mon + 1) << 5) + tm->tm_mday);
}
#endif

#ifndef __MINIZ_NO_STDIO
static __mz_bool __mz_zip_get_file_modified_time(const char *pFilename, __mz_uint16 *pDOS_time, __mz_uint16 *pDOS_date)
{
#ifdef __MINIZ_NO_TIME
  (void)pFilename; *pDOS_date = *pDOS_time = 0;
#else
  struct __MZ_FILE_STAT_STRUCT file_stat;
  // On Linux with x86 glibc, this call will fail on large files (>= 0x80000000 bytes) unless you compiled with _LARGEFILE64_SOURCE. Argh.
  if (__MZ_FILE_STAT(pFilename, &file_stat) != 0)
    return __MZ_FALSE;
  __mz_zip_time_to_dos_time(file_stat.st_mtime, pDOS_time, pDOS_date);
#endif // #ifdef __MINIZ_NO_TIME
  return __MZ_TRUE;
}

#ifndef __MINIZ_NO_TIME
static __mz_bool __mz_zip_set_file_times(const char *pFilename, time_t access_time, time_t modified_time)
{
  struct utimbuf t; t.actime = access_time; t.modtime = modified_time;
  return !utime(pFilename, &t);
}
#endif // #ifndef __MINIZ_NO_TIME
#endif // #ifndef __MINIZ_NO_STDIO

static __mz_bool __mz_zip_reader_init_internal(__mz_zip_archive *pZip, __mz_uint32 flags)
{
  (void)flags;
  if ((!pZip) || (pZip->m_pState) || (pZip->m_zip_mode != __MZ_ZIP_MODE_INVALID))
    return __MZ_FALSE;

  if (!pZip->m_pAlloc) pZip->m_pAlloc = __def_alloc_func;
  if (!pZip->m_pFree) pZip->m_pFree = __def_free_func;
  if (!pZip->m_pRealloc) pZip->m_pRealloc = __def_realloc_func;

  pZip->m_zip_mode = __MZ_ZIP_MODE_READING;
  pZip->m_archive_size = 0;
  pZip->m_central_directory_file_ofs = 0;
  pZip->m_total_files = 0;

  if (NULL == (pZip->m_pState = (__mz_zip_internal_state *)pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, sizeof(__mz_zip_internal_state))))
    return __MZ_FALSE;
  memset(pZip->m_pState, 0, sizeof(__mz_zip_internal_state));
  __MZ_ZIP_ARRAY_SET_ELEMENT_SIZE(&pZip->m_pState->m_central_dir, sizeof(__mz_uint8));
  __MZ_ZIP_ARRAY_SET_ELEMENT_SIZE(&pZip->m_pState->m_central_dir_offsets, sizeof(__mz_uint32));
  __MZ_ZIP_ARRAY_SET_ELEMENT_SIZE(&pZip->m_pState->m_sorted_central_dir_offsets, sizeof(__mz_uint32));
  return __MZ_TRUE;
}

static __MZ_FORCEINLINE __mz_bool __mz_zip_reader_filename_less(const __mz_zip_array *pCentral_dir_array, const __mz_zip_array *pCentral_dir_offsets, __mz_uint l_index, __mz_uint r_index)
{
  const __mz_uint8 *pL = &__MZ_ZIP_ARRAY_ELEMENT(pCentral_dir_array, __mz_uint8, __MZ_ZIP_ARRAY_ELEMENT(pCentral_dir_offsets, __mz_uint32, l_index)), *pE;
  const __mz_uint8 *pR = &__MZ_ZIP_ARRAY_ELEMENT(pCentral_dir_array, __mz_uint8, __MZ_ZIP_ARRAY_ELEMENT(pCentral_dir_offsets, __mz_uint32, r_index));
  __mz_uint l_len = __MZ_READ_LE16(pL + __MZ_ZIP_CDH_FILENAME_LEN_OFS), r_len = __MZ_READ_LE16(pR + __MZ_ZIP_CDH_FILENAME_LEN_OFS);
  __mz_uint8 l = 0, r = 0;
  pL += __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE; pR += __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE;
  pE = pL + __MZ_MIN(l_len, r_len);
  while (pL < pE)
  {
    if ((l = __MZ_TOLOWER(*pL)) != (r = __MZ_TOLOWER(*pR)))
      break;
    pL++; pR++;
  }
  return (pL == pE) ? (l_len < r_len) : (l < r);
}

#define __MZ_SWAP_UINT32(a, b) do { __mz_uint32 t = a; a = b; b = t; } __MZ_MACRO_END

// Heap sort of lowercased filenames, used to help accelerate plain central directory searches by mz_zip_reader_locate_file(). (Could also use qsort(), but it could allocate memory.)
static void __mz_zip_reader_sort_central_dir_offsets_by_filename(__mz_zip_archive *pZip)
{
  __mz_zip_internal_state *pState = pZip->m_pState;
  const __mz_zip_array *pCentral_dir_offsets = &pState->m_central_dir_offsets;
  const __mz_zip_array *pCentral_dir = &pState->m_central_dir;
  __mz_uint32 *pIndices = &__MZ_ZIP_ARRAY_ELEMENT(&pState->m_sorted_central_dir_offsets, __mz_uint32, 0);
  const int size = pZip->m_total_files;
  int start = (size - 2) >> 1, end;
  while (start >= 0)
  {
    int child, root = start;
    for ( ; ; )
    {
      if ((child = (root << 1) + 1) >= size)
        break;
      child += (((child + 1) < size) && (__mz_zip_reader_filename_less(pCentral_dir, pCentral_dir_offsets, pIndices[child], pIndices[child + 1])));
      if (!__mz_zip_reader_filename_less(pCentral_dir, pCentral_dir_offsets, pIndices[root], pIndices[child]))
        break;
      __MZ_SWAP_UINT32(pIndices[root], pIndices[child]); root = child;
    }
    start--;
  }

  end = size - 1;
  while (end > 0)
  {
    int child, root = 0;
    __MZ_SWAP_UINT32(pIndices[end], pIndices[0]);
    for ( ; ; )
    {
      if ((child = (root << 1) + 1) >= end)
        break;
      child += (((child + 1) < end) && __mz_zip_reader_filename_less(pCentral_dir, pCentral_dir_offsets, pIndices[child], pIndices[child + 1]));
      if (!__mz_zip_reader_filename_less(pCentral_dir, pCentral_dir_offsets, pIndices[root], pIndices[child]))
        break;
      __MZ_SWAP_UINT32(pIndices[root], pIndices[child]); root = child;
    }
    end--;
  }
}

static __mz_bool __mz_zip_reader_read_central_dir(__mz_zip_archive *pZip, __mz_uint32 flags)
{
  __mz_uint cdir_size, num_this_disk, cdir_disk_index;
  __mz_uint64 cdir_ofs;
  __mz_int64 cur_file_ofs;
  const __mz_uint8 *p;
  __mz_uint32 buf_u32[4096 / sizeof(__mz_uint32)]; __mz_uint8 *pBuf = (__mz_uint8 *)buf_u32;
  __mz_bool sort_central_dir = ((flags & __MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY) == 0);
  // Basic sanity checks - reject files which are too small, and check the first 4 bytes of the file to make sure a local header is there.
  if (pZip->m_archive_size < __MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE)
    return __MZ_FALSE;
  // Find the end of central directory record by scanning the file from the end towards the beginning.
  cur_file_ofs = __MZ_MAX((__mz_int64)pZip->m_archive_size - (__mz_int64)sizeof(buf_u32), 0);
  for ( ; ; )
  {
    int i, n = (int)__MZ_MIN(sizeof(buf_u32), pZip->m_archive_size - cur_file_ofs);
    if (pZip->m_pRead(pZip->m_pIO_opaque, cur_file_ofs, pBuf, n) != (__mz_uint)n)
      return __MZ_FALSE;
    for (i = n - 4; i >= 0; --i)
      if (__MZ_READ_LE32(pBuf + i) == __MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIG)
        break;
    if (i >= 0)
    {
      cur_file_ofs += i;
      break;
    }
    if ((!cur_file_ofs) || ((pZip->m_archive_size - cur_file_ofs) >= (0xFFFF + __MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE)))
      return __MZ_FALSE;
    cur_file_ofs = __MZ_MAX(cur_file_ofs - (sizeof(buf_u32) - 3), 0);
  }
  // Read and verify the end of central directory record.
  if (pZip->m_pRead(pZip->m_pIO_opaque, cur_file_ofs, pBuf, __MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE) != __MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE)
    return __MZ_FALSE;
  if ((__MZ_READ_LE32(pBuf + __MZ_ZIP_ECDH_SIG_OFS) != __MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIG) ||
      ((pZip->m_total_files = __MZ_READ_LE16(pBuf + __MZ_ZIP_ECDH_CDIR_TOTAL_ENTRIES_OFS)) != __MZ_READ_LE16(pBuf + __MZ_ZIP_ECDH_CDIR_NUM_ENTRIES_ON_DISK_OFS)))
    return __MZ_FALSE;

  num_this_disk = __MZ_READ_LE16(pBuf + __MZ_ZIP_ECDH_NUM_THIS_DISK_OFS);
  cdir_disk_index = __MZ_READ_LE16(pBuf + __MZ_ZIP_ECDH_NUM_DISK_CDIR_OFS);
  if (((num_this_disk | cdir_disk_index) != 0) && ((num_this_disk != 1) || (cdir_disk_index != 1)))
    return __MZ_FALSE;

  if ((cdir_size = __MZ_READ_LE32(pBuf + __MZ_ZIP_ECDH_CDIR_SIZE_OFS)) < pZip->m_total_files * __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE)
    return __MZ_FALSE;

  cdir_ofs = __MZ_READ_LE32(pBuf + __MZ_ZIP_ECDH_CDIR_OFS_OFS);
  if ((cdir_ofs + (__mz_uint64)cdir_size) > pZip->m_archive_size)
    return __MZ_FALSE;

  pZip->m_central_directory_file_ofs = cdir_ofs;

  if (pZip->m_total_files)
  {
     __mz_uint i, n;

    // Read the entire central directory into a heap block, and allocate another heap block to hold the unsorted central dir file record offsets, and another to hold the sorted indices.
    if ((!__mz_zip_array_resize(pZip, &pZip->m_pState->m_central_dir, cdir_size, __MZ_FALSE)) ||
        (!__mz_zip_array_resize(pZip, &pZip->m_pState->m_central_dir_offsets, pZip->m_total_files, __MZ_FALSE)))
      return __MZ_FALSE;

    if (sort_central_dir)
    {
      if (!__mz_zip_array_resize(pZip, &pZip->m_pState->m_sorted_central_dir_offsets, pZip->m_total_files, __MZ_FALSE))
        return __MZ_FALSE;
    }

    if (pZip->m_pRead(pZip->m_pIO_opaque, cdir_ofs, pZip->m_pState->m_central_dir.m_p, cdir_size) != cdir_size)
      return __MZ_FALSE;

    // Now create an index into the central directory file records, do some basic sanity checking on each record, and check for zip64 entries (which are not yet supported).
    p = (const __mz_uint8 *)pZip->m_pState->m_central_dir.m_p;
    for (n = cdir_size, i = 0; i < pZip->m_total_files; ++i)
    {
      __mz_uint total_header_size, comp_size, decomp_size, disk_index;
      if ((n < __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE) || (__MZ_READ_LE32(p) != __MZ_ZIP_CENTRAL_DIR_HEADER_SIG))
        return __MZ_FALSE;
      __MZ_ZIP_ARRAY_ELEMENT(&pZip->m_pState->m_central_dir_offsets, __mz_uint32, i) = (__mz_uint32)(p - (const __mz_uint8 *)pZip->m_pState->m_central_dir.m_p);
      if (sort_central_dir)
        __MZ_ZIP_ARRAY_ELEMENT(&pZip->m_pState->m_sorted_central_dir_offsets, __mz_uint32, i) = i;
      comp_size = __MZ_READ_LE32(p + __MZ_ZIP_CDH_COMPRESSED_SIZE_OFS);
      decomp_size = __MZ_READ_LE32(p + __MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS);
      if (((!__MZ_READ_LE32(p + __MZ_ZIP_CDH_METHOD_OFS)) && (decomp_size != comp_size)) || (decomp_size && !comp_size) || (decomp_size == 0xFFFFFFFF) || (comp_size == 0xFFFFFFFF))
        return __MZ_FALSE;
      disk_index = __MZ_READ_LE16(p + __MZ_ZIP_CDH_DISK_START_OFS);
      if ((disk_index != num_this_disk) && (disk_index != 1))
        return __MZ_FALSE;
      if (((__mz_uint64)__MZ_READ_LE32(p + __MZ_ZIP_CDH_LOCAL_HEADER_OFS) + __MZ_ZIP_LOCAL_DIR_HEADER_SIZE + comp_size) > pZip->m_archive_size)
        return __MZ_FALSE;
      if ((total_header_size = __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE + __MZ_READ_LE16(p + __MZ_ZIP_CDH_FILENAME_LEN_OFS) + __MZ_READ_LE16(p + __MZ_ZIP_CDH_EXTRA_LEN_OFS) + __MZ_READ_LE16(p + __MZ_ZIP_CDH_COMMENT_LEN_OFS)) > n)
        return __MZ_FALSE;
      n -= total_header_size; p += total_header_size;
    }
  }

  if (sort_central_dir)
    __mz_zip_reader_sort_central_dir_offsets_by_filename(pZip);

  return __MZ_TRUE;
}

__mz_bool __mz_zip_reader_init(__mz_zip_archive *pZip, __mz_uint64 size, __mz_uint32 flags)
{
  if ((!pZip) || (!pZip->m_pRead))
    return __MZ_FALSE;
  if (!__mz_zip_reader_init_internal(pZip, flags))
    return __MZ_FALSE;
  pZip->m_archive_size = size;
  if (!__mz_zip_reader_read_central_dir(pZip, flags))
  {
    __mz_zip_reader_end(pZip);
    return __MZ_FALSE;
  }
  return __MZ_TRUE;
}

static size_t __mz_zip_mem_read_func(void *pOpaque, __mz_uint64 file_ofs, void *pBuf, size_t n)
{
  __mz_zip_archive *pZip = (__mz_zip_archive *)pOpaque;
  size_t s = (file_ofs >= pZip->m_archive_size) ? 0 : (size_t)__MZ_MIN(pZip->m_archive_size - file_ofs, n);
  memcpy(pBuf, (const __mz_uint8 *)pZip->m_pState->m_pMem + file_ofs, s);
  return s;
}

__mz_bool __mz_zip_reader_init_mem(__mz_zip_archive *pZip, const void *pMem, size_t size, __mz_uint32 flags)
{
  if (!__mz_zip_reader_init_internal(pZip, flags))
    return __MZ_FALSE;
  pZip->m_archive_size = size;
  pZip->m_pRead = __mz_zip_mem_read_func;
  pZip->m_pIO_opaque = pZip;
#ifdef __cplusplus
  pZip->m_pState->m_pMem = const_cast<void *>(pMem);
#else
  pZip->m_pState->m_pMem = (void *)pMem;
#endif
  pZip->m_pState->m_mem_size = size;
  if (!__mz_zip_reader_read_central_dir(pZip, flags))
  {
    __mz_zip_reader_end(pZip);
    return __MZ_FALSE;
  }
  return __MZ_TRUE;
}

#ifndef __MINIZ_NO_STDIO
static size_t __mz_zip_file_read_func(void *pOpaque, __mz_uint64 file_ofs, void *pBuf, size_t n)
{
  __mz_zip_archive *pZip = (__mz_zip_archive *)pOpaque;
  __mz_int64 cur_ofs = __MZ_FTELL64(pZip->m_pState->m_pFile);
  if (((__mz_int64)file_ofs < 0) || (((cur_ofs != (__mz_int64)file_ofs)) && (__MZ_FSEEK64(pZip->m_pState->m_pFile, (__mz_int64)file_ofs, SEEK_SET))))
    return 0;
  return __MZ_FREAD(pBuf, 1, n, pZip->m_pState->m_pFile);
}

__mz_bool __mz_zip_reader_init_file(__mz_zip_archive *pZip, const char *pFilename, __mz_uint32 flags)
{
  __mz_uint64 file_size;
  __MZ_FILE *pFile = __MZ_FOPEN(pFilename, "rb");
  if (!pFile)
    return __MZ_FALSE;
  if (__MZ_FSEEK64(pFile, 0, SEEK_END))
  {
    __MZ_FCLOSE(pFile);
    return __MZ_FALSE;
  }
  file_size = __MZ_FTELL64(pFile);
  if (!__mz_zip_reader_init_internal(pZip, flags))
  {
    __MZ_FCLOSE(pFile);
    return __MZ_FALSE;
  }
  pZip->m_pRead = __mz_zip_file_read_func;
  pZip->m_pIO_opaque = pZip;
  pZip->m_pState->m_pFile = pFile;
  pZip->m_archive_size = file_size;
  if (!__mz_zip_reader_read_central_dir(pZip, flags))
  {
    __mz_zip_reader_end(pZip);
    return __MZ_FALSE;
  }
  return __MZ_TRUE;
}
#endif // #ifndef __MINIZ_NO_STDIO

__mz_uint __mz_zip_reader_get_num_files(__mz_zip_archive *pZip)
{
  return pZip ? pZip->m_total_files : 0;
}

static __MZ_FORCEINLINE const __mz_uint8 *__mz_zip_reader_get_cdh(__mz_zip_archive *pZip, __mz_uint file_index)
{
  if ((!pZip) || (!pZip->m_pState) || (file_index >= pZip->m_total_files) || (pZip->m_zip_mode != __MZ_ZIP_MODE_READING))
    return NULL;
  return &__MZ_ZIP_ARRAY_ELEMENT(&pZip->m_pState->m_central_dir, __mz_uint8, __MZ_ZIP_ARRAY_ELEMENT(&pZip->m_pState->m_central_dir_offsets, __mz_uint32, file_index));
}

__mz_bool __mz_zip_reader_is_file_encrypted(__mz_zip_archive *pZip, __mz_uint file_index)
{
  __mz_uint m_bit_flag;
  const __mz_uint8 *p = __mz_zip_reader_get_cdh(pZip, file_index);
  if (!p)
    return __MZ_FALSE;
  m_bit_flag = __MZ_READ_LE16(p + __MZ_ZIP_CDH_BIT_FLAG_OFS);
  return (m_bit_flag & 1);
}

__mz_bool __mz_zip_reader_is_file_a_directory(__mz_zip_archive *pZip, __mz_uint file_index)
{
  __mz_uint filename_len, external_attr;
  const __mz_uint8 *p = __mz_zip_reader_get_cdh(pZip, file_index);
  if (!p)
    return __MZ_FALSE;

  // First see if the filename ends with a '/' character.
  filename_len = __MZ_READ_LE16(p + __MZ_ZIP_CDH_FILENAME_LEN_OFS);
  if (filename_len)
  {
    if (*(p + __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE + filename_len - 1) == '/')
      return __MZ_TRUE;
  }

  // Bugfix: This code was also checking if the internal attribute was non-zero, which wasn't correct.
  // Most/all zip writers (hopefully) set DOS file/directory attributes in the low 16-bits, so check for the DOS directory flag and ignore the source OS ID in the created by field.
  // FIXME: Remove this check? Is it necessary - we already check the filename.
  external_attr = __MZ_READ_LE32(p + __MZ_ZIP_CDH_EXTERNAL_ATTR_OFS);
  if ((external_attr & 0x10) != 0)
    return __MZ_TRUE;

  return __MZ_FALSE;
}

__mz_bool __mz_zip_reader_file_stat(__mz_zip_archive *pZip, __mz_uint file_index, __mz_zip_archive_file_stat *pStat)
{
  __mz_uint n;
  const __mz_uint8 *p = __mz_zip_reader_get_cdh(pZip, file_index);
  if ((!p) || (!pStat))
    return __MZ_FALSE;

  // Unpack the central directory record.
  pStat->m_file_index = file_index;
  pStat->m_central_dir_ofs = __MZ_ZIP_ARRAY_ELEMENT(&pZip->m_pState->m_central_dir_offsets, __mz_uint32, file_index);
  pStat->m_version_made_by = __MZ_READ_LE16(p + __MZ_ZIP_CDH_VERSION_MADE_BY_OFS);
  pStat->m_version_needed = __MZ_READ_LE16(p + __MZ_ZIP_CDH_VERSION_NEEDED_OFS);
  pStat->m_bit_flag = __MZ_READ_LE16(p + __MZ_ZIP_CDH_BIT_FLAG_OFS);
  pStat->m_method = __MZ_READ_LE16(p + __MZ_ZIP_CDH_METHOD_OFS);
#ifndef __MINIZ_NO_TIME
  pStat->m_time = __mz_zip_dos_to_time_t(__MZ_READ_LE16(p + __MZ_ZIP_CDH_FILE_TIME_OFS), __MZ_READ_LE16(p + __MZ_ZIP_CDH_FILE_DATE_OFS));
#endif
  pStat->m_crc32 = __MZ_READ_LE32(p + __MZ_ZIP_CDH_CRC32_OFS);
  pStat->m_comp_size = __MZ_READ_LE32(p + __MZ_ZIP_CDH_COMPRESSED_SIZE_OFS);
  pStat->m_uncomp_size = __MZ_READ_LE32(p + __MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS);
  pStat->m_internal_attr = __MZ_READ_LE16(p + __MZ_ZIP_CDH_INTERNAL_ATTR_OFS);
  pStat->m_external_attr = __MZ_READ_LE32(p + __MZ_ZIP_CDH_EXTERNAL_ATTR_OFS);
  pStat->m_local_header_ofs = __MZ_READ_LE32(p + __MZ_ZIP_CDH_LOCAL_HEADER_OFS);

  // Copy as much of the filename and comment as possible.
  n = __MZ_READ_LE16(p + __MZ_ZIP_CDH_FILENAME_LEN_OFS); n = __MZ_MIN(n, __MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE - 1);
  memcpy(pStat->m_filename, p + __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE, n); pStat->m_filename[n] = '\0';

  n = __MZ_READ_LE16(p + __MZ_ZIP_CDH_COMMENT_LEN_OFS); n = __MZ_MIN(n, __MZ_ZIP_MAX_ARCHIVE_FILE_COMMENT_SIZE - 1);
  pStat->m_comment_size = n;
  memcpy(pStat->m_comment, p + __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE + __MZ_READ_LE16(p + __MZ_ZIP_CDH_FILENAME_LEN_OFS) + __MZ_READ_LE16(p + __MZ_ZIP_CDH_EXTRA_LEN_OFS), n); pStat->m_comment[n] = '\0';

  return __MZ_TRUE;
}

__mz_uint __mz_zip_reader_get_filename(__mz_zip_archive *pZip, __mz_uint file_index, char *pFilename, __mz_uint filename_buf_size)
{
  __mz_uint n;
  const __mz_uint8 *p = __mz_zip_reader_get_cdh(pZip, file_index);
  if (!p) { if (filename_buf_size) pFilename[0] = '\0'; return 0; }
  n = __MZ_READ_LE16(p + __MZ_ZIP_CDH_FILENAME_LEN_OFS);
  if (filename_buf_size)
  {
    n = __MZ_MIN(n, filename_buf_size - 1);
    memcpy(pFilename, p + __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE, n);
    pFilename[n] = '\0';
  }
  return n + 1;
}

static __MZ_FORCEINLINE __mz_bool __mz_zip_reader_string_equal(const char *pA, const char *pB, __mz_uint len, __mz_uint flags)
{
  __mz_uint i;
  if (flags & __MZ_ZIP_FLAG_CASE_SENSITIVE)
    return 0 == memcmp(pA, pB, len);
  for (i = 0; i < len; ++i)
    if (__MZ_TOLOWER(pA[i]) != __MZ_TOLOWER(pB[i]))
      return __MZ_FALSE;
  return __MZ_TRUE;
}

static __MZ_FORCEINLINE int __mz_zip_reader_filename_compare(const __mz_zip_array *pCentral_dir_array, const __mz_zip_array *pCentral_dir_offsets, __mz_uint l_index, const char *pR, __mz_uint r_len)
{
  const __mz_uint8 *pL = &__MZ_ZIP_ARRAY_ELEMENT(pCentral_dir_array, __mz_uint8, __MZ_ZIP_ARRAY_ELEMENT(pCentral_dir_offsets, __mz_uint32, l_index)), *pE;
  __mz_uint l_len = __MZ_READ_LE16(pL + __MZ_ZIP_CDH_FILENAME_LEN_OFS);
  __mz_uint8 l = 0, r = 0;
  pL += __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE;
  pE = pL + __MZ_MIN(l_len, r_len);
  while (pL < pE)
  {
    if ((l = __MZ_TOLOWER(*pL)) != (r = __MZ_TOLOWER(*pR)))
      break;
    pL++; pR++;
  }
  return (pL == pE) ? (int)(l_len - r_len) : (l - r);
}

static int __mz_zip_reader_locate_file_binary_search(__mz_zip_archive *pZip, const char *pFilename)
{
  __mz_zip_internal_state *pState = pZip->m_pState;
  const __mz_zip_array *pCentral_dir_offsets = &pState->m_central_dir_offsets;
  const __mz_zip_array *pCentral_dir = &pState->m_central_dir;
  __mz_uint32 *pIndices = &__MZ_ZIP_ARRAY_ELEMENT(&pState->m_sorted_central_dir_offsets, __mz_uint32, 0);
  const int size = pZip->m_total_files;
  const __mz_uint filename_len = (__mz_uint)strlen(pFilename);
  int l = 0, h = size - 1;
  while (l <= h)
  {
    int m = (l + h) >> 1, file_index = pIndices[m], comp = __mz_zip_reader_filename_compare(pCentral_dir, pCentral_dir_offsets, file_index, pFilename, filename_len);
    if (!comp)
      return file_index;
    else if (comp < 0)
      l = m + 1;
    else
      h = m - 1;
  }
  return -1;
}

int __mz_zip_reader_locate_file(__mz_zip_archive *pZip, const char *pName, const char *pComment, __mz_uint flags)
{
  __mz_uint file_index; size_t name_len, comment_len;
  if ((!pZip) || (!pZip->m_pState) || (!pName) || (pZip->m_zip_mode != __MZ_ZIP_MODE_READING))
    return -1;
  if (((flags & (__MZ_ZIP_FLAG_IGNORE_PATH | __MZ_ZIP_FLAG_CASE_SENSITIVE)) == 0) && (!pComment) && (pZip->m_pState->m_sorted_central_dir_offsets.m_size))
    return __mz_zip_reader_locate_file_binary_search(pZip, pName);
  name_len = strlen(pName); if (name_len > 0xFFFF) return -1;
  comment_len = pComment ? strlen(pComment) : 0; if (comment_len > 0xFFFF) return -1;
  for (file_index = 0; file_index < pZip->m_total_files; file_index++)
  {
    const __mz_uint8 *pHeader = &__MZ_ZIP_ARRAY_ELEMENT(&pZip->m_pState->m_central_dir, __mz_uint8, __MZ_ZIP_ARRAY_ELEMENT(&pZip->m_pState->m_central_dir_offsets, __mz_uint32, file_index));
    __mz_uint filename_len = __MZ_READ_LE16(pHeader + __MZ_ZIP_CDH_FILENAME_LEN_OFS);
    const char *pFilename = (const char *)pHeader + __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE;
    if (filename_len < name_len)
      continue;
    if (comment_len)
    {
      __mz_uint file_extra_len = __MZ_READ_LE16(pHeader + __MZ_ZIP_CDH_EXTRA_LEN_OFS), file_comment_len = __MZ_READ_LE16(pHeader + __MZ_ZIP_CDH_COMMENT_LEN_OFS);
      const char *pFile_comment = pFilename + filename_len + file_extra_len;
      if ((file_comment_len != comment_len) || (!__mz_zip_reader_string_equal(pComment, pFile_comment, file_comment_len, flags)))
        continue;
    }
    if ((flags & __MZ_ZIP_FLAG_IGNORE_PATH) && (filename_len))
    {
      int ofs = filename_len - 1;
      do
      {
        if ((pFilename[ofs] == '/') || (pFilename[ofs] == '\\') || (pFilename[ofs] == ':'))
          break;
      } while (--ofs >= 0);
      ofs++;
      pFilename += ofs; filename_len -= ofs;
    }
    if ((filename_len == name_len) && (__mz_zip_reader_string_equal(pName, pFilename, filename_len, flags)))
      return file_index;
  }
  return -1;
}

__mz_bool __mz_zip_reader_extract_to_mem_no_alloc(__mz_zip_archive *pZip, __mz_uint file_index, void *pBuf, size_t buf_size, __mz_uint flags, void *pUser_read_buf, size_t user_read_buf_size)
{
  int status = __TINFL_STATUS_DONE;
  __mz_uint64 needed_size, cur_file_ofs, comp_remaining, out_buf_ofs = 0, read_buf_size, read_buf_ofs = 0, read_buf_avail;
  __mz_zip_archive_file_stat file_stat;
  void *pRead_buf;
  __mz_uint32 local_header_u32[(__MZ_ZIP_LOCAL_DIR_HEADER_SIZE + sizeof(__mz_uint32) - 1) / sizeof(__mz_uint32)]; __mz_uint8 *pLocal_header = (__mz_uint8 *)local_header_u32;
  __tinfl_decompressor inflator;

  if ((buf_size) && (!pBuf))
    return __MZ_FALSE;

  if (!__mz_zip_reader_file_stat(pZip, file_index, &file_stat))
    return __MZ_FALSE;

  // Empty file, or a directory (but not always a directory - I've seen odd zips with directories that have compressed data which inflates to 0 bytes)
  if (!file_stat.m_comp_size)
    return __MZ_TRUE;

  // Entry is a subdirectory (I've seen old zips with dir entries which have compressed deflate data which inflates to 0 bytes, but these entries claim to uncompress to 512 bytes in the headers).
  // I'm torn how to handle this case - should it fail instead?
  if (__mz_zip_reader_is_file_a_directory(pZip, file_index))
    return __MZ_TRUE;

  // Encryption and patch files are not supported.
  if (file_stat.m_bit_flag & (1 | 32))
    return __MZ_FALSE;

  // This function only supports stored and deflate.
  if ((!(flags & __MZ_ZIP_FLAG_COMPRESSED_DATA)) && (file_stat.m_method != 0) && (file_stat.m_method != __MZ_DEFLATED))
    return __MZ_FALSE;

  // Ensure supplied output buffer is large enough.
  needed_size = (flags & __MZ_ZIP_FLAG_COMPRESSED_DATA) ? file_stat.m_comp_size : file_stat.m_uncomp_size;
  if (buf_size < needed_size)
    return __MZ_FALSE;

  // Read and parse the local directory entry.
  cur_file_ofs = file_stat.m_local_header_ofs;
  if (pZip->m_pRead(pZip->m_pIO_opaque, cur_file_ofs, pLocal_header, __MZ_ZIP_LOCAL_DIR_HEADER_SIZE) != __MZ_ZIP_LOCAL_DIR_HEADER_SIZE)
    return __MZ_FALSE;
  if (__MZ_READ_LE32(pLocal_header) != __MZ_ZIP_LOCAL_DIR_HEADER_SIG)
    return __MZ_FALSE;

  cur_file_ofs += __MZ_ZIP_LOCAL_DIR_HEADER_SIZE + __MZ_READ_LE16(pLocal_header + __MZ_ZIP_LDH_FILENAME_LEN_OFS) + __MZ_READ_LE16(pLocal_header + __MZ_ZIP_LDH_EXTRA_LEN_OFS);
  if ((cur_file_ofs + file_stat.m_comp_size) > pZip->m_archive_size)
    return __MZ_FALSE;

  if ((flags & __MZ_ZIP_FLAG_COMPRESSED_DATA) || (!file_stat.m_method))
  {
    // The file is stored or the caller has requested the compressed data.
    if (pZip->m_pRead(pZip->m_pIO_opaque, cur_file_ofs, pBuf, (size_t)needed_size) != needed_size)
      return __MZ_FALSE;
    return ((flags & __MZ_ZIP_FLAG_COMPRESSED_DATA) != 0) || (__mz_crc32(__MZ_CRC32_INIT, (const __mz_uint8 *)pBuf, (size_t)file_stat.m_uncomp_size) == file_stat.m_crc32);
  }

  // Decompress the file either directly from memory or from a file input buffer.
  __tinfl_init(&inflator);

  if (pZip->m_pState->m_pMem)
  {
    // Read directly from the archive in memory.
    pRead_buf = (__mz_uint8 *)pZip->m_pState->m_pMem + cur_file_ofs;
    read_buf_size = read_buf_avail = file_stat.m_comp_size;
    comp_remaining = 0;
  }
  else if (pUser_read_buf)
  {
    // Use a user provided read buffer.
    if (!user_read_buf_size)
      return __MZ_FALSE;
    pRead_buf = (__mz_uint8 *)pUser_read_buf;
    read_buf_size = user_read_buf_size;
    read_buf_avail = 0;
    comp_remaining = file_stat.m_comp_size;
  }
  else
  {
    // Temporarily allocate a read buffer.
    read_buf_size = __MZ_MIN(file_stat.m_comp_size, __MZ_ZIP_MAX_IO_BUF_SIZE);
#ifdef _MSC_VER
    if (((0, sizeof(size_t) == sizeof(__mz_uint32))) && (read_buf_size > 0x7FFFFFFF))
#else
    if (((sizeof(size_t) == sizeof(__mz_uint32))) && (read_buf_size > 0x7FFFFFFF))
#endif
      return __MZ_FALSE;
    if (NULL == (pRead_buf = pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, (size_t)read_buf_size)))
      return __MZ_FALSE;
    read_buf_avail = 0;
    comp_remaining = file_stat.m_comp_size;
  }

  do
  {
    size_t in_buf_size, out_buf_size = (size_t)(file_stat.m_uncomp_size - out_buf_ofs);
    if ((!read_buf_avail) && (!pZip->m_pState->m_pMem))
    {
      read_buf_avail = __MZ_MIN(read_buf_size, comp_remaining);
      if (pZip->m_pRead(pZip->m_pIO_opaque, cur_file_ofs, pRead_buf, (size_t)read_buf_avail) != read_buf_avail)
      {
        status = __TINFL_STATUS_FAILED;
        break;
      }
      cur_file_ofs += read_buf_avail;
      comp_remaining -= read_buf_avail;
      read_buf_ofs = 0;
    }
    in_buf_size = (size_t)read_buf_avail;
    status = __tinfl_decompress(&inflator, (__mz_uint8 *)pRead_buf + read_buf_ofs, &in_buf_size, (__mz_uint8 *)pBuf, (__mz_uint8 *)pBuf + out_buf_ofs, &out_buf_size, __TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF | (comp_remaining ? __TINFL_FLAG_HAS_MORE_INPUT : 0));
    read_buf_avail -= in_buf_size;
    read_buf_ofs += in_buf_size;
    out_buf_ofs += out_buf_size;
  } while (status == __TINFL_STATUS_NEEDS_MORE_INPUT);

  if (status == __TINFL_STATUS_DONE)
  {
    // Make sure the entire file was decompressed, and check its CRC.
    if ((out_buf_ofs != file_stat.m_uncomp_size) || (__mz_crc32(__MZ_CRC32_INIT, (const __mz_uint8 *)pBuf, (size_t)file_stat.m_uncomp_size) != file_stat.m_crc32))
      status = __TINFL_STATUS_FAILED;
  }

  if ((!pZip->m_pState->m_pMem) && (!pUser_read_buf))
    pZip->m_pFree(pZip->m_pAlloc_opaque, pRead_buf);

  return status == __TINFL_STATUS_DONE;
}

__mz_bool __mz_zip_reader_extract_file_to_mem_no_alloc(__mz_zip_archive *pZip, const char *pFilename, void *pBuf, size_t buf_size, __mz_uint flags, void *pUser_read_buf, size_t user_read_buf_size)
{
  int file_index = __mz_zip_reader_locate_file(pZip, pFilename, NULL, flags);
  if (file_index < 0)
    return __MZ_FALSE;
  return __mz_zip_reader_extract_to_mem_no_alloc(pZip, file_index, pBuf, buf_size, flags, pUser_read_buf, user_read_buf_size);
}

__mz_bool __mz_zip_reader_extract_to_mem(__mz_zip_archive *pZip, __mz_uint file_index, void *pBuf, size_t buf_size, __mz_uint flags)
{
  return __mz_zip_reader_extract_to_mem_no_alloc(pZip, file_index, pBuf, buf_size, flags, NULL, 0);
}

__mz_bool __mz_zip_reader_extract_file_to_mem(__mz_zip_archive *pZip, const char *pFilename, void *pBuf, size_t buf_size, __mz_uint flags)
{
  return __mz_zip_reader_extract_file_to_mem_no_alloc(pZip, pFilename, pBuf, buf_size, flags, NULL, 0);
}

void *__mz_zip_reader_extract_to_heap(__mz_zip_archive *pZip, __mz_uint file_index, size_t *pSize, __mz_uint flags)
{
  __mz_uint64 comp_size, uncomp_size, alloc_size;
  const __mz_uint8 *p = __mz_zip_reader_get_cdh(pZip, file_index);
  void *pBuf;

  if (pSize)
    *pSize = 0;
  if (!p)
    return NULL;

  comp_size = __MZ_READ_LE32(p + __MZ_ZIP_CDH_COMPRESSED_SIZE_OFS);
  uncomp_size = __MZ_READ_LE32(p + __MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS);

  alloc_size = (flags & __MZ_ZIP_FLAG_COMPRESSED_DATA) ? comp_size : uncomp_size;
#ifdef _MSC_VER
  if (((0, sizeof(size_t) == sizeof(__mz_uint32))) && (alloc_size > 0x7FFFFFFF))
#else
  if (((sizeof(size_t) == sizeof(__mz_uint32))) && (alloc_size > 0x7FFFFFFF))
#endif
    return NULL;
  if (NULL == (pBuf = pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, (size_t)alloc_size)))
    return NULL;

  if (!__mz_zip_reader_extract_to_mem(pZip, file_index, pBuf, (size_t)alloc_size, flags))
  {
    pZip->m_pFree(pZip->m_pAlloc_opaque, pBuf);
    return NULL;
  }

  if (pSize) *pSize = (size_t)alloc_size;
  return pBuf;
}

void *__mz_zip_reader_extract_file_to_heap(__mz_zip_archive *pZip, const char *pFilename, size_t *pSize, __mz_uint flags)
{
  int file_index = __mz_zip_reader_locate_file(pZip, pFilename, NULL, flags);
  if (file_index < 0)
  {
    if (pSize) *pSize = 0;
    return __MZ_FALSE;
  }
  return __mz_zip_reader_extract_to_heap(pZip, file_index, pSize, flags);
}

__mz_bool __mz_zip_reader_extract_to_callback(__mz_zip_archive *pZip, __mz_uint file_index, __mz_file_write_func pCallback, void *pOpaque, __mz_uint flags)
{
  int status = __TINFL_STATUS_DONE; __mz_uint file_crc32 = __MZ_CRC32_INIT;
  __mz_uint64 read_buf_size, read_buf_ofs = 0, read_buf_avail, comp_remaining, out_buf_ofs = 0, cur_file_ofs;
  __mz_zip_archive_file_stat file_stat;
  void *pRead_buf = NULL; void *pWrite_buf = NULL;
  __mz_uint32 local_header_u32[(__MZ_ZIP_LOCAL_DIR_HEADER_SIZE + sizeof(__mz_uint32) - 1) / sizeof(__mz_uint32)]; __mz_uint8 *pLocal_header = (__mz_uint8 *)local_header_u32;

  if (!__mz_zip_reader_file_stat(pZip, file_index, &file_stat))
    return __MZ_FALSE;

  // Empty file, or a directory (but not always a directory - I've seen odd zips with directories that have compressed data which inflates to 0 bytes)
  if (!file_stat.m_comp_size)
    return __MZ_TRUE;

  // Entry is a subdirectory (I've seen old zips with dir entries which have compressed deflate data which inflates to 0 bytes, but these entries claim to uncompress to 512 bytes in the headers).
  // I'm torn how to handle this case - should it fail instead?
  if (__mz_zip_reader_is_file_a_directory(pZip, file_index))
    return __MZ_TRUE;

  // Encryption and patch files are not supported.
  if (file_stat.m_bit_flag & (1 | 32))
    return __MZ_FALSE;

  // This function only supports stored and deflate.
  if ((!(flags & __MZ_ZIP_FLAG_COMPRESSED_DATA)) && (file_stat.m_method != 0) && (file_stat.m_method != __MZ_DEFLATED))
    return __MZ_FALSE;

  // Read and parse the local directory entry.
  cur_file_ofs = file_stat.m_local_header_ofs;
  if (pZip->m_pRead(pZip->m_pIO_opaque, cur_file_ofs, pLocal_header, __MZ_ZIP_LOCAL_DIR_HEADER_SIZE) != __MZ_ZIP_LOCAL_DIR_HEADER_SIZE)
    return __MZ_FALSE;
  if (__MZ_READ_LE32(pLocal_header) != __MZ_ZIP_LOCAL_DIR_HEADER_SIG)
    return __MZ_FALSE;

  cur_file_ofs += __MZ_ZIP_LOCAL_DIR_HEADER_SIZE + __MZ_READ_LE16(pLocal_header + __MZ_ZIP_LDH_FILENAME_LEN_OFS) + __MZ_READ_LE16(pLocal_header + __MZ_ZIP_LDH_EXTRA_LEN_OFS);
  if ((cur_file_ofs + file_stat.m_comp_size) > pZip->m_archive_size)
    return __MZ_FALSE;

  // Decompress the file either directly from memory or from a file input buffer.
  if (pZip->m_pState->m_pMem)
  {
    pRead_buf = (__mz_uint8 *)pZip->m_pState->m_pMem + cur_file_ofs;
    read_buf_size = read_buf_avail = file_stat.m_comp_size;
    comp_remaining = 0;
  }
  else
  {
    read_buf_size = __MZ_MIN(file_stat.m_comp_size, __MZ_ZIP_MAX_IO_BUF_SIZE);
    if (NULL == (pRead_buf = pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, (size_t)read_buf_size)))
      return __MZ_FALSE;
    read_buf_avail = 0;
    comp_remaining = file_stat.m_comp_size;
  }

  if ((flags & __MZ_ZIP_FLAG_COMPRESSED_DATA) || (!file_stat.m_method))
  {
    // The file is stored or the caller has requested the compressed data.
    if (pZip->m_pState->m_pMem)
    {
#ifdef _MSC_VER
      if (((0, sizeof(size_t) == sizeof(__mz_uint32))) && (file_stat.m_comp_size > 0xFFFFFFFF))
#else
      if (((sizeof(size_t) == sizeof(__mz_uint32))) && (file_stat.m_comp_size > 0xFFFFFFFF))
#endif
        return __MZ_FALSE;
      if (pCallback(pOpaque, out_buf_ofs, pRead_buf, (size_t)file_stat.m_comp_size) != file_stat.m_comp_size)
        status = __TINFL_STATUS_FAILED;
      else if (!(flags & __MZ_ZIP_FLAG_COMPRESSED_DATA))
        file_crc32 = (__mz_uint32)__mz_crc32(file_crc32, (const __mz_uint8 *)pRead_buf, (size_t)file_stat.m_comp_size);
      cur_file_ofs += file_stat.m_comp_size;
      out_buf_ofs += file_stat.m_comp_size;
      comp_remaining = 0;
    }
    else
    {
      while (comp_remaining)
      {
        read_buf_avail = __MZ_MIN(read_buf_size, comp_remaining);
        if (pZip->m_pRead(pZip->m_pIO_opaque, cur_file_ofs, pRead_buf, (size_t)read_buf_avail) != read_buf_avail)
        {
          status = __TINFL_STATUS_FAILED;
          break;
        }

        if (!(flags & __MZ_ZIP_FLAG_COMPRESSED_DATA))
          file_crc32 = (__mz_uint32)__mz_crc32(file_crc32, (const __mz_uint8 *)pRead_buf, (size_t)read_buf_avail);

        if (pCallback(pOpaque, out_buf_ofs, pRead_buf, (size_t)read_buf_avail) != read_buf_avail)
        {
          status = __TINFL_STATUS_FAILED;
          break;
        }
        cur_file_ofs += read_buf_avail;
        out_buf_ofs += read_buf_avail;
        comp_remaining -= read_buf_avail;
      }
    }
  }
  else
  {
    __tinfl_decompressor inflator;
    __tinfl_init(&inflator);

    if (NULL == (pWrite_buf = pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, __TINFL_LZ_DICT_SIZE)))
      status = __TINFL_STATUS_FAILED;
    else
    {
      do
      {
        __mz_uint8 *pWrite_buf_cur = (__mz_uint8 *)pWrite_buf + (out_buf_ofs & (__TINFL_LZ_DICT_SIZE - 1));
        size_t in_buf_size, out_buf_size = __TINFL_LZ_DICT_SIZE - (out_buf_ofs & (__TINFL_LZ_DICT_SIZE - 1));
        if ((!read_buf_avail) && (!pZip->m_pState->m_pMem))
        {
          read_buf_avail = __MZ_MIN(read_buf_size, comp_remaining);
          if (pZip->m_pRead(pZip->m_pIO_opaque, cur_file_ofs, pRead_buf, (size_t)read_buf_avail) != read_buf_avail)
          {
            status = __TINFL_STATUS_FAILED;
            break;
          }
          cur_file_ofs += read_buf_avail;
          comp_remaining -= read_buf_avail;
          read_buf_ofs = 0;
        }

        in_buf_size = (size_t)read_buf_avail;
        status = __tinfl_decompress(&inflator, (const __mz_uint8 *)pRead_buf + read_buf_ofs, &in_buf_size, (__mz_uint8 *)pWrite_buf, pWrite_buf_cur, &out_buf_size, comp_remaining ? __TINFL_FLAG_HAS_MORE_INPUT : 0);
        read_buf_avail -= in_buf_size;
        read_buf_ofs += in_buf_size;

        if (out_buf_size)
        {
          if (pCallback(pOpaque, out_buf_ofs, pWrite_buf_cur, out_buf_size) != out_buf_size)
          {
            status = __TINFL_STATUS_FAILED;
            break;
          }
          file_crc32 = (__mz_uint32)__mz_crc32(file_crc32, pWrite_buf_cur, out_buf_size);
          if ((out_buf_ofs += out_buf_size) > file_stat.m_uncomp_size)
          {
            status = __TINFL_STATUS_FAILED;
            break;
          }
        }
      } while ((status == __TINFL_STATUS_NEEDS_MORE_INPUT) || (status == __TINFL_STATUS_HAS_MORE_OUTPUT));
    }
  }

  if ((status == __TINFL_STATUS_DONE) && (!(flags & __MZ_ZIP_FLAG_COMPRESSED_DATA)))
  {
    // Make sure the entire file was decompressed, and check its CRC.
    if ((out_buf_ofs != file_stat.m_uncomp_size) || (file_crc32 != file_stat.m_crc32))
      status = __TINFL_STATUS_FAILED;
  }

  if (!pZip->m_pState->m_pMem)
    pZip->m_pFree(pZip->m_pAlloc_opaque, pRead_buf);
  if (pWrite_buf)
    pZip->m_pFree(pZip->m_pAlloc_opaque, pWrite_buf);

  return status == __TINFL_STATUS_DONE;
}

__mz_bool __mz_zip_reader_extract_file_to_callback(__mz_zip_archive *pZip, const char *pFilename, __mz_file_write_func pCallback, void *pOpaque, __mz_uint flags)
{
  int file_index = __mz_zip_reader_locate_file(pZip, pFilename, NULL, flags);
  if (file_index < 0)
    return __MZ_FALSE;
  return __mz_zip_reader_extract_to_callback(pZip, file_index, pCallback, pOpaque, flags);
}

#ifndef __MINIZ_NO_STDIO
static size_t __mz_zip_file_write_callback(void *pOpaque, __mz_uint64 ofs, const void *pBuf, size_t n)
{
  (void)ofs; return __MZ_FWRITE(pBuf, 1, n, (__MZ_FILE*)pOpaque);
}

__mz_bool __mz_zip_reader_extract_to_file(__mz_zip_archive *pZip, __mz_uint file_index, const char *pDst_filename, __mz_uint flags)
{
  __mz_bool status;
  __mz_zip_archive_file_stat file_stat;
  __MZ_FILE *pFile;
  if (!__mz_zip_reader_file_stat(pZip, file_index, &file_stat))
    return __MZ_FALSE;
  pFile = __MZ_FOPEN(pDst_filename, "wb");
  if (!pFile)
    return __MZ_FALSE;
  status = __mz_zip_reader_extract_to_callback(pZip, file_index, __mz_zip_file_write_callback, pFile, flags);
  if (__MZ_FCLOSE(pFile) == EOF)
    return __MZ_FALSE;
#ifndef __MINIZ_NO_TIME
  if (status)
    __mz_zip_set_file_times(pDst_filename, file_stat.m_time, file_stat.m_time);
#endif
  return status;
}
#endif // #ifndef __MINIZ_NO_STDIO

__mz_bool __mz_zip_reader_end(__mz_zip_archive *pZip)
{
  if ((!pZip) || (!pZip->m_pState) || (!pZip->m_pAlloc) || (!pZip->m_pFree) || (pZip->m_zip_mode != __MZ_ZIP_MODE_READING))
    return __MZ_FALSE;

  if (pZip->m_pState)
  {
    __mz_zip_internal_state *pState = pZip->m_pState; pZip->m_pState = NULL;
    __mz_zip_array_clear(pZip, &pState->m_central_dir);
    __mz_zip_array_clear(pZip, &pState->m_central_dir_offsets);
    __mz_zip_array_clear(pZip, &pState->m_sorted_central_dir_offsets);

#ifndef __MINIZ_NO_STDIO
    if (pState->m_pFile)
    {
      __MZ_FCLOSE(pState->m_pFile);
      pState->m_pFile = NULL;
    }
#endif // #ifndef __MINIZ_NO_STDIO

    pZip->m_pFree(pZip->m_pAlloc_opaque, pState);
  }
  pZip->m_zip_mode = __MZ_ZIP_MODE_INVALID;

  return __MZ_TRUE;
}

#ifndef __MINIZ_NO_STDIO
__mz_bool __mz_zip_reader_extract_file_to_file(__mz_zip_archive *pZip, const char *pArchive_filename, const char *pDst_filename, __mz_uint flags)
{
  int file_index = __mz_zip_reader_locate_file(pZip, pArchive_filename, NULL, flags);
  if (file_index < 0)
    return __MZ_FALSE;
  return __mz_zip_reader_extract_to_file(pZip, file_index, pDst_filename, flags);
}
#endif

// ------------------- .ZIP archive writing

#ifndef __MINIZ_NO_ARCHIVE_WRITING_APIS

static void __mz_write_le16(__mz_uint8 *p, __mz_uint16 v) { p[0] = (__mz_uint8)v; p[1] = (__mz_uint8)(v >> 8); }
static void __mz_write_le32(__mz_uint8 *p, __mz_uint32 v) { p[0] = (__mz_uint8)v; p[1] = (__mz_uint8)(v >> 8); p[2] = (__mz_uint8)(v >> 16); p[3] = (__mz_uint8)(v >> 24); }
#define __MZ_WRITE_LE16(p, v) __mz_write_le16((__mz_uint8 *)(p), (__mz_uint16)(v))
#define __MZ_WRITE_LE32(p, v) __mz_write_le32((__mz_uint8 *)(p), (__mz_uint32)(v))

__mz_bool __mz_zip_writer_init(__mz_zip_archive *pZip, __mz_uint64 existing_size)
{
  if ((!pZip) || (pZip->m_pState) || (!pZip->m_pWrite) || (pZip->m_zip_mode != __MZ_ZIP_MODE_INVALID))
    return __MZ_FALSE;

  if (pZip->m_file_offset_alignment)
  {
    // Ensure user specified file offset alignment is a power of 2.
    if (pZip->m_file_offset_alignment & (pZip->m_file_offset_alignment - 1))
      return __MZ_FALSE;
  }

  if (!pZip->m_pAlloc) pZip->m_pAlloc = __def_alloc_func;
  if (!pZip->m_pFree) pZip->m_pFree = __def_free_func;
  if (!pZip->m_pRealloc) pZip->m_pRealloc = __def_realloc_func;

  pZip->m_zip_mode = __MZ_ZIP_MODE_WRITING;
  pZip->m_archive_size = existing_size;
  pZip->m_central_directory_file_ofs = 0;
  pZip->m_total_files = 0;

  if (NULL == (pZip->m_pState = (__mz_zip_internal_state *)pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, sizeof(__mz_zip_internal_state))))
    return __MZ_FALSE;
  memset(pZip->m_pState, 0, sizeof(__mz_zip_internal_state));
  __MZ_ZIP_ARRAY_SET_ELEMENT_SIZE(&pZip->m_pState->m_central_dir, sizeof(__mz_uint8));
  __MZ_ZIP_ARRAY_SET_ELEMENT_SIZE(&pZip->m_pState->m_central_dir_offsets, sizeof(__mz_uint32));
  __MZ_ZIP_ARRAY_SET_ELEMENT_SIZE(&pZip->m_pState->m_sorted_central_dir_offsets, sizeof(__mz_uint32));
  return __MZ_TRUE;
}

static size_t __mz_zip_heap_write_func(void *pOpaque, __mz_uint64 file_ofs, const void *pBuf, size_t n)
{
  __mz_zip_archive *pZip = (__mz_zip_archive *)pOpaque;
  __mz_zip_internal_state *pState = pZip->m_pState;
  __mz_uint64 new_size = __MZ_MAX(file_ofs + n, pState->m_mem_size);
#ifdef _MSC_VER
  if ((!n) || ((0, sizeof(size_t) == sizeof(__mz_uint32)) && (new_size > 0x7FFFFFFF)))
#else
  if ((!n) || ((sizeof(size_t) == sizeof(__mz_uint32)) && (new_size > 0x7FFFFFFF)))
#endif
    return 0;
  if (new_size > pState->m_mem_capacity)
  {
    void *pNew_block;
    size_t new_capacity = __MZ_MAX(64, pState->m_mem_capacity); while (new_capacity < new_size) new_capacity *= 2;
    if (NULL == (pNew_block = pZip->m_pRealloc(pZip->m_pAlloc_opaque, pState->m_pMem, 1, new_capacity)))
      return 0;
    pState->m_pMem = pNew_block; pState->m_mem_capacity = new_capacity;
  }
  memcpy((__mz_uint8 *)pState->m_pMem + file_ofs, pBuf, n);
  pState->m_mem_size = (size_t)new_size;
  return n;
}

__mz_bool __mz_zip_writer_init_heap(__mz_zip_archive *pZip, size_t size_to_reserve_at_beginning, size_t initial_allocation_size)
{
  pZip->m_pWrite = __mz_zip_heap_write_func;
  pZip->m_pIO_opaque = pZip;
  if (!__mz_zip_writer_init(pZip, size_to_reserve_at_beginning))
    return __MZ_FALSE;
  if (0 != (initial_allocation_size = __MZ_MAX(initial_allocation_size, size_to_reserve_at_beginning)))
  {
    if (NULL == (pZip->m_pState->m_pMem = pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, initial_allocation_size)))
    {
      __mz_zip_writer_end(pZip);
      return __MZ_FALSE;
    }
    pZip->m_pState->m_mem_capacity = initial_allocation_size;
  }
  return __MZ_TRUE;
}

#ifndef __MINIZ_NO_STDIO
static size_t __mz_zip_file_write_func(void *pOpaque, __mz_uint64 file_ofs, const void *pBuf, size_t n)
{
  __mz_zip_archive *pZip = (__mz_zip_archive *)pOpaque;
  __mz_int64 cur_ofs = __MZ_FTELL64(pZip->m_pState->m_pFile);
  if (((__mz_int64)file_ofs < 0) || (((cur_ofs != (__mz_int64)file_ofs)) && (__MZ_FSEEK64(pZip->m_pState->m_pFile, (__mz_int64)file_ofs, SEEK_SET))))
    return 0;
  return __MZ_FWRITE(pBuf, 1, n, pZip->m_pState->m_pFile);
}

__mz_bool __mz_zip_writer_init_file(__mz_zip_archive *pZip, const char *pFilename, __mz_uint64 size_to_reserve_at_beginning)
{
  __MZ_FILE *pFile;
  pZip->m_pWrite = __mz_zip_file_write_func;
  pZip->m_pIO_opaque = pZip;
  if (!__mz_zip_writer_init(pZip, size_to_reserve_at_beginning))
    return __MZ_FALSE;
  if (NULL == (pFile = __MZ_FOPEN(pFilename, "wb")))
  {
    __mz_zip_writer_end(pZip);
    return __MZ_FALSE;
  }
  pZip->m_pState->m_pFile = pFile;
  if (size_to_reserve_at_beginning)
  {
    __mz_uint64 cur_ofs = 0; char buf[4096]; __MZ_CLEAR_OBJ(buf);
    do
    {
      size_t n = (size_t)__MZ_MIN(sizeof(buf), size_to_reserve_at_beginning);
      if (pZip->m_pWrite(pZip->m_pIO_opaque, cur_ofs, buf, n) != n)
      {
        __mz_zip_writer_end(pZip);
        return __MZ_FALSE;
      }
      cur_ofs += n; size_to_reserve_at_beginning -= n;
    } while (size_to_reserve_at_beginning);
  }
  return __MZ_TRUE;
}
#endif // #ifndef __MINIZ_NO_STDIO

__mz_bool __mz_zip_writer_init_from_reader(__mz_zip_archive *pZip, const char *pFilename)
{
  __mz_zip_internal_state *pState;
  if ((!pZip) || (!pZip->m_pState) || (pZip->m_zip_mode != __MZ_ZIP_MODE_READING))
    return __MZ_FALSE;
  // No sense in trying to write to an archive that's already at the support max size
  if ((pZip->m_total_files == 0xFFFF) || ((pZip->m_archive_size + __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE + __MZ_ZIP_LOCAL_DIR_HEADER_SIZE) > 0xFFFFFFFF))
    return __MZ_FALSE;

  pState = pZip->m_pState;

  if (pState->m_pFile)
  {
#ifdef __MINIZ_NO_STDIO
    pFilename; return __MZ_FALSE;
#else
    // Archive is being read from stdio - try to reopen as writable.
    if (pZip->m_pIO_opaque != pZip)
      return __MZ_FALSE;
    if (!pFilename)
      return __MZ_FALSE;
    pZip->m_pWrite = __mz_zip_file_write_func;
    if (NULL == (pState->m_pFile = __MZ_FREOPEN(pFilename, "r+b", pState->m_pFile)))
    {
      // The mz_zip_archive is now in a bogus state because pState->m_pFile is NULL, so just close it.
      __mz_zip_reader_end(pZip);
      return __MZ_FALSE;
    }
#endif // #ifdef __MINIZ_NO_STDIO
  }
  else if (pState->m_pMem)
  {
    // Archive lives in a memory block. Assume it's from the heap that we can resize using the realloc callback.
    if (pZip->m_pIO_opaque != pZip)
      return __MZ_FALSE;
    pState->m_mem_capacity = pState->m_mem_size;
    pZip->m_pWrite = __mz_zip_heap_write_func;
  }
  // Archive is being read via a user provided read function - make sure the user has specified a write function too.
  else if (!pZip->m_pWrite)
    return __MZ_FALSE;

  // Start writing new files at the archive's current central directory location.
  pZip->m_archive_size = pZip->m_central_directory_file_ofs;
  pZip->m_zip_mode = __MZ_ZIP_MODE_WRITING;
  pZip->m_central_directory_file_ofs = 0;

  return __MZ_TRUE;
}

__mz_bool __mz_zip_writer_add_mem(__mz_zip_archive *pZip, const char *pArchive_name, const void *pBuf, size_t buf_size, __mz_uint level_and_flags)
{
  return __mz_zip_writer_add_mem_ex(pZip, pArchive_name, pBuf, buf_size, NULL, 0, level_and_flags, 0, 0);
}

typedef struct
{
  __mz_zip_archive *m_pZip;
  __mz_uint64 m_cur_archive_file_ofs;
  __mz_uint64 m_comp_size;
} mz_zip_writer_add_state;

static __mz_bool __mz_zip_writer_add_put_buf_callback(const void* pBuf, int len, void *pUser)
{
  mz_zip_writer_add_state *pState = (mz_zip_writer_add_state *)pUser;
  if ((int)pState->m_pZip->m_pWrite(pState->m_pZip->m_pIO_opaque, pState->m_cur_archive_file_ofs, pBuf, len) != len)
    return __MZ_FALSE;
  pState->m_cur_archive_file_ofs += len;
  pState->m_comp_size += len;
  return __MZ_TRUE;
}

static __mz_bool __mz_zip_writer_create_local_dir_header(__mz_zip_archive *pZip, __mz_uint8 *pDst, __mz_uint16 filename_size, __mz_uint16 extra_size, __mz_uint64 uncomp_size, __mz_uint64 comp_size, __mz_uint32 uncomp_crc32, __mz_uint16 method, __mz_uint16 bit_flags, __mz_uint16 dos_time, __mz_uint16 dos_date)
{
  (void)pZip;
  memset(pDst, 0, __MZ_ZIP_LOCAL_DIR_HEADER_SIZE);
  __MZ_WRITE_LE32(pDst + __MZ_ZIP_LDH_SIG_OFS, __MZ_ZIP_LOCAL_DIR_HEADER_SIG);
  __MZ_WRITE_LE16(pDst + __MZ_ZIP_LDH_VERSION_NEEDED_OFS, method ? 20 : 0);
  __MZ_WRITE_LE16(pDst + __MZ_ZIP_LDH_BIT_FLAG_OFS, bit_flags);
  __MZ_WRITE_LE16(pDst + __MZ_ZIP_LDH_METHOD_OFS, method);
  __MZ_WRITE_LE16(pDst + __MZ_ZIP_LDH_FILE_TIME_OFS, dos_time);
  __MZ_WRITE_LE16(pDst + __MZ_ZIP_LDH_FILE_DATE_OFS, dos_date);
  __MZ_WRITE_LE32(pDst + __MZ_ZIP_LDH_CRC32_OFS, uncomp_crc32);
  __MZ_WRITE_LE32(pDst + __MZ_ZIP_LDH_COMPRESSED_SIZE_OFS, comp_size);
  __MZ_WRITE_LE32(pDst + __MZ_ZIP_LDH_DECOMPRESSED_SIZE_OFS, uncomp_size);
  __MZ_WRITE_LE16(pDst + __MZ_ZIP_LDH_FILENAME_LEN_OFS, filename_size);
  __MZ_WRITE_LE16(pDst + __MZ_ZIP_LDH_EXTRA_LEN_OFS, extra_size);
  return __MZ_TRUE;
}

static __mz_bool __mz_zip_writer_create_central_dir_header(__mz_zip_archive *pZip, __mz_uint8 *pDst, __mz_uint16 filename_size, __mz_uint16 extra_size, __mz_uint16 comment_size, __mz_uint64 uncomp_size, __mz_uint64 comp_size, __mz_uint32 uncomp_crc32, __mz_uint16 method, __mz_uint16 bit_flags, __mz_uint16 dos_time, __mz_uint16 dos_date, __mz_uint64 local_header_ofs, __mz_uint32 ext_attributes)
{
  (void)pZip;
  memset(pDst, 0, __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE);
  __MZ_WRITE_LE32(pDst + __MZ_ZIP_CDH_SIG_OFS, __MZ_ZIP_CENTRAL_DIR_HEADER_SIG);
  __MZ_WRITE_LE16(pDst + __MZ_ZIP_CDH_VERSION_NEEDED_OFS, method ? 20 : 0);
  __MZ_WRITE_LE16(pDst + __MZ_ZIP_CDH_BIT_FLAG_OFS, bit_flags);
  __MZ_WRITE_LE16(pDst + __MZ_ZIP_CDH_METHOD_OFS, method);
  __MZ_WRITE_LE16(pDst + __MZ_ZIP_CDH_FILE_TIME_OFS, dos_time);
  __MZ_WRITE_LE16(pDst + __MZ_ZIP_CDH_FILE_DATE_OFS, dos_date);
  __MZ_WRITE_LE32(pDst + __MZ_ZIP_CDH_CRC32_OFS, uncomp_crc32);
  __MZ_WRITE_LE32(pDst + __MZ_ZIP_CDH_COMPRESSED_SIZE_OFS, comp_size);
  __MZ_WRITE_LE32(pDst + __MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS, uncomp_size);
  __MZ_WRITE_LE16(pDst + __MZ_ZIP_CDH_FILENAME_LEN_OFS, filename_size);
  __MZ_WRITE_LE16(pDst + __MZ_ZIP_CDH_EXTRA_LEN_OFS, extra_size);
  __MZ_WRITE_LE16(pDst + __MZ_ZIP_CDH_COMMENT_LEN_OFS, comment_size);
  __MZ_WRITE_LE32(pDst + __MZ_ZIP_CDH_EXTERNAL_ATTR_OFS, ext_attributes);
  __MZ_WRITE_LE32(pDst + __MZ_ZIP_CDH_LOCAL_HEADER_OFS, local_header_ofs);
  return __MZ_TRUE;
}

static __mz_bool __mz_zip_writer_add_to_central_dir(__mz_zip_archive *pZip, const char *pFilename, __mz_uint16 filename_size, const void *pExtra, __mz_uint16 extra_size, const void *pComment, __mz_uint16 comment_size, __mz_uint64 uncomp_size, __mz_uint64 comp_size, __mz_uint32 uncomp_crc32, __mz_uint16 method, __mz_uint16 bit_flags, __mz_uint16 dos_time, __mz_uint16 dos_date, __mz_uint64 local_header_ofs, __mz_uint32 ext_attributes)
{
  __mz_zip_internal_state *pState = pZip->m_pState;
  __mz_uint32 central_dir_ofs = (__mz_uint32)pState->m_central_dir.m_size;
  size_t orig_central_dir_size = pState->m_central_dir.m_size;
  __mz_uint8 central_dir_header[__MZ_ZIP_CENTRAL_DIR_HEADER_SIZE];

  // No zip64 support yet
  if ((local_header_ofs > 0xFFFFFFFF) || (((__mz_uint64)pState->m_central_dir.m_size + __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE + filename_size + extra_size + comment_size) > 0xFFFFFFFF))
    return __MZ_FALSE;

  if (!__mz_zip_writer_create_central_dir_header(pZip, central_dir_header, filename_size, extra_size, comment_size, uncomp_size, comp_size, uncomp_crc32, method, bit_flags, dos_time, dos_date, local_header_ofs, ext_attributes))
    return __MZ_FALSE;

  if ((!__mz_zip_array_push_back(pZip, &pState->m_central_dir, central_dir_header, __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE)) ||
      (!__mz_zip_array_push_back(pZip, &pState->m_central_dir, pFilename, filename_size)) ||
      (!__mz_zip_array_push_back(pZip, &pState->m_central_dir, pExtra, extra_size)) ||
      (!__mz_zip_array_push_back(pZip, &pState->m_central_dir, pComment, comment_size)) ||
      (!__mz_zip_array_push_back(pZip, &pState->m_central_dir_offsets, &central_dir_ofs, 1)))
  {
    // Try to push the central directory array back into its original state.
    __mz_zip_array_resize(pZip, &pState->m_central_dir, orig_central_dir_size, __MZ_FALSE);
    return __MZ_FALSE;
  }

  return __MZ_TRUE;
}

static __mz_bool __mz_zip_writer_validate_archive_name(const char *pArchive_name)
{
  // Basic ZIP archive filename validity checks: Valid filenames cannot start with a forward slash, cannot contain a drive letter, and cannot use DOS-style backward slashes.
  if (*pArchive_name == '/')
    return __MZ_FALSE;
  while (*pArchive_name)
  {
    if ((*pArchive_name == '\\') || (*pArchive_name == ':'))
      return __MZ_FALSE;
    pArchive_name++;
  }
  return __MZ_TRUE;
}

static __mz_uint __mz_zip_writer_compute_padding_needed_for_file_alignment(__mz_zip_archive *pZip)
{
  __mz_uint32 n;
  if (!pZip->m_file_offset_alignment)
    return 0;
  n = (__mz_uint32)(pZip->m_archive_size & (pZip->m_file_offset_alignment - 1));
  return (pZip->m_file_offset_alignment - n) & (pZip->m_file_offset_alignment - 1);
}

static __mz_bool __mz_zip_writer_write_zeros(__mz_zip_archive *pZip, __mz_uint64 cur_file_ofs, __mz_uint32 n)
{
  char buf[4096];
  memset(buf, 0, __MZ_MIN(sizeof(buf), n));
  while (n)
  {
    __mz_uint32 s = __MZ_MIN(sizeof(buf), n);
    if (pZip->m_pWrite(pZip->m_pIO_opaque, cur_file_ofs, buf, s) != s)
      return __MZ_FALSE;
    cur_file_ofs += s; n -= s;
  }
  return __MZ_TRUE;
}

__mz_bool __mz_zip_writer_add_mem_ex(__mz_zip_archive *pZip, const char *pArchive_name, const void *pBuf, size_t buf_size, const void *pComment, __mz_uint16 comment_size, __mz_uint level_and_flags, __mz_uint64 uncomp_size, __mz_uint32 uncomp_crc32)
{
  __mz_uint16 method = 0, dos_time = 0, dos_date = 0;
  __mz_uint level, ext_attributes = 0, num_alignment_padding_bytes;
  __mz_uint64 local_dir_header_ofs = pZip->m_archive_size, cur_archive_file_ofs = pZip->m_archive_size, comp_size = 0;
  size_t archive_name_size;
  __mz_uint8 local_dir_header[__MZ_ZIP_LOCAL_DIR_HEADER_SIZE];
  __tdefl_compressor *pComp = NULL;
  __mz_bool store_data_uncompressed;
  __mz_zip_internal_state *pState;

  if ((int)level_and_flags < 0)
    level_and_flags = __MZ_DEFAULT_LEVEL;
  level = level_and_flags & 0xF;
  store_data_uncompressed = ((!level) || (level_and_flags & __MZ_ZIP_FLAG_COMPRESSED_DATA));

  if ((!pZip) || (!pZip->m_pState) || (pZip->m_zip_mode != __MZ_ZIP_MODE_WRITING) || ((buf_size) && (!pBuf)) || (!pArchive_name) || ((comment_size) && (!pComment)) || (pZip->m_total_files == 0xFFFF) || (level > __MZ_UBER_COMPRESSION))
    return __MZ_FALSE;

  pState = pZip->m_pState;

  if ((!(level_and_flags & __MZ_ZIP_FLAG_COMPRESSED_DATA)) && (uncomp_size))
    return __MZ_FALSE;
  // No zip64 support yet
  if ((buf_size > 0xFFFFFFFF) || (uncomp_size > 0xFFFFFFFF))
    return __MZ_FALSE;
  if (!__mz_zip_writer_validate_archive_name(pArchive_name))
    return __MZ_FALSE;

#ifndef __MINIZ_NO_TIME
  {
    time_t cur_time; time(&cur_time);
    __mz_zip_time_to_dos_time(cur_time, &dos_time, &dos_date);
  }
#endif // #ifndef __MINIZ_NO_TIME

  archive_name_size = strlen(pArchive_name);
  if (archive_name_size > 0xFFFF)
    return __MZ_FALSE;

  num_alignment_padding_bytes = __mz_zip_writer_compute_padding_needed_for_file_alignment(pZip);

  // no zip64 support yet
  if ((pZip->m_total_files == 0xFFFF) || ((pZip->m_archive_size + num_alignment_padding_bytes + __MZ_ZIP_LOCAL_DIR_HEADER_SIZE + __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE + comment_size + archive_name_size) > 0xFFFFFFFF))
    return __MZ_FALSE;

  if ((archive_name_size) && (pArchive_name[archive_name_size - 1] == '/'))
  {
    // Set DOS Subdirectory attribute bit.
    ext_attributes |= 0x10;
    // Subdirectories cannot contain data.
    if ((buf_size) || (uncomp_size))
      return __MZ_FALSE;
  }

  // Try to do any allocations before writing to the archive, so if an allocation fails the file remains unmodified. (A good idea if we're doing an in-place modification.)
  if ((!__mz_zip_array_ensure_room(pZip, &pState->m_central_dir, __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE + archive_name_size + comment_size)) || (!__mz_zip_array_ensure_room(pZip, &pState->m_central_dir_offsets, 1)))
    return __MZ_FALSE;

  if ((!store_data_uncompressed) && (buf_size))
  {
    if (NULL == (pComp = (__tdefl_compressor *)pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, sizeof(__tdefl_compressor))))
      return __MZ_FALSE;
  }

  if (!__mz_zip_writer_write_zeros(pZip, cur_archive_file_ofs, num_alignment_padding_bytes + sizeof(local_dir_header)))
  {
    pZip->m_pFree(pZip->m_pAlloc_opaque, pComp);
    return __MZ_FALSE;
  }
  local_dir_header_ofs += num_alignment_padding_bytes;
  if (pZip->m_file_offset_alignment) { __MZ_ASSERT((local_dir_header_ofs & (pZip->m_file_offset_alignment - 1)) == 0); }
  cur_archive_file_ofs += num_alignment_padding_bytes + sizeof(local_dir_header);

  __MZ_CLEAR_OBJ(local_dir_header);
  if (pZip->m_pWrite(pZip->m_pIO_opaque, cur_archive_file_ofs, pArchive_name, archive_name_size) != archive_name_size)
  {
    pZip->m_pFree(pZip->m_pAlloc_opaque, pComp);
    return __MZ_FALSE;
  }
  cur_archive_file_ofs += archive_name_size;

  if (!(level_and_flags & __MZ_ZIP_FLAG_COMPRESSED_DATA))
  {
    uncomp_crc32 = (__mz_uint32)__mz_crc32(__MZ_CRC32_INIT, (const __mz_uint8*)pBuf, buf_size);
    uncomp_size = buf_size;
    if (uncomp_size <= 3)
    {
      level = 0;
      store_data_uncompressed = __MZ_TRUE;
    }
  }

  if (store_data_uncompressed)
  {
    if (pZip->m_pWrite(pZip->m_pIO_opaque, cur_archive_file_ofs, pBuf, buf_size) != buf_size)
    {
      pZip->m_pFree(pZip->m_pAlloc_opaque, pComp);
      return __MZ_FALSE;
    }

    cur_archive_file_ofs += buf_size;
    comp_size = buf_size;

    if (level_and_flags & __MZ_ZIP_FLAG_COMPRESSED_DATA)
      method = __MZ_DEFLATED;
  }
  else if (buf_size)
  {
    mz_zip_writer_add_state state;

    state.m_pZip = pZip;
    state.m_cur_archive_file_ofs = cur_archive_file_ofs;
    state.m_comp_size = 0;

    if ((__tdefl_init(pComp, __mz_zip_writer_add_put_buf_callback, &state, __tdefl_create_comp_flags_from_zip_params(level, -15, __MZ_DEFAULT_STRATEGY)) != __TDEFL_STATUS_OKAY) ||
        (__tdefl_compress_buffer(pComp, pBuf, buf_size, __TDEFL_FINISH) != __TDEFL_STATUS_DONE))
    {
      pZip->m_pFree(pZip->m_pAlloc_opaque, pComp);
      return __MZ_FALSE;
    }

    comp_size = state.m_comp_size;
    cur_archive_file_ofs = state.m_cur_archive_file_ofs;

    method = __MZ_DEFLATED;
  }

  pZip->m_pFree(pZip->m_pAlloc_opaque, pComp);
  pComp = NULL;

  // no zip64 support yet
  if ((comp_size > 0xFFFFFFFF) || (cur_archive_file_ofs > 0xFFFFFFFF))
    return __MZ_FALSE;

  if (!__mz_zip_writer_create_local_dir_header(pZip, local_dir_header, (__mz_uint16)archive_name_size, 0, uncomp_size, comp_size, uncomp_crc32, method, 0, dos_time, dos_date))
    return __MZ_FALSE;

  if (pZip->m_pWrite(pZip->m_pIO_opaque, local_dir_header_ofs, local_dir_header, sizeof(local_dir_header)) != sizeof(local_dir_header))
    return __MZ_FALSE;

  if (!__mz_zip_writer_add_to_central_dir(pZip, pArchive_name, (__mz_uint16)archive_name_size, NULL, 0, pComment, comment_size, uncomp_size, comp_size, uncomp_crc32, method, 0, dos_time, dos_date, local_dir_header_ofs, ext_attributes))
    return __MZ_FALSE;

  pZip->m_total_files++;
  pZip->m_archive_size = cur_archive_file_ofs;

  return __MZ_TRUE;
}

#ifndef __MINIZ_NO_STDIO
__mz_bool __mz_zip_writer_add_file(__mz_zip_archive *pZip, const char *pArchive_name, const char *pSrc_filename, const void *pComment, __mz_uint16 comment_size, __mz_uint level_and_flags)
{
  __mz_uint uncomp_crc32 = __MZ_CRC32_INIT, level, num_alignment_padding_bytes;
  __mz_uint16 method = 0, dos_time = 0, dos_date = 0, ext_attributes = 0;
  __mz_uint64 local_dir_header_ofs = pZip->m_archive_size, cur_archive_file_ofs = pZip->m_archive_size, uncomp_size = 0, comp_size = 0;
  size_t archive_name_size;
  __mz_uint8 local_dir_header[__MZ_ZIP_LOCAL_DIR_HEADER_SIZE];
  __MZ_FILE *pSrc_file = NULL;

  if ((int)level_and_flags < 0)
    level_and_flags = __MZ_DEFAULT_LEVEL;
  level = level_and_flags & 0xF;

  if ((!pZip) || (!pZip->m_pState) || (pZip->m_zip_mode != __MZ_ZIP_MODE_WRITING) || (!pArchive_name) || ((comment_size) && (!pComment)) || (level > __MZ_UBER_COMPRESSION))
    return __MZ_FALSE;
  if (level_and_flags & __MZ_ZIP_FLAG_COMPRESSED_DATA)
    return __MZ_FALSE;
  if (!__mz_zip_writer_validate_archive_name(pArchive_name))
    return __MZ_FALSE;

  archive_name_size = strlen(pArchive_name);
  if (archive_name_size > 0xFFFF)
    return __MZ_FALSE;

  num_alignment_padding_bytes = __mz_zip_writer_compute_padding_needed_for_file_alignment(pZip);

  // no zip64 support yet
  if ((pZip->m_total_files == 0xFFFF) || ((pZip->m_archive_size + num_alignment_padding_bytes + __MZ_ZIP_LOCAL_DIR_HEADER_SIZE + __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE + comment_size + archive_name_size) > 0xFFFFFFFF))
    return __MZ_FALSE;

  if (!__mz_zip_get_file_modified_time(pSrc_filename, &dos_time, &dos_date))
    return __MZ_FALSE;
    
  pSrc_file = __MZ_FOPEN(pSrc_filename, "rb");
  if (!pSrc_file)
    return __MZ_FALSE;
  __MZ_FSEEK64(pSrc_file, 0, SEEK_END);
  uncomp_size = __MZ_FTELL64(pSrc_file);
  __MZ_FSEEK64(pSrc_file, 0, SEEK_SET);

  if (uncomp_size > 0xFFFFFFFF)
  {
    // No zip64 support yet
    __MZ_FCLOSE(pSrc_file);
    return __MZ_FALSE;
  }
  if (uncomp_size <= 3)
    level = 0;

  if (!__mz_zip_writer_write_zeros(pZip, cur_archive_file_ofs, num_alignment_padding_bytes + sizeof(local_dir_header)))
  {
    __MZ_FCLOSE(pSrc_file);
    return __MZ_FALSE;
  }
  local_dir_header_ofs += num_alignment_padding_bytes;
  if (pZip->m_file_offset_alignment) { __MZ_ASSERT((local_dir_header_ofs & (pZip->m_file_offset_alignment - 1)) == 0); }
  cur_archive_file_ofs += num_alignment_padding_bytes + sizeof(local_dir_header);

  __MZ_CLEAR_OBJ(local_dir_header);
  if (pZip->m_pWrite(pZip->m_pIO_opaque, cur_archive_file_ofs, pArchive_name, archive_name_size) != archive_name_size)
  {
    __MZ_FCLOSE(pSrc_file);
    return __MZ_FALSE;
  }
  cur_archive_file_ofs += archive_name_size;

  if (uncomp_size)
  {
    __mz_uint64 uncomp_remaining = uncomp_size;
    void *pRead_buf = pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, __MZ_ZIP_MAX_IO_BUF_SIZE);
    if (!pRead_buf)
    {
      __MZ_FCLOSE(pSrc_file);
      return __MZ_FALSE;
    }

    if (!level)
    {
      while (uncomp_remaining)
      {
        __mz_uint n = (__mz_uint)__MZ_MIN(__MZ_ZIP_MAX_IO_BUF_SIZE, uncomp_remaining);
        if ((__MZ_FREAD(pRead_buf, 1, n, pSrc_file) != n) || (pZip->m_pWrite(pZip->m_pIO_opaque, cur_archive_file_ofs, pRead_buf, n) != n))
        {
          pZip->m_pFree(pZip->m_pAlloc_opaque, pRead_buf);
          __MZ_FCLOSE(pSrc_file);
          return __MZ_FALSE;
        }
        uncomp_crc32 = (__mz_uint32)__mz_crc32(uncomp_crc32, (const __mz_uint8 *)pRead_buf, n);
        uncomp_remaining -= n;
        cur_archive_file_ofs += n;
      }
      comp_size = uncomp_size;
    }
    else
    {
      __mz_bool result = __MZ_FALSE;
      mz_zip_writer_add_state state;
      __tdefl_compressor *pComp = (__tdefl_compressor *)pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, sizeof(__tdefl_compressor));
      if (!pComp)
      {
        pZip->m_pFree(pZip->m_pAlloc_opaque, pRead_buf);
        __MZ_FCLOSE(pSrc_file);
        return __MZ_FALSE;
      }

      state.m_pZip = pZip;
      state.m_cur_archive_file_ofs = cur_archive_file_ofs;
      state.m_comp_size = 0;

      if (__tdefl_init(pComp, __mz_zip_writer_add_put_buf_callback, &state, __tdefl_create_comp_flags_from_zip_params(level, -15, __MZ_DEFAULT_STRATEGY)) != __TDEFL_STATUS_OKAY)
      {
        pZip->m_pFree(pZip->m_pAlloc_opaque, pComp);
        pZip->m_pFree(pZip->m_pAlloc_opaque, pRead_buf);
        __MZ_FCLOSE(pSrc_file);
        return __MZ_FALSE;
      }

      for ( ; ; )
      {
        size_t in_buf_size = (__mz_uint32)__MZ_MIN(uncomp_remaining, __MZ_ZIP_MAX_IO_BUF_SIZE);
        __tdefl_status status;

        if (__MZ_FREAD(pRead_buf, 1, in_buf_size, pSrc_file) != in_buf_size)
          break;

        uncomp_crc32 = (__mz_uint32)__mz_crc32(uncomp_crc32, (const __mz_uint8 *)pRead_buf, in_buf_size);
        uncomp_remaining -= in_buf_size;

        status = __tdefl_compress_buffer(pComp, pRead_buf, in_buf_size, uncomp_remaining ? __TDEFL_NO_FLUSH : __TDEFL_FINISH);
        if (status == __TDEFL_STATUS_DONE)
        {
          result = __MZ_TRUE;
          break;
        }
        else if (status != __TDEFL_STATUS_OKAY)
          break;
      }

      pZip->m_pFree(pZip->m_pAlloc_opaque, pComp);

      if (!result)
      {
        pZip->m_pFree(pZip->m_pAlloc_opaque, pRead_buf);
        __MZ_FCLOSE(pSrc_file);
        return __MZ_FALSE;
      }

      comp_size = state.m_comp_size;
      cur_archive_file_ofs = state.m_cur_archive_file_ofs;

      method = __MZ_DEFLATED;
    }

    pZip->m_pFree(pZip->m_pAlloc_opaque, pRead_buf);
  }

  __MZ_FCLOSE(pSrc_file); pSrc_file = NULL;

  // no zip64 support yet
  if ((comp_size > 0xFFFFFFFF) || (cur_archive_file_ofs > 0xFFFFFFFF))
    return __MZ_FALSE;

  if (!__mz_zip_writer_create_local_dir_header(pZip, local_dir_header, (__mz_uint16)archive_name_size, 0, uncomp_size, comp_size, uncomp_crc32, method, 0, dos_time, dos_date))
    return __MZ_FALSE;

  if (pZip->m_pWrite(pZip->m_pIO_opaque, local_dir_header_ofs, local_dir_header, sizeof(local_dir_header)) != sizeof(local_dir_header))
    return __MZ_FALSE;

  if (!__mz_zip_writer_add_to_central_dir(pZip, pArchive_name, (__mz_uint16)archive_name_size, NULL, 0, pComment, comment_size, uncomp_size, comp_size, uncomp_crc32, method, 0, dos_time, dos_date, local_dir_header_ofs, ext_attributes))
    return __MZ_FALSE;

  pZip->m_total_files++;
  pZip->m_archive_size = cur_archive_file_ofs;

  return __MZ_TRUE;
}
#endif // #ifndef __MINIZ_NO_STDIO

__mz_bool __mz_zip_writer_add_from_zip_reader(__mz_zip_archive *pZip, __mz_zip_archive *pSource_zip, __mz_uint file_index)
{
  __mz_uint n, bit_flags, num_alignment_padding_bytes;
  __mz_uint64 comp_bytes_remaining, local_dir_header_ofs;
  __mz_uint64 cur_src_file_ofs, cur_dst_file_ofs;
  __mz_uint32 local_header_u32[(__MZ_ZIP_LOCAL_DIR_HEADER_SIZE + sizeof(__mz_uint32) - 1) / sizeof(__mz_uint32)]; __mz_uint8 *pLocal_header = (__mz_uint8 *)local_header_u32;
  __mz_uint8 central_header[__MZ_ZIP_CENTRAL_DIR_HEADER_SIZE];
  size_t orig_central_dir_size;
  __mz_zip_internal_state *pState;
  void *pBuf; const __mz_uint8 *pSrc_central_header;

  if ((!pZip) || (!pZip->m_pState) || (pZip->m_zip_mode != __MZ_ZIP_MODE_WRITING))
    return __MZ_FALSE;
  if (NULL == (pSrc_central_header = __mz_zip_reader_get_cdh(pSource_zip, file_index)))
    return __MZ_FALSE;
  pState = pZip->m_pState;

  num_alignment_padding_bytes = __mz_zip_writer_compute_padding_needed_for_file_alignment(pZip);

  // no zip64 support yet
  if ((pZip->m_total_files == 0xFFFF) || ((pZip->m_archive_size + num_alignment_padding_bytes + __MZ_ZIP_LOCAL_DIR_HEADER_SIZE + __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE) > 0xFFFFFFFF))
    return __MZ_FALSE;

  cur_src_file_ofs = __MZ_READ_LE32(pSrc_central_header + __MZ_ZIP_CDH_LOCAL_HEADER_OFS);
  cur_dst_file_ofs = pZip->m_archive_size;

  if (pSource_zip->m_pRead(pSource_zip->m_pIO_opaque, cur_src_file_ofs, pLocal_header, __MZ_ZIP_LOCAL_DIR_HEADER_SIZE) != __MZ_ZIP_LOCAL_DIR_HEADER_SIZE)
    return __MZ_FALSE;
  if (__MZ_READ_LE32(pLocal_header) != __MZ_ZIP_LOCAL_DIR_HEADER_SIG)
    return __MZ_FALSE;
  cur_src_file_ofs += __MZ_ZIP_LOCAL_DIR_HEADER_SIZE;

  if (!__mz_zip_writer_write_zeros(pZip, cur_dst_file_ofs, num_alignment_padding_bytes))
    return __MZ_FALSE;
  cur_dst_file_ofs += num_alignment_padding_bytes;
  local_dir_header_ofs = cur_dst_file_ofs;
  if (pZip->m_file_offset_alignment) { __MZ_ASSERT((local_dir_header_ofs & (pZip->m_file_offset_alignment - 1)) == 0); }

  if (pZip->m_pWrite(pZip->m_pIO_opaque, cur_dst_file_ofs, pLocal_header, __MZ_ZIP_LOCAL_DIR_HEADER_SIZE) != __MZ_ZIP_LOCAL_DIR_HEADER_SIZE)
    return __MZ_FALSE;
  cur_dst_file_ofs += __MZ_ZIP_LOCAL_DIR_HEADER_SIZE;

  n = __MZ_READ_LE16(pLocal_header + __MZ_ZIP_LDH_FILENAME_LEN_OFS) + __MZ_READ_LE16(pLocal_header + __MZ_ZIP_LDH_EXTRA_LEN_OFS);
  comp_bytes_remaining = n + __MZ_READ_LE32(pSrc_central_header + __MZ_ZIP_CDH_COMPRESSED_SIZE_OFS);

  if (NULL == (pBuf = pZip->m_pAlloc(pZip->m_pAlloc_opaque, 1, (size_t)__MZ_MAX(sizeof(__mz_uint32) * 4, __MZ_MIN(__MZ_ZIP_MAX_IO_BUF_SIZE, comp_bytes_remaining)))))
    return __MZ_FALSE;

  while (comp_bytes_remaining)
  {
    n = (__mz_uint)__MZ_MIN(__MZ_ZIP_MAX_IO_BUF_SIZE, comp_bytes_remaining);
    if (pSource_zip->m_pRead(pSource_zip->m_pIO_opaque, cur_src_file_ofs, pBuf, n) != n)
    {
      pZip->m_pFree(pZip->m_pAlloc_opaque, pBuf);
      return __MZ_FALSE;
    }
    cur_src_file_ofs += n;

    if (pZip->m_pWrite(pZip->m_pIO_opaque, cur_dst_file_ofs, pBuf, n) != n)
    {
      pZip->m_pFree(pZip->m_pAlloc_opaque, pBuf);
      return __MZ_FALSE;
    }
    cur_dst_file_ofs += n;

    comp_bytes_remaining -= n;
  }

  bit_flags = __MZ_READ_LE16(pLocal_header + __MZ_ZIP_LDH_BIT_FLAG_OFS);
  if (bit_flags & 8)
  {
    // Copy data descriptor
    if (pSource_zip->m_pRead(pSource_zip->m_pIO_opaque, cur_src_file_ofs, pBuf, sizeof(__mz_uint32) * 4) != sizeof(__mz_uint32) * 4)
    {
      pZip->m_pFree(pZip->m_pAlloc_opaque, pBuf);
      return __MZ_FALSE;
    }

    n = sizeof(__mz_uint32) * ((__MZ_READ_LE32(pBuf) == 0x08074b50) ? 4 : 3);
    if (pZip->m_pWrite(pZip->m_pIO_opaque, cur_dst_file_ofs, pBuf, n) != n)
    {
      pZip->m_pFree(pZip->m_pAlloc_opaque, pBuf);
      return __MZ_FALSE;
    }

    cur_src_file_ofs += n;
    cur_dst_file_ofs += n;
  }
  pZip->m_pFree(pZip->m_pAlloc_opaque, pBuf);

  // no zip64 support yet
  if (cur_dst_file_ofs > 0xFFFFFFFF)
    return __MZ_FALSE;

  orig_central_dir_size = pState->m_central_dir.m_size;

  memcpy(central_header, pSrc_central_header, __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE);
  __MZ_WRITE_LE32(central_header + __MZ_ZIP_CDH_LOCAL_HEADER_OFS, local_dir_header_ofs);
  if (!__mz_zip_array_push_back(pZip, &pState->m_central_dir, central_header, __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE))
    return __MZ_FALSE;

  n = __MZ_READ_LE16(pSrc_central_header + __MZ_ZIP_CDH_FILENAME_LEN_OFS) + __MZ_READ_LE16(pSrc_central_header + __MZ_ZIP_CDH_EXTRA_LEN_OFS) + __MZ_READ_LE16(pSrc_central_header + __MZ_ZIP_CDH_COMMENT_LEN_OFS);
  if (!__mz_zip_array_push_back(pZip, &pState->m_central_dir, pSrc_central_header + __MZ_ZIP_CENTRAL_DIR_HEADER_SIZE, n))
  {
    __mz_zip_array_resize(pZip, &pState->m_central_dir, orig_central_dir_size, __MZ_FALSE);
    return __MZ_FALSE;
  }

  if (pState->m_central_dir.m_size > 0xFFFFFFFF)
    return __MZ_FALSE;
  n = (__mz_uint32)orig_central_dir_size;
  if (!__mz_zip_array_push_back(pZip, &pState->m_central_dir_offsets, &n, 1))
  {
    __mz_zip_array_resize(pZip, &pState->m_central_dir, orig_central_dir_size, __MZ_FALSE);
    return __MZ_FALSE;
  }

  pZip->m_total_files++;
  pZip->m_archive_size = cur_dst_file_ofs;

  return __MZ_TRUE;
}

__mz_bool __mz_zip_writer_finalize_archive(__mz_zip_archive *pZip)
{
  __mz_zip_internal_state *pState;
  __mz_uint64 central_dir_ofs, central_dir_size;
  __mz_uint8 hdr[__MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE];

  if ((!pZip) || (!pZip->m_pState) || (pZip->m_zip_mode != __MZ_ZIP_MODE_WRITING))
    return __MZ_FALSE;

  pState = pZip->m_pState;

  // no zip64 support yet
  if ((pZip->m_total_files > 0xFFFF) || ((pZip->m_archive_size + pState->m_central_dir.m_size + __MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE) > 0xFFFFFFFF))
    return __MZ_FALSE;

  central_dir_ofs = 0;
  central_dir_size = 0;
  if (pZip->m_total_files)
  {
    // Write central directory
    central_dir_ofs = pZip->m_archive_size;
    central_dir_size = pState->m_central_dir.m_size;
    pZip->m_central_directory_file_ofs = central_dir_ofs;
    if (pZip->m_pWrite(pZip->m_pIO_opaque, central_dir_ofs, pState->m_central_dir.m_p, (size_t)central_dir_size) != central_dir_size)
      return __MZ_FALSE;
    pZip->m_archive_size += central_dir_size;
  }

  // Write end of central directory record
  __MZ_CLEAR_OBJ(hdr);
  __MZ_WRITE_LE32(hdr + __MZ_ZIP_ECDH_SIG_OFS, __MZ_ZIP_END_OF_CENTRAL_DIR_HEADER_SIG);
  __MZ_WRITE_LE16(hdr + __MZ_ZIP_ECDH_CDIR_NUM_ENTRIES_ON_DISK_OFS, pZip->m_total_files);
  __MZ_WRITE_LE16(hdr + __MZ_ZIP_ECDH_CDIR_TOTAL_ENTRIES_OFS, pZip->m_total_files);
  __MZ_WRITE_LE32(hdr + __MZ_ZIP_ECDH_CDIR_SIZE_OFS, central_dir_size);
  __MZ_WRITE_LE32(hdr + __MZ_ZIP_ECDH_CDIR_OFS_OFS, central_dir_ofs);

  if (pZip->m_pWrite(pZip->m_pIO_opaque, pZip->m_archive_size, hdr, sizeof(hdr)) != sizeof(hdr))
    return __MZ_FALSE;
#ifndef __MINIZ_NO_STDIO
  if ((pState->m_pFile) && (__MZ_FFLUSH(pState->m_pFile) == EOF))
    return __MZ_FALSE;
#endif // #ifndef __MINIZ_NO_STDIO

  pZip->m_archive_size += sizeof(hdr);

  pZip->m_zip_mode = __MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED;
  return __MZ_TRUE;
}

__mz_bool __mz_zip_writer_finalize_heap_archive(__mz_zip_archive *pZip, void **pBuf, size_t *pSize)
{
  if ((!pZip) || (!pZip->m_pState) || (!pBuf) || (!pSize))
    return __MZ_FALSE;
  if (pZip->m_pWrite != __mz_zip_heap_write_func)
    return __MZ_FALSE;
  if (!__mz_zip_writer_finalize_archive(pZip))
    return __MZ_FALSE;

  *pBuf = pZip->m_pState->m_pMem;
  *pSize = pZip->m_pState->m_mem_size;
  pZip->m_pState->m_pMem = NULL;
  pZip->m_pState->m_mem_size = pZip->m_pState->m_mem_capacity = 0;
  return __MZ_TRUE;
}

__mz_bool __mz_zip_writer_end(__mz_zip_archive *pZip)
{
  __mz_zip_internal_state *pState;
  __mz_bool status = __MZ_TRUE;
  if ((!pZip) || (!pZip->m_pState) || (!pZip->m_pAlloc) || (!pZip->m_pFree) || ((pZip->m_zip_mode != __MZ_ZIP_MODE_WRITING) && (pZip->m_zip_mode != __MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED)))
    return __MZ_FALSE;

  pState = pZip->m_pState;
  pZip->m_pState = NULL;
  __mz_zip_array_clear(pZip, &pState->m_central_dir);
  __mz_zip_array_clear(pZip, &pState->m_central_dir_offsets);
  __mz_zip_array_clear(pZip, &pState->m_sorted_central_dir_offsets);

#ifndef __MINIZ_NO_STDIO
  if (pState->m_pFile)
  {
    __MZ_FCLOSE(pState->m_pFile);
    pState->m_pFile = NULL;
  }
#endif // #ifndef __MINIZ_NO_STDIO

  if ((pZip->m_pWrite == __mz_zip_heap_write_func) && (pState->m_pMem))
  {
    pZip->m_pFree(pZip->m_pAlloc_opaque, pState->m_pMem);
    pState->m_pMem = NULL;
  }

  pZip->m_pFree(pZip->m_pAlloc_opaque, pState);
  pZip->m_zip_mode = __MZ_ZIP_MODE_INVALID;
  return status;
}

#ifndef __MINIZ_NO_STDIO
__mz_bool __mz_zip_add_mem_to_archive_file_in_place(const char *pZip_filename, const char *pArchive_name, const void *pBuf, size_t buf_size, const void *pComment, __mz_uint16 comment_size, __mz_uint level_and_flags)
{
  __mz_bool status, created_new_archive = __MZ_FALSE;
  __mz_zip_archive zip_archive;
  struct __MZ_FILE_STAT_STRUCT file_stat;
  __MZ_CLEAR_OBJ(zip_archive);
  if ((int)level_and_flags < 0)
     level_and_flags = __MZ_DEFAULT_LEVEL;
  if ((!pZip_filename) || (!pArchive_name) || ((buf_size) && (!pBuf)) || ((comment_size) && (!pComment)) || ((level_and_flags & 0xF) > __MZ_UBER_COMPRESSION))
    return __MZ_FALSE;
  if (!__mz_zip_writer_validate_archive_name(pArchive_name))
    return __MZ_FALSE;
  if (__MZ_FILE_STAT(pZip_filename, &file_stat) != 0)
  {
    // Create a new archive.
    if (!__mz_zip_writer_init_file(&zip_archive, pZip_filename, 0))
      return __MZ_FALSE;
    created_new_archive = __MZ_TRUE;
  }
  else
  {
    // Append to an existing archive.
    if (!__mz_zip_reader_init_file(&zip_archive, pZip_filename, level_and_flags | __MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY))
      return __MZ_FALSE;
    if (!__mz_zip_writer_init_from_reader(&zip_archive, pZip_filename))
    {
      __mz_zip_reader_end(&zip_archive);
      return __MZ_FALSE;
    }
  }
  status = __mz_zip_writer_add_mem_ex(&zip_archive, pArchive_name, pBuf, buf_size, pComment, comment_size, level_and_flags, 0, 0);
  // Always finalize, even if adding failed for some reason, so we have a valid central directory. (This may not always succeed, but we can try.)
  if (!__mz_zip_writer_finalize_archive(&zip_archive))
    status = __MZ_FALSE;
  if (!__mz_zip_writer_end(&zip_archive))
    status = __MZ_FALSE;
  if ((!status) && (created_new_archive))
  {
    // It's a new archive and something went wrong, so just delete it.
    int ignoredStatus = __MZ_DELETE_FILE(pZip_filename);
    (void)ignoredStatus;
  }
  return status;
}

void *__mz_zip_extract_archive_file_to_heap(const char *pZip_filename, const char *pArchive_name, size_t *pSize, __mz_uint flags)
{
  int file_index;
  __mz_zip_archive zip_archive;
  void *p = NULL;

  if (pSize)
    *pSize = 0;

  if ((!pZip_filename) || (!pArchive_name))
    return NULL;

  __MZ_CLEAR_OBJ(zip_archive);
  if (!__mz_zip_reader_init_file(&zip_archive, pZip_filename, flags | __MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY))
    return NULL;

  if ((file_index = __mz_zip_reader_locate_file(&zip_archive, pArchive_name, NULL, flags)) >= 0)
    p = __mz_zip_reader_extract_to_heap(&zip_archive, file_index, pSize, flags);

  __mz_zip_reader_end(&zip_archive);
  return p;
}

#endif // #ifndef __MINIZ_NO_STDIO

#endif // #ifndef __MINIZ_NO_ARCHIVE_WRITING_APIS

#endif // #ifndef __MINIZ_NO_ARCHIVE_APIS

#ifdef __cplusplus
}
#endif
#endif // MINIZ_HEADER_FILE_ONLY

/*
  This is free and unencumbered software released into the public domain.

  Anyone is free to copy, modify, publish, use, compile, sell, or
  distribute this software, either in source code form or as a compiled
  binary, for any purpose, commercial or non-commercial, and by any
  means.

  In jurisdictions that recognize copyright laws, the author or authors
  of this software dedicate any and all copyright interest in the
  software to the public domain. We make this dedication for the benefit
  of the public at large and to the detriment of our heirs and
  successors. We intend this dedication to be an overt act of
  relinquishment in perpetuity of all present and future rights to this
  software under copyright law.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
  OTHER DEALINGS IN THE SOFTWARE.

  For more information, please refer to <http://unlicense.org/>
*/
