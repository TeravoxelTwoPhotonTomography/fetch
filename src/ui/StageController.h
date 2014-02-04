#pragma once
#include <QtWidgets>
#include <devices/stage.h>
#include <ui/uiunits.h>

#include <Eigen/Core>
#include "devices/stage.h"
#include "devices/tiling.h"
#include "ui/AgentController.h"

using namespace Eigen;

namespace fetch {
namespace ui {

  class TilingControllerListener:public QObject, public device::StageListener ///< forwards callbacks through qt signals
  {      
    Q_OBJECT
  public:
    void tile_done( size_t index, const Vector3f& pos,uint32_t sts )       {emit sig_tile_done(index,(unsigned int)sts);}
    void tiling_changed()                                                  {emit sig_tiling_changed();}
    void tile_next( size_t index, const Vector3f& pos )                    {emit sig_tile_next((unsigned int)index);}
    void fov_changed(const device::FieldOfViewGeometry *fov)               {emit sig_fov_changed(fov->field_size_um_[0],fov->field_size_um_[1],fov->rotation_radians_);}      
    void moved(void)                                                       {emit sig_moved();}  
    
  signals:
    void sig_tile_done( unsigned index, unsigned int sts );
    void sig_tiling_changed();
    void sig_tile_next( unsigned index );
    void sig_fov_changed(float w_um, float h_um, float rotation_radians);
    void sig_moved();
  };

  class TilingController:public QObject
  {
    Q_OBJECT

  public:
    typedef Matrix<float,3,4>         TRectVerts;
    typedef Transform<float,3,Affine> TTransform;

    TilingController(device::Stage *stage, QObject* parent=0);

    inline TilingControllerListener* listener()                            {return &listener_;}

    inline bool is_valid()                                                 {return stage_->tiling()!=NULL;}
    inline device::Stage* stage()                                          {return stage_;}

    bool fovGeometry      (TRectVerts *out);                               /// \returns false if tiling is invalid
    bool latticeTransform (TTransform *out);                               /// \returns false if tiling is invalid
    bool latticeTransform (QTransform *out);                               /// \returns false if tiling is invalid - QtVersion
    bool latticeShape     (unsigned *width, unsigned *height);             /// \returns false if tiling is invalid
    bool latticeShape     (QRectF *out);                                   /// \returns false if tiling is invalid
    bool latticeAttrImage (QImage *out);                                   /// \returns false if tiling is invalid
    bool latticeAttrImageAtPlane(QImage *out, int iplane);
    bool stageAlignedBBox (QRectF *out);                                   /// \returns false if tiling is invalid

    bool markAddressable();                                                /// \returns false if tiling is invalid
    bool markActive(const QPainterPath& path);                             /// \returns false if tiling is invalid
    bool markInactive(const QPainterPath& path);                           /// \returns false if tiling is invalid
    bool markDone(const QPainterPath& path);                               /// \returns false if tiling is invalid
    bool markNotDone(const QPainterPath& path);                            /// \returns false if tiling is invalid
    bool markExplorable(const QPainterPath& path);                         /// \returns false if tiling is invalid
    bool markNotExplorable(const QPainterPath& path);                      /// \returns false if tiling is invalid
    bool markSafe(const QPainterPath& path);                               /// \returns false if tiling is invalid
    bool markNotSafe(const QPainterPath& path);                            /// \returns false if tiling is invalid
    bool markUserReset(const QPainterPath& path);                          /// \returns false if tiling is invalid
    
    bool markAllPlanesExplorable(const QPainterPath& path);                /// \returns false if tiling is invalid
    bool markAllPlanesNotExplorable(const QPainterPath& path);             /// \returns false if tiling is invalid

    bool mapToIndex(const Vector3f & stage_coord, unsigned *index);        /// \returns false if tiling is invalid or if stage_coord is oob

    QAction* saveDialogAction() {return save_action_;}
    QAction* loadDialogAction() {return load_action_;}
    QAction*   autosaveAction() {return autosave_action_;}

  public slots:
    void update()                                                          {markAddressable(); emit changed(); emit show(stage_->tiling()!=NULL);}
    void stageAttached()                                                   {emit show(true);}
    void stageDetached()                                                   {emit show(false);}    
    void updatePlane()                                                     {markAddressable();}

    void saveViaFileDialog();
    void loadViaFileDialog();
    void saveToFile(const QString& filename);
    void saveToLast();
    void loadFromFile(const QString& filename);
    
    void setAutosave(bool on);    
    
    void fillActive();
    void dilateActive();

  signals:
    void show(bool tf);
    void changed();
    void planeChanged();
    void tileDone(unsigned itile,unsigned int attr);
    void nextTileRequest(unsigned itile);                                  ///< really should be nextTileRequested
    void fovGeometryChanged(float w_um, float h_um, float rotation_radians);
    // other ideas: imaging started, move start, move end

  private:

    device::Stage       *stage_;
    TilingControllerListener listener_;
    QAction             *load_action_,
                        *save_action_,
                        *autosave_action_;
                        
    QTimer              autosave_timer_;

    bool mark(            const QPainterPath& path,                               ///< path should be in scene coords (um)
                          int attr, 
                          QPainter::CompositionMode mode );
    bool mark_all_planes( const QPainterPath& path,                               ///< path should be in scene coords (um)
                          int attr, 
                          QPainter::CompositionMode mode );
    bool mark_all( device::StageTiling::Flags attr,                               ///< marks the whole plane with the attribute
                   QPainter::CompositionMode mode );
  };
  

