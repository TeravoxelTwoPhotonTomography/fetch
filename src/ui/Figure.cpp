#include <util/util-gl.h>
#include <QtGui>
#include <assert.h>
#include "Figure.h"
#include <math.h>

#define MOUSEWHEEL_SCALE (float)120.0
#define MOUSEWHEEL_POW   2.0

namespace fetch {
namespace ui {

class ZoomableView:public QGraphicsView
{
public:
	ZoomableView(QGraphicsScene *scene, QWidget *parent=0):QGraphicsView(scene,parent) 
  {
  }
	
	virtual void	wheelEvent(QWheelEvent *event)
	{
		float s,d;
		d = event->delta()/MOUSEWHEEL_SCALE;
		s = powf(MOUSEWHEEL_POW,-d);
		scale(s,s);
	}
};

Figure::Figure(QWidget *parent/*=0*/)
:QWidget(parent)
{	
  QGLWidget *viewport;
  _view = new ZoomableView(&_scene);  
  _view->setViewport(viewport = new QGLWidget);   
	viewport->makeCurrent();
  checkGLError();
  initOpenGLSentinal();
	assert(viewport->context()->isValid());
  assert(viewport->isValid());  

  _view->setDragMode(QGraphicsView::ScrollHandDrag); //RubberBandDrag would be nice for zooming...but need a change of mode
  _view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  _view->setRenderHints(
    QPainter::HighQualityAntialiasing |
    QPainter::TextAntialiasing);

  _scene.setBackgroundBrush(Qt::black);

  _item = new ImItem;
  _scene.addItem(_item);
  checkGLError();
  
	QGridLayout *layout = new QGridLayout;
	layout->setContentsMargins(0,0,0,0);
	layout->addWidget(_view, 0, 0);
	setLayout(layout);
}

struct OpenImageWidgetSet : QSet<Figure*>
{
  virtual ~OpenImageWidgetSet() 
  {
    //foreach(Figure* w, *this)
    //  delete w;
  }
};

static OpenImageWidgetSet g_open;

Figure* imshow(mylib::Array *im)
{ Figure *w = new Figure;
  g_open.insert(w);
  checkGLError();
	w->imshow(im);
	checkGLError();
  w->show();
	return w;
}

void imclose(Figure *w/*=0*/)
{
  if(!w)
  { foreach(Figure *w,g_open)
    { assert(w);
      imclose(w);
    }
    return; 
  }
  assert(g_open.remove(w));
  w->close();
  delete w;
}

//end fetch::ui
}
}