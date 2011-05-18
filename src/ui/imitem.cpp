#ifdef _WIN32
#include <GL/glew.h>
#endif

#include <QtGui>
#include "ui/uiunits.h"
#include "ImItem.h"
#include <util/util-mylib.h>
namespace mylib {
#include <array.h>
#include <image.h>
}

#include <util/util-gl.h>
#include <assert.h>

#undef HERE
#define HERE printf("[ImItem] At %s(%d)\n",__FILE__,__LINE__)

namespace fetch {
namespace ui {


using namespace units;


ImItem::ImItem()
: _fill(1.0),
  _text("Nothing to see here :/"),
  _hQuadDisplayList(0),
	_hTexture(0),
  _pixel_size_meters(100e-9,200e-9),
  _loaded(0)
{ 	
  _text.setBrush(Qt::yellow);
  _bbox_px = _text.boundingRect();
  _common_setup();
}

ImItem::~ImItem()
{
	glDeleteTextures(1, &_hTexture);
}

QRectF ImItem::boundingRect() const
{	QRectF bbox;
  if(_loaded)
  { 
    float t,b,l,r;
    float ph,pw;
    ph = _pixel_size_meters.height();
    pw = _pixel_size_meters.width();
	  b = ph * _bbox_px.bottom();
	  t = ph * _bbox_px.top();
	  l = pw * _bbox_px.left();
	  r = pw * _bbox_px.right();
    bbox = cvt<PIXEL_SCALE,M>( QRectF(QPointF(t,l),QPointF(b,r)) );
    bbox.moveCenter(QPointF(0.0,0.0));
  } else
  { 
    bbox = _text.boundingRect();
    bbox.moveTopLeft(pos());
  }       
  return bbox;  
}

void ImItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget /* = 0 */)
{ 
  if(_loaded)
  {  
    painter->beginNativePainting();
		checkGLError();                                                                              

    glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, _hTexture);
    glTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    glTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP );
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,_hTexCmap);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		
		glActiveTexture(GL_TEXTURE0); // Omitting this line will give a stack underflow
		_shader.bind();
		_shader.setUniformValue(_hShaderPlane,0);
		_shader.setUniformValue(_hShaderCmap ,1);
    _shader.setUniformValue("nchan",(GLfloat)_nchan);
		_shader.setUniformValue("fill",(GLfloat)_fill);
		_shader.setUniformValue("gain",(GLfloat)1.0  );
		_shader.setUniformValue("bias",(GLfloat)0.0  );
    checkGLError();    
    glRotatef(_rotation_radians*180.0/M_PI,0.0,0.0,1.0);
		glCallList(_hQuadDisplayList);
    _shader.release();

    glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D,0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,0);

    // paint a little square to indicate the paint
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 16, 16, 16);
    glClearColor(0, 1, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    checkGLError();
    painter->endNativePainting();
  } else
  {
    // for some reason I have to draw the rect.  Otherwise the text won't always show.
    painter->setPen(QPen(Qt::white,1));
    painter->drawRect(_text.boundingRect());
    _text.paint(painter,option,widget);
  }
}

// Might change *pa to a copy if a type conversion is required.
GLuint typeMapMylibToGLType(mylib::Array **pa)
{ mylib::Array *a = *pa;
  GLuint ret;
  static GLuint table[] = 
  { GL_UNSIGNED_BYTE             ,   //{ UINT8   = 0,
    GL_UNSIGNED_SHORT            ,   //  UINT16  = 1,
    GL_UNSIGNED_INT              ,   //  UINT32  = 2,
    -1                           ,   //  UINT64  = 3,
    // Signed integer types not supported
    -1, //GL_BYTE                ,   //  INT8    = 4,
    -1, //GL_SHORT               ,   //  INT16   = 5,
    -1, //GL_INT                 ,   //  INT32   = 6,
    -1                           ,   //  INT64   = 7,
    GL_FLOAT                     ,   //  FLOAT32 = 8,
    -1                               //  FLOAT64 = 9 
                                     //} Array_Type; 
  };
  ret = table[a->type];
  if(ret == (GLuint)-1)
  { 
    
    *pa = mylib::Convert_Image(a,mylib::PLAIN_KIND,mylib::FLOAT32_TYPE,32);
    
    //Free_Array(a); //[ngc] don't do this...owner is still responsible for his array.
    ret = GL_FLOAT;
  }
  return ret;
}

void ImItem::_loadTex(mylib::Array *im)
{
  mylib::Array *original = im;
	checkGLError();
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_3D);
	glBindTexture(GL_TEXTURE_3D, _hTexture);
    GLuint gltype = typeMapMylibToGLType(&im);
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	  glTexImage3D(GL_TEXTURE_3D, 0, GL_LUMINANCE,
							   _bbox_px.width(), _bbox_px.height(), _nchan, 0,
							   GL_LUMINANCE,
                 gltype,
							   im->data);  
  glBindTexture(GL_TEXTURE_3D, 0);
  glDisable(GL_TEXTURE_3D);
	checkGLError();
  if(original!=im) // typeMapMylibToGLType() had to make a copy.  Free it now.
    Free_Array(im);
  _loaded = 1;
}

