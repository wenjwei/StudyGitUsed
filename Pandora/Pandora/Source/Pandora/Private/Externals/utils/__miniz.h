#ifndef __MINIZ_HEADER_INCLUDED
#define __MINIZ_HEADER_INCLUDED

#include <stdlib.h>

// Defines to completely disable specific portions of miniz.c:
// If all macros here are defined the only functionality remaining will be CRC-32, adler-32, tinfl, and tdefl.

// Define __MINIZ_NO_STDIO to disable all usage and any functions which rely on stdio for file I/O.
//#define __MINIZ_NO_STDIO

// If __MINIZ_NO_TIME is specified then the ZIP archive functions will not be able to get the current time, or
// get/set file times, and the C run-time funcs that get/set times won't be called.
// The current downside is the times written to your archives will be from 1979.
//#define __MINIZ_NO_TIME

// Define __MINIZ_NO_ARCHIVE_APIS to disable all ZIP archive API's.
//#define __MINIZ_NO_ARCHIVE_APIS

// Define __MINIZ_NO_ARCHIVE_APIS to disable all writing related ZIP archive API's.
//#define __MINIZ_NO_ARCHIVE_WRITING_APIS

// Define __MINIZ_NO_ZLIB_APIS to remove all ZLIB-style compression/decompression API's.
//#define __MINIZ_NO_ZLIB_APIS

// Define MINIZ_NO_ZLIB_COMPATIBLE_NAME to disable zlib names, to prevent conflicts against stock zlib.
//#define __MINIZ_NO_ZLIB_COMPATIBLE_NAMES

// Define __MINIZ_NO_MALLOC to disable all calls to malloc, free, and realloc.
// Note if __MINIZ_NO_MALLOC is defined then the user must always provide custom user alloc/free/realloc
// callbacks to the zlib and archive API's, and a few stand-alone helper API's which don't provide custom user
// functions (such as tdefl_compress_mem_to_heap() and tinfl_decompress_mem_to_heap()) won't work.
//#define __MINIZ_NO_MALLOC

#if defined(__TINYC__) && (defined(__linux) || defined(__linux__))
  // TODO: Work around "error: include file 'sys\utime.h' when compiling with tcc on Linux
  #define __MINIZ_NO_TIME
#endif

#if !defined(__MINIZ_NO_TIME) && !defined(__MINIZ_NO_ARCHIVE_APIS)
  #include <time.h>
#endif

#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__i386) || defined(__i486__) || defined(__i486) || defined(i386) || defined(__ia64__) || defined(__x86_64__)
// __MINIZ_X86_OR_X64_CPU is only used to help set the below macros.
#define __MINIZ_X86_OR_X64_CPU 1
#else
#define __MINIZ_X86_OR_X64_CPU 0
#endif

#if (__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__) || __MINIZ_X86_OR_X64_CPU
// Set __MINIZ_LITTLE_ENDIAN to 1 if the processor is little endian.
#define __MINIZ_LITTLE_ENDIAN 1
#endif

#if __MINIZ_X86_OR_X64_CPU
// Set __MINIZ_USE_UNALIGNED_LOADS_AND_STORES to 1 on CPU's that permit efficient integer loads and stores from unaligned addresses.
#define __MINIZ_USE_UNALIGNED_LOADS_AND_STORES 1
#else 
#define __MINIZ_USE_UNALIGNED_LOADS_AND_STORES 0
#endif

#if defined(_M_X64) || defined(_WIN64) || defined(__MINGW64__) || defined(_LP64) || defined(__LP64__) || defined(__ia64__) || defined(__x86_64__)
// Set __MINIZ_HAS_64BIT_REGISTERS to 1 if operations on 64-bit integers are reasonably fast (and don't involve compiler generated calls to helper functions).
#define __MINIZ_HAS_64BIT_REGISTERS 1
#else
#define __MINIZ_HAS_64BIT_REGISTERS 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ------------------- zlib-style API Definitions.

// For more compatibility with zlib, miniz.c uses unsigned long for some parameters/struct members. Beware: mz_ulong can be either 32 or 64-bits!
typedef unsigned long __mz_ulong;

// mz_free() internally uses the __MZ_FREE() macro (which by default calls free() unless you've modified the __MZ_MALLOC macro) to release a block allocated from the heap.
void __mz_free(void *p);

#define __MZ_ADLER32_INIT (1)
// mz_adler32() returns the initial adler-32 value to use when called with ptr==NULL.
__mz_ulong __mz_adler32(__mz_ulong adler, const unsigned char *ptr, size_t buf_len);

#define __MZ_CRC32_INIT (0)
// mz_crc32() returns the initial CRC-32 value to use when called with ptr==NULL.
__mz_ulong __mz_crc32(__mz_ulong crc, const unsigned char *ptr, size_t buf_len);

// Compression strategies.
enum { __MZ_DEFAULT_STRATEGY = 0, __MZ_FILTERED = 1, __MZ_HUFFMAN_ONLY = 2, __MZ_RLE = 3, __MZ_FIXED = 4 };

// Method
#define __MZ_DEFLATED 8

#ifndef __MINIZ_NO_ZLIB_APIS

// Heap allocation callbacks.
// Note that mz_alloc_func parameter types purpsosely differ from zlib's: items/size is size_t, not unsigned long.
typedef void *(*__mz_alloc_func)(void *opaque, size_t items, size_t size);
typedef void (*__mz_free_func)(void *opaque, void *address);
typedef void *(*__mz_realloc_func)(void *opaque, void *address, size_t items, size_t size);

#define __MZ_VERSION          "9.1.15"
#define __MZ_VERNUM           0x91F0
#define __MZ_VER_MAJOR        9
#define __MZ_VER_MINOR        1
#define __MZ_VER_REVISION     15
#define __MZ_VER_SUBREVISION  0

// Flush values. For typical usage you only need __MZ_NO_FLUSH and __MZ_FINISH. The other values are for advanced use (refer to the zlib docs).
enum { __MZ_NO_FLUSH = 0, __MZ_PARTIAL_FLUSH = 1, __MZ_SYNC_FLUSH = 2, __MZ_FULL_FLUSH = 3, __MZ_FINISH = 4, __MZ_BLOCK = 5 };

