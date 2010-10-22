#include <QtGui>
#include "devices/Microscope.h"
namespace fetch{

  

  namespace ui{

    class VideoAcquisitionDockWidget:public QDockWidget
    {
      Q_OBJECT

    public:
      typedef device::Microscope::Config Config;

      VideoAcquisitionDockWidget(device::Microscope *dc, QWidget* parent=NULL);

    public:
      QLineEdit *_leResonantTurn;
      //QLineEdit *_leDuty;
      QLineEdit *_leLines;
      QLineEdit *_leVerticalRange;
      QLineEdit *_lePockels;

    public slots:
      void setTurn();
      void setLines();
      void setVerticalRange();
      void setPockels();

    private:
      device::Microscope *_dc;
    };

    //end namespace fetch::ui
  }
}
