#include <QtGui>
#include <assert.h>
#include "Figure.h"
#include "Player.h"
namespace mylib {
#include <utilities.h>
#include <image.h>
}
#include <frame.h>
#include <util/util-mylib.h>

using namespace ::mylib;

namespace fetch{
namespace ui {

ArrayPlayer::ArrayPlayer(const char* filename, Figure *w/*=0*/):
  w_(w),
  running_(0)
{
  assert(im_=Read_Image(const_cast<char*>(filename),0));
  if(w_==NULL)
    if(im_->ndims>2)
      w_ = imshow(Get_Array_Plane(im_,0));
    else
      w_ = imshow(im_);
  connect(this,SIGNAL(imageReady(mylib::Array*)),
    w_  ,SLOT  (imshow(mylib::Array*)),
    Qt::BlockingQueuedConnection);
  Print_Inuse_List(stderr,1);
}

ArrayPlayer::~ArrayPlayer()
{
  Free_Array(im_);
  Print_Inuse_List(stderr,1);
}

void ArrayPlayer::run()
{ 
  size_t count =0 ;
  int frame = 0;
  int d;
  running_ = 1;
  if(im_->ndims>2)
    d = im_->dims[2];
  else
    return; // nothing to do
  while(running())
  {
    emit imageReady(Get_Array_Plane(im_,frame));
    frame = ++frame%d; //loop
    ++count;
    //qDebug() << count << "\tFrame: " << frame << "\tArray Usage:"<<Array_Usage();
  }
}

/************************************************************************/
/* FRAMESTREAMPLAYER                                                    */
/************************************************************************/

AsynqPlayer::AsynqPlayer( asynq *in, Figure *w/*=0*/ )
  :w_(w)
  ,in_(0)
  ,running_(0)
{
  in_ = Asynq_Ref(in);
  /* TODO
     o  below won't work when w==NULL, what then?
     o  connecting and disconnecting from different widgets/items?
  */
  connect(
    this,SIGNAL(imageReady(mylib::Array*)),
    w_  ,SLOT  (imshow(mylib::Array*)),
    Qt::BlockingQueuedConnection); //blocks until receiver returns
}

AsynqPlayer::~AsynqPlayer()
{
  Asynq_Unref(in_);
  in_ = NULL;
}

void AsynqPlayer::run()
{
  Frame *buf =  (Frame*)Asynq_Token_Buffer_Alloc(in_);
  size_t nbytes  = in_->q->buffer_size_bytes;
  mylib::Array im;
  size_t dims[3];
  assert(sizeof(size_t)==sizeof(mylib::Dimn_Type));
  im.dims = (Dimn_Type*) dims;
  im.ndims = 3;
  im.kind = PLAIN_KIND;
  im.text = "\0";
  im.tlen = 0;  
  while(running() && Asynq_Pop(in_,(void**)&buf,nbytes))
  {
    nbytes = buf->size_bytes();
    im.data = buf->data;
    im.type  = fetchTypeToArrayType(buf->rtti);
    im.scale = fetchTypeToArrayScale(buf->rtti);
    im.size = dims[0]*dims[1]*dims[2];
    buf->get_shape(dims);
    emit imageReady(&im); //blocks until receiver returns
  }
  Asynq_Token_Buffer_Free(buf);
}

}} //end fetch::ui