#ifdef _WIN32
#include <GL/glew.h>
#endif
#include <QtGui>

#include <util/util-gl.h>
#include "tiles.h"
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
    latticeImage_(NULL),
    is_active_(false)
{ 
  setAcceptHoverEvents(true);
 // setFlags(ItemIsSelectable);

  connect(
    tc,SIGNAL(changed()),
    this,SLOT(update_tiling())
    );
  connect(
    tc,SIGNAL(show(bool)),
    this,SLOT(show(bool))
    );
    
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
  //glEnableClientState(GL_COLOR_ARRAY);
  glColor4f(1.0,0.0,0.5,0.5);

  DBG( vbo_.bind() );
  glVertexPointer(3,GL_FLOAT,3*sizeof(float),(void*)0);
  vbo_.release();  
/*
  DBG( cbo_.bind() );
  glColorPointer(4,GL_UNSIGNED_BYTE,4*sizeof(GLubyte),(void*)0);
  cbo_.release();*/
  
  DBG( ibo_.bind() );
  GLsizei rect_stride = 4, // verts per rect
          nrect = ibo_.size()/sizeof(GLuint)/rect_stride;  
  glDrawElements(GL_QUADS,nrect*rect_stride,GL_UNSIGNED_INT,NULL);
  ibo_.release();         
     
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);
}

void TilesView::draw_cursor_()
{
  const size_t rect_stride_bytes = sizeof(GLuint)*4; //4 verts per rect
  if(icursor_>=0)
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
  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

  draw_cursor_();

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
  QPainter painter(latticeImage_);
  QTransform l2s,s2l;
  
  if(!tc_->latticeTransform(&l2s))
    return;
  s2l = l2s.inverted();
  painter.setTransform(s2l);                                               // - path is in scene coords...need to transform to lattice coords
  painter.setPen(pc);                                                      // - lattice to scene is a scaling by (W,H)
  painter.setBrush(bc);                                                    // - tiles are anchored in the center.   

  unsigned w,h;
  tc_->latticeShape(&w,&h);
  QPointF offset(0.0f,h);
  offset = l2s.map(offset);
  for(int ivert=0;ivert<4;++ivert)
   {     
     painter.drawPath(path);
     painter.translate(offset);                                            // translation has to be in stage coords
   }                                                                       
   updateCBO();
   update();
}

void TilesView::addSelection( const QPainterPath& path )
{  QColor c(255,255,0,255);
   tc_->markActive(path);
   paint_lattice_(path,c,c);
}

void TilesView::removeSelection( const QPainterPath& path )
{
   QColor c(0,255,255,255);
   tc_->markInactive(path);
   paint_lattice_(path,c,c);   
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
    vbo_.allocate(sizeof(GLuint)*12*w*h);                   // 4 verts per rect, 3 floats per vert = 12 floats per rect
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
    DBG( vbo_.bind() );
    float *vdata = (float*)vbo_.map(QGLBuffer::WriteOnly);
    { 
      unsigned w,h;
      tc_->latticeShape(&w,&h);
      TTransform l2s;
      const int 
        vert_stride = 3,
        latt_stride = w*h*vert_stride;
      Vector3f r(0.0f,0.0f,0.0f),v;
      tc_->latticeTransform(&l2s);
      
      for(int ivert=0;ivert<4;++ivert)
      { 
        float *dest = vdata+ivert*latt_stride;
        for(unsigned ix=0; ix<w;++ix)
        { r(0)=ix;
          for(unsigned iy=0; iy<h;++iy)      
          { 
            r(1)= iy;
            v = l2s*r + verts.col(ivert);

            int irect = ix+iy*w;
            float *d = dest + irect*vert_stride;
            memcpy(d,v.data(),vert_stride*sizeof(float));
          }
        }
      }
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

  //QPainter painter(latticeImage_);  
  //// QT->OpenGL switches red and blue.  This is a -60 degress rotation in the hue colorwheel.
  //painter.fillRect(QRectF(latticeImage_->rect()),QBrush(QColor::fromHsvF(0.7,1.0,1.0,0.3)));
  //QRectF stage_addressable_rect(GRID_COLS/4.0,GRID_ROWS/4.0,GRID_COLS/2.0,GRID_COLS/2.0);
  //QColor stage_addressable_color(QColor::fromHsvF(0.3,1.0,1.0,0.5));
  //painter.fillRect(stage_addressable_rect,QBrush(stage_addressable_color));
  //for(int i=0;i<3;++i)
  //{
  //  stage_addressable_rect.translate(0.0,GRID_ROWS);
  //  painter.fillRect(stage_addressable_rect,QBrush(stage_addressable_color));
  //}

  ///// copy the local buffer to the vertex color buffer object
  //DBG( cbo_.bind() );
  //GLubyte *cdata = (GLubyte*)cbo_.map(QGLBuffer::WriteOnly);
  //memcpy(cdata,latticeImage_->constBits(),latticeImage_->byteCount());
  //DBG( cbo_.unmap() );
  //cbo_.release();
  //checkGLError();
}

void TilesView::update_tiling()
{  
  if(!tc_->stageAlignedBBox(&bbox_))    
  {
    
  }

}

void TilesView::show( bool tf )
{
   setVisible(tf); 
}

}} // end namepsace fetch::ui 
