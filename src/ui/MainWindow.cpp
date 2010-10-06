#include "MainWindow.h"
#include <assert.h>
#include "common.h"

fetch::ui::MainWindow::MainWindow()
  :quitAct(0),
  openAct(0),
  saveToAct(0),
  fullscreenAct(0),
  fullscreenStateOff(0),
  fullscreenStateOn(0)
{
  createActions();
  createStateMachines();
  createMenus();
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
    // TODO: 
  }

  {
    saveToAct = new QAction(QIcon(":/icons/saveto"),"&Save To",this);
    saveToAct->setShortcut(QKeySequence::Save);
    saveToAct->setStatusTip("Set the data destination.");
    // TODO: 
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
