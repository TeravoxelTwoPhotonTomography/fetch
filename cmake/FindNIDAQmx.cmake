##
# Locate NI-DAQmx
#
# REQUIRED ENVIRONMENT VARIABLES
# ------------------------------
#
# NIEXTCCOMPILERSUPP (windows)
#
# OUTPUT VARIABLES
# ----------------
# NIDAQMX_LIBRARY
# NIDAQMX_INCLUDE_DIR
# NIDAQMX_FOUND
# HAVE_NIDAQMX
#

#
set(NIDAQMX_FOUND "NO")
set(HAVE_NIDAQMX 0)

find_path(NIDAQMX_INCLUDE_DIR NIDAQmx.h
    HINTS
    $ENV{NIEXTCCOMPILERSUPP}
    PATH_SUFFIXES include
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(dir lib64/msvc)
else()
  set(dir lib32/msvc)
endif()

find_library(NIDAQMX_LIBRARY NIDAQmx.lib 
  PATHS $ENV{NIEXTCCOMPILERSUPP}
  PATH_SUFFIXES ${dir}
  )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NIDAQMX
        REQUIRED_VARS
          NIDAQMX_INCLUDE_DIR
          NIDAQMX_LIBRARY
          )
if(NIDAQMX_FOUND)
  set(HAVE_NIDAQMX 1)
endif()
