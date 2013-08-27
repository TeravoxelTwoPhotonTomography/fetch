#pragma once
#include "units.h"
#include <QtWidgets>

#include <Eigen\Core>
using namespace Eigen;

namespace fetch {
namespace units {

  template<Length dst, Length src> 
  inline Vector3f cvt(const Vector3f& r)
  { return cvt<dst,src>(1.0)*r;
  }

  template<Length dst, Length src> 
  inline QPointF cvt(const QPointF& r)
  { return QPointF(cvt<dst,src>(r.x()),cvt<dst,src>(r.y()));
  }

  template<Length dst, Length src> 
  inline QRectF cvt(const QRectF& r)
  { return QRectF(cvt<dst,src>(r.topLeft()),cvt<dst,src>(r.bottomRight())); 
  }

}//namespace fetch { 
}//namespace units { 
