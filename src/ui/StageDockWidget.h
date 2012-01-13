#include <QtGui>
#include "MainWindow.h"

namespace fetch{
  namespace device{class Stage;}
  namespace ui{

    class StageMovingIndicator:public QLabel
    { Q_OBJECT

    public:
      StageMovingIndicator(device::Stage *dc,QWidget  *parent=NULL);
      virtual ~StageMovingIndicator();

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
      virtual ~StageOnTargetIndicator();

    protected slots:
      void poll();     

    private:
      device::Stage *dc_;
      int last_ont_;

    };

    class StageDockWidget:public QDockWidget
    {      
      Q_OBJECT
      Q_PROPERTY(double posx READ posx);
      Q_PROPERTY(double posy READ posy);
      Q_PROPERTY(double posy READ posy);

      device::Microscope *dc_;
      QDoubleSpinBox *ps_[3],*vs_[3],*pstep_,*vstep_;

    public:
      StageDockWidget(device::Microscope *dc, MainWindow* parent);
      virtual ~StageDockWidget();

      double posx() {return ps_[0]->value();}
      double posy() {return ps_[0]->value();}
      double posz() {return ps_[0]->value();}

    public slots:
      void setPosStep(double v);
      void setPosStepByThickness();
      void setVelStep(double v);    

    private:
      void restoreSettings();
      void saveSettings();

    };

  /*
   *    StageBoundsButton
   */

  class StageBoundsButton : public QPushButton
  { Q_OBJECT

    QDoubleSpinBox *target_;
    const char     *targetprop_;
    const char     *posprop_;

    double          original_; // original value of property, used on "cleared" state

  protected slots:
    void handleToggled(bool state);
  public:
    // e.g StageBoundsButton(ps_[0],"maximum","posx",this)
    StageBoundsButton(QDoubleSpinBox *target, const char* targetprop, QWidget *parent=0);

  };
    //end namespace fetch::ui
  }
}
