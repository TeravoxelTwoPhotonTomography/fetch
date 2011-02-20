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
FUNCTION(_C843_ASSERT _VAR _MSG)
IF(NOT ${_VAR})
  IF(C843_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR ${_MSG}) 
  ELSE()
    MESSAGE(STATUS ${_MSG}) 
  ENDIF()
ENDIF()
ENDFUNCTION(_C843_ASSERT)

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
#_C843_ASSERT(C843_INCLUDE_DIR "[C843] Could not find C843_GCS_DLL.h")

find_library(C843_LIBRARY C843_GCS_DLL.lib
  HINTS
    ${_C843_HINTS}
  PATH_SUFFIXES
    win32
    x64
)
#_C843_ASSERT(C843_LIBRARY "[C843] Could not find C843_GCS_DLL.lib")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(C843
        REQUIRED_VARS
          C843_INCLUDE_DIR
          C843_LIBRARY)

# message("C843_INCLUDE_DIR is ${C843_INCLUDE_DIR}")
# message("C843_LIBRARY is ${C843_LIBRARY}")


