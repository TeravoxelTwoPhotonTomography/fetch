#include <QtWidgets>
#include <QtDebug>
#include <QAction>
#include "StageScene.h"
#include "common.h"

#undef HERE
#define _HERE_ "[STAGESCENE] At " << __FILE__ << "("<<__LINE__<<")\n"
#define HERE qDebug() << _HERE_;
#define DBG(e) if(!(e)) qDebug() << HERE << "(!)\tCheck failed for Expression "#e << "\n" 
#define SHOW(e) qDebug() << _HERE_ << "\t"#e << " is " << e <<"\n"

//////////////////////////////////////////////////////////////////////////
//  SelectionRectGraphicsWidget  /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


SelectionRectGraphicsWidget::RectOp SelectionRectGraphicsWidget::lastop_ = SelectionRectGraphicsWidget::Add;

SelectionRectGraphicsWidget::SelectionRectGraphicsWidget(QGraphicsItem *parent/*=0*/ )
  : op_(SelectionRectGraphicsWidget::lastop_)
{
  //QGraphicsAnchorLayout *layout = new QGraphicsAnchorLayout();
  //QGraphicsProxyWidget  *button = new QGraphicsProxyWidget();
  //button->setWidget(new QPushButton(tr("Accept","SelectionRectGraphicsWidget|Button")));
  //layout->addCornerAnchors(button,Qt::BottomLeftCorner,layout,Qt::BottomLeftCorner);
  //setLayout(layout);

  setFocusPolicy(Qt::StrongFocus);

  createActions();
}

void SelectionRectGraphicsWidget::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget /*= 0 */ )
{
  QColor basecolor(getBaseColor());

  QPen pen(getPenColor());
  pen.setWidth(0);
  painter->setPen(pen);
  basecolor.setAlphaF(0.2);
  painter->setBrush(basecolor);

  painter->drawRect(mapRectFromParent(geometry()));
}

QColor SelectionRectGraphicsWidget::getPenColor()
{ 
  if(hasFocus())
    return Qt::white;
  return getBaseColor();
}

QColor SelectionRectGraphicsWidget::getBaseColor()
{
  switch(op_)
  {
  case Add:          return Qt::green;
  case Remove:       return Qt::red;
  case Done:         return Qt::blue;
  case UnDone:       return Qt::magenta; 
  case Explorable:   return Qt::cyan;
  case UnExplorable: return Qt::yellow;
  case Safe:         return Qt::darkGreen;
  case UnSafe:       return Qt::darkRed;
  case Reset:        return Qt::gray;
  default:
    UNREACHABLE;
  }
}

void SelectionRectGraphicsWidget::commit()
{  
  switch(op_)
  {
  case Add:          emit addSelectedArea                (mapToScene(shape())); break;
  case Remove:       emit removeSelectedArea             (mapToScene(shape())); break;
  case Done:         emit markSelectedAreaAsDone         (mapToScene(shape())); break;
  case UnDone:       emit markSelectedAreaAsNotDone      (mapToScene(shape())); break;
  case Explorable:   emit markSelectedAreaAsExplorable   (mapToScene(shape())); break;
  case UnExplorable: emit markSelectedAreaAsNotExplorable(mapToScene(shape())); break;
  case Safe:         emit markSelectedAreaAsSafe         (mapToScene(shape())); break;
  case UnSafe:       emit markSelectedAreaAsNotSafe      (mapToScene(shape())); break;  
  case Reset:        emit markSelectedAreaUserReset      (mapToScene(shape())); break;
  }
  scene()->removeItem(this);
  deleteLater();
}

void SelectionRectGraphicsWidget::cancel()
{  
  scene()->removeItem(this);
  deleteLater();
}

