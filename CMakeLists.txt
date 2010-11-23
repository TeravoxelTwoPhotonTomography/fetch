##
# NOTES:
#
# - Had to modify the stock FindProtobuf.cmake module.  My changes can be found
#   in a personal git repo mirroring the stock installed Modules.  See
#   \\dm11.isi.janelia.org\myerslab\clackn\repo\cmake-modules
#
cmake_minimum_required(VERSION 2.6)
project(fetch)

##
# 3rdParty
#
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

#Paths
set(ROOT_3RDPARTY_PATH
  ${CMAKE_SOURCE_DIR}/3rdParty
# "$ENV{USERPROFILE}/Desktop/src/libs"
  CACHE PATH "Root Location of 3rd party libraries"
)

##
# Config
#

#protobuf
set(PROTO_DIR "src/proto")
include("${PROTO_DIR}/GenProto.cmake")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

#nidaq
find_package(NIDAQmx REQUIRED)
include_directories(${NIDAQMX_INCLUDE_DIR})

#niscope
find_package(NIScope REQUIRED)
include_directories(${NISCOPE_INCLUDE_DIR})

#mylib
find_package(MyLib REQUIRED)
include_directories(${MYLIB_INCLUDE_DIR})

#OpenGL
find_package(OpenGL)

# #[depricated] DirectX
#find_package(DirectX)
#include_directories(${DIRECTX_INCLUDE_DIR})

#glew
if(WIN32)
find_package(Glew REQUIRED)
include_directories(${GLEW_INCLUDE_DIR})
endif(WIN32)

#Alazar
find_package(Alazar REQUIRED)
include_directories(${ALAZAR_INCLUDE_DIR})

#PI C-843 Stage controller
find_package(C843 REQUIRED)
include_directories(${C843_INCLUDE_DIR})

#Qt
SET(QTLIBLIST
  QtMain
  QtCore
  QtGui
  QtOpenGL
)
SET(QT_USE_IMPORTED_TARGETS TRUE)
find_package(Qt4 4.7
  COMPONENTS ${QTLIBLIST}
  REQUIRED
)
include(${QT_USE_FILE})
include_directories(${QT_INCLUDE_DIRS})

##
# Fetch
# Source groups
# - automatically add and organize sources from certain sub-directories

file(GLOB FETCH_WORKERS_HDRS src/workers/*.h)
source_group("Header Files\\Workers" FILES ${FETCH_WORKERS_HDRS})
file(GLOB FETCH_WORKERS_SRCS src/workers/*.cpp)
source_group("Source Files\\Workers" FILES ${FETCH_WORKERS_SRCS})

file(GLOB FETCH_DEVICES_HDRS src/devices/*.h)
source_group("Header Files\\Devices" FILES ${FETCH_DEVICES_HDRS})
file(GLOB FETCH_DEVICES_SRCS src/devices/*.cpp)
source_group("Source Files\\Devices" FILES ${FETCH_DEVICES_SRCS})

file(GLOB FETCH_TASKS_HDRS src/tasks/*.h)
source_group("Header Files\\Tasks" FILES ${FETCH_TASKS_HDRS})
file(GLOB FETCH_TASKS_SRCS src/tasks/*.cpp)
source_group("Source Files\\Tasks" FILES ${FETCH_TASKS_SRCS})

file(GLOB FETCH_UI_HDRS src/ui/*.h)
source_group("Header Files\\ui" FILES ${FETCH_UI_HDRS})
file(GLOB FETCH_UI_SRCS src/ui/*.cpp)
source_group("Source Files\\ui" FILES ${FETCH_UI_SRCS})

file(GLOB FETCH_UTIL_HDRS src/util/*.h)
source_group("Header Files\\util" FILES ${FETCH_UTIL_HDRS})
file(GLOB FETCH_UTIL_SRCS src/util/*.cpp)
source_group("Source Files\\util" FILES ${FETCH_UTIL_SRCS})

file(GLOB FETCH_CORE_HDRS src/*.h)
file(GLOB FETCH_CORE_SRCS src/*.cpp)
include_directories(src)

source_group("DirectX Shaders" REGULAR_EXPRESSION \\.fx$)
source_group("GLSL Shaders\\Vertex Programs" REGULAR_EXPRESSION \\.vert$)
source_group("GLSL Shaders\\Fragments Programs" REGULAR_EXPRESSION \\.frag$)

qt4_wrap_cpp(MOC_SRCS
  src/ui/MainWindow.h
  src/ui/Player.h
  src/ui/Figure.h
  src/ui/VideoAcquisitionDockWidget.h
)
qt4_add_resources(RCC_SRCS
  src/ui/icons.qrc
  src/ui/shaders.qrc
  src/ui/colormaps.qrc
  src/apps/fetch.qrc
)
source_group("Source Files\\Qt Generated" FILES
  ${MOC_SRCS}
  ${RCC_SRCS}
)
#  REGULAR_EXPRESSION "(qrc|moc)_.+\\.cxx$")

##
# Fetch
# Executable
#

# FETCH_SRCS is used by testing, for example
set(FETCH_SRCS
  ${PROTO_FILES}
  ${FETCH_WORKERS_HDRS}
  ${FETCH_WORKERS_SRCS}
  ${FETCH_DEVICES_HDRS}
  ${FETCH_DEVICES_SRCS}
  ${FETCH_TASKS_HDRS}
  ${FETCH_TASKS_SRCS}
  ${FETCH_UTIL_HDRS}
  ${FETCH_UTIL_SRCS}
  ${FETCH_CORE_HDRS}
  ${FETCH_CORE_SRCS}
  )
add_executable(fetch
  src/apps/fetch.cpp
  ${PROTO_HDRS}
  ${PROTO_SRCS}
  ${FETCH_SRCS}
  ${FETCH_UI_HDRS}
  ${FETCH_UI_SRCS}
  ${MOC_SRCS}
  ${RCC_SRCS}
)
target_link_libraries(fetch
  ${GTEST_BOTH_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${NIDAQMX_LIBRARY}
  ${NISCOPE_LIBRARIES}
  ${MYLIB_LIBRARIES}
  ${QT_LIBRARIES}
  ${OPENGL_LIBRARY}
  ${GLEW_LIBRARIES}
  ${ALAZAR_LIBRARY}
  ${C843_LIBRARY}
# ${DIRECTX_LIBRARIES} 
)

##
# Apps
#

add_executable(dumpMessage
  src/apps/dumpMessage.cc
  src/util/util-protobuf.h
  src/util/util-protobuf.cpp
  src/common.h
  src/common.cpp
  ${PROTO_FILES}
  ${PROTO_HDRS}
  ${PROTO_SRCS}
  )
target_link_libraries(dumpMessage
  ${PROTOBUF_LIBRARIES}
  )

add_executable(queryStages
    src/apps/queryStages.cc
    src/common.h
    src/common.cpp
    )
target_link_libraries(queryStages
    ${C843_LIBRARY}
    )
##
# Testing
#
enable_testing(true)
add_subdirectory(tests)
