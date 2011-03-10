#include <util/util-gl.h>
#include <QtGui>
#include <assert.h>
#include "Figure.h"
#include <math.h>

#define MOUSEWHEEL_SCALE (float)120.0
#define MOUSEWHEEL_POW   2.0



#define HERE printf("[xxxx] At %s(%d)\n",__FILE__,__LINE__)
namespace mylib {
#include <array.h>
}
using namespace mylib;

namespace fetch{
namespace ui{

ZoomableView::ZoomableView(QGraphicsScene *scene, QWidget *parent)
	: QGraphicsView(scene,parent) 
  , _scalebar(1.0/(200e-9))
{	QObject::connect(this     ,SIGNAL(zoomChanged(double)),
									&_scalebar,SLOT(setZoom(double)));
  setRenderHints(QPainter::HighQualityAntialiasing|QPainter::TextAntialiasing);
}

void	ZoomableView::wheelEvent(QWheelEvent *event)
{
	float s,d;
	d = event->delta()/MOUSEWHEEL_SCALE;
	s = powf(MOUSEWHEEL_POW,-d);
	scale(s,s);
	emit zoomChanged(s);
}

void
ZoomableView::drawForeground(QPainter* painter, const QRectF& rect)
{ QRectF vp(viewport()->geometry());
	QRectF sc = _scalebar.boundingRect();
	sc.moveBottomRight(vp.bottomRight());
	
	painter->resetTransform();
	// translate, but allow for a one px border 
	// between text and the edge of the viewport
	painter->translate(sc.topLeft()-QPointF(2,2));
	_scalebar.paint(painter, rect);
}

Figure::Figure(QWidget *parent/*=0*/)
:QWidget(parent)
{	
  QGLWidget *viewport;
  _view = new ZoomableView(&_scene);  
	//_view = new QGraphicsView(&_scene);  
  _view->setViewport(viewport = new QGLWidget);   
	viewport->makeCurrent();
  checkGLError();
  initOpenGLSentinal();
	assert(viewport->context()->isValid());
  assert(viewport->isValid());
  
  _view->setDragMode(QGraphicsView::ScrollHandDrag); //RubberBandDrag would be nice for zooming...but need a change of mode
  _view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

  _scene.setBackgroundBrush(Qt::black);

	_item = new ImItem;
	_scene.addItem(_item);
  checkGLError();

	QGridLayout *layout = new QGridLayout;
	layout->setContentsMargins(0,0,0,0);
	layout->addWidget(_view, 0, 0);
	setLayout(layout);

  readSettings();
}

void
Figure::readSettings()
{ QSettings settings;
  settings.beginGroup("figure");
  //resize(settings.value("size",size()).toSize());
  move(  settings.value("pos" ,pos()).toPoint());
  settings.endGroup();
}

void
Figure::writeSettings()
{ QSettings settings;
  settings.beginGroup("figure");
  //settings.setValue("size",size());
  settings.setValue("pos" ,pos());
  settings.endGroup();
}

void
Figure::closeEvent(QCloseEvent *event)
{ //always accept
  writeSettings();
  event->accept();
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
{ HERE; Print_Inuse_List(stderr,1);
	Figure *w = new Figure;
  g_open.insert(w);
  checkGLError();
  HERE; Print_Inuse_List(stderr,1);
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

}} //end namespace fetch::ui
