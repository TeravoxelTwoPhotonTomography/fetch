##
# Locate AlazarTech ATS-SDK
#
# OUTPUT VARIABLES
# ----------------
# ALAZAR_LIBRARY
# ALAZAR_INCLUDE_DIR
# ALAZAR_FOUND
# HAVE_ALAZAR         - Used with #cmakedefine for configuration.
#

#
set(ALAZAR_FOUND "NO")
set(HAVE_ALAZAR 0)
set(ALAZAR_VERSION 6.0.3)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(sys x64)
else()
  set(sys Win32)
endif()

find_path(ALAZAR_INCLUDE_DIR AlazarApi.h
    HINTS
    ${CMAKE_SOURCE_DIR}/3rdParty/AlazarTech/ATS-SDK/${ALAZAR_VERSION}
    PATH_SUFFIXES Include
)

find_library(ALAZAR_LIBRARY ATSApi.lib
  HINTS
  ${CMAKE_SOURCE_DIR}/3rdParty/AlazarTech/ATS-SDK/${ALAZAR_VERSION}
  PATH_SUFFIXES Library/${sys}
  )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ALAZAR
        REQUIRED_VARS
          ALAZAR_INCLUDE_DIR
          ALAZAR_LIBRARY
          )
if(ALAZAR_FOUND)
  set(HAVE_ALAZAR 1)
endif()
