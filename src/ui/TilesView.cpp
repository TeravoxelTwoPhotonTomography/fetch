//#ifdef _WIN32
//#include <GL/glew.h>
//#endif
#include <iostream>
#include <QtWidgets>

#include <util/util-gl.h>
#include "TilesView.h"
#include "StageController.h"


#undef HERE
#define HERE "[TILES] At " << __FILE__ << "("<<__LINE__<<")\n"
#define DBG(e) if(!(e)) qDebug() << HERE << "(!)\tCheck failed for Expression "#e << "\n"
#define SHOW(e) qDebug() << HERE << "\t"#e << " is " << e <<"\n"

//const int   GRID_ROWS = 500,
//            GRID_COLS = 700;
//const float         W = 80.0,
//                    H = 80.0;

  /*  Buffer object format:
   *       --------------
   *       |            |
   *       | Upper Left |
   *       | [x,y,z]    |
   *       +------------+   COLS*ROWS*NVERTS
   *       |            |
   *       | Upper Right|
   *       |            |
   *       +------------+ 2*COLS*ROWS*NVERTS
   *       |            |
   *       | Lower Right|
   *       |            |
   *       +------------+ 3*COLS*ROWS*NVERTS
   *       |            |
   *       | Lower Left |
   *       |            |
   *       +------------+
   *
   */

namespace fetch {
namespace ui {


TilesView::TilesView(TilingController *tc, QGraphicsItem *parent)
  :
    QGraphicsObject(parent),
    tc_(tc),
    vbo_(QGLBuffer::VertexBuffer),
    ibo_(QGLBuffer::IndexBuffer),
    cbo_(QGLBuffer::VertexBuffer),
    icursor_(-1),
    bbox_(0,0,10,10),
    color_table_attr2idx_(256),
    color_table_idx2rgb_(256),
    latticeImage_(NULL),
    is_active_(false)
{
  setAcceptHoverEvents(true);
  init_color_tables();
 // setFlags(ItemIsSelectable);

  connect(
    tc,SIGNAL(changed()),
    this,SLOT(update_tiling())
    );
  connect(
    tc,SIGNAL(show(bool)),
    this,SLOT(show(bool))
    );
  connect(
    tc,SIGNAL(tileDone(unsigned, unsigned int)),
    this,SLOT(refreshLatticeAttributes(unsigned,unsigned int))
    );
  connect(
    tc,SIGNAL(planeChanged()),
    this,SLOT(refreshPlane()));

  initVBO();
  updateVBO();

  initCBO();
  updateCBO();

  initIBO();
  updateIBO();
}

#define myround(e) floor(0.5+(e))
void TilesView::init_cursor_(const QPointF& pos)
{
  unsigned i;
  Vector3f r(pos.x(),pos.y(),0.0);
  if(tc_->mapToIndex(r,&i))
    icursor_ = i;
  else
    icursor_ = -1;
}

QRectF TilesView::boundingRect() const
{
  return bbox_;  // updated by update_tiling()
}

void TilesView::draw_grid_()
{
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  //glColor4f(1.0,0.0,0.5,0.5);

  DBG( vbo_.bind() );
  glVertexPointer(3,GL_FLOAT,3*sizeof(float),(void*)0);
  vbo_.release();

  DBG( cbo_.bind() );
  glColorPointer(4,GL_UNSIGNED_BYTE,4*sizeof(GLubyte),(void*)0);
  cbo_.release();

  DBG( ibo_.bind() );
  GLsizei rect_stride = 4, // verts per rect
          nrect = ibo_.size()/sizeof(GLuint)/rect_stride;
  glDrawElements(GL_QUADS,nrect*rect_stride,GL_UNSIGNED_INT,NULL);
  ibo_.release();

  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);
  checkGLError();
}

void TilesView::draw_cursor_()
{
  const size_t rect_stride_bytes = sizeof(GLuint)*4; //4 verts per rect
  if(icursor_>=0 && tc_->is_valid())
  {
    DBG( vbo_.bind() );
    DBG( ibo_.bind() );
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3,GL_FLOAT,3*sizeof(float),NULL);
    glColor4f(1.0f,1.0f,1.0f,0.3f);    // hover-cursor color
    glDrawElements(GL_QUADS,4,GL_UNSIGNED_INT,(void*)(icursor_*rect_stride_bytes));
    glDisableClientState(GL_VERTEX_ARRAY);
    ibo_.release();
    vbo_.release();
    checkGLError();
  }
}

void TilesView::paint(QPainter                       *painter,
                      const QStyleOptionGraphicsItem *option,
                      QWidget                        *widget)
{
  painter->beginNativePainting();
  checkGLError();

  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

  glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
  draw_grid_();
  draw_cursor_();
  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

  // paint a little square to indicate the paint
  glEnable(GL_SCISSOR_TEST);
  glScissor(16, 16, 16, 16);
  glClearColor(1, 1, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_SCISSOR_TEST);
  checkGLError();

  painter->endNativePainting();
}

