#include "VibratomeDockWidget.h"
#include "devices\microscope.h"
#include "AgentController.h"
#include "google\protobuf\descriptor.h"
#include "ui\StageDockWidget.h"

namespace fetch {
namespace ui {


  //
  // VibratomeDockWidget
  //

  VibratomeDockWidget::VibratomeDockWidget(device::Microscope *dc, MainWindow *parent)
    :QDockWidget("Vibratome",parent)
    ,dc_(dc)
    ,cmb_feed_axis_(NULL)
  {
    Guarded_Assert(parent->_stageDockWidget!=NULL);

    QWidget *formwidget = new QWidget(this);
    QFormLayout *form = new QFormLayout;
    formwidget->setLayout(form);
    setWidget(formwidget);

    parent->_vibratome_amp_controller->createLineEditAndAddToLayout(form);
    parent->_vibratome_feed_distance_controller->createLineEditAndAddToLayout(form);
    parent->_vibratome_feed_velocity_controller->createLineEditAndAddToLayout(form);
    parent->_vibratome_feed_axis_controller->createComboBoxAndAddToLayout(form);
    parent->_vibratome_feed_pos_x_controller->createLineEditAndAddToLayout(form);
    parent->_vibratome_feed_pos_y_controller->createLineEditAndAddToLayout(form);
    parent->_vibratome_thick_controller->createLineEditAndAddToLayout(form);
    parent->_fov_overlap_z_controller->createLabelAndAddToLayout(form);

    { QHBoxLayout *row = new QHBoxLayout();
      QPushButton *b;
      b = new QPushButton("Update stack depth",this);
      connect(b,SIGNAL(clicked()),this,SLOT(updateStackDepth()));
      row->addWidget(b);
      b = new QPushButton("Update Z Overlap",this);
      connect(b,SIGNAL(clicked()),this,SLOT(updateZOverlap()));
      row->addWidget(b);
      b = new QPushButton("Set Stage Step",this);
      connect(b,SIGNAL(clicked()),parent->_stageDockWidget,SLOT(setPosStepByThickness()));
      row->addWidget(b);
      form->addRow(row);      
    } 

    { QHBoxLayout *row = new QHBoxLayout();
      QPushButton *b;
      b = new QPushButton("Set cut origin",this);
      connect(b,SIGNAL(clicked()),this,SLOT(setCutPosToCurrent()));
      row->addWidget(b);
      b = new QPushButton("Move to cut origin",this);
      connect(b,SIGNAL(clicked()),this,SLOT(moveToCutPos()));
      row->addWidget(b);
      form->addRow(row);      
    }   

#if 0
    qDebug() << dc->vibratome()->_config->VibratomeFeedAxis_descriptor()->value_count();
    qDebug() << dc->vibratome()->_config->VibratomeFeedAxis_descriptor()->value(0)->name().c_str();
    qDebug() << dc->vibratome()->_config->VibratomeFeedAxis_descriptor()->value(1)->name().c_str();
#endif
    
    QHBoxLayout *row = new QHBoxLayout();
    
    QPushButton *b = new QPushButton("On",this);
    connect(b,SIGNAL(clicked()),this,SLOT(start()));
    row->addWidget(b);

    b = new QPushButton("Off",this);
    connect(b,SIGNAL(clicked()),this,SLOT(stop()));
    row->addWidget(b);

    form->addRow(row);

    //AgentControllerButtonPanel *btns = new AgentControllerButtonPanel(&parent->_scope_state_controller,&dc->cut_task);
    //form->addRow(btns);    

    connect(this,SIGNAL(configUpdated()),parent,SIGNAL(configUpdated()));
  }

  void 
    VibratomeDockWidget::
    setCutPosToCurrent()
  { float x,y,z;
    dc_->stage()->getTarget(&x,&y,&z);
    dc_->vibratome()->set_feed_begin_pos_mm(x,y);
    emit configUpdated();
  }

  void
    VibratomeDockWidget::
    moveToCutPos()
  { float x,y,z;
    dc_->stage()->getTarget(&x,&y,&z);
    dc_->vibratome()->feed_begin_pos_mm(&x,&y);
    dc_->stage()->setPosNoWait(x,y,z);
  }

  void 
    VibratomeDockWidget::
    updateZOverlap()
  { dc_->updateFovFromStackDepth(1);
    emit configUpdated();
  }

  void
    VibratomeDockWidget::
    updateStackDepth()
  { dc_->updateStackDepthFromFov(1);
    emit configUpdated();
  }

  //
  //  VibratomeBoundsModel
  //  - QAbstractDataModel for interacting with a particular protobuf RepeatedField 
  //

    VibratomeBoundsModel::VibratomeBoundsModel(device::Microscope *dc, QObject *parent)
      : QAbstractTableModel(parent)
      , dc_(dc) 
    {}

