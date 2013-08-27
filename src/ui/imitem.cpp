#if _MSC_VER
#include <GL/glew.h>
#endif
#include <QtWidgets>
//#include <QtOpenGLExtensions\qopenglextensions.h>



#include "ui/uiunits.h"
#include "ImItem.h"
#include <util/util-mylib.h>
namespace mylib {
#include <array.h>
#include <image.h>
#include <histogram.h>
}

#include <util/util-gl.h>
#include <assert.h>
#include "config.h"


#undef HERE
#define HERE printf("[ImItem] At %s(%d)\n",__FILE__,__LINE__)

#define CHKERR( expr )     {if(expr) {error("(%s,%d) Expression indicated failure:\r\n\t%s\r\n",__FILE__,__LINE__,#expr);}} 0 //( (expr), #expr, error  ))
#define CHKJMP( expr,lbl )  goto_if(!(expr),lbl)

namespace fetch {
namespace ui {


using namespace units;


ImItem::ImItem()
: _fill(1.0),
  _gain(1.0),
  _bias(0.0),
  _gamma(1.0),
  _text("Nothing to see here :/"),
  _hQuadDisplayList(0),
  _hTexture(0),
  _bbox_um(),
  //_pixel_size_meters(100e-9,100e-9),
  _loaded(0),
  _autoscale_next(false),
  _resetscale_next(false),
  _selected_channel(0),
  _show_mode(0),
  _nchan(3),
  _cmap_ctrl_count(1<<14),
  _cmap_ctrl_last_size(0),
  _cmap_ctrl_s(NULL),
  _cmap_ctrl_t(NULL)
{
  _text.setBrush(Qt::yellow);
  //  m_context->makeCurrent(this);
  //initializeOpenGLFunctions();
  initializeOpenGLFunctions();
  //_bbox_px = _text.boundingRect();
  _common_setup();
}

ImItem::~ImItem()
{
  glDeleteTextures(1, &_hTexture);
  glDeleteTextures(1, &_hTexCmapCtrlS);
  glDeleteTextures(1, &_hTexCmapCtrlT);
  glDeleteTextures(1, &_hTexCmap);
}

QRectF ImItem::boundingRect() const
{	QRectF bbox;
  if(_loaded)
  {
    bbox = cvt<PIXEL_SCALE,UM>( _bbox_um );
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
    checkGLError();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,_hTexCmap);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    checkGLError();

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D,_hTexCmapCtrlS);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D,_hTexCmapCtrlT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glActiveTexture(GL_TEXTURE0); // Omitting this line will give a stack underflow
    _shader.bind();
    _shader.setUniformValue(_hShaderPlane,0);
    _shader.setUniformValue(_hShaderCmap ,1);
    _shader.setUniformValue("sctrl",2);
    _shader.setUniformValue("tctrl",3);
    _shader.setUniformValue("nchan",(GLfloat)_nchan);
    _shader.setUniformValue("fill" ,(GLfloat)_fill);
    _shader.setUniformValue("gain" ,(GLfloat)_gain);
    _shader.setUniformValue("bias" ,(GLfloat)_bias);
    _shader.setUniformValue("gamma",(GLfloat)_gamma);
    _shader.setUniformValue("show_mode",(GLint)(_show_mode%4));
    checkGLError();
    //glPushMatrix();
    //glRotatef(_rotation_radians*180.0/M_PI,0.0,0.0,1.0);
    glCallList(_hQuadDisplayList);
    //glPopMatrix();
    _shader.release();
    checkGLError();

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_3D,0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D,0);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D,0);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D,0);
    glActiveTexture(GL_TEXTURE0);
    checkGLError();

    // paint a little square to indicate the paint
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 16, 16, 16);
    glClearColor(0, 1, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    checkGLError();
    painter->endNativePainting();

    painter->setPen(Qt::yellow);
    painter->drawRect(boundingRect());
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

    //mylib::Write_Image("typeMapMylibToGLType_before.tif",a,mylib::DONT_PRESS);
    *pa = mylib::Convert_Image(a,mylib::PLAIN_KIND,mylib::FLOAT32_TYPE,32);
    if(mylib::UINT64_TYPE<a->type && a->type<=mylib::INT64_TYPE)
      mylib::Scale_Array(*pa,0.5,1.0); // 0.5*(x+1.0)
    //mylib::Write_Image("typeMapMylibToGLType_after.tif",*pa,mylib::DONT_PRESS);

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
  //glEnable(GL_TEXTURE_3D);
  glBindTexture(GL_TEXTURE_3D, _hTexture);
    GLuint gltype = typeMapMylibToGLType(&im);
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_LUMINANCE,
                 im->dims[0], im->dims[1], _nchan, 0,
                 GL_LUMINANCE,
                 gltype,
                 im->data);
  glBindTexture(GL_TEXTURE_3D, 0);
  //glDisable(GL_TEXTURE_3D);
  checkGLError();
  if(original!=im) // typeMapMylibToGLType() had to make a copy.  Free it now.
    Free_Array(im);
  _loaded = 1;
}

