#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "native-buffered-stream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
TODO
  - open file
  - close file

  Not a priority
  - read mode
  - read/write (append) mode
 */
#define NTHREADS (16ULL)

#if 0
#define ECHO(estr)   LOG("---%30s()\t%s\n",__FUNCTION__,estr)
#else
#define ECHO(estr)
#endif
#define LOG(...)     debug(__VA_ARGS__)
#define REPORT(estr,msg) LOG("%s(%d): %s()\n\t%s\n\t%s\n",__FILE__,__LINE__,__FUNCTION__,estr,msg)
#define TRY(e)       do{ECHO(#e);if(!(e)){REPORT(#e,"Evaluated to false.");goto Error;}}while(0)
#define FAIL(msg)    do{ REPORT("Something went wrong.",msg); goto Error; }while(0)
#define NEW(T,e,N)   TRY((e)=(T*)malloc(sizeof(T)*(N)))
#define ZERO(T,e,N)  memset((e),0,sizeof(T)*(N))
#define countof(e)   (sizeof(e)/sizeof(*(e)))

#define NBS_NEW(T,e,N)     TRY((e)=(T*)(ctx)->malloc(sizeof(T)*(N)))
#define NBS_REALLOC(T,e,N) TRY((e)=(T*)(ctx)->realloc((e),sizeof(T)*(N)))

/** Global error flag will record if a deferred write fails.
    A failure causes future nbs_open calls to fail.
*/
static struct nbs_error_t
{ SRWLOCK lock;
  unsigned eflag;
} nbs_error_g={SRWLOCK_INIT,0};

static void nbs_errset()
{ AcquireSRWLockExclusive(&nbs_error_g.lock);
  nbs_error_g.eflag=1;
  ReleaseSRWLockExclusive(&nbs_error_g.lock);
}
static unsigned nbs_isok()
{ unsigned isok;
  AcquireSRWLockShared(&nbs_error_g.lock);
  isok=nbs_error_g.eflag==0;
  ReleaseSRWLockShared(&nbs_error_g.lock);
  return isok;
}

typedef struct _nbs_stream_t
{ native_buffered_stream_malloc_func  malloc;
  native_buffered_stream_realloc_func realloc;
  native_buffered_stream_free_func    free;
  HANDLE        fd;
  size_t        pos,len,cap; // position, length, capacity
  stream_mode_t mode;
  HANDLE        evts      [NTHREADS];
  OVERLAPPED    overlapped[NTHREADS];
  void          *buf;
} *nbs_stream_t;

static size_t nbs_read    (void* ptr,size_t size,size_t count, stream_t stream);
static size_t nbs_write   (const void * ptr, size_t size, size_t count, stream_t stream);
static int    nbs_seek    (stream_t stream, off_t offset, stream_seek_t origin);
static off_t  nbs_tell    (stream_t stream);
static int    nbs_truncate(stream_t stream, off_t length);
static void   nbs_close   (stream_t stream);

#if 1
#include "common.h"
#define TIME(e) do{ TicTocTimer t=tic(); e; LOG("[TIME] %10.6f msec\t%s\n",toc(&t)*1000.0,#e); }while(0)
#else
#define TIME(e) e
#endif
stream_t native_buffered_stream_open(const char *filename,stream_mode_t mode)
{ stream_t self=0;
  nbs_stream_t ctx=0;
  int i;
  TRY(nbs_isok());
  NEW(struct _nbs_stream_t,ctx,1);
  ZERO(struct _nbs_stream_t,ctx,1);
  switch(mode)
  { case STREAM_MODE_WRITE:
      TIME( TRY(ctx->fd=CreateFile(filename,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,NULL)) );
      break;
    default:
      FAIL("Not implemented");
  }
  for(i=0;i<NTHREADS;++i)
    TRY(ctx->evts[i]=CreateEvent(NULL,TRUE,FALSE,NULL));
  for(i=0;i<NTHREADS;++i)
    ctx->overlapped[i].hEvent=ctx->evts[i];
  ctx->mode=mode;
  TRY(self=stream_create());
  stream_set_user_data    (self,(void*)ctx,sizeof(struct _nbs_stream_t));
  stream_set_read_func    (self,nbs_read);
  stream_set_write_func   (self,nbs_write);
  stream_set_seek_func    (self,nbs_seek);
  stream_set_tell_func    (self,nbs_tell);
  stream_set_truncate_func(self,nbs_truncate);
  stream_set_close_func   (self,nbs_close);
  native_buffered_stream_set_malloc_func  (self, malloc);
  native_buffered_stream_set_realloc_func (self, realloc);
  native_buffered_stream_set_free_func    (self, free);
  return self;
Error:
  if(self) stream_close(self);
  if(ctx)  free(ctx);
  return 0;
}
#undef TIME