// Return status codes. __MZ_PARAM_ERROR is non-standard.
enum { __MZ_OK = 0, __MZ_STREAM_END = 1, __MZ_NEED_DICT = 2, __MZ_ERRNO = -1, __MZ_STREAM_ERROR = -2, __MZ_DATA_ERROR = -3, __MZ_MEM_ERROR = -4, __MZ_BUF_ERROR = -5, __MZ_VERSION_ERROR = -6, __MZ_PARAM_ERROR = -10000 };

// Compression levels: 0-9 are the standard zlib-style levels, 10 is best possible compression (not zlib compatible, and may be very slow), __MZ_DEFAULT_COMPRESSION=MZ_DEFAULT_LEVEL.
enum { __MZ_NO_COMPRESSION = 0, __MZ_BEST_SPEED = 1, __MZ_BEST_COMPRESSION = 9, __MZ_UBER_COMPRESSION = 10, __MZ_DEFAULT_LEVEL = 6, __MZ_DEFAULT_COMPRESSION = -1 };

// Window bits
#define __MZ_DEFAULT_WINDOW_BITS 15

struct __mz_internal_state;

// Compression/decompression stream struct.
typedef struct __mz_stream_s
{
  const unsigned char *next_in;     // pointer to next byte to read
  unsigned int avail_in;            // number of bytes available at next_in
  __mz_ulong total_in;                // total number of bytes consumed so far

  unsigned char *next_out;          // pointer to next byte to write
  unsigned int avail_out;           // number of bytes that can be written to next_out
  __mz_ulong total_out;               // total number of bytes produced so far

  char *msg;                        // error msg (unused)
  struct __mz_internal_state *state;  // internal state, allocated by zalloc/zfree

  __mz_alloc_func zalloc;             // optional heap allocation function (defaults to malloc)
  __mz_free_func zfree;               // optional heap free function (defaults to free)
  void *opaque;                     // heap alloc function user pointer

  int data_type;                    // data_type (unused)
  __mz_ulong adler;                   // adler32 of the source or uncompressed data
  __mz_ulong reserved;                // not used
} __mz_stream;

typedef __mz_stream *__mz_streamp;

// Returns the version string of miniz.c.
const char *__mz_version(void);

// mz_deflateInit() initializes a compressor with default options:
// Parameters:
//  pStream must point to an initialized mz_stream struct.
//  level must be between [__MZ_NO_COMPRESSION, __MZ_BEST_COMPRESSION].
//  level 1 enables a specially optimized compression function that's been optimized purely for performance, not ratio.
//  (This special func. is currently only enabled when __MINIZ_USE_UNALIGNED_LOADS_AND_STORES and __MINIZ_LITTLE_ENDIAN are defined.)
// Return values:
//  __MZ_OK on success.
//  __MZ_STREAM_ERROR if the stream is bogus.
//  __MZ_PARAM_ERROR if the input parameters are bogus.
//  __MZ_MEM_ERROR on out of memory.
int __mz_deflateInit(__mz_streamp pStream, int level);

// mz_deflateInit2() is like mz_deflate(), except with more control:
// Additional parameters:
//   method must be __MZ_DEFLATED
//   window_bits must be __MZ_DEFAULT_WINDOW_BITS (to wrap the deflate stream with zlib header/adler-32 footer) or -__MZ_DEFAULT_WINDOW_BITS (raw deflate/no header or footer)
//   mem_level must be between [1, 9] (it's checked but ignored by miniz.c)
int __mz_deflateInit2(__mz_streamp pStream, int level, int method, int window_bits, int mem_level, int strategy);

// Quickly resets a compressor without having to reallocate anything. Same as calling mz_deflateEnd() followed by mz_deflateInit()/mz_deflateInit2().
int __mz_deflateReset(__mz_streamp pStream);

// mz_deflate() compresses the input to output, consuming as much of the input and producing as much output as possible.
// Parameters:
//   pStream is the stream to read from and write to. You must initialize/update the next_in, avail_in, next_out, and avail_out members.
//   flush may be __MZ_NO_FLUSH, __MZ_PARTIAL_FLUSH/__MZ_SYNC_FLUSH, __MZ_FULL_FLUSH, or __MZ_FINISH.
// Return values:
//   __MZ_OK on success (when flushing, or if more input is needed but not available, and/or there's more output to be written but the output buffer is full).
//   __MZ_STREAM_END if all input has been consumed and all output bytes have been written. Don't call mz_deflate() on the stream anymore.
//   __MZ_STREAM_ERROR if the stream is bogus.
//   __MZ_PARAM_ERROR if one of the parameters is invalid.
//   __MZ_BUF_ERROR if no forward progress is possible because the input and/or output buffers are empty. (Fill up the input buffer or free up some output space and try again.)
int __mz_deflate(__mz_streamp pStream, int flush);

// mz_deflateEnd() deinitializes a compressor:
// Return values:
//  __MZ_OK on success.
//  __MZ_STREAM_ERROR if the stream is bogus.
int __mz_deflateEnd(__mz_streamp pStream);

// mz_deflateBound() returns a (very) conservative upper bound on the amount of data that could be generated by deflate(), assuming flush is set to only __MZ_NO_FLUSH or __MZ_FINISH.
__mz_ulong __mz_deflateBound(__mz_streamp pStream, __mz_ulong source_len);

// Single-call compression functions mz_compress() and mz_compress2():
// Returns __MZ_OK on success, or one of the error codes from mz_deflate() on failure.
int __mz_compress(unsigned char *pDest, __mz_ulong *pDest_len, const unsigned char *pSource, __mz_ulong source_len);
int __mz_compress2(unsigned char *pDest, __mz_ulong *pDest_len, const unsigned char *pSource, __mz_ulong source_len, int level);

// mz_compressBound() returns a (very) conservative upper bound on the amount of data that could be generated by calling mz_compress().
__mz_ulong __mz_compressBound(__mz_ulong source_len);

// Initializes a decompressor.
int __mz_inflateInit(__mz_streamp pStream);

// mz_inflateInit2() is like mz_inflateInit() with an additional option that controls the window size and whether or not the stream has been wrapped with a zlib header/footer:
// window_bits must be __MZ_DEFAULT_WINDOW_BITS (to parse zlib header/footer) or -__MZ_DEFAULT_WINDOW_BITS (raw deflate).
int __mz_inflateInit2(__mz_streamp pStream, int window_bits);

