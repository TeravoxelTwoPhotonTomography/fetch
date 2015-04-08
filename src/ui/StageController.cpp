#include "StageController.h"
#include "tiling.pb.h"
#include "microscope.pb.h"

namespace mylib {
#include <image.h>
}

#include <google\protobuf\text_format.h>

#include <QtWidgets>
#include <qaction>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <functional>

#include <iostream>

const char defaultTilingConfigPathKey[] = "Microscope/Config/Tiling/DefaultFilename";

using namespace std;
using namespace Eigen;

#define DEBUG_DUMP_TILING_MARK_DATA 0

#undef HERE
#define HERE "[STAGECONTROLLER] At " << __FILE__ << "("<<__LINE__<<")\n"
#define DBG(e) if(!(e)) qDebug() << HERE << "(!)\tCheck failed for Expression "#e << "\n" 
#define SHOW(e) qDebug() << HERE << "\t"#e << " is " << e <<"\n" 

#define HERE2 "[STAGECONTROLLER] At " << __FILE__ << "("<<__LINE__<<")\n"
#define DBG2(e) if(!(e)) std::cout << HERE << "(!)\tCheck failed for Expression "#e << std::endl
#define SHOW2(e) std::cout << HERE << "\t"#e << " is " << std::endl << e << std::endl << "---" << std::endl

#define TRY(e) do{if(!(e)) { warning("%s(%d)"ENDL "\tExpression evaluated as false."ENDL "\t%s"ENDL,__FILE__,__LINE__,#e); goto Error;}}while(0)

//////////////////////////////////////////////////////////////////////////
//  TilingController  ////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

fetch::ui::TilingController::TilingController( device::Stage *stage, QObject* parent/*=0*/ )
  : QObject(parent)
  , stage_(stage)
{ 
  connect(
    &listener_,SIGNAL(sig_tile_done(unsigned,unsigned int)),
    this,      SIGNAL(tileDone(unsigned,unsigned int)),        // hooked up to the tileview
    Qt::QueuedConnection);
  connect(
    &listener_,SIGNAL(sig_tiling_changed()),
    this,        SLOT(update()),    
    Qt::DirectConnection
    );
  connect(
    &listener_,SIGNAL(sig_tile_next(unsigned)),
    this,      SIGNAL(nextTileRequest(unsigned)), // Not connected to anything right now
    Qt::QueuedConnection);       
  connect(
    &listener_,SIGNAL(sig_fov_changed(float,float,float)),
    this,      SIGNAL(fovGeometryChanged(float,float,float)),
    Qt::QueuedConnection);
  connect(
    &listener_,SIGNAL(sig_moved()),
    this,        SLOT(updatePlane()),
    Qt::QueuedConnection);  

  {
    load_action_ = new QAction(QIcon(":/icons/open"),"&Open",this);
    //load_action_->setShortcut(QKeySequence::Open);
    load_action_->setStatusTip("Open tiling data.");
    connect(load_action_,SIGNAL(triggered()),this,SLOT(loadViaFileDialog()));
  }
  {
    save_action_ = new QAction(QIcon(":/icons/saveas"),"&Save As",this);
    //save_action_->setShortcut(QKeySequence::Open);
    save_action_->setStatusTip("Save tiling data.");
    connect(save_action_,SIGNAL(triggered()),this,SLOT(saveViaFileDialog()));
  }
  {
    autosave_action_ = new QAction("&Autosave",this);
    autosave_action_->setCheckable(true);
    autosave_action_->setChecked(false);
    autosave_action_->setStatusTip("Automatically save tiling data.");
    connect(autosave_action_,SIGNAL(toggled(bool)),this,SLOT(setAutosave(bool)));
  }
  
  ///// Autosave timer
  connect(&autosave_timer_,SIGNAL(timeout()),
          this,SLOT(saveToLast()));
  autosave_timer_.stop();
}

