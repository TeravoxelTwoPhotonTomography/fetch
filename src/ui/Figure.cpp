#include <util/util-gl.h>
#include <QtGui>
#include <QtDebug>
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

/************************************************************************/
/* ZOOMABLE VIEW                                                        */
/************************************************************************/

ZoomableView::ZoomableView(QGraphicsScene *scene, QWidget *parent)
	: QGraphicsView(scene,parent) 
  , _scalebar()
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
  notifyZoomChanged();	
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

/************************************************************************/
/* FIGURE                                                               */
/************************************************************************/

Figure::Figure(PlanarStageController *stageController, QWidget *parent/*=0*/)
:QWidget(parent)
{	
  _view = new ZoomableView(&_scene); 
  QGLWidget *viewport; 
	//_view = new QGraphicsView(&_scene);  
  _view->setViewport(viewport = new QGLWidget);   
	viewport->makeCurrent();
  checkGLError();
  initOpenGLSentinal();
	assert(viewport->context()->isValid());
  assert(viewport->isValid());
  
  _view->setDragMode(QGraphicsView::ScrollHandDrag); //RubberBandDrag would be nice for zooming...but need a change of mode
  _view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

  _scene.setBackgroundBrush(Qt::darkRed);

  _stage = new StageView(stageController);
  _scene.addItem(_stage);
  _stage->setZValue(-1); // ensure it gets drawn behind the usual items

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
{
  QSettings settings;
  QStringList keys = settings.allKeys();
#if 0  
  HERE;
  qDebug() << settings.organizationName();
  qDebug() << settings.applicationName();
  qDebug() << (settings.isWritable()?"Writable":"!!! Settings are not writable !!!");
  foreach(QString k,keys)
    qDebug() << "\t" << k;
#endif 
  settings.beginGroup("voxel");
  double w = settings.value("width_um",0.1).toDouble(),
         h = settings.value("height_um",0.1).toDouble();
  setPixelSizeMicrons(w,h);
  settings.endGroup();
}

void
Figure::writeSettings()
{ QSettings settings;
  settings.beginGroup("voxel");
  QSizeF px = _item->pixelSizeMeters();
  settings.setValue("width_um",px.width()*1e6);
  settings.setValue("height_um",px.height()*1e6);
  settings.endGroup();
  settings.sync();
}

Figure::~Figure()
{
  writeSettings();
}

#if 0
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
#endif

}} //end namespace fetch::ui
