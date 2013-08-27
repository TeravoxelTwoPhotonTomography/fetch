#pragma once
#include <QtWidgets>

namespace fetch {
namespace device { class Microscope; }
namespace ui {
class MainWindow;

class AutoTileDockWidget:public QDockWidget
{
  
public:
  AutoTileDockWidget(device::Microscope *dc, MainWindow *parent);
  
};

}} // fetch::ui::