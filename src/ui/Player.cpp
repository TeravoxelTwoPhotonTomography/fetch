#include <QtWidgets>
#include <assert.h>
#include "Figure.h"
#include "Player.h"
namespace mylib {
#include <utilities.h>
#include <image.h>
}
#include <frame.h>
#include <util/util-mylib.h>
#include "chan.h"

using namespace ::mylib;

namespace fetch{
namespace ui {

/************************************************************************/
/* IPlayerThread                                                        */
/************************************************************************/
IPlayerThread::IPlayerThread( Figure *w/*=0*/ )
  :w_(NULL)
  ,running_(0)
{
  setFigure(w);
}

void IPlayerThread::setFigure( Figure *w )
{ QMutexLocker lock(&lock_);
  if(w_)
  {
    disconnect(
      this,SIGNAL(imageReady(mylib::Array*)),
      w_ ,SLOT  (imshow(mylib::Array*)));
    w_ = NULL;
  } 
  w_ = w;
  if(w)
  {
    connect(
      this,SIGNAL(imageReady(mylib::Array*)),
      w_ ,SLOT   (imshow(mylib::Array*)),
      Qt::BlockingQueuedConnection);
  }
}

/************************************************************************/
/* AsynqPlayer                                                          */
/************************************************************************/

AsynqPlayer::AsynqPlayer( Chan *in, Figure *w/*=0*/ )
  :IPlayerThread(w)
  ,in_(0)  
  ,peek_timeout_ms_(10)
{
  in_ = Chan_Open(in,CHAN_NONE);
}

AsynqPlayer::~AsynqPlayer()
{
  Chan_Close(in_);
  in_ = NULL;
}

void AsynqPlayer::run()
{
  Frame *buf =  (Frame*)Chan_Token_Buffer_Alloc(in_);
  size_t nbytes  = Chan_Buffer_Size_Bytes(in_);
  mylib::Array im;
  mylib::Dimn_Type dims[3];
  Chan *reader = Chan_Open(in_,CHAN_PEEK);
  TicTocTimer t = tic();
  // Notes: o Peek copies from current data into frame buffer.
  //        o Peek might realloc the input frame buffer.

  { QMutexLocker locker(&lock_); 
    running_ = 1;
    const float fps = 60.0;
    float ms = 1000.0/fps - toc(&t)*1000.0; // in ms;
    while(running(ms))
    { if(CHAN_SUCCESS( Chan_Peek_Timed(reader,(void**)&buf,nbytes,peek_timeout_ms_) ))
      { 
        nbytes = buf->size_bytes();
        buf->format(buf);                                                    // resets the data pointer  - super gross and icky
        castFetchFrameToDummyArray(&im,buf,dims);
        emit imageReady(&im);                                                // blocks until receiver returns      //...what if there's no reciever?        
      }       
      ms = 1000.0/fps - toc(&t)*1000.0; // in ms
      ms = (ms>0)?ms:0;                                                                        
    }
  }
  Chan_Close(reader);  
  Chan_Token_Buffer_Free(buf);
  //debug("%s(%d): player out!"ENDL,__FILE__,__LINE__);
}


}} //end fetch::ui