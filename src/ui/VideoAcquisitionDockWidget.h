#include <QtGui>
#include "devices/Microscope.h"
#include "AgentController.h"
namespace fetch{

  

  namespace ui{

    class VideoAcquisitionDockWidget:public QDockWidget
    {
      Q_OBJECT

    public:
      typedef device::Microscope::Config Config;

      VideoAcquisitionDockWidget(device::Microscope *dc, AgentController *ac, QWidget* parent=NULL);

    public:
      QLineEdit *_leResonantTurn;
      //QLineEdit *_leDuty;
      QLineEdit *_leLines;
      QLineEdit *_leVerticalRange;
      QLineEdit *_lePockels;
      QPushButton *_btnFocus;

      QStateMachine _focusButtonStateMachine;

    signals:

      void onArmVideoTask();

    public slots:

      // Handlers for events from the form widgets
      void setTurn();
      void setLines();
      void setVerticalRange();
      void setPockels();

      void onArmFilter(Task* t);

    private:      
      void createForm();

      device::Microscope *_dc;
      AgentController *_ac;
    };

    //end namespace fetch::ui
  }
}
