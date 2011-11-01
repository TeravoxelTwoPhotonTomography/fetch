#include <QtGui>
#include "MainWindow.h"

namespace fetch{
  namespace device{class Stage;}
  namespace ui{

    class StageDockWidget:public QDockWidget
    {      
      Q_OBJECT

      device::Stage *dc_;
      QDoubleSpinBox *ps_[3],*vs_[3],*pstep_,*vstep_;


    public:
      StageDockWidget(device::Stage *dc, MainWindow* parent);
      virtual ~StageDockWidget();

    public slots:
      void setPosStep(double v);
      void setVelStep(double v);

    protected:
      //virtual void closeEvent(QCloseEvent *event);

    private:
      void restoreSettings();
      void saveSettings();

    };

    //end namespace fetch::ui
  }
}
