#include "MainWindow.h"
#include <assert.h>

fetch::ui::MainWindow::MainWindow()
{
  createActions();
  createMenus();  // must follow create actions
}

void fetch::ui::MainWindow::fullscreen()
{

}

void fetch::ui::MainWindow::createActions()
{
  fullscreenAct = new QAction(QIcon(":/icons/fullscreen.svg"),"&Full Screen",this);
  fullscreenAct->setShortcut(Qt::ALT + Qt::Key_F);
  fullscreenAct->setStatusTip("Display over full screen");
  connect(fullscreenAct,SIGNAL(triggered()),this,SLOT(fullscreen()));
}

void fetch::ui::MainWindow::createMenus()
{
  viewMenu = menuBar()->addMenu("&View");
  viewMenu->addAction(fullscreenAct);
}