void ImItem::updateDisplayLists()
{
  float t,b,l,r;//,cx,cy;
  //float ph,pw;
  checkGLError();
  b = cvt<PIXEL_SCALE,UM>(_bbox_um.bottom());
  t = cvt<PIXEL_SCALE,UM>(_bbox_um.top());
  l = cvt<PIXEL_SCALE,UM>(_bbox_um.left());
  r = cvt<PIXEL_SCALE,UM>(_bbox_um.right());

  glNewList(_hQuadDisplayList, GL_COMPILE);
  glBegin(GL_QUADS);
#if 1  // x=0 on right
  glTexCoord2f(1.0, 1.0); glVertex2f(l, t);//glVertex2f(r, b);
  glTexCoord2f(1.0, 0.0); glVertex2f(l, b);//glVertex2f(r, t);
  glTexCoord2f(0.0, 0.0); glVertex2f(r, b);//glVertex2f(l, t);
  glTexCoord2f(0.0, 1.0); glVertex2f(r, t);//glVertex2f(l, b);
#else // x=0 on left
  glTexCoord2f(0.0, 1.0); glVertex2f(l, t);//glVertex2f(r, b);
  glTexCoord2f(0.0, 0.0); glVertex2f(l, b);//glVertex2f(r, t);
  glTexCoord2f(1.0, 0.0); glVertex2f(r, b);//glVertex2f(l, t);
  glTexCoord2f(1.0, 1.0); glVertex2f(r, t);//glVertex2f(l, b);
#endif

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
  {
    if(_nchan!=plane->dims[2])
    {
      _nchan = plane->dims[2];
      _updateCmapCtrlPoints();
    }
  }
  else
    _nchan = 1;

  if(_autoscale_next)
  { _autoscale(plane,_selected_channel,0.01f);
    _autoscale_next = false;
  }
  if(_resetscale_next)
  { _resetscale(_selected_channel);
    _resetscale_next = false;
  }

  //Guess the "fill" for mixing different channels
  // - this is needed for when there are a ton of channels
  //   Typically, there are just a handful but in general there
  //   may be a bunch.
  // - this is not really relevant for the microscope, but
  //   the code is used in other projects of mine so I'm leaving
  //   it.
  _fill = 10.0/_nchan;
  _fill = (_fill>1.0)?1.0:_fill;

  _loadTex(plane);
  checkGLError();
}

#define DBL_EQ(a,b) (abs((b)-(a))<1e-6)

void ImItem::setRotation( double radians )
{ if(!DBL_EQ(radians,_rotation_radians))
  {
    _rotation_radians = radians;
    QGraphicsItem::setRotation(radians*180.0/M_PI);
  }
}

void ImItem::setFOVGeometry( float w_um, float h_um, float rotation_radians )
{
  prepareGeometryChange();
  setRotation(rotation_radians);

  _bbox_um.setHeight(h_um);
  _bbox_um.setWidth(w_um);
  _bbox_um.moveCenter(QPointF(0.0f,0.0f));
  updateDisplayLists();
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
  _shader.setUniformValue("nchan",(GLfloat)_nchan);
  _shader.release();

  loadColormap(":/cmap/2");
}

void ImItem::loadColormap(const QString& filename)
{ QImage cmap;
  CHKJMP(cmap.load(filename),Error);
  //qDebug()<<cmap.format();
  glActiveTexture(GL_TEXTURE1);
  {
    glGenTextures(1, &_hTexCmap);
    glBindTexture(GL_TEXTURE_2D, _hTexCmap);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 cmap.width(), cmap.height(), 0,
                 GL_BGRA, GL_UNSIGNED_BYTE,
                 cmap.constBits());
    glBindTexture(GL_TEXTURE_2D,0);
  }
  glActiveTexture(GL_TEXTURE0);
  checkGLError();
Error:
  return;
}

