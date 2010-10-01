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

#Paths
set(ROOT_3RDPARTY_PATH
  "$ENV{USERPROFILE}/Desktop/src/libs"
  CACHE PATH "Root Location of 3rd party libraries"
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

# Protobuf 
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_BINARY_DIRS})

##
# Assembly
#
set(PROTO_DIR src/proto)
include("${PROTO_DIR}/GenProto.cmake")

##
# Testing
#
enable_testing(true)
add_subdirectory(tests)
