##
# Locate Physik Instrument's GCS API for the C-843
#        stage controller
#
# OUTPUT VARIABLES
# ----------------
# C843_LIBRARY
# C843_INCLUDE_DIR
# C843_FOUND
# HAVE_C843
#

#
set(C843_FOUND "NO")
set(HAVE_C843 0)

set(_C843_HINTS
    ${ROOT_3RDPARTY_PATH}/C843_GCS_DLL 
    $ENV{PROGRAMFILES}/PI/C-843/C843_GCS_DLL
   )

find_path(C843_INCLUDE_DIR C843_GCS_DLL.H
    HINTS
    ${_C843_HINTS}
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set( libname C843_GCS_DLL_x64 )
else()
    set( libname C843_GCS_DLL )
endif()
  
find_library(C843_LIBRARY ${libname}
  HINTS ${_C843_HINTS}
  PATH_SUFFIXES win32 x64
)
FIND_FILE(C843_DLL_LIBRARY
  ${libname}.dll
  HINTS ${_C843_HINTS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(C843
        REQUIRED_VARS
          C843_INCLUDE_DIR
          C843_LIBRARY)

# message("C843_INCLUDE_DIR is ${C843_INCLUDE_DIR}")
# message("C843_LIBRARY is ${C843_LIBRARY}")

if(C843_FOUND)
  include_directories(${C843_INCLUDE_DIR})
  FILE(COPY ${C843_DLL_LIBRARY} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  set(HAVE_C843 1)
endif(C843_FOUND)