// Decompresses the input stream to the output, consuming only as much of the input as needed, and writing as much to the output as possible.
// Parameters:
//   pStream is the stream to read from and write to. You must initialize/update the next_in, avail_in, next_out, and avail_out members.
//   flush may be __MZ_NO_FLUSH, __MZ_SYNC_FLUSH, or __MZ_FINISH.
//   On the first call, if flush is __MZ_FINISH it's assumed the input and output buffers are both sized large enough to decompress the entire stream in a single call (this is slightly faster).
//   __MZ_FINISH implies that there are no more source bytes available beside what's already in the input buffer, and that the output buffer is large enough to hold the rest of the decompressed data.
// Return values:
//   __MZ_OK on success. Either more input is needed but not available, and/or there's more output to be written but the output buffer is full.
//   __MZ_STREAM_END if all needed input has been consumed and all output bytes have been written. For zlib streams, the adler-32 of the decompressed data has also been verified.
//   __MZ_STREAM_ERROR if the stream is bogus.
//   __MZ_DATA_ERROR if the deflate stream is invalid.
//   __MZ_PARAM_ERROR if one of the parameters is invalid.
//   __MZ_BUF_ERROR if no forward progress is possible because the input buffer is empty but the inflater needs more input to continue, or if the output buffer is not large enough. Call mz_inflate() again
//   with more input data, or with more room in the output buffer (except when using single call decompression, described above).
int __mz_inflate(__mz_streamp pStream, int flush);

// Deinitializes a decompressor.
int __mz_inflateEnd(__mz_streamp pStream);

// Single-call decompression.
// Returns __MZ_OK on success, or one of the error codes from mz_inflate() on failure.
int __mz_uncompress(unsigned char *pDest, __mz_ulong *pDest_len, const unsigned char *pSource, __mz_ulong source_len);

// Returns a string description of the specified error code, or NULL if the error code is invalid.
const char *__mz_error(int err);

// Redefine zlib-compatible names to miniz equivalents, so miniz.c can be used as a drop-in replacement for the subset of zlib that miniz.c supports.
// Define __MINIZ_NO_ZLIB_COMPATIBLE_NAMES to disable zlib-compatibility if you use zlib in the same project.
#ifndef __MINIZ_NO_ZLIB_COMPATIBLE_NAMES
  typedef unsigned char Byte;
  typedef unsigned int uInt;
  typedef __mz_ulong uLong;
  typedef Byte Bytef;
  typedef uInt uIntf;
  typedef char charf;
  typedef int intf;
  typedef void *voidpf;
  typedef uLong uLongf;
  typedef void *voidp;
  //typedef void *const voidpc;
  #define __Z_NULL                0
  #define __Z_NO_FLUSH            __MZ_NO_FLUSH
  #define __Z_PARTIAL_FLUSH       __MZ_PARTIAL_FLUSH
  #define __Z_SYNC_FLUSH          __MZ_SYNC_FLUSH
  #define __Z_FULL_FLUSH          __MZ_FULL_FLUSH
  #define __Z_FINISH              __MZ_FINISH
  #define __Z_BLOCK               __MZ_BLOCK
  #define __Z_OK                  __MZ_OK
  #define __Z_STREAM_END          __MZ_STREAM_END
  #define __Z_NEED_DICT           __MZ_NEED_DICT
  #define __Z_ERRNO               __MZ_ERRNO
  #define __Z_STREAM_ERROR        __MZ_STREAM_ERROR
  #define __Z_DATA_ERROR          __MZ_DATA_ERROR
  #define __Z_MEM_ERROR           __MZ_MEM_ERROR
  #define __Z_BUF_ERROR           __MZ_BUF_ERROR
  #define __Z_VERSION_ERROR       __MZ_VERSION_ERROR
  #define __Z_PARAM_ERROR         __MZ_PARAM_ERROR
  #define __Z_NO_COMPRESSION      __MZ_NO_COMPRESSION
  #define __Z_BEST_SPEED          __MZ_BEST_SPEED
  #define __Z_BEST_COMPRESSION    __MZ_BEST_COMPRESSION
  #define __Z_DEFAULT_COMPRESSION __MZ_DEFAULT_COMPRESSION
  #define __Z_DEFAULT_STRATEGY    __MZ_DEFAULT_STRATEGY
  #define __Z_FILTERED            __MZ_FILTERED
  #define __Z_HUFFMAN_ONLY        __MZ_HUFFMAN_ONLY
  #define __Z_RLE                 __MZ_RLE
  #define __Z_FIXED               __MZ_FIXED
  #define __Z_DEFLATED            __MZ_DEFLATED
  #define __Z_DEFAULT_WINDOW_BITS __MZ_DEFAULT_WINDOW_BITS
  #define __alloc_func            __mz_alloc_func
  #define __free_func             __mz_free_func
  #define __internal_state        __mz_internal_state
  #define __z_stream              __mz_stream
  #define __deflateInit           __mz_deflateInit
  #define __deflateInit2          __mz_deflateInit2
  #define __deflateReset          __mz_deflateReset
  #define __deflate               __mz_deflate
  #define __deflateEnd            __mz_deflateEnd
  #define __deflateBound          __mz_deflateBound
  #define __compress              __mz_compress
  #define __compress2             __mz_compress2
  #define __compressBound         __mz_compressBound
  #define __inflateInit           __mz_inflateInit
  #define __inflateInit2          __mz_inflateInit2
  #define __inflate               __mz_inflate
  #define __inflateEnd            __mz_inflateEnd
  #define __uncompress            __mz_uncompress
  #define __crc32                 __mz_crc32
  #define __adler32               __mz_adler32
  #define __MAX_WBITS             15
  #define __MAX_MEM_LEVEL         9
  #define __zError                __mz_error
  #define __ZLIB_VERSION          __MZ_VERSION
  #define __ZLIB_VERNUM           __MZ_VERNUM
  #define __ZLIB_VER_MAJOR        __MZ_VER_MAJOR
  #define __ZLIB_VER_MINOR        __MZ_VER_MINOR
  #define __ZLIB_VER_REVISION     __MZ_VER_REVISION
  #define __ZLIB_VER_SUBREVISION  __MZ_VER_SUBREVISION
  #define __zlibVersion           __mz_version
  #define __zlib_version          __mz_version()
#endif // #ifndef __MINIZ_NO_ZLIB_COMPATIBLE_NAMES

#endif // __MINIZ_NO_ZLIB_APIS

// ------------------- Types and macros