void ImItem::updateDisplayLists()
{ 
	float t,b,l,r;//,cx,cy;
  float ph,pw;
	checkGLError();
  ph = _pixel_size_meters.height();
  pw = _pixel_size_meters.width();
	b = cvt<PIXEL_SCALE,M>(ph * _bbox_px.bottom());
	t = cvt<PIXEL_SCALE,M>(ph * _bbox_px.top());
	l = cvt<PIXEL_SCALE,M>(pw * _bbox_px.left());
	r = cvt<PIXEL_SCALE,M>(pw * _bbox_px.right());
  //cx = cvt<PIXEL_SCALE,M>(pw * _bbox_px.center().x());
  //cy = cvt<PIXEL_SCALE,M>(pw * _bbox_px.center().y());
	
	glNewList(_hQuadDisplayList, GL_COMPILE);
	glBegin(GL_QUADS);
	glTexCoord2f(1.0, 0.0); glVertex2f(r, b);
	glTexCoord2f(1.0, 1.0); glVertex2f(r, t);				
	glTexCoord2f(0.0, 1.0); glVertex2f(l, t);				
	glTexCoord2f(0.0, 0.0); glVertex2f(l, b);		
	glEnd();
	glEndList();
	
	checkGLError();
}

void ImItem::flip(int isupdate/*=0*/)
{
	checkGLError();
  if( isupdate )
    update();
	checkGLError();
}

// double-buffers, so you might not see your image right away.
void ImItem::push(mylib::Array *plane)
{
	checkGLError();

	float w,h;
	w = plane->dims[0];
	h = plane->dims[1];
  if(plane->ndims>2)
    _nchan = plane->dims[2];
  else
    _nchan = 1;
	
	//Guess the opacity of different planes
	_fill = 10.0/_nchan;
	_fill = (_fill>1.0)?1.0:_fill;
	
  { QRectF b = QRectF(-w/2.0,-h/2.0,w,h);  
    if(b!=_bbox_px)
      prepareGeometryChange();
  	_bbox_px = b;
  }
	_loadTex(plane);
	updateDisplayLists();
	checkGLError();
}

void ImItem::setPixelSizeMicrons(double w_um, double h_um)
{ QSizeF t;
  t.setHeight(h_um*1e-6);
  t.setWidth(w_um*1e-6);
  if(t!=_pixel_size_meters)
  {
    prepareGeometryChange();
    _pixel_size_meters = t;
    updateDisplayLists();
  }
}

#define DBL_EQ(a,b) (abs((b)-(a))<1e-6)

void ImItem::setRotation( double radians )
{ if(!DBL_EQ(radians,_rotation_radians))
  {
    prepareGeometryChange();
    _rotation_radians = radians;
    updateDisplayLists();
  }
}
        
void ImItem::setPixelGeometry( double w_um, double h_um, double radians )
{ QSizeF t;
  t.setHeight(h_um*1e-6);
  t.setWidth(w_um*1e-6);
  if(  (t!=_pixel_size_meters) ||  !DBL_EQ(radians,_rotation_radians)  )
  {
    prepareGeometryChange();
    _pixel_size_meters = t;
    _rotation_radians = radians;
    updateDisplayLists();
  }

}

#include <QtDebug>
#define SHADERASSERT(expr) if(!(expr)) qCritical()<<_shader.log()
void ImItem::_setupShader()
{
	checkGLError();
  // shader
  SHADERASSERT( _shader.addShaderFromSourceFile(
        QGLShader::Vertex,
        ":/shaders/vert/trivial") );
  SHADERASSERT( _shader.addShaderFromSourceFile(
        QGLShader::Fragment,
        ":/shaders/frag/cmap") );
  SHADERASSERT( _shader.link() );
	SHADERASSERT( _shader.bind() );
	_hShaderPlane = _shader.uniformLocation("plane");
	_hShaderCmap  = _shader.uniformLocation("cmap");
  _shader.setUniformValue("nchan",(GLfloat)1);
  _shader.release();

	// Colormap
	QImage cmap;
  glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
  {
	  assert(cmap.load(":/cmap/2","JPG"));
    //qDebug()<<cmap.format();
	  glGenTextures(1, &_hTexCmap);
	  glBindTexture(GL_TEXTURE_2D, _hTexCmap);
    {
	    glTexImage2D(GL_TEXTURE_2D, 
							     0, 
							     GL_RGBA, 
							     cmap.width(), cmap.height(), 0, 
							     GL_BGRA, GL_UNSIGNED_BYTE,
							     cmap.constBits());
    }
    glBindTexture(GL_TEXTURE_2D,0);
  }
  glDisable(GL_TEXTURE_2D);
	checkGLError();
}

void ImItem::_common_setup()
{
  _hQuadDisplayList = glGenLists(1);
  if(!_hQuadDisplayList)
    qDebug("glGenLists failed?  You probably forgot to setup an active opngl context.");
  _setupShader();
  glGenTextures(1, &_hTexture);
	checkGLError();
}

}} // end fetch::ui