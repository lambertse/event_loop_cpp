project("event-loop")
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(.)
if(APPLE)
    set(PLATFORM "macOS")
    MESSAGE(STATUS "Building for macOS")
elseif(UNIX)
    set(PLATFORM "linux")
    MESSAGE(STATUS "Building for Linux")
endif()
#Add some debugging flags

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    MESSAGE(STATUS "Building in Debug mode")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

add_executable(server_${PROJECT_NAME}
    server/server.cpp
    server/server.h
    server/server_impl.h
    server/${PLATFORM}/server_impl.cpp
    server/server_define.h
    server/server_main.cpp
    shared/utils.h
)

add_executable(client_${PROJECT_NAME}
    client/client_main.cpp
    shared/utils.h
)
