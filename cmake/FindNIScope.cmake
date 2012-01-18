##
# Locate NIScope 
#
# REQUIRED ENVIRONMENT VARIABLES
# ------------------------------
#
# NIEXTCCOMPILERSUPP (windows)
#
# OUTPUT VARIABLES
# ----------------
# NISCOPE_LIBRARIES
# NISCOPE_INCLUDE_DIR
# NISCOPE_FOUND
# HAVE_NISCOPE
#
# NISCOPE_NISCOPE_LIBRARY
# NISCOPE_IVI_LIBRARY
# NISCOPE_NIMODINST_LIBRARY
#

#
set(NISCOPE_FOUND "NO")
set(HAVE_NISCOPE 0)

# 64 or 32 bit
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(hints  $ENV{VXIPNPPATH64}/Win64)
  set(suffix lib/msc Lib_x64/msc)
else()
  set(hints  $ENV{VXIPNPPATH}/win95 $ENV{VXIPNPPATH}/winnt)
  set(suffix lib/msc)
endif()


# includes
find_path(NISCOPE_INCLUDE_DIR niscope.h
    HINTS ${hints}
    PATH_SUFFIXES include
)
      
# libs
find_library(NISCOPE_NISCOPE_LIBRARY niscope.lib# ivi.lib niModInst.lib
  HINTS ${hints}
  PATH_SUFFIXES ${suffix}
  )

find_library(NISCOPE_IVI_LIBRARY ivi.lib
  HINTS ${hints}
  PATH_SUFFIXES ${suffix}
  )

find_library(NISCOPE_NIMODINST_LIBRARY niModInst.lib 
  HINTS ${hints}
  PATH_SUFFIXES ${suffix}
  )

set(NISCOPE_LIBRARIES
    ${NISCOPE_NISCOPE_LIBRARY}
    ${NISCOPE_IVI_LIBRARY}
    ${NISCOPE_NIMODINST_LIBRARY}
   )
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NISCOPE
        REQUIRED_VARS
          NISCOPE_NISCOPE_LIBRARY
          NISCOPE_IVI_LIBRARY
          NISCOPE_NIMODINST_LIBRARY
          NISCOPE_INCLUDE_DIR
          )
if(NISCOPE_FOUND)
  set(HAVE_NISCOPE 1)
endif()