void fetch::ui::TilingController::
  setAutosave(bool on)
  { if(!on)
    { autosave_timer_.stop();
      return;
    }
    autosave_timer_.setSingleShot(false);
    autosave_timer_.setInterval(30*1000); // interval msec   
    autosave_timer_.start();
  }
  
void 
  fetch::ui::TilingController::
  saveToFile(const QString& filename)
{ device::StageTiling *t;
  if(t=stage_->tilingLocked())
  { 
    cfg::data::Tiling metadata;
    QString arrayFileName = filename + QString(metadata.attr_ext().c_str());
    t->fov().write(metadata.mutable_fov());    
    
    QFileInfo finfo(arrayFileName);    
    metadata.set_rel_path( finfo.fileName().toLocal8Bit().data() );

    QFile metafile(filename);
    std::string buf;
    google::protobuf::TextFormat::PrintToString(metadata,&buf);
    if(!metafile.open(QFile::ReadWrite))                goto ErrorOpenMetafile;
    if( buf.length() != metafile.write(buf.c_str()) )   goto ErrorWriteMetafile;
    if(mylib::Write_Image(arrayFileName.toLocal8Bit().data(),t->attributeArray(),mylib::DONT_PRESS))
      goto ErrorWriteAttrImage;
    debug("%s(%d)"ENDL "\tTiling saved to %s.",__FILE__,__LINE__,filename.toLocal8Bit().data());
    stage_->tilingUnlock();
    return;
ErrorWriteAttrImage:
    warning("%s(%d)"ENDL "\tSomething went wrong writing tiling attribute data to the file at"ENDL"\t%s"ENDL,
      __FILE__,__LINE__,arrayFileName.toLocal8Bit().data());
    metafile.remove();
    stage_->tilingUnlock();
    return;
ErrorWriteMetafile:
    warning("%s(%d)"ENDL "\tSomething went wrong with writing tiling metadata to the file at"ENDL"\t%s"ENDL,
      __FILE__,__LINE__,metafile.fileName().toLocal8Bit().data());
    metafile.remove();
    stage_->tilingUnlock();
    return;
ErrorOpenMetafile:
    warning("%s(%d)"ENDL "\tCould not open file at %s"ENDL,
      __FILE__,__LINE__,metafile.fileName().toLocal8Bit().data());
    stage_->tilingUnlock();
    return;
  }

}

void 
  fetch::ui::TilingController::
  saveToLast()
{ QSettings settings;
  QString filename = settings.value(defaultTilingConfigPathKey).toString();
  saveToFile(filename);
}

void
  fetch::ui::TilingController::
  saveViaFileDialog()
{ QSettings settings;
  QString filename = QFileDialog::getSaveFileName(0,
    tr("Save tiling data"),
    settings.value(defaultTilingConfigPathKey,QDir::currentPath()).toString(),
    tr("Tiling Data (*.tiling);;Text Files (*.txt);;Any (*.*)"));
  if(filename.isEmpty())
    return;
  settings.setValue(defaultTilingConfigPathKey,filename);

  saveToFile(filename);
}

void 
  fetch::ui::TilingController::
  loadViaFileDialog()
{ QSettings settings;
  QString filename = QFileDialog::getOpenFileName(0,
    tr("Load tiling data"),
    settings.value(defaultTilingConfigPathKey,QDir::currentPath()).toString(),
    tr("Tiling Data (*.tiling);;Text Files (*.txt);;Any (*.*)"));
  if(filename.isEmpty())
    return;
  settings.setValue(defaultTilingConfigPathKey,filename);

  loadFromFile(filename);
}

