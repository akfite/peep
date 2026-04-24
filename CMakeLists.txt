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
option(TERMIMAGE_FETCH_GTEST "Download GoogleTest with FetchContent when an installed package is not found" OFF)
set(TERMIMAGE_GTEST_SOURCE_DIR "" CACHE PATH "Local GoogleTest source tree to use for tests")

if(TERMIMAGE_BUILD_TESTS)
    include(CTest)
    enable_testing()

    find_package(GTest CONFIG QUIET)
    if(NOT TARGET GTest::gtest_main)
        find_package(GTest QUIET)
    endif()

    set(TERMIMAGE_GTEST_SOURCE "")
    if(TERMIMAGE_GTEST_SOURCE_DIR)
        if(EXISTS "${TERMIMAGE_GTEST_SOURCE_DIR}/CMakeLists.txt")
            set(TERMIMAGE_GTEST_SOURCE "${TERMIMAGE_GTEST_SOURCE_DIR}")
        else()
            message(FATAL_ERROR "TERMIMAGE_GTEST_SOURCE_DIR does not contain a CMakeLists.txt")
        endif()
    elseif(DEFINED FETCHCONTENT_SOURCE_DIR_GOOGLETEST
           AND EXISTS "${FETCHCONTENT_SOURCE_DIR_GOOGLETEST}/CMakeLists.txt")
        set(TERMIMAGE_GTEST_SOURCE "${FETCHCONTENT_SOURCE_DIR_GOOGLETEST}")
    elseif(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/_deps/googletest-src/CMakeLists.txt")
        set(TERMIMAGE_GTEST_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/_deps/googletest-src")
    endif()

    if(TARGET GTest::gtest_main)
        set(TERMIMAGE_GTEST_MAIN_TARGET GTest::gtest_main)
    elseif(TARGET GTest::Main)
        set(TERMIMAGE_GTEST_MAIN_TARGET GTest::Main)
    elseif(TERMIMAGE_GTEST_SOURCE)
        # Prevent GoogleTest from overriding our compiler/linker options
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        # Keep termimage installs focused on termimage, even from test-enabled builds.
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        add_subdirectory(
            "${TERMIMAGE_GTEST_SOURCE}"
            "${CMAKE_CURRENT_BINARY_DIR}/_deps/googletest-build"
            EXCLUDE_FROM_ALL
        )
        if(TARGET GTest::gtest_main)
            set(TERMIMAGE_GTEST_MAIN_TARGET GTest::gtest_main)
        elseif(TARGET gtest_main)
            set(TERMIMAGE_GTEST_MAIN_TARGET gtest_main)
        else()
            message(FATAL_ERROR "GoogleTest source tree did not provide a gtest_main target")
        endif()
    elseif(TERMIMAGE_FETCH_GTEST)
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
        set(TERMIMAGE_GTEST_MAIN_TARGET GTest::gtest_main)
    else()
        message(FATAL_ERROR
            "GoogleTest was not found. Install GoogleTest, configure "
            "GTest_DIR, or re-run with -DTERMIMAGE_FETCH_GTEST=ON to allow "
            "CMake to download it with FetchContent."
        )
    endif()

    add_executable(termimage_test tests/termimage_test.cpp)
    target_link_libraries(termimage_test PRIVATE termimage ${TERMIMAGE_GTEST_MAIN_TARGET})
    target_compile_features(termimage_test PRIVATE cxx_std_11)

    include(GoogleTest)
    gtest_discover_tests(termimage_test)
endif()
