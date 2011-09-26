# Defines the following
#
#   MYLIB_FOUND
#   MYLIB_DIR
#   MYLIB_INCLUDE_DIR
#   MYLIB_LIBRARIES
#
SET(MYLIB_FOUND FALSE)

# The HINTS variable is used for values computed from the system
SET(_MYLIB_HINTS
  ~/src/mylib
  "$ENV{USERPROFILE}/Desktop/src/mylib"
  ${ROOT_3RDPARTY_PATH}/mylib
)

FIND_PATH(MYLIB_INCLUDE_DIR
  array.h
  HINTS ${_MYLIB_HINTS}
  DOC "Root directory for Mylib include files."
)

#Some path variables
if(MSVC OR XCODE)
  set(_DEBUG_HINTS   ${CMAKE_GENERATOR}/Debug)
  set(_RELEASE_HINTS ${CMAKE_GENERATOR}/RelWithDebInfo)
else()
  set(_DEBUG_HINTS   build/Debug)
  set(_RELEASE_HINTS build/RelWithDebInfo)
endif()

#Macro: Handle missing release version (MSVC chokes in some cases)
macro(HANDLE_MISSING N A B)
  if(NOT EXISTS ${${A}} )
    message(WARNING "Could not find release version for ${N}. Replacing with debug version.")
    set(${A} ${${B}} CACHE filepath "" FORCE)
  endif()
endmacro()

macro(SHOW V)
  message("${V} is ${${V}}")
endmacro()

#mylib
set(mylib_names libmylib.a mylib.lib)
FIND_LIBRARY(MYLIB_MYLIB_LIBRARY_D
  NAMES ${mylib_names}
  HINTS ${_MYLIB_HINTS}
  PATH_SUFFIXES ${_DEBUG_HINTS}
  DOC "Location of the debug version of Mylib library"
)
FIND_LIBRARY(MYLIB_MYLIB_LIBRARY_R
  NAMES ${mylib_names}
  HINTS ${_MYLIB_HINTS}
  PATH_SUFFIXES ${_RELEASE_HINTS}
  DOC "Location of the optimized version of Mylib library"
)
HANDLE_MISSING(Mylib MYLIB_MYLIB_LIBRARY_R MYLIB_MYLIB_LIBRARY_D)
set(MYLIB_MYLIB_LIBRARY debug ${MYLIB_MYLIB_LIBRARY_D} optimized ${MYLIB_MYLIB_LIBRARY_R})

#mytiff
set(mytiff_names libmytiff.a mytiff.lib)
FIND_LIBRARY(MYLIB_MYTIFF_LIBRARY_D
  NAMES ${mytiff_names}
  HINTS ${_MYLIB_HINTS}
  PATH_SUFFIXES ${_DEBUG_HINTS}
  DOC "Location of the debug version of Mylib TIFF library"
)
FIND_LIBRARY(MYLIB_MYTIFF_LIBRARY_R
  NAMES ${mytiff_names}
  HINTS ${_MYLIB_HINTS}
  PATH_SUFFIXES ${_RELEASE_HINTS}
  DOC "Location of the optimized version of Mylib TIFF library"
)
HANDLE_MISSING(MyTIFF MYLIB_MYTIFF_LIBRARY_R MYLIB_MYTIFF_LIBRARY_D)
set(MYLIB_MYTIFF_LIBRARY debug ${MYLIB_MYTIFF_LIBRARY_D} optimized ${MYLIB_MYTIFF_LIBRARY_R})

#myfft
set(myfft_names libmyfft.a myfft.lib)
FIND_LIBRARY(MYLIB_MYFFT_LIBRARY_D
  NAMES ${myfft_names}
  HINTS ${_MYLIB_HINTS}
  PATH_SUFFIXES ${_DEBUG_HINTS}
  DOC "Location of the debug version of Mylib FFT library"
)
FIND_LIBRARY(MYLIB_MYFFT_LIBRARY_R
  NAMES ${myfft_names}
  HINTS ${_MYLIB_HINTS}
  PATH_SUFFIXES ${_RELEASE_HINTS}
  DOC "Location of the optimized version of Mylib FFT library"
)
HANDLE_MISSING(MyFFT MYLIB_MYFFT_LIBRARY_R MYLIB_MYFFT_LIBRARY_D)
set(MYLIB_MYFFT_LIBRARY debug ${MYLIB_MYFFT_LIBRARY_D} optimized ${MYLIB_MYFFT_LIBRARY_R})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MYLIB
        REQUIRED_VARS
          MYLIB_MYLIB_LIBRARY
          MYLIB_MYTIFF_LIBRARY
          MYLIB_MYFFT_LIBRARY
          MYLIB_INCLUDE_DIR
          )
SET(MYLIB_LIBRARIES ${MYLIB_MYLIB_LIBRARY} ${MYLIB_MYTIFF_LIBRARY} ${MYLIB_MYFFT_LIBRARY} )
SET(MYLIB_DIR ${MYLIB_INCLUDE_DIR})
SET(MYLIB_FOUND TRUE)

if(MYLIB_FOUND)
  include_directories(${MYLIB_INCLUDE_DIR})
endif(MYLIB_FOUND)
  
