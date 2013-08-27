#include <QtWidgets>
#include "MainWindow.h"

namespace fetch{
  namespace device{class Stage;}
  namespace ui{

    class StageIndicator:public QLabel
    { Q_OBJECT

    public:
      StageIndicator(device::Stage *dc,const QString& ontext, const QString& offtext, QWidget  *parent=NULL);
      virtual ~StageIndicator();
     
    protected slots:
      void poll();

    protected:
      virtual bool state() = 0;
      QString ontext_,offtext_;
      device::Stage *dc_;
      int last_;
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