void SelectionRectGraphicsWidget::createActions()
{
  QAction *c;
  { c = new QAction(tr("Commit"),this);
    QList<QKeySequence> shortcuts;
    shortcuts.push_back( QKeySequence(Qt::Key_Return));
    shortcuts.push_back( QKeySequence(Qt::Key_Enter ));
    c->setShortcuts(shortcuts);    
    c->setShortcutContext(Qt::WidgetShortcut);
    connect(c,SIGNAL(triggered()),this,SLOT(commit()));
    addAction(c);
  }
  { c = new QAction(tr("Cancel"),this);
    c->setShortcut(Qt::Key_Backspace);
    c->setShortcutContext(Qt::WidgetShortcut);
    connect(c,SIGNAL(triggered()),this,SLOT(cancel()));
    addAction(c);
  }
  { c = new QAction(tr("Mark Active"),this);
    QList<QKeySequence> shortcuts;
    shortcuts.push_back( QKeySequence(tr("+")));
    shortcuts.push_back( QKeySequence(tr("=")));
    c->setShortcuts(shortcuts);
    c->setShortcutContext(Qt::WidgetShortcut);
    connect(c,SIGNAL(triggered()),this,SLOT(setOpAdd()));
    addAction(c);
  }
  { c = new QAction(tr("Mark Not Active"),this);
    c->setShortcut(Qt::Key_Minus);
    c->setShortcutContext(Qt::WidgetShortcut);
    connect(c,SIGNAL(triggered()),this,SLOT(setOpRemove()));
    addAction(c);
  }
  { c = new QAction(tr("Mark Done"),this);        
    connect(c,SIGNAL(triggered()),this,SLOT(setOpDone()));
    addAction(c);
  }  
  {
    c = new QAction(tr("Mark Not Done"),this);        
    connect(c,SIGNAL(triggered()),this,SLOT(setOpUnDone()));
    addAction(c);
  }
  { c = new QAction(tr("Mark Explorable"),this);
    connect(c,SIGNAL(triggered()),this,SLOT(setOpExplorable()));
    addAction(c);
  }
  { c = new QAction(tr("Mark Not Explorable"),this);
    connect(c,SIGNAL(triggered()),this,SLOT(setOpUnExplorable()));
    addAction(c);
  }
  { c = new QAction(tr("Mark Safe"),this);
    connect(c,SIGNAL(triggered()),this,SLOT(setOpSafe()));
    addAction(c);
  }
  { c = new QAction(tr("Mark Not Safe"),this);
    connect(c,SIGNAL(triggered()),this,SLOT(setOpUnSafe()));
    addAction(c);
  }
  { c = new QAction(tr("Reset"),this);
    connect(c,SIGNAL(triggered()),this,SLOT(setOpUserReset()));
    addAction(c);
  }
}

void SelectionRectGraphicsWidget::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;   
  menu.addActions(actions());
  menu.exec(event->screenPos());
}

void SelectionRectGraphicsWidget::focusInEvent( QFocusEvent *event )
{ 
  QGraphicsWidget::focusInEvent(event);
  //update();    
}

void SelectionRectGraphicsWidget::focusOutEvent( QFocusEvent *event )
{  
  
  QGraphicsWidget::focusOutEvent(event);
  //update();  
}


//////////////////////////////////////////////////////////////////////////
//  StageScene  //////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

StageScene::StageScene( QObject *parent/*=0*/ )
  : QGraphicsScene(parent)
  , current_item_(NULL)
  , mode_(DoNothing)
{

} 