void TilesView::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{ init_cursor_(event->pos());
  update();
}

void TilesView::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{ icursor_ = -1;
  update();
}

TilesView::~TilesView()
{
  if(latticeImage_) delete latticeImage_;
}

void TilesView::paint_lattice_(const QPainterPath& path, const QColor& pc, const QColor &bc)
{

  QTransform l2s,s2l;
  if(!tc_->latticeTransform(&l2s))
    return;
  s2l = l2s.inverted();
  {
    QPainter painter(latticeImage_);
    QPainterPath p = s2l.map(path); //path in lattice space
    painter.setPen(pc);
    painter.setBrush(bc);

    unsigned w,h;
    tc_->latticeShape(&w,&h);
    QPointF offset(0.0f,h);
    for(int ivert=0;ivert<4;++ivert)
    {
      painter.drawPath(p);
      painter.translate(offset);
    }
  }
  updateCBO();
  update();
}

void TilesView::addSelection( const QPainterPath& path )
{
  tc_->markActive(path);
  paint_lattice_attribute_image_();
  updateCBO();
  update();
}

void TilesView::removeSelection( const QPainterPath& path )
{
  tc_->markInactive(path);
  paint_lattice_attribute_image_();
  updateCBO();
  update();
}

void TilesView::markSelectedAreaUserReset(const QPainterPath& path)
{
  tc_->markUserReset(path);
  paint_lattice_attribute_image_();
  updateCBO();
  update();
}

void TilesView::markSelectedAreaAsDone(const QPainterPath& path)
{
  tc_->markDone(path);
  paint_lattice_attribute_image_();
  updateCBO();
  update();
}

void TilesView::markSelectedAreaAsSafe(const QPainterPath& path)
{
  tc_->markSafe(path);
  paint_lattice_attribute_image_();
  updateCBO();
  update();
}

void TilesView::markSelectedAreaAsNotSafe(const QPainterPath& path)
{
  tc_->markNotSafe(path);
  paint_lattice_attribute_image_();
  updateCBO();
  update();
}

void TilesView::markSelectedAreaAsNotDone(const QPainterPath& path)
{
  tc_->markNotDone(path);
  paint_lattice_attribute_image_();
  updateCBO();
  update();
}

void TilesView::markSelectedAreaAsExplorable(const QPainterPath& path)
{
  tc_->markAllPlanesExplorable(path);
  paint_lattice_attribute_image_();
  updateCBO();
  update();
}

void TilesView::markSelectedAreaAsNotExplorable(const QPainterPath& path)
{
  tc_->markAllPlanesNotExplorable(path);
  paint_lattice_attribute_image_();
  updateCBO();
  update();
}

void TilesView::fillActive()
{ tc_->fillActive();
  paint_lattice_attribute_image_();
  updateCBO();
  update();
}

void TilesView::dilateActive()
{ tc_->dilateActive();
  paint_lattice_attribute_image_();
  updateCBO();
  update();
}

void TilesView::initIBO()
{
  unsigned w,h;
  if(!ibo_.isCreated())
  {
    DBG(ibo_.create());
    ibo_.setUsagePattern(QGLBuffer::StaticDraw);
  }
  if(tc_->latticeShape(&w,&h))
  {
    DBG( ibo_.bind() );
    ibo_.allocate(sizeof(GLuint)*4*w*h);                   // number of verts
    ibo_.release();
    checkGLError();
  }
}

void TilesView::updateIBO()
{
  unsigned w,h;
  if(!ibo_.isCreated())
    initIBO();
  if(tc_->latticeShape(&w,&h))
  {
    GLuint vert_stride = w*h;
    DBG( ibo_.bind() );
    GLuint *idata = (GLuint*)ibo_.map(QGLBuffer::WriteOnly);
    for(GLuint irect=0;irect<w*h;++irect)
    {
      GLuint *row = idata + 4*irect;
      for(GLuint ivert=0;ivert<4;++ivert)
        row[ivert] = irect + ivert*vert_stride;
    }
    DBG( ibo_.unmap() );
    ibo_.release();
    checkGLError();
  }
}

void TilesView::initVBO()
{
  unsigned w,h;
  if(!vbo_.isCreated())
  {
    DBG(vbo_.create());
    vbo_.setUsagePattern(QGLBuffer::StaticDraw);
  }
  if(tc_->latticeShape(&w,&h))
  {
    DBG( vbo_.bind() );
    vbo_.allocate(sizeof(float)*12*w*h);                   // 4 verts per rect, 3 floats per vert = 12 floats per rect
    vbo_.release();
    checkGLError();
  }
}

