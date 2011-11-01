#include <QtGui>
#include "MainWindow.h"

namespace fetch{
  namespace device{class Stage;}
  namespace ui{

    class StageMovingIndicator:public QLabel
    { Q_OBJECT

    public:
      StageMovingIndicator(device::Stage *dc,QWidget  *parent=NULL);

    protected slots:
      void poll();

    private:
      device::Stage *dc_;
      int last_moving_;
    };
    
    class StageOnTargetIndicator:public QLabel
    { Q_OBJECT

    public:
      StageOnTargetIndicator(device::Stage *dc,QWidget  *parent=NULL);

    protected slots:
      void poll();     

    private:
      device::Stage *dc_;
      int last_ont_;

    };

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
