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

#32 or 64 bit
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(arch x64)
else()
  set(arch x32)
endif()

SET(_GLEW_HINTS
  ~/src/mylib
  "$ENV{USERPROFILE}/Desktop/src/libs/glew-1.8.0/${arch}"
  ${CMAKE_SOURCE_DIR}/3rdParty/glew-1.8.0/${arch}
  "$ENV{USERPROFILE}/Desktop/src/libs/glew-1.6.0/${arch}"
  ${CMAKE_SOURCE_DIR}/3rdParty/glew-1.6.0/${arch}
  "$ENV{USERPROFILE}/Desktop/src/libs/glew-1.5.6/${arch}"
  ${CMAKE_SOURCE_DIR}/3rdParty/glew-1.5.6/${arch}
)

FIND_LIBRARY(GLEW_GLEW_LIBRARY
  NAMES glew32.lib
        glew32s.lib
  HINTS ${_GLEW_HINTS}
  PATH_SUFFIXES 
        lib
  DOC "Location of Glew library"
)

FIND_FILE(GLEW_DLL_LIBRARY
  glew32.dll
  HINTS ${_GLEW_HINTS}
  PATH_SUFFIXES bin
  DOC "Location of Glew DLL"
)

FIND_PATH(GLEW_INCLUDE_DIR
  NAMES GL/glew.h
        GL/glxew.h
        GL/wglew.h
  HINTS ${_GLEW_HINTS}
  PATH_SUFFIXES include
  DOC "Root directory for Glew include files."
)

SET(GLEW_LIBRARIES ${GLEW_GLEW_LIBRARY})
SET(GLEW_DIR ${GLEW_INCLUDE_DIR})
SET(GLEW_FOUND TRUE)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLEW
        REQUIRED_VARS
          GLEW_INCLUDE_DIR
          GLEW_LIBRARIES
          )

if(GLEW_FOUND)
  include_directories(${GLEW_INCLUDE_DIR})
  FILE(COPY ${GLEW_DLL_LIBRARY} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif(GLEW_FOUND)
