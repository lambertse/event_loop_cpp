cmake_minimum_required(VERSION 3.16)
project(event_loop_cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(ENABLE_TESTING OFF)

# ============================================================================
# Platform Detection
# ============================================================================
if(APPLE)
    set(PLATFORM "osx")
    message(STATUS "Building for macOS")
elseif(UNIX)
    set(PLATFORM "linux")
    message(STATUS "Building for Linux")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# ============================================================================
# Build Configuration
# ============================================================================
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building in Debug mode")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Building in Release mode")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")
endif()

# ============================================================================
# Dependencies
# ============================================================================
include(FetchContent)

# Protobuf dependency
FetchContent_Declare(
    protobuf
    GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
    GIT_TAG        v21.12
    GIT_SUBMODULES_RECURSE ON
)

# Configure protobuf options
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_PROTOC_BINARIES ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(protobuf)

# ============================================================================
# Protobuf Code Generation
# ============================================================================
file(GLOB PROTO_FILES "proto/*.proto")
set(GENERATED_PROTO_SRCS)
set(GENERATED_PROTO_HDRS)

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    set(GENERATED_SRC ${CMAKE_CURRENT_SOURCE_DIR}/include/event_loop/proto/${PROTO_NAME}.pb.cc)
    set(GENERATED_HDR ${CMAKE_CURRENT_SOURCE_DIR}/include/event_loop/proto/${PROTO_NAME}.pb.h)

    add_custom_command(
        OUTPUT ${GENERATED_SRC} ${GENERATED_HDR}
        COMMAND protobuf::protoc
        ARGS --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto
             --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/include/event_loop/proto
             ${PROTO_FILE}
        DEPENDS ${PROTO_FILE} protobuf::protoc
        COMMENT "Generating C++ code from ${PROTO_FILE}"
        VERBATIM
    )
    
    list(APPEND GENERATED_PROTO_SRCS ${GENERATED_SRC})
    list(APPEND GENERATED_PROTO_HDRS ${GENERATED_HDR})
endforeach()

# Create a library for generated protobuf sources
add_library(proto_generated STATIC ${GENERATED_PROTO_SRCS})
target_include_directories(proto_generated PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/proto
    ${protobuf_SOURCE_DIR}/src
)
target_link_libraries(proto_generated PUBLIC protobuf::libprotobuf)

# ============================================================================
# Server Executable
# ============================================================================

file(GLOB PROTO_SRCS "include/event_loop/proto/*.cc")
file(GLOB SHARED_UTILS_SRC "src/shared/*.cpp")
file(GLOB SERVER_SRC "src/server/*.cpp")
file(GLOB SERVER_PLATFORM_SRC "src/server/${PLATFORM}/*.cpp")

add_executable(server_${PROJECT_NAME} 
    ${PROTO_SRCS}
    ${SHARED_UTILS_SRC}
    ${SERVER_SRC}
    ${SERVER_PLATFORM_SRC}
)

target_include_directories(server_${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(server_${PROJECT_NAME} PRIVATE
    protobuf::libprotobuf
)

# ============================================================================
# Client Executable
# ============================================================================
set(CLIENT_SOURCES
    src/client/client_main.cpp
)

add_executable(client_${PROJECT_NAME}
    ${CLIENT_SOURCES}
    ${PROTO_SRCS}
    ${SHARED_UTILS_SRC})

target_include_directories(client_${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
)

target_link_libraries(client_${PROJECT_NAME} PRIVATE
    protobuf::libprotobuf
)

# ============================================================================
# Testing (Optional)
# ============================================================================
if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