#include <Eigen\Core>
void TilesView::updateVBO()
{
  if(!vbo_.isCreated())
    initVBO();

  TilingController::TRectVerts verts;
  if(tc_->fovGeometry(&verts))
  {
    unsigned w,h;
    tc_->latticeShape(&w,&h);
    DBG( vbo_.bind() );
    float *vdata = (float*)vbo_.map(QGLBuffer::WriteOnly);

    /// Load in verts
    {
      float *d = vdata;
      for(int ivert = 0;ivert<4;++ivert)
      {
        const float
          x = verts(0,ivert),
          y = verts(1,ivert),
          z = verts(2,ivert);
        for(unsigned i=0;i<w*h;++i)
        {
          *d++ = x;        // [ngc][2013-09] FIXME ug here, verts array must be out of sink w tiling
          *d++ = y;
          *d++ = z;
        }
      }

    }

    Map<Matrix3Xf,Unaligned> vmat(vdata,3,w*h*4);

    /// make lattice coords
    Matrix3Xf lcoord(3,w*h),scoord(3,w*h);
    {
      float *d = lcoord.data();
      for(float iy=0;iy<h;++iy)
      {
        for(float ix=0;ix<w;++ix)
        {
          *d++ = ix;
          *d++ = iy;
          *d++ = 0.0;
        }
      }

    }

    /// transform lattice coords
    TTransform l2s;
    tc_->latticeTransform(&l2s);
    scoord.noalias() = l2s * lcoord;  // FIXME: SLOW faster but still slower than it ought to be [~1-2s per 1e6 rect debug]

    /// translate verts
    {
      const int N = w*h;
      for(int ivert=0;ivert<4;++ivert) // FIXME: SLOW
        vmat.block(0,ivert*N,3,N).noalias() += scoord;
    }

    DBG( vbo_.unmap() );
    vbo_.release();
    checkGLError();
  }
}

void TilesView::initCBO()
{ unsigned w,h;
  if(!cbo_.isCreated())
    DBG( cbo_.create() );

  if(tc_->latticeShape(&w,&h))
  {
    latticeImage_ = new QImage(w,4*h,QImage::Format_ARGB32_Premultiplied);
    paint_lattice_attribute_image_();

    cbo_.setUsagePattern(QGLBuffer::StaticDraw);
    DBG( cbo_.bind() );
    cbo_.allocate(sizeof(GLubyte)*16*w*h);                   // 4 verts per rect, 4 ubytes per vert = 16 ubytes per rect
    cbo_.release();
    checkGLError();
  }
}
void TilesView::updateCBO()
{
  /// Do the initial draw to a local buffer
  if(!cbo_.isCreated())
    initCBO();

  if(tc_->is_valid())
  {
    //QPainter painter(latticeImage_);
    // QT->OpenGL switches red and blue.  This is a -60 degress rotation in the hue colorwheel.


    //painter.setCompositionMode(QPainter::CompositionMode_Source);
    //painter.fillRect(r,QColor::fromHsvF(0.5,1.0,1.0,0.4));

    /*
    QRectF stage_addressable_rect(GRID_COLS/4.0,GRID_ROWS/4.0,GRID_COLS/2.0,GRID_COLS/2.0);
    QColor stage_addressable_color(QColor::fromHsvF(0.3,1.0,1.0,0.5));
    painter.fillRect(stage_addressable_rect,QBrush(stage_addressable_color));
    for(int i=0;i<3;++i)
    {
    stage_addressable_rect.translate(0.0,GRID_ROWS);
    painter.fillRect(stage_addressable_rect,QBrush(stage_addressable_color));
    }
    */

    /// copy the local buffer to the vertex color buffer object
    DBG( cbo_.bind() );
    GLubyte *cdata = (GLubyte*)cbo_.map(QGLBuffer::WriteOnly);
    memcpy(cdata,latticeImage_->constBits(),latticeImage_->byteCount());
    DBG( cbo_.unmap() );
    cbo_.release();
    checkGLError();
  }
}

void TilesView::update_tiling()
{
  if(!tc_->stageAlignedBBox(&bbox_))
    return;
  prepareGeometryChange();
  initVBO(); // these must be called in case a realloc is needed:(
  initIBO();
  initCBO();

  updateVBO();
  updateIBO();
  updateCBO();
}

static unsigned tobgra(unsigned x)
{ unsigned y=x;
  unsigned char t,*p=(unsigned char*)&y;
  t=p[0]; p[0]=p[2]; p[2]=t;
  return y;
}

