cmake_minimum_required(VERSION 3.15)
project(DistributedKeyValueStore VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

set(PROTO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(PROTO_FILE "${PROTO_PATH}/kvstore.proto")
set(GENERATED_PROTOBUF_PATH "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_PROTOBUF_PATH})

set(PROTO_SRCS "${GENERATED_PROTOBUF_PATH}/kvstore.pb.cc")
set(PROTO_HDRS "${GENERATED_PROTOBUF_PATH}/kvstore.pb.h")
set(GRPC_SRCS "${GENERATED_PROTOBUF_PATH}/kvstore.grpc.pb.cc")
set(GRPC_HDRS "${GENERATED_PROTOBUF_PATH}/kvstore.grpc.pb.h")

get_target_property(PROTOC_EXECUTABLE protobuf::protoc LOCATION)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

add_custom_command(
    OUTPUT ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS}
    COMMAND ${PROTOC_EXECUTABLE}
    ARGS --grpc_out=${GENERATED_PROTOBUF_PATH}
         --cpp_out=${GENERATED_PROTOBUF_PATH}
         --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
         -I${PROTO_PATH}
         ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating protobuf and gRPC files"
)

add_library(proto_lib
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

target_link_libraries(proto_lib
    protobuf::libprotobuf
    gRPC::grpc++
)

target_include_directories(proto_lib PUBLIC
    ${GENERATED_PROTOBUF_PATH}
)

add_library(replication
    src/replication/replication_manager.cpp
    src/replication/replication_manager.h
)

target_link_libraries(replication
    proto_lib
    gRPC::grpc++
)

target_include_directories(replication PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GENERATED_PROTOBUF_PATH}
)

add_library(persistence
    src/persistence/aof_persistence.cpp
    src/persistence/aof_persistence.h
    src/persistence/rdb_persistence.cpp
    src/persistence/rdb_persistence.h
)

target_include_directories(persistence PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_library(sharding
    src/sharding/hash_ring.cpp
    src/sharding/hash_ring.h
    src/sharding/shard_info.h
    src/sharding/shard_router.cpp
    src/sharding/shard_router.h
)

target_link_libraries(sharding
    proto_lib
    gRPC::grpc++
)

target_include_directories(sharding PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GENERATED_PROTOBUF_PATH}
)

add_library(storage
    src/storage/storage.cpp
    src/storage/storage.h
)

target_link_libraries(storage
    persistence
    replication
)

target_include_directories(storage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_library(service
    src/service/kvstore_service.cpp
    src/service/kvstore_service.h
)

target_link_libraries(service
    storage
    persistence
    replication
    proto_lib
    gRPC::grpc++
)

target_include_directories(service PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GENERATED_PROTOBUF_PATH}
)

add_library(server_lib
    src/server/server.cpp
    src/server/server.h
)

target_link_libraries(server_lib
    service
    storage
    persistence
    replication
    proto_lib
    gRPC::grpc++
)

target_include_directories(server_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GENERATED_PROTOBUF_PATH}
)

add_executable(kvstore_server
    src/main.cpp
)

target_link_libraries(kvstore_server
    server_lib
    service
    storage
    persistence
    replication
    proto_lib
    gRPC::grpc++
    Threads::Threads
)

target_include_directories(kvstore_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GENERATED_PROTOBUF_PATH}
)

# Test client executables used by test scripts
add_executable(kvstore_client tests/client_test.cpp)
target_link_libraries(kvstore_client proto_lib gRPC::grpc++ Threads::Threads)
target_include_directories(kvstore_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${GENERATED_PROTOBUF_PATH})

add_executable(snapshot_test tests/snapshot_test.cpp)
target_link_libraries(snapshot_test proto_lib gRPC::grpc++ Threads::Threads)
target_include_directories(snapshot_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${GENERATED_PROTOBUF_PATH})

add_executable(verify_persistence tests/verify_persistence.cpp)
target_link_libraries(verify_persistence proto_lib gRPC::grpc++ Threads::Threads)
target_include_directories(verify_persistence PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${GENERATED_PROTOBUF_PATH})

add_executable(read_test tests/read_test.cpp)
target_link_libraries(read_test proto_lib gRPC::grpc++ Threads::Threads)
target_include_directories(read_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${GENERATED_PROTOBUF_PATH})

add_executable(test_hash_ring tests/test_hash_ring.cpp)
target_link_libraries(test_hash_ring sharding)
target_include_directories(test_hash_ring PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(test_shard_router tests/test_shard_router.cpp)
target_link_libraries(test_shard_router sharding proto_lib gRPC::grpc++ Threads::Threads)
target_include_directories(test_shard_router PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${GENERATED_PROTOBUF_PATH})

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
