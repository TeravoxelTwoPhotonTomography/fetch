#include <util/util-gl.h>
#include <QtWidgets>
#include <QtDebug>
#include <assert.h>
#include <math.h>
#include "Figure.h"

#define MOUSEWHEEL_SCALE (float)120.0
#define MOUSEWHEEL_POW   2.0



#define _HERE_ printf("[xxxx] At %s(%d)\n",__FILE__,__LINE__)
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
  s = powf(MOUSEWHEEL_POW,d);
  scale(s,s);
  notifyZoomChanged();
}

void
ZoomableView::drawForeground(QPainter* painter, const QRectF& rect)
{ QRectF vp(viewport()->geometry());
  QRectF sc = _scalebar.boundingRect();
  sc.moveBottomRight(vp.bottomRight());
  painter->resetTransform();                                               //painter starts with the scene transform

  {
    QPointF p  = QPointF(   mapFromGlobal(QCursor::pos())),
            ps = mapToScene(mapFromGlobal(QCursor::pos()));
    QStaticText txt(QString("(%1,%2)").arg(ps.x()).arg(ps.y()));
    txt.prepare();
    painter->setPen(Qt::white);
    painter->setBrush(Qt::white);
    painter->drawStaticText(p,txt);
    QRectF r(0.0f,0.0f,10.0f,10.0f);
    r.moveCenter(p);
    painter->drawEllipse(r);
  }

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
,_sc(stageController)
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
  _stage->setZValue(-1);                                                   // ensure it gets drawn behind the usual items
  connect(stageController,SIGNAL(moved()),
          this,SLOT(updatePos()));
  connect(stageController,SIGNAL(moved(QPointF)),
          this,SLOT(updatePos(QPointF)));
  connect(stageController->tiling(),SIGNAL(fovGeometryChanged(float,float,float)),
          this,SLOT(fovGeometryChanged(float,float,float)));

  _item = new ImItem;
  _scene.addItem(_item);
  checkGLError();

  _tv = new TilesView(stageController->tiling());
  _scene.addItem(_tv);
  checkGLError();
  connect(&_scene,SIGNAL(   addSelectedArea(const QPainterPath&)),
              _tv,SLOT(     addSelection(const QPainterPath&)));
  connect(&_scene,SIGNAL(removeSelectedArea(const QPainterPath&)),
              _tv,SLOT(  removeSelection(const QPainterPath&)));

  connect(&_scene,SIGNAL(   markSelectedAreaAsDone(const QPainterPath&)),
              _tv,SLOT(     markSelectedAreaAsDone(const QPainterPath&)));
  connect(&_scene,SIGNAL(markSelectedAreaAsNotDone(const QPainterPath&)),
              _tv,SLOT(  markSelectedAreaAsNotDone(const QPainterPath&)));

  connect(&_scene,SIGNAL(   markSelectedAreaAsExplorable(const QPainterPath&)),
              _tv,SLOT(     markSelectedAreaAsExplorable(const QPainterPath&)));
  connect(&_scene,SIGNAL(markSelectedAreaAsNotExplorable(const QPainterPath&)),
              _tv,SLOT(  markSelectedAreaAsNotExplorable(const QPainterPath&)));

  connect(&_scene,SIGNAL(   markSelectedAreaAsSafe   (const QPainterPath&)),
              _tv,SLOT(     markSelectedAreaAsSafe   (const QPainterPath&)));
  connect(&_scene,SIGNAL(   markSelectedAreaAsNotSafe (const QPainterPath&)),
              _tv,SLOT(     markSelectedAreaAsNotSafe (const QPainterPath&)));
  connect(&_scene,SIGNAL(   markSelectedAreaUserReset(const QPainterPath&)),
              _tv,SLOT(     markSelectedAreaUserReset(const QPainterPath&)));


  QGridLayout *layout = new QGridLayout;
  layout->setContentsMargins(0,0,0,0);
  layout->addWidget(_view, 0, 0);
  setLayout(layout);

  readSettings();
  updatePos();
  createActions();

  setContextMenuPolicy(Qt::ActionsContextMenu);
}

