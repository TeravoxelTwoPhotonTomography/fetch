#pragma once
#ifdef __cplusplus
typedef mylib::stream_mode_t stream_mode_t;
typedef mylib::stream_t stream_t;

extern "C" {
#endif

#include <MY_TIFF/stream.h>

stream_t native_buffered_stream_open(const char *filename,stream_mode_t mode);
int      native_buffered_stream_reserve(stream_t stream, size_t nbytes);
int      native_buffered_stream_flush(stream_t stream); ///< Write in-memory contents to disk.  Returns 1 on success, 0 otherwise.  No backwards seek should be done after a flush.  Flush is called on stream_close(), but any errors are ignored.

// Customizable memory interface
typedef void* (*native_buffered_stream_malloc_func)(size_t nbytes);
typedef void* (*native_buffered_stream_realloc_func)(void* ptr,size_t nbytes);
typedef void  (*native_buffered_stream_free_func)(void* ptr);

void native_buffered_stream_set_malloc_func (stream_t stream, native_buffered_stream_malloc_func f);
void native_buffered_stream_set_realloc_func(stream_t stream, native_buffered_stream_realloc_func f);
void native_buffered_stream_set_free_func   (stream_t stream, native_buffered_stream_free_func f);

#ifdef __cplusplus
}
#endif