# Defines the following
#
#   GLEW_FOUND
#   GLEW_DIR
#   GLEW_INCLUDE_DIR
#   GLEW_LIBRARIES
#
# Currently does NOT respect
# - Version numebers
# - expected install path described in glew docs
# - 32 vs 64 bit builds
SET(GLEW_FOUND FALSE)

# Handle missing requirements
FUNCTION(_GLEW_ASSERT _VAR _MSG)
IF(NOT ${_VAR})
  message(${_VAR})
  IF(${GLEW_FIND_REQUIRED})
    MESSAGE(FATAL_ERROR ${_MSG}) 
  ELSE()
    MESSAGE(STATUS ${_MSG}) 
  ENDIF()
ENDIF()
ENDFUNCTION(_GLEW_ASSERT)

# The HINTS variable is used for values computed from the system
SET(_GLEW_HINTS
  ~/src/mylib
  "$ENV{USERPROFILE}/Desktop/src/libs/glew-1.5.6/x32"
)

FIND_LIBRARY(GLEW_GLEW_LIBRARY
  NAMES glew32.lib
        glew32s.lib
  HINTS ${_GLEW_HINTS}
  PATH_SUFFIXES 
        lib
  DOC "Location of Glew library"
)
_GLEW_ASSERT(GLEW_GLEW_LIBRARY "Could not find Glew library.")

FIND_FILE(GLEW_DLL_LIBRARY
  glew32.dll
  HINTS ${_GLEW_HINTS}
  PATH_SUFFIXES bin
  DOC "Location of Glew DLL"
)
_GLEW_ASSERT(GLEW_DLL_LIBRARY "Could not find Glew DLL.")

FIND_PATH(GLEW_INCLUDE_DIR
  NAMES GL/glew.h
        GL/glxew.h
        GL/wglew.h
  HINTS ${_GLEW_HINTS}
  PATH_SUFFIXES include
  DOC "Root directory for Glew include files."
)
_GLEW_ASSERT(GLEW_INCLUDE_DIR "Could not find Glew include files.")

SET(GLEW_LIBRARIES ${GLEW_GLEW_LIBRARY})
SET(GLEW_DIR ${GLEW_INCLUDE_DIR})
SET(GLEW_FOUND TRUE)

if(GLEW_FOUND)
  include_directories(${GLEW_INCLUDE_DIR})
  FILE(COPY ${GLEW_DLL_LIBRARY} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif(GLEW_FOUND)