#define DECL_CTX nbs_stream_t ctx=(nbs_stream_t)stream_get_user_data(stream,NULL)

void native_buffered_stream_set_malloc_func (stream_t stream, native_buffered_stream_malloc_func f)  {DECL_CTX;ctx->malloc=f;}
void native_buffered_stream_set_realloc_func(stream_t stream, native_buffered_stream_realloc_func f) {DECL_CTX;ctx->realloc=f;}
void native_buffered_stream_set_free_func   (stream_t stream, native_buffered_stream_free_func f)    {DECL_CTX;ctx->free=f;}

//
// --- PRIVATE HELPERS ---
//

static int nbs_maybe_resize(nbs_stream_t ctx, size_t request)
{ size_t c=4096*( ((size_t)(1.2f*request+50.0f)/4096)+1 ); // geometric increase, page aligned
  if(request>ctx->cap)
  { NBS_REALLOC(char,ctx->buf,c);
    ctx->cap=c;
  }
  return 1;
Error:
  return 0;
}

static void CALLBACK done(DWORD ecode, DWORD bytes, OVERLAPPED *o)
{ //LOG("done ecode: %10d\tbytes: %d\n",(int)ecode,(int)bytes);
  TRY(SetEvent(o->hEvent));
Error:;
}

typedef struct _thread_ctx_t
{ nbs_stream_t nbs;
  int i;
} *thread_ctx_t;

static DWORD WINAPI writer(void* p)
{ thread_ctx_t tc =(thread_ctx_t)p;
  nbs_stream_t nbs=tc->nbs;
  unsigned long long i     =(int)(tc->i),
                     chunk =(nbs->len+NTHREADS-1)/NTHREADS, // ciel(len/NTHREADS)
                     offset=i*chunk,
                     rem=nbs->len-offset,
                     n=(chunk>rem)?rem:chunk;
  //LOG("Piece: %3llu - offset %20llu\tchunk %20llu\n",i,offset,n);
  ResetEvent(nbs->overlapped[i].hEvent);
  TRY(WriteFileEx(nbs->fd,((char*)nbs->buf)+offset,n,nbs->overlapped+i,done));
  WaitForSingleObjectEx(nbs->overlapped[i].hEvent,INFINITE,TRUE);
  return 0;
Error:
  nbs_errset();
  return 1;
}

//
// --- PUBLIC HELPERS ---
//

int      native_buffered_stream_reserve(stream_t stream, size_t nbytes)
{ DECL_CTX;
  nbs_maybe_resize(ctx, nbytes);
  return 1;
Error:
  return 0;
}

