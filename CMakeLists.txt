cmake_minimum_required(VERSION 3.1)
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
