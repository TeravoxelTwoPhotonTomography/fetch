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
  
