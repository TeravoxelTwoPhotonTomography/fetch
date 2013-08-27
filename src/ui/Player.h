#pragma once

#include <QtWidgets>
#include "Figure.h"
#include "Chan.h"
namespace mylib {
#include <utilities.h>
#include <image.h>
}

namespace fetch{
namespace ui {
 
/************************************************************************/
/* IPlayerThread                                                        */
/************************************************************************/
/*
Methods
  IPlayerThread  - sets up the connection between the Figure and the 
                    imageReady event.  The event is a Qt:BlockingQueueConnection.

  setFigure()    - connects the imageReady signal to a Figure.  This will
                   disconnect the last Figure set, if any.  If NULL is passed
                   as input, the last Figure will be disconnected and no new
                   Figure connected (I don't know if the thread will block on 
                   emitting imageReady, but nothing should receive).

                   Receivers connected outside of this call should not be 
                   effected.

  running()      - returns true if the "running" flag is set.  Used to 
                    signal termination to the main loop for the thread.
                    Synchronized.
  stop()         - Sets the "running" flag to stop.  Synchronized.

  imageReady()   - Qt signal communicates that an image is ready.
                    Posts an Array*.  Only a dummy Array struct gets 
                    copied onto the Qt event queue; the actual pixel data
                    is not copied anywhere. It's necessary that
                    the Player thread blocks until the imageReady signal is 
                    handled.
*/
class IPlayerThread:public QThread
{
  Q_OBJECT
public:
  IPlayerThread(Figure *w=0);

  void setFigure(Figure *w); //passing NULL will disconnect imageReady from the last assigned Figure.

  virtual int  running(unsigned long ms=0) 
  { bool timeout = 0;    
    timeout = (0==is_stopping_.wait(&lock_,ms));    
    return timeout && running_;
  }    
  virtual void stop()                      {running_=0; is_stopping_.wakeAll();} 

signals:
  void imageReady(mylib::Array *im);

protected:
  Figure *w_;
  QMutex lock_;
  QWaitCondition is_stopping_;
  int running_;
};

class AsynqPlayer:public IPlayerThread
{
public:
  AsynqPlayer(Chan *in, Figure *w=0);
  virtual ~AsynqPlayer();

protected:
  virtual void run();

protected:
  Chan *in_;
  int peek_timeout_ms_;
};

}} //end fetch::ui