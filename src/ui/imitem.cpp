#ifdef _WIN32
#include <GL/glew.h>
#endif
#include <QtGui>
#include "ImItem.h"

namespace mylib {
#include <array.h>
#include <image.h>
}

#include <util/util-gl.h>
#include <assert.h>

namespace fetch {
namespace ui {

ImItem::ImItem()
: _fill(1.0),
  _text("Nothing to see here :/"),
  _hQuadDisplayList(0),
	_hTexture(0),
  _loaded(0)
{ 	
  _text.setBrush(Qt::yellow);
  _bbox = _text.boundingRect();
  _common_setup();
}

ImItem::~ImItem()
{
	glDeleteTextures(1, &_hTexture);
}

QRectF ImItem::boundingRect() const
{
  return _bbox;
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
		glCallList(_hQuadDisplayList);
    _shader.release();
		
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
    // Signed integer types no supported
    -1, //GL_BYTE                      ,   //  INT8    = 4,
    -1, //GL_SHORT                     ,   //  INT16   = 5,
    -1, //GL_INT                       ,   //  INT32   = 6,
    -1                           ,   //  INT64   = 7,
    GL_FLOAT                     ,   //  FLOAT32 = 8,
    -1                               //  FLOAT64 = 9 
                                     //} Array_Type; 
  };
  ret = table[a->type];
  if(ret == (GLuint)-1)
  { *pa = mylib::Convert_Image_Copy(a,mylib::PLAIN_KIND,mylib::FLOAT32,32);
    Free_Array(a); // release the old reference
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
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_LUMINANCE,
							 _bbox.width(), _bbox.height(), _nchan, 0,
							 GL_LUMINANCE,
               gltype,
							 im->data);  
	checkGLError();
  if(original!=im) // typeMapMylibToGLType() had to make a copy.  Free it now.
    Free_Array(im);
  _loaded = 1;
}

void ImItem::updateDisplayLists()
{ 
	float t,b,l,r;
	checkGLError();
	b = _bbox.bottom();
	t = _bbox.top();
	l = _bbox.left();
	r = _bbox.right();
	
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
	
	_bbox = QRectF(-w/2.0,-h/2.0,w,h);
	_loadTex(plane);
	updateDisplayLists();
	checkGLError();
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
	assert(cmap.load(":/cmap/3","JPG"));
  //qDebug()<<cmap.format();
	glGenTextures(1, &_hTexCmap);
	glBindTexture(GL_TEXTURE_2D, _hTexCmap);
	glTexImage2D(GL_TEXTURE_2D, 
							 0, 
							 GL_RGBA, 
							 cmap.width(), cmap.height(), 0, 
							 GL_BGRA, GL_UNSIGNED_BYTE,
							 cmap.constBits());
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