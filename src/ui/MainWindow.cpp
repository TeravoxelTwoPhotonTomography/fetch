#include "MainWindow.h"
#include <assert.h>
#include "common.h"
#include "devices/Microscope.h"
#include "VideoAcquisitionDockWidget.h"
#include "Figure.h"
#include "Player.h"

fetch::ui::MainWindow::MainWindow(device::Microscope *dc)
  :_dc(dc)
  ,quitAct(0)
  ,openAct(0)
  ,saveToAct(0)
  ,fullscreenAct(0)
  ,fullscreenStateOff(0)
  ,fullscreenStateOn(0)
  ,_videoAcquisitionDockWidget(0)
  ,_display(0)
  ,_player(0)
  ,_scope_state_broadcast(&dc->__self_agent)
{
  createActions();
  createStateMachines();
  createMenus();
  createDockWidgets();
  createViews();
  
  connect(&_poller,SIGNAL(timeout()),&_scope_state_broadcast,SLOT(poll()));
  _poller.start(50 /*ms*/);  
}

void fetch::ui::MainWindow::createActions()
{
  {
	  fullscreenAct = new QAction(QIcon(":/icons/fullscreen"),"&Full Screen",this);
	  fullscreenAct->setShortcut(Qt::CTRL + Qt::Key_F);
	  fullscreenAct->setStatusTip("Display over full screen");
  }

  {
    quitAct = new QAction(QIcon(":/icons/quit"),"&Quit",this);
    QList<QKeySequence> quitShortcuts;
    quitShortcuts 
      << QKeySequence::Close
      << QKeySequence::Quit
      << (Qt::CTRL+Qt::Key_Q);
    quitAct->setShortcuts(quitShortcuts);
    quitAct->setStatusTip("Shutdown the microscope and exit.");
    connect(quitAct,SIGNAL(triggered()),this,SLOT(close()));
  }

  {
    openAct = new QAction(QIcon(":/icons/open"),"&Open",this);
    openAct->setShortcut(QKeySequence::Open);
    openAct->setStatusTip("Open a data source.");
    // TODO: connect to something
  }

  {
    saveToAct = new QAction(QIcon(":/icons/saveto"),"&Save To",this);
    saveToAct->setShortcut(QKeySequence::Save);
    saveToAct->setStatusTip("Set the data destination.");
    // TODO:  connect to something
  }
}

void fetch::ui::MainWindow::createMenus()
{
  assert(fullscreenAct);
  assert(quitAct);
  assert(openAct);
  assert(saveToAct);
  
  fileMenu = menuBar()->addMenu("&File");
  fileMenu->addAction(openAct);
  fileMenu->addAction(saveToAct);
  fileMenu->addAction(quitAct);                                                                                                                                                                                                                                           

  viewMenu = menuBar()->addMenu("&View");
  viewMenu->addAction(fullscreenAct);
}

void fetch::ui::MainWindow::createStateMachines()
{
  assert(fullscreenAct);

  { //Fullscreen
	  fullscreenStateOn  = new QState();
	  fullscreenStateOff = new QState();
	  connect(fullscreenStateOn, SIGNAL(entered()),this,SLOT(showFullScreen()));
	  connect(fullscreenStateOff,SIGNAL(entered()),this,SLOT(showNormal()));  
	  fullscreenStateOn->addTransition( fullscreenAct,SIGNAL(triggered()),fullscreenStateOff);
	  fullscreenStateOff->addTransition(fullscreenAct,SIGNAL(triggered()),fullscreenStateOn);
	
	  fullscreenStateMachine.addState(fullscreenStateOff);
	  fullscreenStateMachine.addState(fullscreenStateOn);
	  fullscreenStateMachine.setInitialState(fullscreenStateOff);
	  fullscreenStateMachine.start();
  }
}

void fetch::ui::MainWindow::createDockWidgets()
{
  _videoAcquisitionDockWidget = new VideoAcquisitionDockWidget(_dc);
  addDockWidget(Qt::LeftDockWidgetArea,_videoAcquisitionDockWidget);
  viewMenu->addAction(_videoAcquisitionDockWidget->toggleViewAction());
}

void fetch::ui::MainWindow::createViews()
{
  _display = new Figure;
  _player  = new AsynqPlayer(_dc->getVideoChannel(),_display);
  setCentralWidget(_display);
  _player->start();
}

void fetch::ui::MainWindow::closeEvent( QCloseEvent *event )
{
  _player->stop();
  _poller.stop();
}

fetch::ui::MainWindow::~MainWindow()
{
  if(_player) delete _player;
}