    Qt::ItemFlags VibratomeBoundsModel::flags       (const QModelIndex &index ) const { return Qt::ItemIsEditable 
                                                                                             | Qt::ItemIsEnabled
                                                                                           //| Qt::ItemIsDragEnabled
                                                                                           //| Qt::ItemIsDropEnabled
                                                                                           //| Qt::ItemIsSelectable
                                                                                             ; }
    int           VibratomeBoundsModel::rowCount    (const QModelIndex &parent) const { return dc_->vibratome()->_config->geometry().bounds_mm().size(); }
    int           VibratomeBoundsModel::columnCount (const QModelIndex &parent) const { return 2; }

    QVariant 
      VibratomeBoundsModel::
      headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/ ) const
    { switch(role)
      { case Qt::DisplayRole:
          switch(orientation)
          { case Qt::Horizontal: return section?"Y (mm)":"X(mm)";            
            case Qt::Vertical:   return QString("%1").arg(section);            
            default: break;
          }
          break;
        default: 
          break;
      }
      return QVariant();
    }

    QVariant 
      VibratomeBoundsModel::
      data(const QModelIndex &index, int role /*= Qt::DisplayRole*/ ) const 
    { cfg::device::Point2d p;
      float v;

      // Guard: prefilter roles that don't require data access
      switch(role)
      { case Qt::DisplayRole:
        case Qt::EditRole:
          break;
        case Qt::TextAlignmentRole: return Qt::AlignRight;
        default: goto Error;
      }

      // now access the data
      if(index.row() < rowCount(index.parent()))
        p = dc_->vibratome()->_config->geometry().bounds_mm().Get(index.row());
      else goto Error;

      switch(index.column())
      { case 0: v=p.x(); break;
        case 1: v=p.y(); break;
        default: goto Error;
      }
      
      switch(role)
      { case Qt::DisplayRole: return v;
        case Qt::EditRole: return QString("%1").arg(v);
        default: break;
      }
    Error:
      return QVariant();
    }

    bool  
      VibratomeBoundsModel::
      setData ( const QModelIndex & index, const QVariant & value, int role /*= Qt::EditRole*/ )
    { bool ok = false;
      switch(role)
      { case Qt::EditRole:        
          if(index.row()<rowCount(index.parent()))
          { cfg::device::Point2d *p = dc_->vibratome()->_config->mutable_geometry()->mutable_bounds_mm(index.row());
            switch(index.column())
            { case 0: p->set_x(value.toFloat(&ok)); break;
              case 1: p->set_y(value.toFloat(&ok)); break;
              default:
                goto Error;
            }
          }
          break;
        default:
          break;
      }
      return ok;
    Error:
      return false;
    }

    bool  
      VibratomeBoundsModel::
      insertRows( int row, int count, const QModelIndex & parent /*= QModelIndex()*/ )
    { Eigen::Vector3f v = dc_->stage()->getTarget();
      int sz = rowCount(parent)+count;
      cfg::device::VibratomeGeometry *g = dc_->vibratome()->_config->mutable_geometry(); 
      cfg::device::Point2d *o;
      beginInsertRows(parent,row,row+count-1);      
      for(int i=0   ;i<count       ;++i)  g->add_bounds_mm();                              // 1. Append elements        
      for(int i=sz-1;i>=(row+count);--i)  g->mutable_bounds_mm()->SwapElements(i,i-count); // 2. Copy down elements              
      for(int i=0   ;i<count       ;++i)                                                   // 3. Set the first elements to current stage pos (x,y)
      { o = g->mutable_bounds_mm()->Mutable(row+i); 
        o->set_x(v.x());
        o->set_y(v.y());
      }
      endInsertRows();
      return true;
    }

    static inline int mod(int x, int m) {
      return (x%m + m)%m;
    }
    bool  
      VibratomeBoundsModel::
      removeRows ( int row, int count, const QModelIndex & parent /*= QModelIndex()*/ )
    { ::google::protobuf::RepeatedPtrField< cfg::device::Point2d >* r = dc_->vibratome()->_config->mutable_geometry()->mutable_bounds_mm();
      int sz = rowCount(parent);
      beginRemoveRows(parent,row,row+count-1);
      qDebug() << "Remove: " << (row) << "\tCount: " << count;
      if(count>1) for(int i=0;i<count;++i)                    // 1. Circularly permute dudes from the end into interval.
        r->SwapElements(row+i,row+mod(i-1,sz-row));                                                       
      for(int i=0;i<count;++i)  r->RemoveLast();  // 2. Remove the end
      sz-=count;                                  
      if(count>1) for(int i=count-1;i>=0;--i)                 // 3. Reverse circularly permute.
        r->SwapElements(row+i,row+mod(i-1,sz-row));       
      endRemoveRows();
      return true;
    }        

    //
    // VibratomeGeometryDockWidget
    //
    