  class PlanarStageControllerListener:public QObject, public device::StageListener
  {
    Q_OBJECT
  public:
    virtual void moved(void)           {emit sig_moved();}
    virtual void velocityChanged(void) {emit sig_velocityChanged();}
    virtual void referenced(void)      {emit sig_referenced();}
  signals:
    void sig_moved();
    void sig_velocityChanged();
    void sig_referenced();

  };

  class PositionHistoryModel:public QAbstractListModel
  { Q_OBJECT
  
  public:
    PositionHistoryModel(QObject *parent=0) : QAbstractListModel(parent) {}
    void push(float x, float y, float z)
    { int n;      
      v3 v = {x,y,z};
      beginInsertRows(index(0).parent(),0,0);
      history_.prepend(v);
      endInsertRows();
      if(n=history_.length()>10 /* MAX DEPTH*/)              /// \todo grab MAXDEPTH from QSettings.  Allows one to set this via prefs later (or manually edit registry).
      { beginRemoveRows(index(n-1).parent(),n-1,n-1);
        history_.removeLast();
        endRemoveRows();
      }
      emit suggestIndex(0);
    }
    int  get(int i, float *x, float *y, float *z)
    { if(i<0 || i>=history_.size())
        return 0;
      v3 v = history_.at(i);
      *x = v.x; *y = v.y; *z = v.z;
      return 1;
    }
  signals:
    void suggestIndex(int idx);
   
  protected:
    virtual Qt::ItemFlags flags(const QModelIndex & index) const
    { return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
    virtual QVariant data(const QModelIndex & index, int role=Qt::DisplayRole) const
    { v3 v;
      // Guard: prefilter roles that don't require data access
      switch(role)
      { case Qt::DisplayRole:
          break;
        case Qt::TextAlignmentRole: return Qt::AlignRight;
        default: goto Error;
      }

      // now access the data
      if(index.row() < rowCount(index.parent()))
        v = history_.at(index.row());        
      else goto Error;
      
      switch(role)
      { case Qt::DisplayRole: return QString("(%1, %2, %3) mm").arg(v.x,0,'f',4).arg(v.y,0,'f',4).arg(v.z,0,'f',4);        
        default: break;
      }
    Error:
      return QVariant();
    }
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/ ) const
    { switch(role)
      { case Qt::DisplayRole:
          switch(orientation)
          { case Qt::Horizontal: return "Past Targets";            
            case Qt::Vertical:   return QString("%1").arg(section);
            default: break;
          }
          break;
        default: 
          break;
      }
      return QVariant();
    }
    virtual int rowCount(const QModelIndex & parent) const { return history_.length(); }

  private:
    typedef struct v3_t {double x,y,z;} v3;

    QList<v3> history_;
  };

  class PlanarStageController:public QObject
  { 
    Q_OBJECT    

    public:

      static const units::Length Unit = units::MM;

      PlanarStageController(device::Stage *stage, QObject *parent=0);

      QRectF  travel()                                                     { device::StageTravel t; stage_->getTravel(&t); return QRectF(QPointF(t.x.min,t.y.min),QPointF(t.x.max,t.y.max)); }
      QPointF velocity()                                                   { float vx,vy,vz; stage_->getVelocity(&vx,&vy,&vz); return QPointF(vx,vy); }
      QPointF pos()                                                        { float  x, y, z; stage_->getPos(&x,&y,&z); return QPointF(x,y); } 
      QPointF target()                                                     { float  x, y, z; stage_->getTarget(&x,&y,&z); return QPointF(x,y); } 

      TilingController* tiling()                                           {return &tiling_controller_;}
      PlanarStageControllerListener* listener()                            {return &listener_;}
      device::Stage *stage()                                               {return stage_;}

      QComboBox *createHistoryComboBox(QWidget *parent=0);
                                                                     
   signals:
      void moved();            ///< eventually updates the imitem's position
      void moved(QPointF pos); ///< eventually updates the imitem's position
      void velocityChanged();
      void referenced();       ///< eventually updates the imitem's position

    public slots:
      
      void setVelocity(QPointF v)                                          { stage_->setVelocity(v.x(),v.y(),0.0); }
      void moveTo3d(float x, float y, float z)                             { stage_->setPosNoWait(x,y,z); history_.push(x,y,z); emit moved(QPointF(x,y));}
      void moveTo(QPointF r)                                               { float  x, y, z; stage_->getTarget(&x,&y,&z); moveTo3d(r.x(),r.y(),z); }
      void moveRel(QPointF dr)                                             { float  x, y, z; stage_->getTarget(&x,&y,&z); moveTo3d(x+dr.x(),y+dr.y(),z);} 
      void moveToHistoryItem(int i)                                        { float  x, y, z; if(history_.get(i,&x,&y,&z)) moveTo3d(x,y,z); }
      void savePosToHistory()                                              { float  x, y, z; stage_->getTarget(&x,&y,&z); history_.push(x,y,z); }
      void reference()                                                     { stage_->referenceNoWait(); }
      void clear()                                                         { stage_->clear(); } ///< Clears teh stage's error state, if any

      void updateTiling()                                                  { tiling_controller_.update();}
      //void invalidateTiling()                                              { tiling_controller_.update();}

    private:

      device::Stage   *stage_;
      AgentController  agent_controller_;
      TilingController tiling_controller_; 

      PlanarStageControllerListener listener_;

      PositionHistoryModel history_;
  };


}} //end fetch::ui
