#
# INPUT VARIABLES
# ---------------
#
# ROOT_3RDPARTY_PATH - used to search for protobuf 
# PROTO_DIR          - used to search for .proto files
#
# OUTPUT VARIABLES
# ----------------
#
# PROTO_FILES
# PROTO_SRCS
# PROTO_HDRS
#
# EXAMPLE
# -------
# set(PROTO_DIR "../src/proto")
# include("${PROTO_DIR}/GenProto.cmake")
#

set(CMAKE_LIBRARY_PATH
  ${CMAKE_LIBRARY_PATH}
  "${ROOT_3RDPARTY_PATH}/protobuf-2.4.0a/vsprojects/Debug"
  "${ROOT_3RDPARTY_PATH}/protobuf-2.3.0/vsprojects/Debug"
)
set(CMAKE_INCLUDE_PATH
  ${CMAKE_INCLUDE_PATH}
  "${ROOT_3RDPARTY_PATH}/protobuf-2.4.0a/src"
  "${ROOT_3RDPARTY_PATH}/protobuf-2.3.0/src"
)
set(CMAKE_PROGRAM_PATH
  ${CMAKE_PROGRAM_PATH}
  "${ROOT_3RDPARTY_PATH}/protobuf-2.4.0a/vsprojects/Release"
  "${ROOT_3RDPARTY_PATH}/protobuf-2.3.0/vsprojects/Release"
)

find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})

file(GLOB PROTO_FILES ${PROTO_DIR}/*.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_DIR} ${PROTO_FILES})
PROTOBUF_GENERATE_PYTHON(PROTO_PYTHON ${PROTO_DIR} ${PROTO_FILES})

#foreach(F ${PROTO_SRCS})
#  message(${F})
#endforeach()
source_group("Source Files\\Protobuf Generated"
  FILES ${PROTO_SRCS}
)
source_group("Header Files\\Protobuf Generated"
  FILES ${PROTO_HDRS}
)
source_group("Python Files\\Protobuf Generated"
  FILES ${PROTO_PYTHON}
)
source_group("Protobuf Files"
  REGULAR_EXPRESSION \\.proto$
)
