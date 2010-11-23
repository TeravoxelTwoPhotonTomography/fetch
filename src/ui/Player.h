#pragma once

#include <QtGui>
#include "Figure.h"
#include "asynq.h"
namespace mylib {
#include <utilities.h>
#include <image.h>
}

namespace fetch{
namespace ui {


class ArrayPlayer:public QThread
{
  Q_OBJECT
  public:
    ArrayPlayer(const char* filename, Figure *w=0);
    virtual ~ArrayPlayer();
     
    inline int  running() {QMutexLocker l(&lock_); return running_;}
    inline void stop()    {QMutexLocker l(&lock_); running_=0;} 
  signals:
    void imageReady(mylib::Array *im);
  protected:
    virtual void run();

  protected:
    mylib::Array *im_;
    Figure *w_;
    int running_;
    QMutex lock_;
};

class AsynqPlayer:public QThread
{
  Q_OBJECT
public:
  AsynqPlayer(asynq *in, Figure *w=0);
  virtual ~AsynqPlayer();

  inline int  running() {QMutexLocker l(&lock_); return running_;}
  inline void stop()    {QMutexLocker l(&lock_); running_=0;} 
signals:
  void imageReady(mylib::Array *im);
protected:
  virtual void run();

protected:
  Figure *w_;
  asynq *in_;
  int running_;
  QMutex lock_;

};

}} //end fetch::ui