typedef unsigned char __mz_uint8;
typedef signed short __mz_int16;
typedef unsigned short __mz_uint16;
typedef unsigned int __mz_uint32;
typedef unsigned int __mz_uint;
typedef long long __mz_int64;
typedef unsigned long long __mz_uint64;
typedef int __mz_bool;

#define __MZ_FALSE (0)
#define __MZ_TRUE (1)

// An attempt to work around MSVC's spammy "warning C4127: conditional expression is constant" message.
#ifdef _MSC_VER
   #define __MZ_MACRO_END while (0, 0)
#else
   #define __MZ_MACRO_END while (0)
#endif

// ------------------- ZIP archive reading/writing

#ifndef __MINIZ_NO_ARCHIVE_APIS

enum
{
  __MZ_ZIP_MAX_IO_BUF_SIZE = 64*1024,
  __MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE = 260,
  __MZ_ZIP_MAX_ARCHIVE_FILE_COMMENT_SIZE = 256
};

typedef struct
{
  __mz_uint32 m_file_index;
  __mz_uint32 m_central_dir_ofs;
  __mz_uint16 m_version_made_by;
  __mz_uint16 m_version_needed;
  __mz_uint16 m_bit_flag;
  __mz_uint16 m_method;
#ifndef __MINIZ_NO_TIME
  time_t m_time;
#endif
  __mz_uint32 m_crc32;
  __mz_uint64 m_comp_size;
  __mz_uint64 m_uncomp_size;
  __mz_uint16 m_internal_attr;
  __mz_uint32 m_external_attr;
  __mz_uint64 m_local_header_ofs;
  __mz_uint32 m_comment_size;
  char m_filename[__MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE];
  char m_comment[__MZ_ZIP_MAX_ARCHIVE_FILE_COMMENT_SIZE];
} __mz_zip_archive_file_stat;

typedef size_t (*__mz_file_read_func)(void *pOpaque, __mz_uint64 file_ofs, void *pBuf, size_t n);
typedef size_t (*__mz_file_write_func)(void *pOpaque, __mz_uint64 file_ofs, const void *pBuf, size_t n);

struct __mz_zip_internal_state_tag;
typedef struct __mz_zip_internal_state_tag __mz_zip_internal_state;

typedef enum
{
  __MZ_ZIP_MODE_INVALID = 0,
  __MZ_ZIP_MODE_READING = 1,
  __MZ_ZIP_MODE_WRITING = 2,
  __MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED = 3
} __mz_zip_mode;

typedef struct __mz_zip_archive_tag
{
  __mz_uint64 m_archive_size;
  __mz_uint64 m_central_directory_file_ofs;
  __mz_uint m_total_files;
  __mz_zip_mode m_zip_mode;

  __mz_uint m_file_offset_alignment;

  __mz_alloc_func m_pAlloc;
  __mz_free_func m_pFree;
  __mz_realloc_func m_pRealloc;
  void *m_pAlloc_opaque;

  __mz_file_read_func m_pRead;
  __mz_file_write_func m_pWrite;
  void *m_pIO_opaque;

  __mz_zip_internal_state *m_pState;

} __mz_zip_archive;

typedef enum
{
  __MZ_ZIP_FLAG_CASE_SENSITIVE                = 0x0100,
  __MZ_ZIP_FLAG_IGNORE_PATH                   = 0x0200,
  __MZ_ZIP_FLAG_COMPRESSED_DATA               = 0x0400,
  __MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY = 0x0800
} __mz_zip_flags;

// ZIP archive reading

// Inits a ZIP archive reader.
// These functions read and validate the archive's central directory.
__mz_bool __mz_zip_reader_init(__mz_zip_archive *pZip, __mz_uint64 size, __mz_uint32 flags);
__mz_bool __mz_zip_reader_init_mem(__mz_zip_archive *pZip, const void *pMem, size_t size, __mz_uint32 flags);

#ifndef __MINIZ_NO_STDIO
__mz_bool __mz_zip_reader_init_file(__mz_zip_archive *pZip, const char *pFilename, __mz_uint32 flags);
#endif

// Returns the total number of files in the archive.
__mz_uint __mz_zip_reader_get_num_files(__mz_zip_archive *pZip);

// Returns detailed information about an archive file entry.
__mz_bool __mz_zip_reader_file_stat(__mz_zip_archive *pZip, __mz_uint file_index, __mz_zip_archive_file_stat *pStat);

// Determines if an archive file entry is a directory entry.
__mz_bool __mz_zip_reader_is_file_a_directory(__mz_zip_archive *pZip, __mz_uint file_index);
__mz_bool __mz_zip_reader_is_file_encrypted(__mz_zip_archive *pZip, __mz_uint file_index);

// Retrieves the filename of an archive file entry.
// Returns the number of bytes written to pFilename, or if filename_buf_size is 0 this function returns the number of bytes needed to fully store the filename.
__mz_uint __mz_zip_reader_get_filename(__mz_zip_archive *pZip, __mz_uint file_index, char *pFilename, __mz_uint filename_buf_size);

// Attempts to locates a file in the archive's central directory.
// Valid flags: MZ_ZIP_FLAG_CASE_SENSITIVE, MZ_ZIP_FLAG_IGNORE_PATH
// Returns -1 if the file cannot be found.
int __mz_zip_reader_locate_file(__mz_zip_archive *pZip, const char *pName, const char *pComment, __mz_uint flags);

// Extracts a archive file to a memory buffer using no memory allocation.
__mz_bool __mz_zip_reader_extract_to_mem_no_alloc(__mz_zip_archive *pZip, __mz_uint file_index, void *pBuf, size_t buf_size, __mz_uint flags, void *pUser_read_buf, size_t user_read_buf_size);
__mz_bool __mz_zip_reader_extract_file_to_mem_no_alloc(__mz_zip_archive *pZip, const char *pFilename, void *pBuf, size_t buf_size, __mz_uint flags, void *pUser_read_buf, size_t user_read_buf_size);

// Extracts a archive file to a memory buffer.
__mz_bool __mz_zip_reader_extract_to_mem(__mz_zip_archive *pZip, __mz_uint file_index, void *pBuf, size_t buf_size, __mz_uint flags);
__mz_bool __mz_zip_reader_extract_file_to_mem(__mz_zip_archive *pZip, const char *pFilename, void *pBuf, size_t buf_size, __mz_uint flags);

