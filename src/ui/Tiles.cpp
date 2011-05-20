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

const int   GRID_ROWS = 500,
            GRID_COLS = 700;
const float         W = 80.0,
                    H = 80.0;

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
{ QPoint r(myround(pos.x()/W),myround(pos.y()/H));
  const QRect bounds(0,0,GRID_COLS,GRID_ROWS);
  if(bounds.contains(r))
    icursor_ = r.y()*GRID_COLS+r.x();
   
}

QRectF TilesView::boundingRect() const
{
  return bbox_;
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
  GLsizei rect_stride = 4,
          nrect = ibo_.size()/sizeof(GLuint)/rect_stride;  
  glDrawElements(GL_QUADS,nrect*rect_stride,GL_UNSIGNED_INT,NULL);
  ibo_.release();         
     
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);
}

void TilesView::draw_cursor_()
{
  const size_t rect_stride_bytes = sizeof(GLuint)*4; 
  if(icursor_>=0)
  {
    DBG( vbo_.bind() );
    DBG( ibo_.bind() );
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3,GL_FLOAT,3*sizeof(float),NULL);
    glColor4f(1.0f,1.0f,1.0f,0.3f);
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

void TilesView::addSelection( const QPainterPath& path )
{  QColor c(255,255,0,255);
   QPainter painter(latticeImage_);       
   painter.scale(1.0/W,1.0/H);                                             // - path is in scene coords...need to transform to lattice coords
   painter.setPen(c);                                                      // - lattice to scene is a scaling by (W,H)
   painter.setBrush(c);                                                    // - tiles are anchored in the center.   
   for(int ivert=0;ivert<4;++ivert)
   {     
     painter.drawPath(path);
     painter.translate(0.0,GRID_ROWS*H);
   }   
   updateCBO();
   update();
}

void TilesView::removeSelection( const QPainterPath& path )
{
   QColor c(0,255,255,255);
   QPainter painter(latticeImage_);

   painter.scale(1.0/W,1.0/H);                                             // - path is in scene coords...need to transform to lattice coords
   painter.setPen(c);                                                      // - lattice to scene is a scaling by (W,H)
   painter.setBrush(c);                                                    // - tiles are anchored in the center.   
     
   for(int ivert=0;ivert<4;++ivert)                                        // - need to draw for all four vertexes
   {     
     painter.drawPath(path);
     painter.translate(0.0,GRID_ROWS*H);                                   // must translate before the scaling
   }   
   updateCBO();
   update();
}

void TilesView::initIBO()
{
  DBG( ibo_.create() );
  DBG( ibo_.bind() );
  ibo_.setUsagePattern(QGLBuffer::StaticDraw);
  ibo_.allocate(sizeof(GLuint)*4*GRID_ROWS*GRID_COLS);                   // number of verts
  ibo_.release();
  checkGLError();
}

void TilesView::updateIBO()
{ 
  GLuint vert_stride = GRID_COLS*GRID_ROWS;
  DBG( ibo_.bind() );  
  GLuint *idata = (GLuint*)ibo_.map(QGLBuffer::WriteOnly);
  for(GLuint irect=0;irect<GRID_ROWS*GRID_COLS;++irect)
  { GLuint *row = idata + 4*irect;
  for(GLuint ivert=0;ivert<4;++ivert)
    row[ivert] = irect + ivert*vert_stride;
  }
  DBG( ibo_.unmap() );
  ibo_.release();
  checkGLError();
}

void TilesView::initVBO()
{
  DBG( vbo_.create() );
  DBG( vbo_.bind() );
  vbo_.setUsagePattern(QGLBuffer::StaticDraw);
  vbo_.allocate(sizeof(float)*12*GRID_COLS*GRID_ROWS);                     // 4 verts per rect, 3 floats per vert = 12 floats per rect
  vbo_.release();
  checkGLError();
}

void TilesView::updateVBO()
{
  
  float verts[] = {      //V3F - use this as a template
    -59.f,   -39.f,    0.0f,             
    39.f,   -59.f,    0.0f,             
    59.f,    39.f,    0.0f,             
    -39.f,    59.f,    0.0f,             
  }; 
  float dr[] = {W,H,0.0};  

  bbox_.setTopLeft(QPointF(verts[0],verts[1]));
  bbox_.setWidth (W*(GRID_COLS+1));
  bbox_.setHeight(H*(GRID_ROWS+1));  

  DBG( vbo_.bind() );
  float *vdata = (float*)vbo_.map(QGLBuffer::WriteOnly);
  { 
    const int 
      vert_stride = 3,
      latt_stride = GRID_COLS*GRID_ROWS*vert_stride;
    for(int ivert=0;ivert<4;++ivert)
    { 
      float *dest = vdata+ivert*latt_stride,
        *src  = verts+ivert*vert_stride;
      for(int ix=0; ix<GRID_COLS;++ix)
        for(int iy=0; iy<GRID_ROWS;++iy)      
        { 
          int irect = ix+iy*GRID_COLS;
          float *r = dest + irect*vert_stride;        
          memcpy(r,src,vert_stride*sizeof(float));
          r[0]+=dr[0]*ix;
          r[1]+=dr[1]*iy;
        }
    }
  }
  DBG( vbo_.unmap() );
  vbo_.release();
  checkGLError();

}

void TilesView::initCBO()
{
  latticeImage_ = new QImage(GRID_COLS,4*GRID_ROWS,QImage::Format_ARGB32_Premultiplied);

  DBG( cbo_.create() );
  cbo_.setUsagePattern(QGLBuffer::StaticDraw);  
  DBG( cbo_.bind() );
  cbo_.allocate(sizeof(GLubyte)*16*GRID_COLS*GRID_ROWS);                   // 4 verts per rect, 4 ubytes per vert = 16 ubytes per rect
  cbo_.release();  
  checkGLError();
}
void TilesView::updateCBO()
{
  /// Do the initial draw to a local buffer

  QPainter painter(latticeImage_);  
  // QT->OpenGL switches red and blue.  This is a -60 degress rotation in the hue colorwheel.
  painter.fillRect(QRectF(latticeImage_->rect()),QBrush(QColor::fromHsvF(0.7,1.0,1.0,0.3)));
  QRectF stage_addressable_rect(GRID_COLS/4.0,GRID_ROWS/4.0,GRID_COLS/2.0,GRID_COLS/2.0);
  QColor stage_addressable_color(QColor::fromHsvF(0.3,1.0,1.0,0.5));
  painter.fillRect(stage_addressable_rect,QBrush(stage_addressable_color));
  for(int i=0;i<3;++i)
  {
    stage_addressable_rect.translate(0.0,GRID_ROWS);
    painter.fillRect(stage_addressable_rect,QBrush(stage_addressable_color));
  }

  /// copy the local buffer to the vertex color buffer object
  DBG( cbo_.bind() );
  GLubyte *cdata = (GLubyte*)cbo_.map(QGLBuffer::WriteOnly);
  memcpy(cdata,latticeImage_->constBits(),latticeImage_->byteCount());
  DBG( cbo_.unmap() );
  cbo_.release();
  checkGLError();
}

void TilesView::update_tiling()
{

}

void TilesView::show( bool tf )
{

}

}} // end namepsace fetch::ui 
