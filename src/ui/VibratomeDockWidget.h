#include <QtGui>
#include "AgentController.h"
#include "MainWindow.h"

namespace fetch{
  namespace device{class Microscope;}
  namespace ui{

    class VibratomeDockWidget:public QDockWidget
    {      
      Q_OBJECT

      device::Microscope *dc_;
      QComboBox *cmb_feed_axis_;
    public:
      VibratomeDockWidget(device::Microscope *dc, MainWindow* parent);
    public slots:
      void start() {dc_->vibratome()->start();}
      void stop()  {dc_->vibratome()->stop();}
    };


    class VibratomeBoundsModel:public QAbstractTableModel
    { Q_OBJECT

      device::Microscope *dc_;

    public:
      VibratomeBoundsModel(device::Microscope *dc, QObject *parent=0);

      Qt::ItemFlags flags       (const QModelIndex &index ) const;
      int           rowCount    (const QModelIndex &parent) const;
      int           columnCount (const QModelIndex &parent) const;
      QVariant      headerData  (int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
      QVariant      data        (const QModelIndex &index, int role = Qt::DisplayRole ) const; 

      bool          setData     (const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
      bool          insertRows  (int row, int count, const QModelIndex & parent = QModelIndex() );
      bool          removeRows  (int row, int count, const QModelIndex & parent = QModelIndex() );
    };

    class VibratomeGeometryDockWidget:public QDockWidget
    {
      Q_OBJECT

      device::Microscope *dc_;
    public:
      VibratomeGeometryDockWidget(device::Microscope *dc, MainWindow* parent);
    protected slots:
      void geometryCtxMenu(const QPoint &);
      void insert();
      void remove();

    private:
      QTableView *t_;
      QPoint lastCtxMenuPos_;
    };

    //end namespace fetch::ui
  }
}
