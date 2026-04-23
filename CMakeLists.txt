cmake_minimum_required(VERSION 3.14)
project(termimage LANGUAGES CXX)

# --- Header-only library target ----------------------------------------------
add_library(termimage INTERFACE)
target_include_directories(termimage INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(termimage INTERFACE cxx_std_11)

# --- Examples ----------------------------------------------------------------
option(TERMIMAGE_BUILD_EXAMPLES "Build example programs" ON)

if(TERMIMAGE_BUILD_EXAMPLES)
    add_executable(demo examples/demo.cpp)
    target_link_libraries(demo PRIVATE termimage)
endif()

# --- Tests -------------------------------------------------------------------
option(TERMIMAGE_BUILD_TESTS "Build unit tests" ON)

if(TERMIMAGE_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    # Prevent GoogleTest from overriding our compiler/linker options
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(termimage_test tests/termimage_test.cpp)
    target_link_libraries(termimage_test PRIVATE termimage GTest::gtest_main)
    target_compile_features(termimage_test PRIVATE cxx_std_11)

    include(GoogleTest)
    gtest_discover_tests(termimage_test)
endif()
