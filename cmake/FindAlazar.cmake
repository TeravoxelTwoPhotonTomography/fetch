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
FUNCTION(_ALAZAR_ASSERT _VAR _MSG)
IF(NOT ${_VAR})
  IF(${ALAZAR_FIND_REQUIRED})
    MESSAGE(FATAL_ERROR ${_MSG}) 
  ELSE()
    MESSAGE(STATUS ${_MSG}) 
  ENDIF()
ENDIF()
ENDFUNCTION(_ALAZAR_ASSERT)

#
set(ALAZAR_FOUND "NO")
set(HAVE_ALAZAR 0)

find_path(ALAZAR_INCLUDE_DIR AlazarApi.h
    HINTS
    ${CMAKE_SOURCE_DIR}/3rdParty/AlazarTech/ATS-SDK/5.8.2
    PATH_SUFFIXES Include
)

find_library(ALAZAR_LIBRARY ATSApi.lib
  HINTS
  ${CMAKE_SOURCE_DIR}/3rdParty/AlazarTech/ATS-SDK/5.8.2
  PATH_SUFFIXES Library/Win32 Library/x64
  )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MYLIB
        REQUIRED_VARS
          ALAZAR_INCLUDE_DIR
          ALAZAR_LIBRARY
          )

