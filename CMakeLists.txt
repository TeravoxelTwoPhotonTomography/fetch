cmake_minimum_required(VERSION 2.8.10)
##
# NOTES:
#
# - Had to modify the stock FindProtobuf.cmake module.  My changes can be found
#   in a personal git repo mirroring the stock installed Modules.  See
#   \\dm11.isi.janelia.org\myerslab\clackn\repo\cmake-modules
# - There's some cross-platform config stuff included here even though
#   this is a windows only build, really.  It's here because some parts
#   of the code base are used in other projects that are cross-platform.
#[ ] "algo" subdirectory needs better name?
#
project(fetch)
set(FETCH_MAJOR_VERSION 0)
set(FETCH_MINOR_VERSION 0)
set(FETCH_PATCH_VERSION develop)

######################################################################
#  Table of contents (for assisted text searching)
#
#   o   Useful options
#   o   Config
#   o   Source groups
#   o   Targets
#   o   Tests
#
######################################################################

macro(show v)
message(${v} is ${${v}})
endmacro()

######################################################################
#  Useful options
######################################################################
set(CMAKE_VERBOSE_MAKEFILE 1) ## Useful for debuging the build

##
# 3rdParty
#
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

#Paths
set(ROOT_3RDPARTY_PATH
  ${CMAKE_SOURCE_DIR}/3rdParty
  ${CMAKE_SOURCE_DIR}/../3rdParty
# "$ENV{USERPROFILE}/Desktop/src/libs"
  CACHE PATH "Root Location of 3rd party libraries"
)

###############################################################################
#  Config
###############################################################################

include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckCSourceCompiles)
include(CheckTypeSize)
check_type_size(size_t SIZET_BYTES)
check_function_exists(usleep HAVE_USLEEP)
check_c_source_compiles("main(){int a=0; return __sync_add_and_fetch(&a,10);}" HAVE_ATOMIC_INTRINSICS_GCC)
if(WIN32)
  set(HAVE_ATOMIC_INTRINSICS_MSVC TRUE)
else()
  set(HAVE_ATOMIC_INTRINSICS_MSVC FALSE)
endif()
check_include_file(unistd.h HAVE_UNISTD)
check_include_file(stdint.h HAVE_STDINT)

find_package(Threads)
set(USE_PTHREAD       ${CMAKE_USE_PTHREADS_INIT})
set(USE_WIN32_THREADS ${CMAKE_USE_WIN32_THREADS_INIT})

#protobuf
set(PROTO_DIR "${PROJECT_SOURCE_DIR}/src/proto")
include("${PROTO_DIR}/GenProto.cmake")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

#Eigen
find_package(Eigen3 3.0)
include_directories(${EIGEN3_INCLUDE_DIR})
add_definitions(-DNOMINMAX) #prevent windows from defineing min and max

#nidaq
find_package(NIDAQmx)
if(NIDAQMX_FOUND)
        set(OPTIONAL_LIBS ${OPTIONAL_LIBS} ${NIDAQMX_LIBRARY})
        include_directories(${NIDAQMX_INCLUDE_DIR})
endif(NIDAQMX_FOUND)

#niscope
find_package(NIScope)
if(NISCOPE_FOUND)
        set(OPTIONAL_LIBS ${OPTIONAL_LIBS} ${NISCOPE_LIBRARIES})
        include_directories(${NISCOPE_INCLUDE_DIR})
endif(NISCOPE_FOUND)

#mylib
find_package(MyLib REQUIRED)
include_directories(${MYLIB_INCLUDE_DIR})

#OpenGL
find_package(OpenGL REQUIRED)

# #[depricated] DirectX
#find_package(DirectX)
#include_directories(${DIRECTX_INCLUDE_DIR})

#glew
if(WIN32)
find_package(Glew REQUIRED)
include_directories(${GLEW_INCLUDE_DIR})
endif(WIN32)

#Alazar
find_package(Alazar)
if(ALAZAR_FOUND)
        set(OPTIONAL_LIBS ${OPTIONAL_LIBS} ${ALAZAR_LIBRARY})
        include_directories(${ALAZAR_INCLUDE_DIR})
endif(ALAZAR_FOUND)


#PI C-843 Stage controller
find_package(C843)
if(C843_FOUND)
        set(OPTIONAL_LIBS ${OPTIONAL_LIBS} ${C843_LIBRARY})
        include_directories(${C843_INCLUDE_DIR})
endif(C843_FOUND)

#Qt
#set(CMAKE_AUTOMOC ON)
find_package(Qt5Core)
find_package(Qt5Widgets)
find_package(Qt5Network)
find_package(Qt5OpenGL)
find_package(Qt5OpenGLExtensions)
find_package(Qt5PrintSupport)
#SET(QTLIBLIST
#  QtMain
#  QtCore
#  QtGui
#  QtOpenGL
#)
#SET(QT_USE_IMPORTED_TARGETS TRUE)
#find_package(Qt4 4.7
#  COMPONENTS ${QTLIBLIST}
#  REQUIRED
#)
#include(${QT_USE_FILE})
#include_directories(${QT_INCLUDE_DIRS})


#CUDA
find_package(CUDA 4.0)
if(CUDA_FOUND)
  set(HAVE_CUDA TRUE)
  include_directories(${CUDA_INCLUDE_DIR})
  cuda_build_clean_target()
  set(cuda_build_cubin on)
set(CUOPTS
  -Xcompiler -fPIC --generate-code arch=compute_30,code=sm_30
  DEBUG -g;-G
  RELEASE --use_fast_math
  RELWITHDEBINFO --use_fast_math;-g
  MINSIZEREL --use_fast_math
  )
