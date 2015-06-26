#pragma once
#include "stage.pb.h"
namespace mylib {
#include <array.h>
}
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Set>
#include <stdint.h>

#include "thread.h"
#include "devices/Stage.h"

using namespace Eigen;

namespace fetch {
namespace device {

  typedef Matrix<size_t,1,3> Vector3z;

  //  StageTiling  /////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////
  class StageListener;
  struct TileSearchContext;
  class StageTiling
  {
  public:
    typedef fetch::cfg::device::Stage_TilingMode Mode;
    typedef Transform<float,3,Affine>            TTransform;
    typedef std::set<StageListener*>             TListeners;

  public: // pseudo-private
    mylib::Array              *attr_;                                      ///< tile attribute database
    mylib::Indx_Type           cursor_;                                    ///< marks the current tile
    mylib::Indx_Type           current_plane_offset_;                      ///< marks the current plane
    mylib::Indx_Type           sz_plane_nelem_;                            ///< the size of a plane in the tile database
    TTransform                 latticeToStage_;                            ///< Transforms lattice coordinates to the tiles anchor point on the stage
    TListeners                 listeners_;                                 ///< set of objects to be notified of tiling events
    FieldOfViewGeometry        fov_;                                       ///< the geometry used to generate the tiling
    f64                        z_offset_um_;
    device::StageTravel        travel_;                                    ///< the travel used to generate the tiling
    Mutex*                     lock_;                                      ///< protects access to attribute data.
    Mode                       mode_;
  public:

    enum Flags
    {
      Addressable = 1,                                                     ///< indicates the stage should be allowed to move to this tile
      Active      = 2,                                                     ///< indicates this tile is in the region of interest.
      Done        = 4,                                                     ///< indicates the tile has been imaged
      Explorable  = 8,                                                     ///< indicates the tile should be expored for auto-tiling
      TileError   = 16,                                                    ///< indicates there was some error moving to or imaging this tile
      Explored    = 32,                                                    ///< indicates area has already been looked at
      Detected    = 64,                                                    ///< indicates some signal was found at the bootom of this tile
      Safe        = 128,                                                   ///< indicates a tile is safe to image; it is within the allowed travel of the stages
      Reserved    = 512,                                                   ///< used internally to temporarily mark tiles
      Reserved2   = 256                                                    ///< used internally to temporarily mark tiles
    };

             StageTiling(const device::StageTravel& travel,
                         const FieldOfViewGeometry& fov,
                         const Mode                 alignment);
    virtual ~StageTiling();

    void     set_z_offset_um(f64 z_um);
    void     inc_z_offset_um(f64 z_um);
    f64          z_offset_um();
    inline void     set_z_offset_mm(f64 z_mm) {set_z_offset_um(1000.0*z_mm);}
    inline void     inc_z_offset_mm(f64 z_mm) {inc_z_offset_um(1000.0*z_mm);}
    inline f64          z_offset_mm()         {return z_offset_um()*1e-3;}

    void     resetCursor();
    void     setCursorToPlane(size_t iplane);
    bool     nextInPlanePosition(Vector3f& pos);
    bool     nextInPlaneExplorablePosition(Vector3f &pos);
    bool     nextPosition(Vector3f& pos);
    bool     nextInPlaneQuery(Vector3f &pos,uint32_t attrmask,uint32_t attr);

    bool     nextSearchPosition(int iplane, int ntimes, Vector3f &pos,TileSearchContext **ctx);     ///< *ctx should be NULL on the first call.  It will be internally managed.
    void     tileSearchCleanup(TileSearchContext *ctx);

    void     markDone(bool success);
    void     markActive(); // used by gui to explicitly set tiles to image
    void     markSafe(bool tf=true);
    void     markExplored(bool tf=true);
    void     markDetected(bool tf=true);
    void     markAddressable(size_t iplane); ///< Marks the indicated plane as addressable according to the travel.
    void     markUserReset(); ///< Resets user-settable flags to default

    int      anyExplored(int iplane);                                      //   2d
    int      updateActive(size_t iplane);                                  //   2d - returns 1 if any tiles were marked active, otherwise 0.
    void     fillHolesInActive(size_t iplane);                             //   2d
    void     dilateActive(size_t iplane);                                  //   2d

    void     fillHoles(size_t iplane, StageTiling::Flags flag);            //   2d
    void     dilate(size_t iplane, int ntimes, StageTiling::Flags query_flag, StageTiling::Flags write_flag, int explorable_only); // 2d

    inline mylib::Array*     attributeArray()                              {return attr_;}
    inline const TTransform& latticeToStageTransform()                     {return latticeToStage_; }
    inline const FieldOfViewGeometry& fov()                                {return fov_;}
    inline const device::StageTravel& travel()                             {return travel_;}
    inline size_t plane()                                                  {return (size_t)(current_plane_offset_/sz_plane_nelem_); }
           float  plane_mm();
    const TListeners *listeners()                                          {return &listeners_;}
    inline void addListener(StageListener *listener)                       {listeners_.insert(listener);}
    inline void delListener(StageListener *listener)                       {listeners_.erase(listener);}

    void lock()                                                            {Mutex_Lock(lock_);}
    void unlock()                                                          {Mutex_Unlock(lock_);}

    bool on_plane(uint32_t *p); //used by TileSearch    

    int minDistTo( // used by adaptive tiling
    uint32_t search_mask,uint32_t search_flags,   // area to search 
    uint32_t query_mask ,uint32_t query_flags);  // tile to find
    void getCursorLatticePosition(int* x,int* y,int* z);
  protected:
    void computeLatticeToStageTransform_
                        (const FieldOfViewGeometry& fov,
                         const Mode                 alignment);
    mylib::Coordinate* computeLatticeExtents_(const device::StageTravel& travel); ///< returned pointer needs to be freed (w Free_Array).
    void initAttr_(mylib::Coordinate *shape);                                     ///< Free's shape

    void notifyDone(size_t i, const Vector3f& pos, uint32_t sts);
    void notifyNext(size_t i, const Vector3f& pos);

    const Vector3f computeCursorPos();
    

  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  };

  // ERMAHGERD C++
  // THESE ARE PRIVATE CLASSES - DONT USE
  // Need these in the header so their destructors will be called properly?  GROSS
  struct tile_search_parent_t;
  struct tile_search_history_t
  { tile_search_parent_t *history;
    size_t i,n;
    tile_search_history_t();
    ~tile_search_history_t();
    int push(uint32_t *c,int dir,int n);    
    int pop(uint32_t **pc, uint32_t *pdir, uint32_t* pn);
    int pop(uint32_t **pc, int *pdir, int *pn);
    int request(size_t  i_);
    void flush();
  };

  struct TileSearchContext
  { StageTiling *tiling;
    uint32_t *c, 
              n,        // the next neighbor to look at
              dir,      // the direction of the last move
              mode,     // 0 - hunt mode, 1 - outline mode
              radius;   // radius for dilation of explorable area post-detection
    size_t line, plane; // strides in elements
    tile_search_history_t history;
    TileSearchContext(StageTiling *t,int radius=0);
    ~TileSearchContext();
    void sync();
    void set_outline_mode(bool tf=true);
    bool is_outline_mode();
    bool is_hunt_mode();
    uint32_t *next_neighbor();
    void flush();
    int push(uint32_t* p);
    int pop();
    bool detected();
    void reserve();
  };

}} //end namespace fetch