// Extracts a archive file to a dynamically allocated heap buffer.
void *__mz_zip_reader_extract_to_heap(__mz_zip_archive *pZip, __mz_uint file_index, size_t *pSize, __mz_uint flags);
void *__mz_zip_reader_extract_file_to_heap(__mz_zip_archive *pZip, const char *pFilename, size_t *pSize, __mz_uint flags);

// Extracts a archive file using a callback function to output the file's data.
__mz_bool __mz_zip_reader_extract_to_callback(__mz_zip_archive *pZip, __mz_uint file_index, __mz_file_write_func pCallback, void *pOpaque, __mz_uint flags);
__mz_bool __mz_zip_reader_extract_file_to_callback(__mz_zip_archive *pZip, const char *pFilename, __mz_file_write_func pCallback, void *pOpaque, __mz_uint flags);

#ifndef __MINIZ_NO_STDIO
// Extracts a archive file to a disk file and sets its last accessed and modified times.
// This function only extracts files, not archive directory records.
__mz_bool __mz_zip_reader_extract_to_file(__mz_zip_archive *pZip, __mz_uint file_index, const char *pDst_filename, __mz_uint flags);
__mz_bool __mz_zip_reader_extract_file_to_file(__mz_zip_archive *pZip, const char *pArchive_filename, const char *pDst_filename, __mz_uint flags);
#endif

// Ends archive reading, freeing all allocations, and closing the input archive file if mz_zip_reader_init_file() was used.
__mz_bool __mz_zip_reader_end(__mz_zip_archive *pZip);

// ZIP archive writing

#ifndef __MINIZ_NO_ARCHIVE_WRITING_APIS

// Inits a ZIP archive writer.
__mz_bool __mz_zip_writer_init(__mz_zip_archive *pZip, __mz_uint64 existing_size);
__mz_bool __mz_zip_writer_init_heap(__mz_zip_archive *pZip, size_t size_to_reserve_at_beginning, size_t initial_allocation_size);

#ifndef __MINIZ_NO_STDIO
__mz_bool __mz_zip_writer_init_file(__mz_zip_archive *pZip, const char *pFilename, __mz_uint64 size_to_reserve_at_beginning);
#endif

// Converts a ZIP archive reader object into a writer object, to allow efficient in-place file appends to occur on an existing archive.
// For archives opened using mz_zip_reader_init_file, pFilename must be the archive's filename so it can be reopened for writing. If the file can't be reopened, mz_zip_reader_end() will be called.
// For archives opened using mz_zip_reader_init_mem, the memory block must be growable using the realloc callback (which defaults to realloc unless you've overridden it).
// Finally, for archives opened using mz_zip_reader_init, the mz_zip_archive's user provided m_pWrite function cannot be NULL.
// Note: In-place archive modification is not recommended unless you know what you're doing, because if execution stops or something goes wrong before
// the archive is finalized the file's central directory will be hosed.
__mz_bool __mz_zip_writer_init_from_reader(__mz_zip_archive *pZip, const char *pFilename);

// Adds the contents of a memory buffer to an archive. These functions record the current local time into the archive.
// To add a directory entry, call this method with an archive name ending in a forwardslash with empty buffer.
// level_and_flags - compression level (0-10, see __MZ_BEST_SPEED, __MZ_BEST_COMPRESSION, etc.) logically OR'd with zero or more mz_zip_flags, or just set to __MZ_DEFAULT_COMPRESSION.
__mz_bool __mz_zip_writer_add_mem(__mz_zip_archive *pZip, const char *pArchive_name, const void *pBuf, size_t buf_size, __mz_uint level_and_flags);
__mz_bool __mz_zip_writer_add_mem_ex(__mz_zip_archive *pZip, const char *pArchive_name, const void *pBuf, size_t buf_size, const void *pComment, __mz_uint16 comment_size, __mz_uint level_and_flags, __mz_uint64 uncomp_size, __mz_uint32 uncomp_crc32);

#ifndef __MINIZ_NO_STDIO
// Adds the contents of a disk file to an archive. This function also records the disk file's modified time into the archive.
// level_and_flags - compression level (0-10, see __MZ_BEST_SPEED, __MZ_BEST_COMPRESSION, etc.) logically OR'd with zero or more mz_zip_flags, or just set to __MZ_DEFAULT_COMPRESSION.
__mz_bool __mz_zip_writer_add_file(__mz_zip_archive *pZip, const char *pArchive_name, const char *pSrc_filename, const void *pComment, __mz_uint16 comment_size, __mz_uint level_and_flags);
#endif

// Adds a file to an archive by fully cloning the data from another archive.
// This function fully clones the source file's compressed data (no recompression), along with its full filename, extra data, and comment fields.
__mz_bool __mz_zip_writer_add_from_zip_reader(__mz_zip_archive *pZip, __mz_zip_archive *pSource_zip, __mz_uint file_index);

// Finalizes the archive by writing the central directory records followed by the end of central directory record.
// After an archive is finalized, the only valid call on the mz_zip_archive struct is mz_zip_writer_end().
// An archive must be manually finalized by calling this function for it to be valid.
__mz_bool __mz_zip_writer_finalize_archive(__mz_zip_archive *pZip);
__mz_bool __mz_zip_writer_finalize_heap_archive(__mz_zip_archive *pZip, void **pBuf, size_t *pSize);

// Ends archive writing, freeing all allocations, and closing the output file if mz_zip_writer_init_file() was used.
// Note for the archive to be valid, it must have been finalized before ending.
__mz_bool __mz_zip_writer_end(__mz_zip_archive *pZip);

// Misc. high-level helper functions:

// mz_zip_add_mem_to_archive_file_in_place() efficiently (but not atomically) appends a memory blob to a ZIP archive.
// level_and_flags - compression level (0-10, see __MZ_BEST_SPEED, __MZ_BEST_COMPRESSION, etc.) logically OR'd with zero or more mz_zip_flags, or just set to __MZ_DEFAULT_COMPRESSION.
__mz_bool __mz_zip_add_mem_to_archive_file_in_place(const char *pZip_filename, const char *pArchive_name, const void *pBuf, size_t buf_size, const void *pComment, __mz_uint16 comment_size, __mz_uint level_and_flags);

// Reads a single file from an archive into a heap block.
// Returns NULL on failure.
void *__mz_zip_extract_archive_file_to_heap(const char *pZip_filename, const char *pArchive_name, size_t *pSize, __mz_uint zip_flags);

#endif // #ifndef __MINIZ_NO_ARCHIVE_WRITING_APIS

