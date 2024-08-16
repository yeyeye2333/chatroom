cmake_minimum_required(VERSION 3.26.5)

project(client)


find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
# include_directories(${CMAKE_CURRENT_BINARY_DIR})
# FILE(GLOB protofiles "${CMAKE_CURRENT_SOURCE_DIR}/../*.proto")
# message(STATUS ${Protobuf_INCLUDE_DIRS})
# message(STATUS ${Protobuf_LIBRARIES})
# protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS protofiles)
# message ("PROTO_SRCS = ${PROTO_SRCS}")
# message ("PROTO_HDRS = ${PROTO_HDRS}")
# include_directories(${PROTO_HDRS})
# set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS}
#     PROPERTIES COMPILE_FLAGS "-Wno-sign-compare")
# add_library(proto STATIC ${PROTOSRCS} ${PROTOHDRS}) 
# target_link_libraries(proto PUBLIC protobuf) 
# target_include_directories(proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

add_executable(chatclient chat_Client.cpp ../chatroom.pb.cc)
target_link_libraries(chatclient PUBLIC ${Protobuf_LIBRARIES})
# target_link_libraries(chatclient PUBLIC proto)

# find_package(absl REQUIRED)
# target_link_libraries(chatclient PUBLIC absl::absl)
file(GLOB LIB_ABSL /usr/lib/libabsl*)
target_link_libraries(chatclient PUBLIC ${LIB_ABSL})
# target_link_libraries(chatclient PUBLIC mysqlclient)
# target_link_libraries(chatclient PUBLIC hiredis)

target_include_directories(chatclient PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_BINARY_DIR}/..")
target_compile_features(chatclient PUBLIC cxx_std_17)

