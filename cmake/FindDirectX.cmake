# Defines the following
#
#   DIRECTX_FOUND
#   DIRECTX_DIR
#   DIRECTX_INCLUDE_DIR
#   DIRECTX_LIBRARIES
#
# Currently does NOT respect
# - Version numebers
# - expected install path described in glew docs
# - 32 vs 64 bit builds
SET(DIRECTX_FOUND FALSE)

# Handle missing requirements
FUNCTION(_DIRECTX_ASSERT _VAR _MSG)
IF(NOT ${_VAR})
  message(${_VAR})
  IF(${DIRECTX_FIND_REQUIRED})
    MESSAGE(FATAL_ERROR ${_MSG}) 
  ELSE()
    MESSAGE(STATUS ${_MSG}) 
  ENDIF()
ENDIF()
ENDFUNCTION(_DIRECTX_ASSERT)

# The HINTS variable is used for values computed from the system
SET(_DIRECTX_HINTS $ENV{DXSDK_DIR})

FIND_LIBRARY(DIRECTX_D3D_LIBRARY
  d3d10.lib
  HINTS ${_DIRECTX_HINTS}
  PATH_SUFFIXES 
        Lib/x86
  DOC "Location of D3D library"
)
_DIRECTX_ASSERT(DIRECTX_D3D_LIBRARY "Could not find d3d10.lib.")

FIND_LIBRARY(DIRECTX_D3DX_LIBRARY
  d3dx10.lib
  HINTS ${_DIRECTX_HINTS}
  PATH_SUFFIXES 
        Lib/x86
  DOC "Location of D3DX library"
)
_DIRECTX_ASSERT(DIRECTX_D3DX_LIBRARY "Could not find d3dx10d.lib.")

FIND_PATH(DIRECTX_INCLUDE_DIR
  NAMES d3d10.h
        d3dx10.h
  HINTS ${_DIRECTX_HINTS}
  PATH_SUFFIXES Include
  DOC "Root directory for DirectX SDK include files."
)
_DIRECTX_ASSERT(DIRECTX_INCLUDE_DIR "Could not find DirectX include files.")

SET(DIRECTX_LIBRARIES ${DIRECTX_D3D_LIBRARY} ${DIRECTX_D3DX_LIBRARY})
SET(DIRECTX_DIR ${DIRECTX_INCLUDE_DIR})
SET(DIRECTX_FOUND TRUE)
