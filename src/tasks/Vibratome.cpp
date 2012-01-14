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

  unsigned int Cut::config(device::Microscope* dc)
  {
    // validate
    float cx,cy,cz,ax,ay,bx,by;
    dc->stage()->getTarget(&cx,&cy,&cz);    
    dc->vibratome()->feed_begin_pos_mm(&ax,&ay);
    dc->vibratome()->feed_end_pos_mm(&bx,&by);

    int a = dc->stage()->isPosValid(ax,ay,cz), 
        b = dc->stage()->isPosValid(bx,by,cz);
    if(!a) warning("[Vibratome] [Task: Cut] Cut origin is out-of-bounds"ENDL);
    if(!b) warning("[Vibratome] [Task: Cut] Feed goes out of bound during cut."ENDL);
    return a&&b;
  }

  /*
     Motion:
     
     c --> a --> b --> c
       v0    v1    v0

     c is current point, typically where the imaging just stopped
     a is the cut start (x,y) point with a(z) = c(z) - dz
     b is the cut end
     v0 is the currently set stage velocity
     v1 is the cut velocity

  */
#define CHECK_INTERUPTED \
  if( dc->_agent->is_stopping() ) \
  { warning("Task [Vibratome::Cut] - Interupted."ENDL"\t%s(%d)"ENDL,__FILE__,__LINE__); \
    goto Error; \
  }
#define CHK(expr) \
  CHECK_INTERUPTED; \
  if(!(expr))       \
  { warning("Task [Vibratome::Cut] - Expression failed."ENDL"\t%s(%d)"ENDL"\t%s"ENDL,__FILE__,__LINE__,#expr); \
    goto Error;     \
  }

  /* MOTION               
     ======                            (c*)<-- New Image Plane -----
     In-Plane:  (c)        Vertical:                               |
                / ^                    (c) Old Image Plane         | dz
               /   \                    |                          |
              /     \                dz |(a)--------------------->(b) 
            (a)--->(b)                  |_| thick    ___ Knife             
                Cut
  */
  unsigned int Cut::run(device::Microscope* dc)
  {     
    float cx,cy,cz,vx,vy,vz,ax,ay,bx,by,v,dz,thick;
    // get current pos,vel
    CHK( dc->stage()->getTarget(&cx,&cy,&cz));
    CHK( dc->stage()->getVelocity(&vx,&vy,&vz));
    
    // Get parameters    
    dc->vibratome()->feed_begin_pos_mm(&ax,&ay);
    dc->vibratome()->feed_end_pos_mm(&bx,&by);
    thick = dc->vibratome()->thickness_um()*0.001; // um->mm
    dz = dc->vibratome()->verticalOffset();
    CHK( (v = dc->vibratome()->feed_vel_mm_p_s())>0.0); // must be non-zero
    
    // Move to the start of the cut
    CHK( dc->stage()->setPos(cx,cy,1.0));           // Drop to safe z first
    CHK( dc->stage()->setPos(ax,ay,1.0));           // Move on safe z plane
    CHK( dc->stage()->setPos(ax,ay,cz-dz+thick));   // Move to final plane
    
    // do the cut
    CHK( dc->vibratome()->start());    // returns 1 on success
    CHK( dc->stage()->setVelocity(v));
    CHK( dc->stage()->setPos(bx,by,cz-dz));
    CHK( dc->stage()->setVelocity(vx,vy,vz));
    CHK( dc->vibratome()->stop());
    
    // Move back
    CHK( dc->stage()->setPos(bx,by,1.0));           // Drop to safe z first
    CHK( dc->stage()->setPos(cx,cy,1.0));           // Move on safe z plane
    CHK( dc->stage()->setPos(cx,cy,cz+thick));
    
    return 1;
Error:
    dc->vibratome()->stop();
    return 0;
  }

} // end fetch::task::microscope

}} // end fetch::task::vibratome