void 
  fetch::ui::TilingController::
  loadFromFile(const QString& filename)
{ cfg::data::Tiling metadata;
  QFileInfo finfo(filename);
  QFile metafile(filename);
  QDir path(finfo.absolutePath());
  QString attrname;
  mylib::Array *attr = 0;
  const int Bpp[] = {1,2,4,8,1,2,4,8,4,8};

  // Read in the data
  if(!metafile.open(QFile::ReadOnly)) goto ErrorOpenMetafile;
  { QByteArray buf = metafile.readAll();
    if(!google::protobuf::TextFormat::ParseFromString(buf.data(),&metadata)) goto ErrorParseMetadata;
  }
  
  attrname = path.filePath( metadata.rel_path().c_str() );
  { QFile attrfile(attrname);
    if(!attrfile.exists()) goto ErrorAttrFileNotFound;
  }
  if(!(attr = mylib::Read_Image(attrname.toLocal8Bit().data(),0))) goto ErrorReadAttrArray;

  // Commit  
  device::StageTiling *t;
  if(t=stage_->tilingLocked())
  { t->fov_.update(metadata.fov());
    { stage_->tilingUnlock();
      stage_->setFOV(&t->fov_); // forces refresh of tiling if the fov is different
      if(!(t=stage_->tilingLocked())) //reaquire in case tiling changed
        goto ErrorReacquireTiling;
    }
    if( t->attributeArray()->size != attr->size ) // should alredy be the correct size due to setFOV call.
    { stage_->tilingUnlock();
      goto ErrorArrays;
    }
    memcpy( t->attributeArray()->data, attr->data, Bpp[attr->type]*attr->size );
    stage_->tilingUnlock();
  }

  Free_Array(attr);
  emit changed();
  return;
  // Error handling
ErrorReacquireTiling:
  warning("%s(%d)"ENDL"\tAfter FOV change, could not reacquire tiling"ENDL
    __FILE__,__LINE__);
  return;
ErrorArrays:
  Free_Array(attr);
  error("%s(%d)"ENDL"Programing error.  Need attribute arrays to be the same size."ENDL,__FILE__,__LINE__);
  return;
ErrorReadAttrArray:
  warning("%s(%d)"ENDL"\tCould not read attribute array at"ENDL"\t%s"ENDL,
    __FILE__,__LINE__,attrname.toLocal8Bit().data());
  return;
ErrorAttrFileNotFound:  
  warning("%s(%d)"ENDL"\tAttribute array not found at"ENDL"\t%s"ENDL,
    __FILE__,__LINE__,attrname.toLocal8Bit().data());
  return;
ErrorParseMetadata:
  warning("%s(%d)"ENDL"\tError reading metadata from %s"ENDL,
    __FILE__,__LINE__,metafile.fileName().toLocal8Bit().data());
  return;
ErrorOpenMetafile:
  warning("%s(%d)"ENDL"\tCould not open file at %s"ENDL,
    __FILE__,__LINE__,metafile.fileName().toLocal8Bit().data());
  return;
}

bool fetch::ui::TilingController::fovGeometry( TRectVerts *out )
{ device::StageTiling *tiling;
  if(tiling=stage_->tilingLocked())
  { 
    device::FieldOfViewGeometry fov(tiling->fov());
    TRectVerts rect;
    stage_->tilingUnlock();
    rect << 
      -0.5f, 0.5f, 0.5f, -0.5f, // x - hopefully, counter clockwise
      -0.5f,-0.5f, 0.5f,  0.5f, // y
       0.0f, 0.0f, 0.0f,  0.0f; // z
    TTransform t = TTransform::Identity();
    //std::cout << "---" << std::endl << t.matrix() << std::endl << "---" << std::endl;
    t.rotate(AngleAxisf(fov.rotation_radians_,Vector3f::UnitZ()));
    //std::cout << "---" << std::endl << t.matrix() << std::endl << "---" << std::endl;    
    t.scale(fov.field_size_um_);
    //std::cout << "---" << std::endl << t.matrix() << std::endl << "---" << std::endl;    
    *out = t*rect;
    return true;
  }
  return false;
}

