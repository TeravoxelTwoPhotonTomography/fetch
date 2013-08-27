#include <QtWidgets>
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

    signals:
      void configUpdated();

    public slots:
      void start() {dc_->vibratome()->start();}
      void stop()  {dc_->vibratome()->stop();}
      void setCutPosToCurrent();
      void moveToCutPos();
      void updateZOverlap();
      void updateStackDepth();
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

    class MarkButton: public QPushButton
    {
      Q_OBJECT
    public:
      MarkButton(const QString& lbl, QWidget *parent=0) : QPushButton(lbl,parent) {}
    public slots:
      void setValue(float v) {setText(QString("%1 mm").arg(v,5,'f',3));}
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
      void commitToTiling();

      void markImagePlane();
      void markCutPlane();
      void commitOffset();

      void updateFromConfig();

    signals:
      void imagePlane(float z_mm);
      void cutPlane(float z_mm);
      void delta(float z_mm);

    private:
      TilingController *tc_;
      QTableView *t_;
      QPoint lastCtxMenuPos_;

      bool  is_set__image_plane_;
      bool  is_set__cut_plane_;
      float image_plane_mm_;
      float cut_plane_mm_;
    };

    //end namespace fetch::ui
  }
}