// Control points for colormapping
// - support different lookup on each channel
// - lookup is into a 2d colormap indexed by s and t (each go 0 to 1).
void ImItem::_updateCmapCtrlPoints()
{
  //if(_nchan<=1) return;
  assert(_cmap_ctrl_count>=2);

  // adjust size if necessary
  { size_t nelem = _cmap_ctrl_count*_nchan;
    CHKJMP(_cmap_ctrl_s = (float*)realloc(_cmap_ctrl_s,sizeof(float)*nelem),MemoryError); // if NULL, realloc mallocs
    CHKJMP(_cmap_ctrl_t = (float*)realloc(_cmap_ctrl_t,sizeof(float)*nelem),MemoryError);

    // fill in uninitizalized data (if any)
    if(nelem>_cmap_ctrl_last_size)
    { int ir,ic;

      // convention here is that channels are evenly spaced along s and intensity on t
      for(GLuint i=_cmap_ctrl_last_size;i<nelem;++i)
      {
        ir = i/_cmap_ctrl_count;
        ic = i%_cmap_ctrl_count;
        _cmap_ctrl_s[i] = (ir+1)/(_nchan+1.0);
        _cmap_ctrl_t[i] = ic/(_cmap_ctrl_count-1.0f);
      }
    }
    _cmap_ctrl_last_size = (GLuint) nelem;
  }

  // upload to the gpu
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D,_hTexCmapCtrlS);
  glTexImage2D(GL_TEXTURE_2D,
    0,
    GL_LUMINANCE,
    _cmap_ctrl_count, _nchan, 0,
    GL_LUMINANCE, GL_FLOAT,
    _cmap_ctrl_s);
  glBindTexture(GL_TEXTURE_2D,0);
  checkGLError();

  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D,_hTexCmapCtrlT);
  glTexImage2D(GL_TEXTURE_2D,
    0,
    GL_LUMINANCE,
    _cmap_ctrl_count, _nchan, 0,
    GL_LUMINANCE, GL_FLOAT,
    _cmap_ctrl_t);
  glBindTexture(GL_TEXTURE_2D,0);

  glActiveTexture(GL_TEXTURE0);
  checkGLError();

  return;
MemoryError:
  error("(%s:%d) Memory (re)allocation failed."ENDL,__FILE__,__LINE__);
}

void ImItem::_autoscale(mylib::Array *data, GLuint ichannel, float percent)
{ mylib::Array c;
  if(ichannel>=data->dims[2] || (ichannel>=_nchan))
  { warning("(%s:%d) Autoscale: selected channel out of bounds."ENDL,__FILE__,__LINE__);
    return;
  }
  mylib::Array *t = Convert_Image(data,mylib::PLAIN_KIND,mylib::FLOAT32_TYPE,32);
  c = *t;    // For Get_Array_Plane
  if(mylib::UINT64_TYPE<data->type && data->type<=mylib::INT64_TYPE)  // if signed integer type
    mylib::Scale_Array(&c,0.5,1.0);                                   //    x = 0.5*(x+1.0)
  mylib::Get_Array_Plane(&c,(mylib::Dimn_Type)ichannel);


  //mylib::Write_Image("ImItem_autoscale_input.tif",data,mylib::DONT_PRESS);
  //mylib::Write_Image("ImItem_autoscale_channel_float.tif",&c,mylib::DONT_PRESS);


  float max,min,m,b;
  mylib::Range_Bundle range;
#if 0
  /* This here in case existing colormapping scheme
     didn't result in enough dynamic range.
  */
  mylib::Array_Range(&range,t);  // min max of entire array
  max = range.maxval.fval;
  min = range.minval.fval;
  _gain = 1.0f/(max-min);
  _bias = min/(min-max);
#endif

  mylib::Array_Range(&range,&c); // min max of single channel
  mylib::Free_Array(t);
  max = _gain*range.maxval.fval+_bias; // adjust for gain and bias
  min = _gain*range.minval.fval+_bias;
  m = 1.0f/(max-min);
  b = min/(min-max);

  for(GLuint i=0;i<_cmap_ctrl_count;++i)
  { float x = i/(_cmap_ctrl_count-1.0f);
    _cmap_ctrl_t[ichannel*_cmap_ctrl_count+i] = m*x+b; // upload to gpu will clamp to [0,1]
  }

  _updateCmapCtrlPoints();
}

void ImItem::_resetscale(GLuint ichannel)
{
  if(ichannel<_nchan)
  { for(GLuint i=0;i<_cmap_ctrl_count;++i)
      _cmap_ctrl_t[ichannel*_cmap_ctrl_count+i] = i/(_cmap_ctrl_count-1.0f);
    _updateCmapCtrlPoints();
  }
}

void ImItem::mouseDoubleClickEvent (QGraphicsSceneMouseEvent *e)
{ _show_mode++;
}

void ImItem::_common_setup()
{
  _hQuadDisplayList = glGenLists(1);
  if(!_hQuadDisplayList)
    qDebug("glGenLists failed?  You probably forgot to setup an active opengl context.");
  _setupShader();
  glGenTextures(1, &_hTexture);
  glGenTextures(1, &_hTexCmapCtrlS);
  glGenTextures(1, &_hTexCmapCtrlT);
  _updateCmapCtrlPoints();
  checkGLError();
}

}} // end fetch::ui