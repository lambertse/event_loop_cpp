# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${Protobuf_INCLUDE_DIRS})

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Configure googletest options
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)


# Create test executable
add_executable(protobuf_quicktest
    protobuf_quicktest.cpp
)

# Link libraries
target_link_libraries(protobuf_quicktest
    GTest::gtest
    GTest::gtest_main
    proto_generated
    ${Protobuf_LIBRARIES}
)

# Add test to CTest

# Ensure proper C++ standard
target_compile_features(protobuf_quicktest PRIVATE cxx_std_14)