bool fetch::ui::TilingController::latticeTransform( TTransform *out )
{ device::StageTiling *t;
  if(t=stage_->tilingLocked())
  { 
    *out = t->latticeToStageTransform();
    stage_->tilingUnlock();
    return true;
  }
  return false;
}

bool fetch::ui::TilingController::latticeTransform( QTransform *out )
{ if(stage_->tiling())
  {     
    TTransform latticeToStage;
    latticeTransform(&latticeToStage);    
    Matrix4f m  = latticeToStage.matrix();
    //SHOW2(m);
    QTransform t( // Transposed relative to Eigen
      m(0,0), m(1,0), 0.0,
      m(0,1), m(1,1), 0.0,
      m(0,3), m(1,3), 1.0);      
    *out=t;
    return true;
  }
  return false;
}

static void _lattice_shape(fetch::device::StageTiling *t,unsigned *width, unsigned *height )
{   mylib::Array *lattice = t->attributeArray();
    *width  = lattice->dims[0];
    *height = lattice->dims[1];
}

bool fetch::ui::TilingController::latticeShape( unsigned *width, unsigned *height )
{ device::StageTiling *t;
  if(t=stage_->tilingLocked())
  { _lattice_shape(t,width,height);    
    stage_->tilingUnlock();
    return true;
  }
  return false; 
}

bool fetch::ui::TilingController::latticeShape(QRectF *out)
{ 
  unsigned w,h;
  if(latticeShape(&w,&h))
  {      
    out->setTopLeft(QPointF(0.0f,0.0f));
    out->setSize(QSizeF(w,h));
    return true;
  }
  return false; 
}
   
typedef mylib::uint8 uint8;
typedef Matrix<size_t,1,3> Vector3z;
/** 
\returns 0 on failure, 1 on success.
On success, the caller is responsible for unlocking the stage's tiling mutex.
On success, the caller is responsible for unlocking the tiling's attribute array mutex.
*/
bool fetch::ui::TilingController::latticeAttrImage(QImage *out)
{ device::StageTiling *t;
  if(!(t=stage_->tilingLocked()))
    return false;  
  unsigned w,h;
  t->lock();
  _lattice_shape(t,&w,&h);
  mylib::Array 
    *lattice = t->attributeArray(),
     plane   = *lattice;
  Vector3z ir = stage_->getPosInLattice();
// HACK [ngc oct 2013] 
// bc of tiling offset it's possible for stage position to translate to something outside the bounds of the tiling array
// here, I just clamp oob values.  This is mostly a problem when markAddressible is called when the stage moves to the
// safety position (z=0.5 mm).
// - proper solution would be to resize the tiling array appropriately
// - another solution would be for this routine to return false when oob, but then I'd need to make sure the error was
//   handled correctly, and I don't have the ability to test that at the moment. (sample is on)
  if(   0 > ir[2]           ) ir[2]=0;
  if(ir[2]>=lattice->dims[2]) ir[2]=lattice->dims[2]-1;
  mylib::Get_Array_Plane(&plane,ir[2]);  
  *out = QImage(AUINT8(&plane),w,h,QImage::Format_ARGB32_Premultiplied); //QImage requires a uchar* for the data  
  //stage_->tilingUnlock(); DONT unlock here...the QImage still references the tiling data...need to unlock in caller
  //out->save("TilingController_latticeAttrImage.tif");
  return true;
}

