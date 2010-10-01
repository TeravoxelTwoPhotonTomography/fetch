#
# INPUT VARIABLES
# ---------------
#
# ROOT_3RDPARTY_PATH
#

#Some stuff to let this CMakeLists.txt stand alone, if needed
cmake_minimum_required(VERSION 2.8)
enable_testing(true)

#find gtest
set(GTEST_ROOT "${ROOT_3RDPARTY_PATH}/gtest-1.5.0")
set(GTEST_MSVC_SEARCH MD)
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

#add tests
set(PRIMARY_TEST_SOURCES
  proto.cc
  test.cc
  )
set(PROTO_DIR "../src/proto")
include("${PROTO_DIR}/GenProto.cmake")
include_directories(${CMAKE_CURRENT_BINARY_DIR})
set(OTHER_SOURCES
  ${PROTO_SRCS}
  ${PROTO_HDRS}
)
add_executable(alltests alltests.cc 
  ${PRIMARY_TEST_SOURCES} 
  ${OTHER_SOURCES}
)
target_link_libraries(alltests
  ${GTEST_BOTH_LIBRARIES}
  ${PROTOBUF_LIBRARIES})
gtest_add_tests(alltests " " ${PRIMARY_TEST_SOURCES})