endif()


###############################################################################
#  Config header
###############################################################################
configure_file(
        "${PROJECT_SOURCE_DIR}/config.h.in"
        "${PROJECT_BINARY_DIR}/config.h"
        )
include_directories("${PROJECT_BINARY_DIR}")

###############################################################################
#  Source groups
###############################################################################

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
file(GLOB FETCH_DEVICES_SRCS src/devices/*.cpp src/devices/*.c)
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
file(GLOB FETCH_UTIL_SRCS src/util/*.cpp src/util/*.c)
source_group("Source Files\\util" FILES ${FETCH_UTIL_SRCS})

file(GLOB FETCH_ALGO_HDRS src/algo/*.h)
file(GLOB FETCH_ALGO_SRCS src/algo/*.c src/algo/*.cpp)
file(GLOB FETCH_ALGO_CUDA src/algo/*.cu)
source_group("Header Files\\Algorithms" FILES
  ${FETCH_ALGO_HDRS}
  )
source_group("Source Files\\Algorithms" FILES
  ${FETCH_ALGO_SRCS}
  ${FETCH_ALGO_CUDA}
  )

file(GLOB FETCH_CORE_HDRS src/*.h)
file(GLOB FETCH_CORE_SRCS src/*.cpp src/*.c)
include_directories(src)

file(GLOB FETCH_MICROSCOPE_CONFIG src/apps/fetch.resource/*.microscope)
source_group("Config" FILES ${FETCH_MICROSCOPE_CONFIG})

file(GLOB SHADERS src/ui/resources/shaders/*.*)
source_group("DirectX Shaders" REGULAR_EXPRESSION \\.fx$)
source_group("GLSL Shaders\\Vertex Programs" REGULAR_EXPRESSION \\.vert$)
source_group("GLSL Shaders\\Fragments Programs" REGULAR_EXPRESSION \\.frag$)

qt5_wrap_cpp(MOC_SRCS
  src/ui/MainWindow.h
  src/ui/Player.h
  src/ui/Figure.h
  src/ui/ScaleBar.h
  src/ui/StageScene.h
  src/ui/StageView.h
  src/ui/Player.h
  src/ui/TilesView.h
  src/ui/VideoAcquisitionDockWidget.h
  src/ui/StackAcquisitionDockWidget.h
  src/ui/ColormapDockWidget.h
  src/ui/VibratomeDockWidget.h
  src/ui/StageDockWidget.h
  src/ui/AgentController.h
  src/ui/StageController.h
  src/ui/DevicePropController.h
  src/ui/qcustomplot.h
  src/ui/HistogramDockWidget.h
)
qt5_add_resources(RCC_SRCS
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

###############################################################################
#  Targets
###############################################################################

##
# Fetch
# Executable
#

#cuda_compile(FETCH_ALGO_CUDA_OUT STATIC ${FETCH_ALGO_CUDA} OPTIONS -arch=sm_20)
cuda_compile(FETCH_ALGO_CUDA_OUT STATIC ${FETCH_ALGO_CUDA} OPTIONS ${CUDA_OPTS})

# FETCH_SRCS is used below and by testing framework
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
  ${FETCH_ALGO_HDRS}
  ${FETCH_ALGO_SRCS}
  ${FETCH_ALGO_CUDA}
  ${FETCH_ALGO_CUDA_OUT}
  "${PROJECT_SOURCE_DIR}/config.h.in"
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
  ${FETCH_MICROSCOPE_CONFIG}
  ${SHADERS}
)
target_link_libraries(fetch
  ${GTEST_BOTH_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${MYLIB_LIBRARIES}
  ${QT_LIBRARIES}
  ${OPENGL_LIBRARY}
  ${GLEW_LIBRARIES}
  ${OPTIONAL_LIBS}
  ${CUDA_LIBRARIES}
# ${DIRECTX_LIBRARIES}
  Imm32.lib            # wierd extra windows libs that qt needs but doesn't
  Ws2_32.lib           # automatically include
  Winmm.lib
)
qt5_use_modules(fetch Core OpenGL Widgets PrintSupport OpenGLExtensions Network)

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

if(C843_FOUND)
#message("HERE: C843_FOUND is ${C843_FOUND}")
        add_executable(queryStages
            src/apps/queryStages.cc
            src/common.h
            src/common.cpp
            )
        target_link_libraries(queryStages
            ${C843_LIBRARY}
            )
endif(C843_FOUND)

###############################################################################
#  Test
###############################################################################
#enable_testing(true)
#add_subdirectory(tests)

###############################################################################
#  Documentation
###############################################################################

find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in
                 ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile
                 @ONLY
                 )
  add_custom_target(apidocs
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT           "Generating API Docs"
    VERBATIM
    )
endif()

find_package(sphinx)
if(SPHINX_FOUND)
  set(SPHINX_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/doc/source)
  set(SPHINX_CONFPY_DIR ${CMAKE_CURRENT_BINARY_DIR}/doc/)
  set(SPHINX_CACHE_DIR  ${CMAKE_CURRENT_BINARY_DIR}/doc/doctrees)
  set(SPHINX_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doc/html)
  configure_file(${SPHINX_SOURCE_DIR}/conf.py.in
                 ${SPHINX_CONFPY_DIR}/conf.py
                 @ONLY
                 )
  add_custom_target(sphinxdocs
    ${SPHINX_EXECUTABLE} -b html -d ${SPHINX_CACHE_DIR} -c ${SPHINX_CONFPY_DIR} ${SPHINX_SOURCE_DIR} ${SPHINX_OUTPUT_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS apidocs
    COMMENT "Generating Sphinx Docs"
    )
endif()