/** \Returns false if iplane is out-of-bounds, otherwise returns true.
    On success, the caller is responsible for unlocking the stage's tiling mutex.
    On success, the caller is responsible for unlocking the tiling's attribute array mutex.
*/
bool fetch::ui::TilingController::latticeAttrImageAtPlane(QImage *out, int iplane)
{ device::StageTiling *t;
  if(!(t=stage_->tilingLocked()))
    return false;  
  t->lock();
  unsigned w,h;
  _lattice_shape(t,&w,&h);
  mylib::Array 
    *lattice = t->attributeArray(),
     plane   = *lattice;
  if(!(0<=iplane && iplane<lattice->dims[2]))
  { t->unlock();
    stage_->tilingUnlock();
    return false;
  }
  mylib::Get_Array_Plane(&plane,iplane);
  *out = QImage(AUINT8(&plane),w,h,QImage::Format_ARGB32_Premultiplied); //QImage requires a uchar* for the data
  //stage_->tilingUnlock(); DONT unlock here...the QImage still references the tiling data...need to unlock in caller
  //out->save("TilingController_latticeAttrImage.tif");
  return true;
}

bool fetch::ui::TilingController::stageAlignedBBox(QRectF *out)
{ QTransform l2s;
  if(!latticeShape(out))
    return false;
  //SHOW(*out);
  latticeTransform(&l2s);    
  //SHOW(l2s);
  *out = l2s.mapRect(*out);  
  //SHOW(*out);
  return true;
}

static void zero_alpha(QImage &im)
{ uint8_t *d=(uint8_t*)im.constBits()+3; // use constBits() to prevent a copy, then cast away the const
  for(unsigned i=0;i<(im.byteCount()/4);++i)
    d[4*i]=0;                            // zero out the alpha component
}

bool fetch::ui::TilingController::mark( const QPainterPath& path, int attr, QPainter::CompositionMode mode )
{ 
  QColor color((QRgb)attr);
  QImage im;
  // 1. Path is in scene coords.  transform to lattice coords
  //    Getting the transform is a bit of a pain bc we have to go from 
  //    Eigen to Qt :(
  QTransform l2s, s2l;
  TRY(latticeTransform(&l2s));
  s2l = l2s.inverted();                                  
  // 2. Get access to the attribute data  
  TRY(latticeAttrImage(&im)); // locks the stage's tiling mutex, need to unlock when done with im
  // 3. Fill in the path
  { QPainter painter(&im);
    QPainterPath lpath = s2l.map(path);
  #if DEBUG_DUMP_TILING_MARK_DATA
    im.save("TilingController_mark__before.tif");
  #endif
    painter.setCompositionMode(mode);
    painter.fillPath(lpath,color);
  }
  zero_alpha(im);
  stage_->tiling()->unlock();
  stage_->tilingUnlock();
  //SHOW(lpath);
#if DEBUG_DUMP_TILING_MARK_DATA
  im.save("TilingController_mark__after.tif");
  warning("Dumping Tiling mark data"ENDL);
#endif
  return true;
Error:
  return false;
}

bool fetch::ui::TilingController::mark_all_planes( const QPainterPath& path, int attr, QPainter::CompositionMode mode )
{ if(!is_valid()) return false;  
  QColor color((QRgb)attr);
  // 1. Path is in scene coords.  transform to lattice coords
  //    Getting the transform is a bit of a pain bc we have to go from 
  //    Eigen to Qt :(
  QTransform l2s, s2l;
  latticeTransform(&l2s);
  s2l = l2s.inverted();
  QPainterPath lpath = s2l.map(path);
                                  
  // 2. Get access to the attribute data
  int iplane=0;
  QImage im;
  while(latticeAttrImageAtPlane(&im,iplane++)) // acquires the stage's tiling lock
  { QPainter painter(&im);

    // 3. Fill in the path
#if DEBUG_DUMP_TILING_MARK_DATA
    im.save("TilingController_mark__before.tif");
#endif
    painter.setCompositionMode(mode);
    painter.fillPath(lpath,color);           // sets top byte to 0xff, lower bytes to attr
    zero_alpha(im);
    stage_->tiling()->unlock();
    stage_->tilingUnlock();
    //SHOW(lpath);
#if DEBUG_DUMP_TILING_MARK_DATA
    im.save("TilingController_mark__after.tif");
    warning("Dumping Tiling mark data"ENDL);
#endif
  }
  return true;
}