    VibratomeGeometryDockWidget::VibratomeGeometryDockWidget(device::Microscope *dc, MainWindow* parent)
      : QDockWidget("Vibratome Geometry",parent)
      , dc_(dc)
      , is_set__image_plane_(0)
      , is_set__cut_plane_(0)
      , image_plane_mm_(0.0f)
      , cut_plane_mm_(0.0f)
    { QWidget *formwidget = new QWidget(this);
      QFormLayout *form = new QFormLayout;
      formwidget->setLayout(form);
      setWidget(formwidget);

      tc_ = parent->tilingController();

      // Add Cut offset buttons
      { QGridLayout *row = new QGridLayout();        
        MarkButton *b;
        QLabel *w;
        QGroupBox *g;
        QVBoxLayout *v;
        w = new QLabel("Z Offset:");
        row->addWidget(w,0,0);

        g = new QGroupBox("Image");
        b = new MarkButton("Mark");
        v = new QVBoxLayout();
        v->addWidget(b);
        g->setLayout(v);
        row->addWidget(g,0,1);
        connect(b,SIGNAL(clicked()),this,SLOT(markImagePlane()));
        connect(this,SIGNAL(imagePlane(float)),b,SLOT(setValue(float)));
        
        g = new QGroupBox("Cut");
        b = new MarkButton("Mark");
        v = new QVBoxLayout();
        v->addWidget(b);
        g->setLayout(v);
        row->addWidget(g,0,2);
        connect(b,SIGNAL(clicked()),this,SLOT(markCutPlane()));
        connect(this,SIGNAL(cutPlane(float)),b,SLOT(setValue(float)));
                
        g = new QGroupBox("Offset");
        b = new MarkButton("Commit");
        v = new QVBoxLayout();
        v->addWidget(b);
        g->setLayout(v);
        row->addWidget(g,0,3);
        connect(b,SIGNAL(clicked()),this,SLOT(commitOffset()));
        connect(this,SIGNAL(delta(float)),b,SLOT(setValue(float)));
        connect(parent->_vibratome_z_offset_controller, SIGNAL(configUpdated()),
                this,SLOT(updateFromConfig()));
        emit updateFromConfig();
      
        form->addRow(row);
      }

#if 0 // turns out this tableview is completely useless.
      // Add TableView
      t_ = new QTableView(this);
      VibratomeBoundsModel *m = new VibratomeBoundsModel(dc,this);
      t_->setModel(m);
      t_->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(t_, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(geometryCtxMenu(const QPoint &)));

      connect(parent,SIGNAL(configUpdated()),m,SIGNAL(layoutAboutToBeChanged()));
      connect(parent,SIGNAL(configUpdated()),m,SLOT(revert()));
      connect(parent,SIGNAL(configUpdated()),t_,SLOT(reset()));      
      connect(parent,SIGNAL(configUpdated()),m,SIGNAL(layoutChanged()));
      form->addRow(t_);
#endif
    }

    void
    VibratomeGeometryDockWidget::geometryCtxMenu(const QPoint &p)
    { QMenu *menu = new QMenu;
      lastCtxMenuPos_ = t_->mapToGlobal(p);
      menu->addAction("Insert Above: Current Position",this,SLOT(insert()));
      menu->addAction("Remove",this,SLOT(remove()));      
      menu->exec(QCursor::pos());
    }

    void VibratomeGeometryDockWidget::insert()
    { QPoint p = t_->mapFromGlobal(lastCtxMenuPos_);
      int row = t_->rowAt(p.y());      
      if(row<=-1) row=0;
      t_->model()->insertRow(row);
    }

    void VibratomeGeometryDockWidget::remove()
    { QPoint p = t_->mapFromGlobal(lastCtxMenuPos_);
      int row = t_->rowAt(p.y());
      if(row>-1)
        t_->model()->removeRow(row);
    }

    void VibratomeGeometryDockWidget::commitToTiling()
    { // TODO
      // [ ] Implement
      //     1. translate data to a QPainterPath, store in <path>
      //     2. mark Tiles from <lastpath> as addressable in all planes
      //     3. mark Tiles from <path> as unaddressable in all planes
      //     4. notify TilesView to update
      //     5. store <path> in <lastpath>
      // [ ] Connect
      //     [ ] add "Commit" button to dock widget 
      //     [ ] may want a method in the model that yields the QPainterPath
      //     [x] add tiling controller reference to this widget

      // NOTES
      // - Currently this method isn't connected to anything

      // TESTING
      // [ ] confirm paint and update of view
      // [ ] when changing planes, is the unaddressable area still there?
      // [ ] confirm tile task will not move stage into unaddressable area
      //     despite tiles there being marked as active.      

    }

    void 
      VibratomeGeometryDockWidget::
      markImagePlane()
    { float x,y;
      dc_->stage()->getTarget(&x,&y,&image_plane_mm_);
      is_set__image_plane_ = 1;
      emit imagePlane(image_plane_mm_);
    }

    void 
      VibratomeGeometryDockWidget::
      markCutPlane()
    { float x,y;
      dc_->stage()->getTarget(&x,&y,&cut_plane_mm_);
      is_set__cut_plane_ = 1;
      emit cutPlane(cut_plane_mm_);
    }

    void 
      VibratomeGeometryDockWidget::
      commitOffset()
    { if( is_set__cut_plane_ && is_set__image_plane_ )      
        dc_->vibratome()->setVerticalOffsetNoWait(cut_plane_mm_,image_plane_mm_);
      emit delta(image_plane_mm_-cut_plane_mm_);
    }

    void 
      VibratomeGeometryDockWidget::
      updateFromConfig()
    { emit delta(dc_->vibratome()->verticalOffset());
    }
}} //end namespace fetch::ui
