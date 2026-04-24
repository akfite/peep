cmake_minimum_required(VERSION 3.14)
project(termimage VERSION 0.1.0 LANGUAGES CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(TERMIMAGE_IS_TOP_LEVEL ON)
else()
    set(TERMIMAGE_IS_TOP_LEVEL OFF)
endif()

add_library(termimage INTERFACE)
add_library(termimage::termimage ALIAS termimage)
target_include_directories(termimage INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(termimage INTERFACE cxx_std_11)

install(TARGETS termimage
    EXPORT termimageTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT termimageTargets
    FILE termimageTargets.cmake
    NAMESPACE termimage::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/termimage
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/termimageConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/termimageConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/termimage
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/termimageConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/termimageConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/termimageConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/termimage
)

option(TERMIMAGE_BUILD_EXAMPLES "Build example programs" ${TERMIMAGE_IS_TOP_LEVEL})

if(TERMIMAGE_BUILD_EXAMPLES)
    add_executable(demo examples/demo.cpp)
    target_link_libraries(demo PRIVATE termimage)
endif()

option(TERMIMAGE_BUILD_BENCH "Build performance benchmark" OFF)

if(TERMIMAGE_BUILD_BENCH)
    add_executable(bench benchmarks/bench.cpp)
    target_link_libraries(bench PRIVATE termimage)
    target_compile_features(bench PRIVATE cxx_std_11)
    if(MSVC)
        target_compile_options(bench PRIVATE /O2)
    else()
        target_compile_options(bench PRIVATE -O2)
    endif()
endif()

option(TERMIMAGE_BUILD_TESTS "Build unit tests" OFF)

if(TERMIMAGE_BUILD_TESTS)
    include(CTest)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    # Prevent GoogleTest from overriding our compiler/linker options
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    # Keep termimage installs focused on termimage, even from test-enabled builds.
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(termimage_test tests/termimage_test.cpp)
    target_link_libraries(termimage_test PRIVATE termimage GTest::gtest_main)
    target_compile_features(termimage_test PRIVATE cxx_std_11)

    include(GoogleTest)
    gtest_discover_tests(termimage_test)
endif()