bool fetch::ui::TilingController::mark_all( device::StageTiling::Flags attr, QPainter::CompositionMode mode )
{
  if(!is_valid()) return false;
       
  QColor color((QRgb)attr);
  // 1. Get access to the attribute data
  QImage im;
  latticeAttrImage(&im); // locks the stage's tiling mutex, need to unlock when done with im
  QPainter painter(&im);
  // 2. fill   
  //im.save("TilingController_mark_all__before.tif");
  painter.setCompositionMode(mode);
  painter.fillRect(im.rect(),color);
  zero_alpha(im);
  stage_->tiling()->unlock();
  stage_->tilingUnlock();
  //im.save("TilingController_mark_all__after.tif");
  return true;
}

bool fetch::ui::TilingController::markActive( const QPainterPath& path )
{   
  return mark(
    path,
    device::StageTiling::Active,
    QPainter::RasterOp_SourceOrDestination);
}     

bool fetch::ui::TilingController::markInactive( const QPainterPath& path )
{   
  return mark(
    path,
    device::StageTiling::Active,
    QPainter::RasterOp_NotSourceAndDestination);
}

bool fetch::ui::TilingController::markDone(const QPainterPath& path)
{  
  return mark(
    path,
    device::StageTiling::Done,
    QPainter::RasterOp_SourceOrDestination);
}

bool fetch::ui::TilingController::markNotDone(const QPainterPath& path)
{  
  return mark(
    path,
    device::StageTiling::Done,
    QPainter::RasterOp_NotSourceAndDestination);
}

bool fetch::ui::TilingController::markExplorable(const QPainterPath& path)
{  
  return mark(
    path,
    device::StageTiling::Explorable,
    QPainter::RasterOp_SourceOrDestination);
}

bool fetch::ui::TilingController::markNotExplorable(const QPainterPath& path)
{  
  return mark(
    path,
    device::StageTiling::Explorable,
    QPainter::RasterOp_NotSourceAndDestination);
}

bool fetch::ui::TilingController::markSafe(const QPainterPath& path)
{  
  return mark_all_planes(
    path,
    device::StageTiling::Safe,
    QPainter::RasterOp_SourceOrDestination);
}

bool fetch::ui::TilingController::markNotSafe(const QPainterPath& path)
{  
  return mark_all_planes(
    path,
    device::StageTiling::Safe,
    QPainter::RasterOp_NotSourceAndDestination);
}

bool fetch::ui::TilingController::markUserReset(const QPainterPath& path)
{  
  return mark(
    path,    
       device::StageTiling::Active
      |device::StageTiling::Detected
      |device::StageTiling::Explored
      |device::StageTiling::Done
    ,
    QPainter::RasterOp_NotSourceAndDestination) 
    &&
    mark_all_planes(
    path,
       device::StageTiling::Explorable      
      |device::StageTiling::Safe,
    QPainter::RasterOp_NotSourceAndDestination);
    ;
}

bool fetch::ui::TilingController::markAllPlanesExplorable(const QPainterPath& path)
{  
  return mark_all_planes(
    path,
    device::StageTiling::Explorable,
    QPainter::RasterOp_SourceOrDestination);
}

bool fetch::ui::TilingController::markAllPlanesNotExplorable(const QPainterPath& path)
{  
  return mark_all_planes(
    path,
    device::StageTiling::Explorable,
    QPainter::RasterOp_NotSourceAndDestination);
}

// returns false if tiling is invalid or if stage_coord is oob
#if _MSC_VER<1800
static float roundf(float x) {return floor(0.5f+x);}
#endif