void TilesView::show( bool tf )
{
   setVisible(tf);
}
/*
    enum Flags
    {
      Addressable = 1,                                                     ///< indicates the stage should be allowed to move to this tile
      Active      = 2,                                                     ///< indicates this tile is in the region of interest.
      Done        = 4,                                                     ///< indicates the tile has been imaged
      Explorable  = 8,                                                     ///< indicates the tile should be expored for auto-tiling
      TileError   = 16,                                                    ///< indicates there was some error moving to or imaging this tile
      Explored    = 32,                                                    ///< indicates area has already been looked at
      Detected    = 64,                                                    ///< indicates some signal was found at the bootom of this tile
      Safe        = 512,
      Reserved    = 128,                                                   ///< used internally to temporarily mark tiles
      Reserved2   = 256                                                    ///< used internally to temporarily mark tiles
    };
*/
void TilesView::init_color_tables()
{ //Attribute table
  for(int i=0;i<256;++i)
    color_table_attr2idx_[i] = (QRgb)i;
  //Init colormap
  for(int i=0;i<256;++i)
    color_table_idx2rgb_[i] = (QRgb)0xffff00ff; // purple

  // Rules - fetch::device::StageTiling::Flags
  for(int i=0;i<255;++i)
  {
    qreal hue=0.0,sat=0.0,val=0.6,alpha=0.5;
    // base color

    if(i>fetch::device::StageTiling::Addressable)
    { sat=0.7; val=0.7, alpha=0.7; }

#define FLAG(e) ((i&e)==e)
#if 0 // distribute flags trhough the hues
    { unsigned char huebits=0;
    huebits |= FLAG(fetch::device::StageTiling::Explored  )<<0;
    huebits |= FLAG(fetch::device::StageTiling::Detected  )<<1;
    huebits |= FLAG(fetch::device::StageTiling::Active    )<<2;
    huebits |= FLAG(fetch::device::StageTiling::Done      )<<3;
    huebits |= FLAG(fetch::device::StageTiling::TileError )<<4;
    if(huebits==0)
      sat=0.0;
    hue=(qreal)huebits/(qreal)(1<<5);
    }
#else // Layered colors
         if(FLAG(fetch::device::StageTiling::Done))       hue=120.0/360.0; // green
    else if(FLAG(fetch::device::StageTiling::Active))     hue= 60.0/360.0; // yellow
    else if(FLAG(fetch::device::StageTiling::Detected))   hue= 30.0/360.0; // orange
    else if(FLAG(fetch::device::StageTiling::Explorable)) hue=  0.0/360.0; // red
    else if(FLAG(fetch::device::StageTiling::Safe))       hue=180.0/360.0; // lt blue
#endif
#undef FLAG

    if(!(i & fetch::device::StageTiling::Addressable))
    { if(i==0) alpha=0.0;
      val*=0.5;
    }

    if( i & fetch::device::StageTiling::Explorable)        {val=sat=1.0;}
    if( i & fetch::device::StageTiling::Explored)          {val=1.0; sat=0.8;}
    QColor color;
    color.setHsvF(hue,sat,val,alpha);
    color_table_idx2rgb_[i]=tobgra(color.rgba());
  }
}

void TilesView::paint_lattice_attribute_image_()
{
  if(!latticeImage_) return;
  QPainter painter(latticeImage_);
  painter.setCompositionMode(QPainter::CompositionMode_Clear);
  QRectF r(latticeImage_->rect());
  painter.fillRect(r,Qt::SolidPattern);

  QImage attr;
  if(!tc_->latticeAttrImage(&attr)) // locks the stage's tiling mutex on success as well as the tiling
    return;
  //attr.save("TilesView_initCBO__attr.tif");
  QImage indexed_attr = attr.convertToFormat(QImage::Format_Indexed8,color_table_attr2idx_);
  tc_->stage()->tiling()->unlock();
  tc_->stage()->tilingUnlock(); // all done w attr
  //indexed_attr.save("TilesView_initCBO__indexed_attr__before.tif");
  indexed_attr.setColorTable(color_table_idx2rgb_);
  //indexed_attr.save("TilesView_initCBO__indexed_attr__after.tif");

  unsigned w,h;
  tc_->latticeShape(&w,&h);
  QPointF offset(0.0f,h);
  painter.setCompositionMode(QPainter::CompositionMode_Source);
  for(int ivert=0;ivert<4;++ivert)
  {
    painter.drawImage(indexed_attr.rect(),indexed_attr);
    painter.translate(offset);
  }
  //latticeImage_->save("TilesView_initCBO_latticeImage__after.tif");
}

void TilesView::refreshPlane()
{
  if(!tc_->is_valid())
    return;
  paint_lattice_attribute_image_();
  updateCBO();
  update();
}

void TilesView::refreshLatticeAttributes(unsigned itile, unsigned int attr)
{
  if(!tc_->is_valid())
    return;
  paint_lattice_attribute_image_();
  updateCBO();
  update();
}

}} // end namepsace fetch::ui