void StageScene::mousePressEvent( QGraphicsSceneMouseEvent *event )
{ 
  if( event->button() != Qt::LeftButton )
    return;
  QGraphicsScene::mousePressEvent(event);
  if(event->isAccepted())
    return;

  switch(mode_)
  {
  case DrawRect:
  case Pick:
    { 
      SelectionRectGraphicsWidget *pick = dynamic_cast<SelectionRectGraphicsWidget*>(itemAt(event->scenePos(),QTransform()));
      if(pick)
      {
        //HERE;
        /** TODO **/
        /*
        * [ ] Distinguish between pick (to move) and dragging the edge to resize
        * [ ] Shouldn't this be handled by the item?
        * */
        mode_ = Pick;
        current_item_ = pick;
        event->accept();
        return;
      }          
      SelectionRectGraphicsWidget *w = new SelectionRectGraphicsWidget();
      connect(w,SIGNAL(   addSelectedArea(const QPainterPath&)),
           this,SIGNAL(   addSelectedArea(const QPainterPath&)));
      connect(w,SIGNAL(removeSelectedArea(const QPainterPath&)),
           this,SIGNAL(removeSelectedArea(const QPainterPath&)));  
      connect(w,SIGNAL(   markSelectedAreaAsDone(const QPainterPath&)),
           this,SIGNAL(   markSelectedAreaAsDone(const QPainterPath&)));
      connect(w,SIGNAL(markSelectedAreaAsNotDone(const QPainterPath&)),
           this,SIGNAL(markSelectedAreaAsNotDone(const QPainterPath&)));             
      connect(w,SIGNAL(   markSelectedAreaAsExplorable(const QPainterPath&)),
           this,SIGNAL(   markSelectedAreaAsExplorable(const QPainterPath&)));
      connect(w,SIGNAL(markSelectedAreaAsNotExplorable(const QPainterPath&)),
           this,SIGNAL(markSelectedAreaAsNotExplorable(const QPainterPath&)));      
      connect(w,SIGNAL(markSelectedAreaAsSafe(const QPainterPath&)),
           this,SIGNAL(markSelectedAreaAsSafe(const QPainterPath&)));
      connect(w,SIGNAL(markSelectedAreaAsNotSafe(const QPainterPath&)),
           this,SIGNAL(markSelectedAreaAsNotSafe(const QPainterPath&)));
      connect(w,SIGNAL(markSelectedAreaUserReset(const QPainterPath&)),
           this,SIGNAL(markSelectedAreaUserReset(const QPainterPath&)));

      w->setPos(event->scenePos());
      addItem(w);
      current_item_ = w;
    }
    break;
  case DoNothing:
    break;
  default:
    ;

  }

  //QGraphicsScene::mousePressEvent(event);
}

void StageScene::mouseMoveEvent( QGraphicsSceneMouseEvent *event )
{                      
  switch(mode_)
  {
  case DrawRect:
    {
      // Check for enough drag distance
      if ((event->buttonDownScreenPos(Qt::LeftButton) - event->screenPos()).manhattanLength()
        < QApplication::startDragDistance()) {                                       
          return;
      }

      // Stop if the user has let go of all the buttons
      if(!event->buttons())
      {
        current_item_ = NULL;
      }

      // update position
      if(current_item_)
      {      
        QPointF mp = event->scenePos(),
          ep = event->buttonDownScenePos(Qt::LeftButton);
        current_item_->setGeometry(
          qMin(mp.x() , ep.x()), 
          qMin(mp.y() , ep.y()),
          qAbs(mp.x() - ep.x()), 
          qAbs(mp.y() - ep.y())
          );
        event->accept();
      }
    }
    break;
  case Pick:  
    { // Check for enough drag distance
      if ((event->buttonDownScreenPos(Qt::LeftButton) - event->screenPos()).manhattanLength()
        < QApplication::startDragDistance()) {                                       
          return;
      }

      // Stop if the user has let go of all the buttons
      if(!event->buttons())
      {
        current_item_ = NULL;
        mode_ = DrawRect;
      }

      // update position
      if(current_item_)
      {      
        QPointF mp = event->scenePos();
        QRectF r = current_item_->geometry();
        r.moveCenter(mp);
        current_item_->setGeometry(r);
        event->accept();
      }
    }
    break;
  case DoNothing:       
    break;
  }
  if(!event->isAccepted())
    QGraphicsScene::mouseMoveEvent(event);
  update();
}

void StageScene::mouseReleaseEvent( QGraphicsSceneMouseEvent *event )
{ current_item_ = NULL;  
QGraphicsScene::mouseReleaseEvent(event);
}

void StageScene::setMode( Mode mode )
{ mode_=mode;
}