bool fetch::ui::TilingController::mapToIndex(const Vector3f & stage_coord, unsigned *index)
{
  TTransform t;  
  if(latticeTransform(&t))
  { 
    unsigned w,h;
    //SHOW2(t.matrix());
    //SHOW2(t.inverse().matrix());
    Vector3f lr = t.inverse()*stage_coord;
    //SHOW2(lr);
    lr = lr.unaryExpr(ptr_fun(roundf));
    //SHOW2(lr);
    latticeShape(&w,&h);
    QRectF bounds(0,0,w,h);
    if( bounds.contains(QPointF(lr(0),lr(1))) )
    {
      *index = lr(1)*w+lr(0);
      return true;
    }
  }
  return false;
}

bool fetch::ui::TilingController::markAddressable()
{ device::StageTiling *t;
  if(!(t=stage_->tiling()))
    return false;
  
  const device::StageTravel& travel = t->travel();
  float z = stage_->getTarget().z(); //tiling_->plane_mm();
  if( (travel.z.min <= z) && (z <= travel.z.max) )
  { 
    QRectF r(
      QPointF(1000.0f*travel.x.min,1000.0f*travel.y.min),
      QPointF(1000.0f*travel.x.max,1000.0f*travel.y.max)); // convert mm to um
    QPainterPath path;
    path.addRect(r);
    mark(path,device::StageTiling::Addressable,QPainter::RasterOp_SourceOrDestination);
  } else 
  { // Mark the whole thing as unaddressable
    warning("%s(%d): Stage Travel looks crazy.  Marking whole space as unaddressable."ENDL,__FILE__,__LINE__);
    mark_all(device::StageTiling::Addressable,QPainter::RasterOp_NotSourceAndDestination);
  }
  return true;
}

void fetch::ui::TilingController::fillActive()
{ device::StageTiling *t;
  if(!stage_) return;
  size_t iplane = stage_->getPosInLattice().z();
  if(t=stage_->tiling())
    t->fillHolesInActive(iplane);
}

void fetch::ui::TilingController::dilateActive()
{ device::StageTiling *t;
  if(!stage_) return;
  size_t iplane = stage_->getPosInLattice().z();
  if(t=stage_->tiling())
    t->dilateActive(iplane);
}

//////////////////////////////////////////////////////////////////////////
///  PlanarStageController
//////////////////////////////////////////////////////////////////////////
fetch::ui::PlanarStageController::PlanarStageController( device::Stage *stage, QObject *parent/*=0*/ )
   : QObject(parent)
   , stage_(stage)
   , agent_controller_(stage->_agent)
   , tiling_controller_(stage)
{
  { float x,y,z;
    stage->getLastTarget(&x,&y,&z);
    history_.push(x,y,z);
  }

  connect(
    &agent_controller_, SIGNAL(onAttach()),
    this,SLOT(updateTiling()) );

  connect(
    &agent_controller_, SIGNAL(onDetach()),
    this,SLOT(updateTiling()) );
  
  connect(
    &listener_,SIGNAL(sig_moved()),
    this,SIGNAL(moved()),
    Qt::QueuedConnection);   

  connect(
    &listener_,SIGNAL(sig_velocityChanged()),
    this,SIGNAL(velocityChanged()),
    Qt::QueuedConnection);    
    
  connect(
    &listener_,SIGNAL(sig_referenced()),
    this,SIGNAL(referenced()),
    Qt::QueuedConnection); 

  connect(this,SIGNAL(moved()),tiling(),SIGNAL(planeChanged()));
  connect(this,SIGNAL(referenced()),this,SIGNAL(moved()));

  stage_->addListener(&listener_);
  stage_->addListener(tiling_controller_.listener());

  agent_controller_.createPollingTimer()->start(50 /*msec*/);
}

QComboBox*
  fetch::ui::PlanarStageController::
  createHistoryComboBox(QWidget *parent)
{
  QComboBox* cb = new QComboBox(parent);  
  cb->setModel(&history_);
  connect(&history_,SIGNAL(suggestIndex(int)),cb,SLOT(setCurrentIndex(int)));
  connect(cb,SIGNAL(activated(int)),this,SLOT(moveToHistoryItem(int)));
  return cb;
}
