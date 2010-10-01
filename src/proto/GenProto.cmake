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

file(GLOB PROTO_FILES ${PROTO_DIR}/*.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_DIR} ${PROTO_FILES})

source_group(GeneratedProtoSources
  FILES ${PROTO_SRCS}
)
source_group(GeneratedProtoHeaders
  FILES ${PROTO_HDRS}
)
source_group(Proto
  REGULAR_EXPRESSION \\.proto$
)