#endif // #ifndef __MINIZ_NO_ARCHIVE_APIS

// ------------------- Low-level Decompression API Definitions

// Decompression flags used by tinfl_decompress().
// TINFL_FLAG_PARSE_ZLIB_HEADER: If set, the input has a valid zlib header and ends with an adler32 checksum (it's a valid zlib stream). Otherwise, the input is a raw deflate stream.
// __TINFL_FLAG_HAS_MORE_INPUT: If set, there are more input bytes available beyond the end of the supplied input buffer. If clear, the input buffer contains all remaining input.
// TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF: If set, the output buffer is large enough to hold the entire decompressed stream. If clear, the output buffer is at least the size of the dictionary (typically 32KB).
// TINFL_FLAG_COMPUTE_ADLER32: Force adler-32 checksum computation of the decompressed bytes.
enum
{
  __TINFL_FLAG_PARSE_ZLIB_HEADER = 1,
  __TINFL_FLAG_HAS_MORE_INPUT = 2,
  __TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF = 4,
  __TINFL_FLAG_COMPUTE_ADLER32 = 8
};

// High level decompression functions:
// tinfl_decompress_mem_to_heap() decompresses a block in memory to a heap block allocated via malloc().
// On entry:
//  pSrc_buf, src_buf_len: Pointer and size of the Deflate or zlib source data to decompress.
// On return:
//  Function returns a pointer to the decompressed data, or NULL on failure.
//  *pOut_len will be set to the decompressed data's size, which could be larger than src_buf_len on uncompressible data.
//  The caller must call mz_free() on the returned block when it's no longer needed.
void *__tinfl_decompress_mem_to_heap(const void *pSrc_buf, size_t src_buf_len, size_t *pOut_len, int flags);

// tinfl_decompress_mem_to_mem() decompresses a block in memory to another block in memory.
// Returns __TINFL_DECOMPRESS_MEM_TO_MEM_FAILED on failure, or the number of bytes written on success.
#define __TINFL_DECOMPRESS_MEM_TO_MEM_FAILED ((size_t)(-1))
size_t __tinfl_decompress_mem_to_mem(void *pOut_buf, size_t out_buf_len, const void *pSrc_buf, size_t src_buf_len, int flags);

// tinfl_decompress_mem_to_callback() decompresses a block in memory to an internal 32KB buffer, and a user provided callback function will be called to flush the buffer.
// Returns 1 on success or 0 on failure.
typedef int (*__tinfl_put_buf_func_ptr)(const void* pBuf, int len, void *pUser);
int __tinfl_decompress_mem_to_callback(const void *pIn_buf, size_t *pIn_buf_size, __tinfl_put_buf_func_ptr pPut_buf_func, void *pPut_buf_user, int flags);

struct __tinfl_decompressor_tag; typedef struct __tinfl_decompressor_tag __tinfl_decompressor;

// Max size of LZ dictionary.
#define __TINFL_LZ_DICT_SIZE 32768

// Return status.
typedef enum
{
  __TINFL_STATUS_BAD_PARAM = -3,
  __TINFL_STATUS_ADLER32_MISMATCH = -2,
  __TINFL_STATUS_FAILED = -1,
  __TINFL_STATUS_DONE = 0,
  __TINFL_STATUS_NEEDS_MORE_INPUT = 1,
  __TINFL_STATUS_HAS_MORE_OUTPUT = 2
} __tinfl_status;

// Initializes the decompressor to its initial state.
#define __tinfl_init(r) do { (r)->m_state = 0; } __MZ_MACRO_END
#define __tinfl_get_adler32(r) (r)->m_check_adler32

// Main low-level decompressor coroutine function. This is the only function actually needed for decompression. All the other functions are just high-level helpers for improved usability.
// This is a universal API, i.e. it can be used as a building block to build any desired higher level decompression API. In the limit case, it can be called once per every byte input or output.
__tinfl_status __tinfl_decompress(__tinfl_decompressor *r, const __mz_uint8 *pIn_buf_next, size_t *pIn_buf_size, __mz_uint8 *pOut_buf_start, __mz_uint8 *pOut_buf_next, size_t *pOut_buf_size, const __mz_uint32 decomp_flags);

// Internal/private bits follow.
enum
{
  __TINFL_MAX_HUFF_TABLES = 3, __TINFL_MAX_HUFF_SYMBOLS_0 = 288, __TINFL_MAX_HUFF_SYMBOLS_1 = 32, __TINFL_MAX_HUFF_SYMBOLS_2 = 19,
  __TINFL_FAST_LOOKUP_BITS = 10, __TINFL_FAST_LOOKUP_SIZE = 1 << __TINFL_FAST_LOOKUP_BITS
};

typedef struct
{
  __mz_uint8 m_code_size[__TINFL_MAX_HUFF_SYMBOLS_0];
  __mz_int16 m_look_up[__TINFL_FAST_LOOKUP_SIZE], m_tree[__TINFL_MAX_HUFF_SYMBOLS_0 * 2];
} __tinfl_huff_table;

#if __MINIZ_HAS_64BIT_REGISTERS
#define __TINFL_USE_64BIT_BITBUF 1
#else 
#define __TINFL_USE_64BIT_BITBUF 0
#endif

#if __TINFL_USE_64BIT_BITBUF
  typedef __mz_uint64 __tinfl_bit_buf_t;
  #define __TINFL_BITBUF_SIZE (64)
#else
  typedef __mz_uint32 __tinfl_bit_buf_t;
  #define __TINFL_BITBUF_SIZE (32)
#endif

struct __tinfl_decompressor_tag
{
  __mz_uint32 m_state, m_num_bits, m_zhdr0, m_zhdr1, m_z_adler32, m_final, m_type, m_check_adler32, m_dist, m_counter, m_num_extra, m_table_sizes[__TINFL_MAX_HUFF_TABLES];
  __tinfl_bit_buf_t m_bit_buf;
  size_t m_dist_from_out_buf_start;
  __tinfl_huff_table m_tables[__TINFL_MAX_HUFF_TABLES];
  __mz_uint8 m_raw_header[4], m_len_codes[__TINFL_MAX_HUFF_SYMBOLS_0 + __TINFL_MAX_HUFF_SYMBOLS_1 + 137];
};

// ------------------- Low-level Compression API Definitions

