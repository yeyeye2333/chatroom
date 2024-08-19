cmake_minimum_required(VERSION 3.26.5)

project(server)

add_executable(chatserver chat_Server.cpp ../chatroom.pb.cc)

find_package(Protobuf REQUIRED)
# message(STATUS ${Protobuf_INCLUDE_DIRS})
# message(STATUS ${Protobuf_LIBRARIES})
include_directories(${Protobuf_INCLUDE_DIRS})
target_link_libraries(chatserver PUBLIC ${Protobuf_LIBRARIES})

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(chatserver PUBLIC ${OPENSSL_LIBRARIES})

# find_package(absl REQUIRED)
# target_link_libraries(chatserver PUBLIC absl::absl)
file(GLOB LIB_ABSL /usr/lib/libabsl*)
target_link_libraries(chatserver PUBLIC ${LIB_ABSL})
target_link_libraries(chatserver PUBLIC mysqlclient)
# target_link_libraries(chatserver PUBLIC hiredis)

target_include_directories(chatserver PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_BINARY_DIR}/..")
target_compile_features(chatserver PUBLIC cxx_std_17)

