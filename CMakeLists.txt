cmake_minimum_required(VERSION 2.6)
project(fetch)

##
# Testing
#
enable_testing(true)
add_subdirectory(tests)

##
# 3rdParty
#

#Paths
set(ROOT_3RDPARTY_PATH
  "$ENV{USERPROFILE}/Desktop/src/libs"
)
set(CMAKE_LIBRARY_PATH
  ${CMAKE_LIBRARY_PATH}
  "${ROOT_3RDPARTY_PATH}/protobuf-2.3.0/vsprojects/Debug"
)
set(CMAKE_INCLUDE_PATH
  ${CMAKE_INCLUDE_PATH}
  "${ROOT_3RDPARTY_PATH}/protobuf-2.3.0/src"
)
set(CMAKE_PROGRAM_PATH
  ${CMAKE_PROGRAM_PATH}
  "${ROOT_3RDPARTY_PATH}/protobuf-2.3.0/vsprojects/Release"
)
 
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_BINARY_DIRS})

##
# Assembly
#
set(PROTO_DIR
  src/proto
)
set(PROTO_FILES
  ${PROTO_DIR}/zpiezo.proto
  ${PROTO_DIR}/shutter.proto
  ${PROTO_DIR}/scanner3d.proto
  ${PROTO_DIR}/scanner2d.proto
  ${PROTO_DIR}/pockels.proto
  ${PROTO_DIR}/linear_scan_mirror.proto
  ${PROTO_DIR}/digitizer.proto
)

source_group(Proto
  REGULAR_EXPRESSION \\.proto$
)

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
message(${PROTO_SRCS})
message(${PROTO_HDRS})
