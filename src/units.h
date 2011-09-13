#pragma once

namespace fetch {
namespace units {

 typedef enum tagLength
 { SMALLEST_LENGTH = -12,
   PM = -12,
   NM = -9,
   UM = -6,
   MM = -3,
   CM = -2,
   M  =  0,
   KM =  3,
   LARGEST_LENGTH =  3,
   PIXEL_SCALE    = -6                                                     // The physical scale corresponding to a zoom (or lod) of 1 on the view
 } Length;                                                                 // [ ] TODO rename PIXEL_SCALE to something like VIEW or REFERENCE _SCALE 


 // metersBase10
 // o  Returns log10 of the unit converted to meters.
 // o  This is present so that other length types (such as feet) could
 //    potentially be used.
 // o  Currently, this is implemented generically, but if you did want to 
 //    use feet, you could do so by adding specializations in a units.cpp
 //    file.  
 template<Length unit> inline double metersBase10() {return unit;}

 template<Length dst, Length src> inline double cvt(double x)
 { double m = pow(10.0,metersBase10<src>() - metersBase10<dst>());
   return x*m;
 }

}
}