// Set __TDEFL_LESS_MEMORY to 1 to use less memory (compression will be slightly slower, and raw/dynamic blocks will be output more frequently).
#define __TDEFL_LESS_MEMORY 0

// tdefl_init() compression flags logically OR'd together (low 12 bits contain the max. number of probes per dictionary search):
// TDEFL_DEFAULT_MAX_PROBES: The compressor defaults to 128 dictionary probes per dictionary search. 0=Huffman only, 1=Huffman+LZ (fastest/crap compression), 4095=Huffman+LZ (slowest/best compression).
enum
{
  __TDEFL_HUFFMAN_ONLY = 0, __TDEFL_DEFAULT_MAX_PROBES = 128, __TDEFL_MAX_PROBES_MASK = 0xFFF
};

// TDEFL_WRITE_ZLIB_HEADER: If set, the compressor outputs a zlib header before the deflate data, and the Adler-32 of the source data at the end. Otherwise, you'll get raw deflate data.
// TDEFL_COMPUTE_ADLER32: Always compute the adler-32 of the input data (even when not writing zlib headers).
// TDEFL_GREEDY_PARSING_FLAG: Set to use faster greedy parsing, instead of more efficient lazy parsing.
// TDEFL_NONDETERMINISTIC_PARSING_FLAG: Enable to decrease the compressor's initialization time to the minimum, but the output may vary from run to run given the same input (depending on the contents of memory).
// TDEFL_RLE_MATCHES: Only look for RLE matches (matches with a distance of 1)
// TDEFL_FILTER_MATCHES: Discards matches <= 5 chars if enabled.
// TDEFL_FORCE_ALL_STATIC_BLOCKS: Disable usage of optimized Huffman tables.
// TDEFL_FORCE_ALL_RAW_BLOCKS: Only use raw (uncompressed) deflate blocks.
// The low 12 bits are reserved to control the max # of hash probes per dictionary lookup (see TDEFL_MAX_PROBES_MASK).
enum
{
  __TDEFL_WRITE_ZLIB_HEADER             = 0x01000,
  __TDEFL_COMPUTE_ADLER32               = 0x02000,
  __TDEFL_GREEDY_PARSING_FLAG           = 0x04000,
  __TDEFL_NONDETERMINISTIC_PARSING_FLAG = 0x08000,
  __TDEFL_RLE_MATCHES                   = 0x10000,
  __TDEFL_FILTER_MATCHES                = 0x20000,
  __TDEFL_FORCE_ALL_STATIC_BLOCKS       = 0x40000,
  __TDEFL_FORCE_ALL_RAW_BLOCKS          = 0x80000
};

// High level compression functions:
// tdefl_compress_mem_to_heap() compresses a block in memory to a heap block allocated via malloc().
// On entry:
//  pSrc_buf, src_buf_len: Pointer and size of source block to compress.
//  flags: The max match finder probes (default is 128) logically OR'd against the above flags. Higher probes are slower but improve compression.
// On return:
//  Function returns a pointer to the compressed data, or NULL on failure.
//  *pOut_len will be set to the compressed data's size, which could be larger than src_buf_len on uncompressible data.
//  The caller must free() the returned block when it's no longer needed.
void *__tdefl_compress_mem_to_heap(const void *pSrc_buf, size_t src_buf_len, size_t *pOut_len, int flags);

// tdefl_compress_mem_to_mem() compresses a block in memory to another block in memory.
// Returns 0 on failure.
size_t __tdefl_compress_mem_to_mem(void *pOut_buf, size_t out_buf_len, const void *pSrc_buf, size_t src_buf_len, int flags);

// Compresses an image to a compressed PNG file in memory.
// On entry:
//  pImage, w, h, and num_chans describe the image to compress. num_chans may be 1, 2, 3, or 4. 
//  The image pitch in bytes per scanline will be w*num_chans. The leftmost pixel on the top scanline is stored first in memory.
//  level may range from [0,10], use __MZ_NO_COMPRESSION, __MZ_BEST_SPEED, __MZ_BEST_COMPRESSION, etc. or a decent default is MZ_DEFAULT_LEVEL
//  If flip is true, the image will be flipped on the Y axis (useful for OpenGL apps).
// On return:
//  Function returns a pointer to the compressed data, or NULL on failure.
//  *pLen_out will be set to the size of the PNG image file.
//  The caller must mz_free() the returned heap block (which will typically be larger than *pLen_out) when it's no longer needed.
void *__tdefl_write_image_to_png_file_in_memory_ex(const void *pImage, int w, int h, int num_chans, size_t *pLen_out, __mz_uint level, __mz_bool flip);
void *__tdefl_write_image_to_png_file_in_memory(const void *pImage, int w, int h, int num_chans, size_t *pLen_out);

// Output stream interface. The compressor uses this interface to write compressed data. It'll typically be called TDEFL_OUT_BUF_SIZE at a time.
typedef __mz_bool (*__tdefl_put_buf_func_ptr)(const void* pBuf, int len, void *pUser);

// tdefl_compress_mem_to_output() compresses a block to an output stream. The above helpers use this function internally.
__mz_bool __tdefl_compress_mem_to_output(const void *pBuf, size_t buf_len, __tdefl_put_buf_func_ptr pPut_buf_func, void *pPut_buf_user, int flags);

enum { __TDEFL_MAX_HUFF_TABLES = 3, __TDEFL_MAX_HUFF_SYMBOLS_0 = 288, __TDEFL_MAX_HUFF_SYMBOLS_1 = 32, __TDEFL_MAX_HUFF_SYMBOLS_2 = 19, __TDEFL_LZ_DICT_SIZE = 32768, __TDEFL_LZ_DICT_SIZE_MASK = __TDEFL_LZ_DICT_SIZE - 1, __TDEFL_MIN_MATCH_LEN = 3, __TDEFL_MAX_MATCH_LEN = 258 };

