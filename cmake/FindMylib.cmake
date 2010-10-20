# Defines the following
#
#   MYLIB_FOUND
#   MYLIB_DIR
#   MYLIB_INCLUDE_DIR
#   MYLIB_LIBRARIES
#
SET(MYLIB_FOUND FALSE)

#
FUNCTION(_MYLIB_ASSERT _VAR _MSG)
IF(NOT ${_VAR})
  IF(${MYLIB_FIND_REQUIRED})
    MESSAGE(FATAL_ERROR ${_MSG}) 
  ELSE()
    MESSAGE(STATUS ${_MSG}) 
  ENDIF()
ENDIF()
ENDFUNCTION(_MYLIB_ASSERT)

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
_MYLIB_ASSERT(MYLIB_INCLUDE_DIR "Could not find Mylib include files.")

#mylib
FIND_LIBRARY(MYLIB_MYLIB_LIBRARY
  NAMES libmylib.a
        mylib.lib
  HINTS ${_MYLIB_HINTS}
  PATH_SUFFIXES build/Debug
                build/Release
                build
                xcode/Debug
                xcode/Release
                xcode
                vc2010
                vc2010/Debug
                vc2010/Release
                vc2010x64
                vc2010x64/Debug
                vc2010x64/Release
  DOC "Location of Mylib library"
)
_MYLIB_ASSERT(MYLIB_MYLIB_LIBRARY "Could not find Mylib library.")

#mytiff
FIND_LIBRARY(MYLIB_MYTIFF_LIBRARY
  NAMES libmytiff.a
        mytiff.lib
  HINTS ${_MYLIB_HINTS}
  PATH_SUFFIXES build/MY_TIFF/Debug
                build/MY_TIFF/Release
                build/MY_TIFF
                xcode/MY_TIFF/Debug
                xcode/MY_TIFF/Release
                xcode/MY_TIFF
                vc2010/MY_TIFF
                vc2010/MY_TIFF/Debug
                vc2010/MY_TIFF/Release
                vc2010x64/MY_TIFF
                vc2010x64/MY_TIFF/Debug
                vc2010x64/MY_TIFF/Release
  DOC "Location of Mylib TIFF library"
)
_MYLIB_ASSERT(MYLIB_MYTIFF_LIBRARY "Could not find Mylib TIFF library.")

#myfft
FIND_LIBRARY(MYLIB_MYFFT_LIBRARY
  NAMES libmyfft.a
        myfft.lib
  HINTS ${_MYLIB_HINTS}
  PATH_SUFFIXES build/MY_FFT/Debug
                build/MY_FFT/Release
                build/MY_FFT
                xcode/MY_FFT/Debug
                xcode/MY_FFT/Release
                xcode/MY_FFT
                vc2010/MY_FFT
                vc2010/MY_FFT/Debug
                vc2010/MY_FFT/Release
                vc2010x64/MY_FFT
                vc2010x64/MY_FFT/Debug
                vc2010x64/MY_FFT/Release
  DOC "Location of Mylib FFT library"
)
_MYLIB_ASSERT(MYLIB_MYFFT_LIBRARY "Could not find Mylib FFT library.")

IF(WIN32)
  #snprintf
  FIND_LIBRARY(MYLIB_SNPRINTF_LIBRARY
    snprintf.lib
    HINTS ${_MYLIB_HINTS}
    PATH_SUFFIXES build/3rdParty/snprintf/Debug
                  build/3rdParty/snprintf/Release
                  build/3rdParty/snprintf
                  xcode/3rdParty/snprintf/Debug
                  xcode/3rdParty/snprintf/Release
                  xcode/3rdParty/snprintf
                  vc2010/3rdParty/snprintf
                  vc2010/3rdParty/snprintf/Debug
                  vc2010/3rdParty/snprintf/Release
                  vc2010x64/3rdParty/snprintf
                  vc2010x64/3rdParty/snprintf/Debug
                  vc2010x64/3rdParty/snprintf/Release
    DOC "Location of 3rd party snprintf compatibility library"
  )
  _MYLIB_ASSERT(MYLIB_SNPRINTF_LIBRARY "Could not find snprintf compatibility library.")
ENDIF(WIN32)
SET(MYLIB_LIBRARIES ${MYLIB_MYLIB_LIBRARY} ${MYLIB_MYTIFF_LIBRARY} ${MYLIB_MYFFT_LIBRARY} ${MYLIB_SNPRINTF_LIBRARY})
SET(MYLIB_DIR ${MYLIB_INCLUDE_DIR})
SET(MYLIB_FOUND TRUE)

if(MYLIB_FOUND)
  include_directories(${MYLIB_INCLUDE_DIR})
endif(MYLIB_FOUND)
  
