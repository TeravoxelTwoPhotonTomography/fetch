#include "StageController.h"

#include <QtGui>
#include <Eigen/Core>
#include <Eigen/Geometry>
using namespace Eigen;

//////////////////////////////////////////////////////////////////////////
//  TilingController  ///// //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

fetch::ui::TilingController::TilingController( device::StageTiling *tiling, QObject* parent/*=0*/ )
  : QObject(parent)
  , tiling_(tiling)
{ 
  connect(
    &listener_,SIGNAL(sig_tile_done(unsigned,unsigned char)),
    this,      SIGNAL(tileDone(unsigned,unsigned char)),
    Qt::QueuedConnection);
  connect(
    &listener_,SIGNAL(sig_tiling_changed(device::StageTiling*)),
    this,        SLOT(update(device::StageTiling*)),
    Qt::QueuedConnection);
  connect(
    &listener_,SIGNAL(sig_tile_next(unsigned)),
    this,      SIGNAL(nextTileRequest(unsigned)),
    Qt::QueuedConnection);

}


bool fetch::ui::TilingController::fovGeometry( TRectVerts *out )
{ 
  if(tiling_)
  {  
    device::FieldOfViewGeometry fov(tiling_->fov());
    TRectVerts rect;
    rect << 
      -1.0f, 1.0f, 1.0f, -1.0f, // x - hopefully, counter clockwise
      -1.0f,-1.0f, 1.0f,  1.0f, // y
      0.0f, 0.0f, 0.0f,  0.0f; // z
    TTransform t;
    t
      .rotate(AngleAxisf(fov.rotation_radians_,Vector3f::UnitZ()))
      .scale(fov.field_size_um_);
    *out = t*rect;
    return true;
  }
  return false;
}

bool fetch::ui::TilingController::latticeTransform( TTransform *out )
{ 
  if(tiling_)
  { 
    *out = tiling_->latticeToStageTransform();
    return true;
  }
  return false;
}

bool fetch::ui::TilingController::latticeShape( unsigned *width, unsigned *height )
{ 
  if(tiling_)
  {      
    mylib::Array *lattice = tiling_->mask();
    *width  = lattice->dims[0];
    *height = lattice->dims[1]; 
    return true;
  }
  return false; 
}

typedef mylib::uint8 uint8;
bool fetch::ui::TilingController::markActive( const QPainterPath& path )
{    
  if(tiling_)
  {           
    // 1. Get access to the attribute data
    unsigned w,h;
    latticeShape(&w,&h);
    mylib::Array *lattice = tiling_->mask(),
      plane = *lattice;
    mylib::Get_Array_Plane(&plane,tiling_->plane());  
    QImage im(AUINT8(&plane),w,h,w/*stride*/,QImage::Format_Indexed8);
    QPainter painter(&im);

    // 2. Path is in scene coords.  transform to lattice coords
    //    Getting the transform is a bit of a pain bc we have to go from 
    //    Eigen to Qt :(
    TTransform latticeToStage,stageToLattice;
    latticeTransform(&latticeToStage);
    stageToLattice = latticeToStage.inverse();
    Matrix2f m  = stageToLattice.linear().topLeftCorner<2,2>();
    Vector2f dr = stageToLattice.translation().head<2>();
    QTransform t(
      m(0,0), m(0,1), 0.0,
      m(1,0), m(1,1), 0.0,
      dr(0) ,  dr(1), 1.0);

    // 3. Fill in the path
    im.setColorCount(256);
    for(int i=0;i<256;++i) 
      im.setColor(i,qRgb(i,i,i));
    uint8 attr = (uint8)device::StageTiling::Active;
    painter.setTransform(t);
    painter.setCompositionMode(QPainter::RasterOp_SourceAndDestination);
    painter.fillPath(path,QBrush(qRgb(attr,attr,attr)));
    return true;
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////
//  PlanarStageController   //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
fetch::ui::PlanarStageController::PlanarStageController( device::Stage *stage, QObject *parent/*=0*/ )
   : QObject(parent)
   , stage_(stage)
   , agent_controller_(stage->_agent)
   , tiling_controller_(NULL)
{
  connect(
    &agent_controller_, SIGNAL(onAttach()),
    this,SLOT(updateTiling()) );

  connect(
    &agent_controller_, SIGNAL(onDetach()),
    this,SLOT(invalidateTiling()) );

  agent_controller_.createPollingTimer()->start(50 /*msec*/);
}