void Figure::createActions()
{ QAction *c;

  c = new QAction(tr("&Fill Active"),this);
  c->setShortcut(QKeySequence(tr("f","Mark|Fill")));
  c->setStatusTip(tr("Fill holes in tile regions marked as active."));
  connect(c,SIGNAL(triggered()),this,SLOT(fillActive()));
  addAction(c);

  c = new QAction(tr("&Dilate Active"),this);
  c->setShortcut(QKeySequence(tr("d","Mark|Dilate")));
  c->setStatusTip(tr("Dilate tile regions marked as active."));
  connect(c,SIGNAL(triggered()),this,SLOT(dilateActive()));
  addAction(c);

  c = new QAction(tr("&Clear Drag Mode"),this);
  c->setShortcut(QKeySequence(tr("c","DragMode|Clear")));
  c->setStatusTip(tr("Change drag mode to none."));
  connect(c,SIGNAL(triggered()),this,SLOT(setDragModeToNoDrag()));
  addAction(c);
  _noDragModeAct = c;

  c = new QAction(tr("&Pan"),this);
  c->setShortcut(QKeySequence(tr("h","DragMode|Pan")));
  c->setStatusTip(tr("Change drag mode to pan."));
  connect(c,SIGNAL(triggered()),this,SLOT(setDragModeToPan()));
  addAction(c);
  _scrollDragModeAct = c;

  c = new QAction(tr("&Select"),this);
  c->setShortcut(QKeySequence(tr("m","DragMode|Select")));
  c->setStatusTip(tr("Change drag mode to select."));
  connect(c,SIGNAL(triggered()),this,SLOT(setDragModeToSelect()));
  addAction(c);
  _rubberBandModeAct = c;

  c = new QAction(tr("Autoscale &0"),this);
  c->setShortcut(QKeySequence(tr("0","Autoscale|Channel0")));
  c->setStatusTip(tr("Autoscale channel 0."));
  connect(c,SIGNAL(triggered()),this,SLOT(autoscale0()));
  addAction(c);

  c = new QAction(tr("Autoscale &1"),this);
  c->setShortcut(QKeySequence(tr("1","Autoscale|Channel1")));
  c->setStatusTip(tr("Autoscale channel 1."));
  connect(c,SIGNAL(triggered()),this,SLOT(autoscale1()));
  addAction(c);

  c = new QAction(tr("Autoscale &2"),this);
  c->setShortcut(QKeySequence(tr("2","Autoscale|Channel2")));
  c->setStatusTip(tr("Autoscale channel 2."));
  connect(c,SIGNAL(triggered()),this,SLOT(autoscale2()));
  addAction(c);

  c = new QAction(tr("Autoscale &3"),this);
  c->setShortcut(QKeySequence(tr("3","Autoscale|Channel3")));
  c->setStatusTip(tr("Autoscale channel 3."));
  connect(c,SIGNAL(triggered()),this,SLOT(autoscale3()));
  addAction(c);

  c = new QAction(tr("Reset 0"),this);
  c->setShortcut(QKeySequence(tr("Shift+0","Autoscale|Channel0")));
  c->setStatusTip(tr("Autoscale channel 0."));
  connect(c,SIGNAL(triggered()),this,SLOT(resetscale0()));
  addAction(c);

  c = new QAction(tr("Reset 1"),this);
  c->setShortcut(QKeySequence(tr("Shift+1","Autoscale|Channel1")));
  c->setStatusTip(tr("Autoscale channel 1."));
  connect(c,SIGNAL(triggered()),this,SLOT(resetscale1()));
  addAction(c);

  c = new QAction(tr("Reset 2"),this);
  c->setShortcut(QKeySequence(tr("Shift+2","Autoscale|Channel2")));
  c->setStatusTip(tr("Autoscale channel 2."));
  connect(c,SIGNAL(triggered()),this,SLOT(resetscale2()));
  addAction(c);

  c = new QAction(tr("Reset 3"),this);
  c->setShortcut(QKeySequence(tr("Shift+3","Autoscale|Channel3")));
  c->setStatusTip(tr("Autoscale channel 3."));
  connect(c,SIGNAL(triggered()),this,SLOT(resetscale3()));
  addAction(c);

  //c = new QAction(tr("Add Tiles"),this);
  //QList<QKeySequence> shortcuts;
  //shortcuts.push_back( QKeySequence(tr("+","TileMode|Add|+")) );
  //shortcuts.push_back( QKeySequence(tr("=","TileMode|Add|=")) );
  //c->setShortcuts(shortcuts);
  //c->setStatusTip(tr("Add selected tiles."));
  //connect(c,SIGNAL(triggered()),this,SLOT(setAddTilesMode()));
  //addAction(c);
  //_addTilesModeAct = c;
  //
  //c = new QAction(tr("Remove Tiles"),this);
  //c->setShortcut(QKeySequence(tr("-","TileMode|Remove")));
  //c->setStatusTip(tr("Remove selected tiles."));
  //connect(c,SIGNAL(triggered()),this,SLOT(setDelTilesMode()));
  //addAction(c);
  //_delTilesModeAct = c;
}

void Figure::setDragModeToNoDrag()
{
  _view->setDragMode(QGraphicsView::NoDrag);
  _scene.setMode(StageScene::DrawRect);
}

void Figure::setDragModeToSelect()
{
  _view->setDragMode(QGraphicsView::NoDrag);
  _scene.setMode(StageScene::DrawRect);
}

void Figure::setDragModeToPan()
{
  _view->setDragMode(QGraphicsView::ScrollHandDrag);
  _scene.setMode(StageScene::DoNothing);
}

//void Figure::setAddTilesMode()
//{ QPalette p = palette();
//  p.setColor(QPalette::Highlight,QColor(55,255,0));
//  setPalette(p);
//  //_view->setDragMode(QGraphicsView::NoDrag);
//}
//
//void Figure::setDelTilesMode()
//{ QPalette p = palette();
//  p.setColor(QPalette::Highlight,QColor(255,55,0));
//  setPalette(p);
//  //_view->setDragMode(QGraphicsView::NoDrag);
//}

void
Figure::readSettings()
{
  QSettings settings;
  QStringList keys = settings.allKeys();
#if 0
  _HERE_;
  qDebug() << settings.organizationName();
  qDebug() << settings.applicationName();
  qDebug() << (settings.isWritable()?"Writable":"!!! Settings are not writable !!!");
  foreach(QString k,keys)
    qDebug() << "\t" << k;
#endif
  settings.beginGroup("voxel");
  /*
  double w  = settings.value("width_um",0.1).toDouble(),
         h  = settings.value("height_um",0.1).toDouble(),
         th = settings.value("rotation_rad",0.0).toDouble();
  _item->setPixelGeometry(w,h,th);
  */
  settings.endGroup();
}

void
Figure::writeSettings()
{ QSettings settings;
  settings.beginGroup("voxel");
  /*
  QSizeF px = _item->pixelSizeMeters();
  double th = _item->rotationRadians();
  settings.setValue("width_um",px.width()*1e6);
  settings.setValue("height_um",px.height()*1e6);
  settings.setValue("rotation_rad",th);
  */
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
{ _HERE_; Print_Inuse_List(stderr,1);
  Figure *w = new Figure;
  g_open.insert(w);
  checkGLError();
  _HERE_; Print_Inuse_List(stderr,1);
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
