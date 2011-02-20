cmake_minimum_required(VERSION 2.8)
#
# INPUT VARIABLES
# ---------------
#
# ROOT_3RDPARTY_PATH
#

#Some stuff to let this CMakeLists.txt stand alone, if needed
enable_testing(true)

#find gtest
set(GTEST_ROOT "${ROOT_3RDPARTY_PATH}/gtest-1.5.0")
set(GTEST_MSVC_SEARCH MD)
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

##
# Config
#

find_package(OpenMP)

#protobuf
set(PROTO_DIR "${PROJECT_SOURCE_DIR}/src/proto")
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

#glew
find_package(Glew REQUIRED)
include_directories(${GLEW_INCLUDE_DIR})

#Alazar
find_package(Alazar REQUIRED)
include_directories(${ALAZAR_INCLUDE_DIR})

#PI C-843 Stage controller
find_package(C843)
if(C843_FOUND)
        set(OPTIONAL_LIBS ${OPTIONAL_LIBS} ${C843_LIBRARY})
        include_directories(${C843_INCLUDE_DIR})
endif(C843_FOUND)

#Qt
if(NOT ${QT_FOUND})
  SET(QTLIBLIST
    QtMain
    QtCore
    QtGui
    QtOpenGL
  )
  SET(QT_USE_IMPORTED_TARGETS TRUE)
  find_package(Qt4 4.7.0
    COMPONENTS ${QTLIBLIST}
    REQUIRED
  )
  include(${QT_USE_FILE})
  include_directories(${QT_INCLUDE_DIRS})
endif()

##
# Add tests
#
file(GLOB PRIMARY_TEST_SOURCES 
    *.cc
    *.cpp)
#set(PRIMARY_TEST_SOURCES
#  integration.cc
#  test.cc
#  file.cc
#  alazar.cc
#  )
set(OTHER_SOURCES
  ${FETCH_SRCS}
  ${PROTO_SRCS}
  ${PROTO_HDRS}
)
add_executable(alltests alltests.cc 
  ${PRIMARY_TEST_SOURCES} 
  ${OTHER_SOURCES}
)
target_link_libraries(alltests
  ${GTEST_BOTH_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${NIDAQMX_LIBRARY}
  ${NISCOPE_LIBRARIES}
  ${MYLIB_LIBRARIES}
  ${QT_LIBRARIES}
  ${OPENGL_LIBRARY}
  ${GLEW_LIBRARIES}
  ${ALAZAR_LIBRARY}
  ${OPTIONAL_LIBS}
)
set_target_properties(
  alltests
  PROPERTIES
    COMPILE_FLAGS ${OpenMP_C_FLAGS}
)
gtest_add_tests(alltests " " ${PRIMARY_TEST_SOURCES})
