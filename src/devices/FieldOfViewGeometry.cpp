#include "devices\FieldOfViewGeometry.h"
#include "common.h"
#include <math.h>

namespace fetch {
namespace device {

bool operator==(const FieldOfViewGeometry &lhs,const FieldOfViewGeometry &rhs)
{ 
  //debug("change: field size(um): %f"ENDL,(lhs.field_size_um_    - rhs.field_size_um_   ).norm());
  //debug("change: overlap   (um): %f"ENDL,(lhs.overlap_um_       - rhs.overlap_um_      ).norm());
  //debug("change: rotation (rad): %f"ENDL,fabs(lhs.rotation_radians_ - rhs.rotation_radians_));
  return   ((lhs.field_size_um_    - rhs.field_size_um_   ).norm() < 0.1)   // 100 nm tolerance
        && ((lhs.overlap_um_       - rhs.overlap_um_      ).norm() < 0.1)
        && (fabs(lhs.rotation_radians_ - rhs.rotation_radians_)    < 0.001);// ~0.5 deg tolerance

}


}} // end namespace fetch::device