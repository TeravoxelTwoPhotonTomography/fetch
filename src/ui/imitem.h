#pragma once

#include <QtGui>
#include <QtOpenGL>

namespace mylib {
#include <array.h>
}

namespace fetch {
namespace ui {

class ImItem: public QGraphicsItem
{
public:
  ImItem();
  virtual ~ImItem();
	
  QRectF boundingRect  () const;                                           // in meters
  void   paint         (QPainter                       *painter,
                        const QStyleOptionGraphicsItem *option,
                        QWidget                        *widget = 0);
	void push(mylib::Array *plane);
  void flip(int isupdate=1);

	inline const QRectF& bbox_px() {return _bbox_px;}

         void   setPixelSizeMicrons(double width, double height);
  inline QSizeF pixelSizeMeters()                                         {return _pixel_size_meters;}
signals:
  void sizeChanged(const QRectF& bbox);                                    // [?] is this ever used?  What units does bbox have here?
protected:
  void updateDisplayLists();
  void _common_setup();
	void _loadTex(mylib::Array *im);
  void _setupShader();

	float _fill;
	
  QRectF _bbox_px;
  QGraphicsSimpleTextItem _text;
  GLuint _hQuadDisplayList;
	GLuint _hTexture;
  GLuint _nchan;

  QGLShaderProgram _shader;
  GLuint _hShaderPlane;
  GLuint _hShaderCmap;
  GLuint _hTexCmap;

  QSizeF _pixel_size_meters;

  int _loaded;
};


}}//end fetch::ui

