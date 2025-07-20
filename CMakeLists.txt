project("event-loop")
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_C_COMPILER clang++)

include_directories(.)

add_executable(server_${PROJECT_NAME}
    server/server.cpp
    server/server.h
    server/server_define.h
    server/server_main.cpp
    shared/utils.h
)

add_executable(client_${PROJECT_NAME}
    client/client_main.cpp
    shared/utils.h
)
