#include "devices/Vibratome.h"
#include "devices/Microscope.h"
#include "tasks/Vibratome.h"
#include "vibratome.pb.h"

namespace fetch {
namespace task {
namespace vibratome {

  unsigned int Activate::run(device::Vibratome* dc)
  { dc->start();
    while(!dc->_agent->is_stopping());
    dc->stop();
    return 1;
  }
} // end fetch::task::vibratome

namespace microscope {  
  
  // TODO
  // [ ] emergency stop? Interuptable.
  //     - add a stage halt command
  //     - use setposnowait and then block till move completed or halted.
  //     * Too complicated

  // Notes
  // - Technically the vibratome consists of two subdevices.
  //   the stage and the motorized knife.  If I had it organized
  //   this way, the I wouldn't have to make this a Microscope Task.
  //   One ought to be able to cut and image at the same time.
  //   However, that's not really useful right now.  It's more 
  //   expedient this way, and maybe it's for the best anyway.

  unsigned int Cut::run(device::Microscope* dc)
  {     
    float cx,cy,cz,vx,vy,vz,ax,ay,bx,by,v;
    // get current pos,vel
    dc->stage()->getPos(&cx,&cy,&cz);
    dc->stage()->getVelocity(&vx,&vy,&vz);
    // move to position to start the cut
    // Note: don't modify the current z
    dc->vibratome()->feed_begin_pos_mm(&ax,&ay);
    dc->vibratome()->feed_end_pos_mm(&bx,&by);
    v = dc->vibratome()->feed_vel_mm_p_s();
    dc->stage()->setVelocity(10.0,10.0,10.0);
    dc->stage()->setPos(ax,ay,cz);
    dc->stage()->setVelocity(v);
    // do the cut
    dc->vibratome()->start();
    dc->stage()->setPos(bx,by,cz);    
    dc->vibratome()->stop();
    // Move back, and set old velocity
    dc->stage()->setVelocity(10.0,10.0,10.0);
    dc->stage()->setPos(cx,cy,cz);
    dc->stage()->setVelocity(vz,vy,vz);
    return 1;
  }

} // end fetch::task::microscope

}} // end fetch::task::vibratome