// TDEFL_OUT_BUF_SIZE MUST be large enough to hold a single entire compressed output block (using static/fixed Huffman codes).
#if __TDEFL_LESS_MEMORY
enum { __TDEFL_LZ_CODE_BUF_SIZE = 24 * 1024, __TDEFL_OUT_BUF_SIZE = (__TDEFL_LZ_CODE_BUF_SIZE * 13 ) / 10, __TDEFL_MAX_HUFF_SYMBOLS = 288, __TDEFL_LZ_HASH_BITS = 12, __TDEFL_LEVEL1_HASH_SIZE_MASK = 4095, __TDEFL_LZ_HASH_SHIFT = (__TDEFL_LZ_HASH_BITS + 2) / 3, __TDEFL_LZ_HASH_SIZE = 1 << __TDEFL_LZ_HASH_BITS };
#else
enum { __TDEFL_LZ_CODE_BUF_SIZE = 64 * 1024, __TDEFL_OUT_BUF_SIZE = (__TDEFL_LZ_CODE_BUF_SIZE * 13 ) / 10, __TDEFL_MAX_HUFF_SYMBOLS = 288, __TDEFL_LZ_HASH_BITS = 15, __TDEFL_LEVEL1_HASH_SIZE_MASK = 4095, __TDEFL_LZ_HASH_SHIFT = (__TDEFL_LZ_HASH_BITS + 2) / 3, __TDEFL_LZ_HASH_SIZE = 1 << __TDEFL_LZ_HASH_BITS };
#endif

// The low-level tdefl functions below may be used directly if the above helper functions aren't flexible enough. The low-level functions don't make any heap allocations, unlike the above helper functions.
typedef enum
{
  __TDEFL_STATUS_BAD_PARAM = -2,
  __TDEFL_STATUS_PUT_BUF_FAILED = -1,
  __TDEFL_STATUS_OKAY = 0,
  __TDEFL_STATUS_DONE = 1,
} __tdefl_status;

// Must map to __MZ_NO_FLUSH, __MZ_SYNC_FLUSH, etc. enums
typedef enum
{
  __TDEFL_NO_FLUSH = 0,
  __TDEFL_SYNC_FLUSH = 2,
  __TDEFL_FULL_FLUSH = 3,
  __TDEFL_FINISH = 4
} __tdefl_flush;

// tdefl's compression state structure.
typedef struct
{
  __tdefl_put_buf_func_ptr m_pPut_buf_func;
  void *m_pPut_buf_user;
  __mz_uint m_flags, m_max_probes[2];
  int m_greedy_parsing;
  __mz_uint m_adler32, m_lookahead_pos, m_lookahead_size, m_dict_size;
  __mz_uint8 *m_pLZ_code_buf, *m_pLZ_flags, *m_pOutput_buf, *m_pOutput_buf_end;
  __mz_uint m_num_flags_left, m_total_lz_bytes, m_lz_code_buf_dict_pos, m_bits_in, m_bit_buffer;
  __mz_uint m_saved_match_dist, m_saved_match_len, m_saved_lit, m_output_flush_ofs, m_output_flush_remaining, m_finished, m_block_index, m_wants_to_finish;
  __tdefl_status m_prev_return_status;
  const void *m_pIn_buf;
  void *m_pOut_buf;
  size_t *m_pIn_buf_size, *m_pOut_buf_size;
  __tdefl_flush m_flush;
  const __mz_uint8 *m_pSrc;
  size_t m_src_buf_left, m_out_buf_ofs;
  __mz_uint8 m_dict[__TDEFL_LZ_DICT_SIZE + __TDEFL_MAX_MATCH_LEN - 1];
  __mz_uint16 m_huff_count[__TDEFL_MAX_HUFF_TABLES][__TDEFL_MAX_HUFF_SYMBOLS];
  __mz_uint16 m_huff_codes[__TDEFL_MAX_HUFF_TABLES][__TDEFL_MAX_HUFF_SYMBOLS];
  __mz_uint8 m_huff_code_sizes[__TDEFL_MAX_HUFF_TABLES][__TDEFL_MAX_HUFF_SYMBOLS];
  __mz_uint8 m_lz_code_buf[__TDEFL_LZ_CODE_BUF_SIZE];
  __mz_uint16 m_next[__TDEFL_LZ_DICT_SIZE];
  __mz_uint16 m_hash[__TDEFL_LZ_HASH_SIZE];
  __mz_uint8 m_output_buf[__TDEFL_OUT_BUF_SIZE];
} __tdefl_compressor;

// Initializes the compressor.
// There is no corresponding deinit() function because the tdefl API's do not dynamically allocate memory.
// pBut_buf_func: If NULL, output data will be supplied to the specified callback. In this case, the user should call the tdefl_compress_buffer() API for compression.
// If pBut_buf_func is NULL the user should always call the tdefl_compress() API.
// flags: See the above enums (TDEFL_HUFFMAN_ONLY, TDEFL_WRITE_ZLIB_HEADER, etc.)
__tdefl_status __tdefl_init(__tdefl_compressor *d, __tdefl_put_buf_func_ptr pPut_buf_func, void *pPut_buf_user, int flags);

// Compresses a block of data, consuming as much of the specified input buffer as possible, and writing as much compressed data to the specified output buffer as possible.
__tdefl_status __tdefl_compress(__tdefl_compressor *d, const void *pIn_buf, size_t *pIn_buf_size, void *pOut_buf, size_t *pOut_buf_size, __tdefl_flush flush);

// tdefl_compress_buffer() is only usable when the tdefl_init() is called with a non-NULL tdefl_put_buf_func_ptr.
// tdefl_compress_buffer() always consumes the entire input buffer.
__tdefl_status __tdefl_compress_buffer(__tdefl_compressor *d, const void *pIn_buf, size_t in_buf_size, __tdefl_flush flush);

__tdefl_status __tdefl_get_prev_return_status(__tdefl_compressor *d);
__mz_uint32 __tdefl_get_adler32(__tdefl_compressor *d);

// Can't use tdefl_create_comp_flags_from_zip_params if __MINIZ_NO_ZLIB_APIS isn't defined, because it uses some of its macros.
#ifndef __MINIZ_NO_ZLIB_APIS
// Create tdefl_compress() flags given zlib-style compression parameters.
// level may range from [0,10] (where 10 is absolute max compression, but may be much slower on some files)
// window_bits may be -15 (raw deflate) or 15 (zlib)
// strategy may be either __MZ_DEFAULT_STRATEGY, __MZ_FILTERED, __MZ_HUFFMAN_ONLY, __MZ_RLE, or __MZ_FIXED
__mz_uint __tdefl_create_comp_flags_from_zip_params(int level, int window_bits, int strategy);
#endif // #ifndef __MINIZ_NO_ZLIB_APIS

#ifdef __cplusplus
}
#endif

#endif // MINIZ_HEADER_INCLUDED
// ------------------- End of Header: Implementation follows. (If you only want the header, define MINIZ_HEADER_FILE_ONLY.)
