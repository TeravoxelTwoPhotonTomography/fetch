#include "StageController.h"
#include "tiling.pb.h"
#include "microscope.pb.h"

namespace mylib {
#include <image.h>
}

#include <google\protobuf\text_format.h>

#include <QtGui>
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

//////////////////////////////////////////////////////////////////////////
//  TilingController  ////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

fetch::ui::TilingController::TilingController( device::Stage *stage, device::StageTiling *tiling, QObject* parent/*=0*/ )
  : QObject(parent)
  , stage_(stage)
  , tiling_(tiling)
{ 
  connect(
    &listener_,SIGNAL(sig_tile_done(unsigned,unsigned int)),
    this,      SIGNAL(tileDone(unsigned,unsigned int)),        // hooked up to the tileview
    Qt::QueuedConnection);
  connect(
    &listener_,SIGNAL(sig_tiling_changed(device::StageTiling*)),
    this,        SLOT(update(device::StageTiling*)),    
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
}

void 
  fetch::ui::TilingController::
  saveToFile(const QString& filename)
{ if(tiling_)
  { 
    cfg::data::Tiling metadata;
    QString arrayFileName = filename + QString(metadata.attr_ext().c_str());
    tiling_->fov().write(metadata.mutable_fov());    
    
    QFileInfo finfo(arrayFileName);    
    metadata.set_rel_path( finfo.fileName().toLocal8Bit().data() );

    QFile metafile(filename);
    std::string buf;
    google::protobuf::TextFormat::PrintToString(metadata,&buf);
    if(!metafile.open(QFile::ReadWrite))                goto ErrorOpenMetafile;
    if( buf.length() != metafile.write(buf.c_str()) )   goto ErrorWriteMetafile;
    if(mylib::Write_Image(arrayFileName.toLocal8Bit().data(),tiling_->attributeArray(),mylib::DONT_PRESS))
      goto ErrorWriteAttrImage;
    return;
ErrorWriteAttrImage:
    warning("%s(%d)"ENDL"\tSomething went wrong writing tiling attribute data to the file at"ENDL"\t%s"ENDL,
      __FILE__,__LINE__,arrayFileName.toLocal8Bit().data());
    metafile.remove();
    return;
ErrorWriteMetafile:
    warning("%s(%d)"ENDL"\tSomething went wrong with writing tiling metadata to the file at"ENDL"\t%s"ENDL,
      __FILE__,__LINE__,metafile.fileName().toLocal8Bit().data());
    metafile.remove();
    return;
ErrorOpenMetafile:
    warning("%s(%d)"ENDL"\tCould not open file at %s"ENDL,
      __FILE__,__LINE__,metafile.fileName().toLocal8Bit().data());
    return;
  }
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
  int Bpp[] = {1,2,3,4,1,2,3,4,4,8};

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
  tiling_->fov_.update(metadata.fov());
  stage_->setFOV(&tiling_->fov_); // forces refresh of tiling if the fov is different
  if( tiling_->attributeArray()->size != attr->size ) goto ErrorArrays;
  memcpy( tiling_->attributeArray()->data, attr->data, Bpp[attr->type]*attr->size );

  Free_Array(attr);
  emit changed();
  return;
  // Error handling
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
{ 
  if(tiling_)
  { 
    device::FieldOfViewGeometry fov(tiling_->fov());
    TRectVerts rect;
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
{ 
  if(tiling_)
  { 
    *out = tiling_->latticeToStageTransform();
    return true;
  }
  return false;
}

bool fetch::ui::TilingController::latticeTransform( QTransform *out )
{ 
  if(tiling_)
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

bool fetch::ui::TilingController::latticeShape( unsigned *width, unsigned *height )
{ 
  if(tiling_)
  {      
    mylib::Array *lattice = tiling_->attributeArray();
    *width  = lattice->dims[0];
    *height = lattice->dims[1]; 
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
bool fetch::ui::TilingController::latticeAttrImage(QImage *out)
{
  if(!tiling_)
    return false;  
  unsigned w,h;
  latticeShape(&w,&h);
  mylib::Array 
    *lattice = tiling_->attributeArray(),
     plane   = *lattice;
  Vector3z ir = stage_->getPosInLattice();
  mylib::Get_Array_Plane(&plane,ir[2]);  
  *out = QImage(AUINT8(&plane),w,h,QImage::Format_RGB32); //QImage requires a uchar* for the data
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

bool fetch::ui::TilingController::mark( const QPainterPath& path, device::StageTiling::Flags attr, QPainter::CompositionMode mode )
{    
  if(tiling_)
  {      

    // 1. Path is in scene coords.  transform to lattice coords
    //    Getting the transform is a bit of a pain bc we have to go from 
    //    Eigen to Qt :(
    QTransform l2s, s2l;
    latticeTransform(&l2s);
    s2l = l2s.inverted();
    QPainterPath lpath = s2l.map(path);
                                  
    // 2. Get access to the attribute data
    QImage im;
    latticeAttrImage(&im);
    QPainter painter(&im);

    // 3. Fill in the path
#if DEBUG_DUMP_TILING_MARK_DATA
    im.save("TilingController_mark__before.tif");
#endif
    painter.setCompositionMode(mode);
    painter.fillPath(lpath,QColor((QRgb)attr)); // (QRgb) cast is a uint32 cast
    //SHOW(lpath);
#if DEBUG_DUMP_TILING_MARK_DATA
    im.save("TilingController_mark__after.tif");
    warning("Dumping Tiling mark data"ENDL);
#endif
    return true;
  }
  return false;
}

bool fetch::ui::TilingController::mark_all( device::StageTiling::Flags attr, QPainter::CompositionMode mode )
{
  if(!is_valid()) return false;
  // 1. Get access to the attribute data
  QImage im;
  latticeAttrImage(&im);
  QPainter painter(&im);
  // 2. fill   
  //im.save("TilingController_mark_all__before.tif");
  painter.setCompositionMode(mode);
  painter.fillRect(im.rect(),QColor((QRgb)attr));
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

// returns false if tiling is invalid or if stage_coord is oob
float roundf(float x) {return floor(0.5f+x);}
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
{ 
  if(!is_valid())
    return false;
  
  const device::StageTravel& travel = tiling_->travel();
  size_t z = tiling_->plane_mm();
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
    mark_all(device::StageTiling::Addressable,QPainter::RasterOp_NotSourceAndDestination);
  }


  return true;
}

//////////////////////////////////////////////////////////////////////////
//  PlanarStageController   //////////////////////////////////////////////
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
    this,SLOT(invalidateTiling()) );
  
  connect(
    &listener_,SIGNAL(sig_moved()),
    this,SIGNAL(moved()),
    Qt::QueuedConnection);  

  connect(
    &listener_,SIGNAL(sig_velocityChanged()),
    this,SIGNAL(velocityChanged()),
    Qt::QueuedConnection);

  connect(this,SIGNAL(moved()),tiling(),SIGNAL(planeChanged()));

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