static int native_buffered_stream_flush_ctx(nbs_stream_t ctx)
{ int i,isok=1;
  //HANDLE ts[NTHREADS]={0};
#if 1
  struct _thread_ctx_t tc[NTHREADS]={0};
  size_t chunksize=(ctx->len+NTHREADS-1)/NTHREADS; // ciel(len/NTHREADS)
  for(i=0;i<NTHREADS;++i)
  { unsigned long long offset = i*chunksize;
    ctx->overlapped[i].Offset=(DWORD)offset;
    ctx->overlapped[i].OffsetHigh=(DWORD)(offset>>32);
    tc[i].nbs=ctx;
    tc[i].i=i;
  }
  for(i=0;i<NTHREADS;++i)
    TRY(QueueUserWorkItem(writer,(void*)(tc+i),WT_EXECUTEINIOTHREAD));
  //TRY(ts[i]=CreateThread(NULL,0,writer,(void*)(tc+i),0,NULL));
  WaitForMultipleObjects(NTHREADS,ctx->evts,TRUE,INFINITE);
#else
  { FILE *fp=0;
    fp=fopen("test.tif","wb");
    fwrite(ctx->buf,1,ctx->len,fp);
    fclose(fp);
  }
#endif
Finalize:
  //for(i=0;i<NTHREADS;++i) if(ts[i]) CloseHandle(ts[i]);
  ctx->len=0; ctx->pos=0; // empty buffer now that everything is written
  return isok;
Error:
  isok=0;
  nbs_errset();
  goto Finalize;
}

int native_buffered_stream_flush(stream_t stream)
{ DECL_CTX;
  return native_buffered_stream_flush_ctx(ctx);
}
//
// --- PRIVATE IMPLEMENTATION OF STREAM INTERFACE ---
//

size_t nbs_read    (void* ptr,size_t size,size_t count, stream_t stream)
{ DECL_CTX;
  off_t n=(off_t)(size*count),
        r=(off_t)ctx->len-(off_t)ctx->pos;
  n=(n>r)?r:n;
  n=(n<0)?0:n;
  if(n)
    memcpy(ptr,ctx->buf,n);
  ctx->pos+=n;
  return n;
}

size_t nbs_write   (const void * ptr, size_t size, size_t count, stream_t stream)
{ DECL_CTX;
  off_t n=(off_t)(size*count);
  TRY(nbs_maybe_resize(ctx,ctx->pos+n));
  memcpy(((char*)ctx->buf)+ctx->pos,ptr,n);
  ctx->pos+=n;
  ctx->len=(ctx->len<ctx->pos)?ctx->pos:ctx->len;
  return n;
Error:
  return 0;
}

int    nbs_seek    (stream_t stream, off_t offset, stream_seek_t origin)
{ DECL_CTX;
  off_t p[]={0,(off_t)ctx->pos,(off_t)ctx->len},
        newpos=p[origin]+offset;
  switch(ctx->mode)
  { case STREAM_MODE_READ:
      TRY(0<=newpos && newpos<ctx->len);
      break;
    case STREAM_MODE_WRITE:
      TRY(0<=newpos);
      TRY(nbs_maybe_resize(ctx,newpos+1));
      break;
    default: FAIL("Unrecognized mode.");
  }
  ctx->pos=newpos;
  return 0;
Error:
  return -1;
}

off_t  nbs_tell    (stream_t stream)
{ DECL_CTX;
  return (off_t)ctx->pos;
}

int    nbs_truncate(stream_t stream, off_t length)
{ DECL_CTX;
  TRY(length>=0);
  TRY(nbs_maybe_resize(ctx, length));
  if(length>ctx->len)
    memset( ((char*)ctx->buf)+ctx->len,0,length-ctx->len);
  ctx->len=length;
  /* ftruncate() docs say it leaves the offset unmodified, but here
     I put the position back inside the valid interval if the file is shrunk.
   */
  ctx->pos=(ctx->pos>=ctx->len)?(ctx->len-1):ctx->pos;
  return 0;
Error:
  return -1;
}

DWORD WINAPI defered_close(LPVOID p)
{ nbs_stream_t ctx=(nbs_stream_t)p;
  int i;
  if(ctx) native_buffered_stream_flush_ctx(ctx);
  if(ctx && ctx->buf && ctx->free) ctx->free(ctx->buf);
  for(i=0;i<NTHREADS;++i) CloseHandle(ctx->evts[i]);
  if(ctx->fd) CloseHandle(ctx->fd);
  free(ctx);
  return 0;
}

void   nbs_close   (stream_t stream)
{ DECL_CTX;
  stream_set_user_data(stream,0,0);
  TRY(QueueUserWorkItem(defered_close,ctx,WT_EXECUTEDEFAULT));
Error:;
}