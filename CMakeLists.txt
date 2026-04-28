cmake_minimum_required(VERSION 3.14)
project(peep VERSION 0.1.0 LANGUAGES CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(PEEP_IS_TOP_LEVEL ON)
else()
    set(PEEP_IS_TOP_LEVEL OFF)
endif()

add_library(peep INTERFACE)
add_library(peep::peep ALIAS peep)
target_include_directories(peep INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(peep INTERFACE cxx_std_11)

install(TARGETS peep
    EXPORT peepTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT peepTargets
    FILE peepTargets.cmake
    NAMESPACE peep::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/peep
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/peepConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/peepConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/peep
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/peepConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/peepConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/peepConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/peep
)

option(PEEP_BUILD_EXAMPLES "Build example programs" ${PEEP_IS_TOP_LEVEL})

if(PEEP_BUILD_EXAMPLES)
    set(PEEP_EXAMPLE_SOURCES
        examples/01_readme_minimal.cpp
        examples/02_score_matrix.cpp
        examples/03_colormap_gallery.cpp
        examples/04_highlight_nans.cpp
        examples/05_outliers_and_infinities.cpp
        examples/06_crop_debug_chip.cpp
        examples/07_covariance_coolwarm.cpp
        examples/08_correlation_matrix.cpp
        examples/09_confusion_matrix.cpp
        examples/10_activation_heatmap.cpp
        examples/11_residuals_heatmap.cpp
        examples/12_matlab_peaks.cpp
        examples/13_gaussian_blobs.cpp
        examples/14_mandelbrot_preview.cpp
        examples/15_false_color_rgb.cpp
        examples/16_ppm_cat.cpp
        examples/17_rgb_accessor_field.cpp
        examples/18_capture_for_logs.cpp
        examples/19_column_major_layout.cpp
        examples/20_terminal_fit_modes.cpp
    )

    foreach(example_source IN LISTS PEEP_EXAMPLE_SOURCES)
        get_filename_component(example_name "${example_source}" NAME_WE)
        add_executable("${example_name}" "${example_source}")
        target_link_libraries("${example_name}" PRIVATE peep)
        if(example_name STREQUAL "16_ppm_cat")
            target_compile_definitions("${example_name}" PRIVATE
                PEEP_PPM_CAT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/cat.ppm"
            )
        endif()
    endforeach()
endif()

option(PEEP_BUILD_BENCH "Build performance benchmark" OFF)

if(PEEP_BUILD_BENCH)
    add_executable(bench benchmarks/bench.cpp)
    target_link_libraries(bench PRIVATE peep)
    target_compile_features(bench PRIVATE cxx_std_11)
endif()

option(PEEP_BUILD_TESTS "Build unit tests" OFF)
option(PEEP_FETCH_GTEST "Download GoogleTest with FetchContent when an installed package is not found" OFF)
set(PEEP_GTEST_SOURCE_DIR "" CACHE PATH "Local GoogleTest source tree to use for tests")

if(PEEP_BUILD_TESTS)
    include(CTest)
    enable_testing()

    find_package(GTest CONFIG QUIET)
    if(NOT TARGET GTest::gtest_main)
        find_package(GTest QUIET)
    endif()

    set(PEEP_GTEST_SOURCE "")
    if(PEEP_GTEST_SOURCE_DIR)
        if(EXISTS "${PEEP_GTEST_SOURCE_DIR}/CMakeLists.txt")
            set(PEEP_GTEST_SOURCE "${PEEP_GTEST_SOURCE_DIR}")
        else()
            message(FATAL_ERROR "PEEP_GTEST_SOURCE_DIR does not contain a CMakeLists.txt")
        endif()
    elseif(DEFINED FETCHCONTENT_SOURCE_DIR_GOOGLETEST
           AND EXISTS "${FETCHCONTENT_SOURCE_DIR_GOOGLETEST}/CMakeLists.txt")
        set(PEEP_GTEST_SOURCE "${FETCHCONTENT_SOURCE_DIR_GOOGLETEST}")
    elseif(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/_deps/googletest-src/CMakeLists.txt")
        set(PEEP_GTEST_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/_deps/googletest-src")
    endif()

    if(TARGET GTest::gtest_main)
        set(PEEP_GTEST_MAIN_TARGET GTest::gtest_main)
    elseif(TARGET GTest::Main)
        set(PEEP_GTEST_MAIN_TARGET GTest::Main)
    elseif(PEEP_GTEST_SOURCE)
        # Prevent GoogleTest from overriding our compiler/linker options
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        # Keep peep installs focused on peep, even from test-enabled builds.
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        add_subdirectory(
            "${PEEP_GTEST_SOURCE}"
            "${CMAKE_CURRENT_BINARY_DIR}/_deps/googletest-build"
            EXCLUDE_FROM_ALL
        )
        if(TARGET GTest::gtest_main)
            set(PEEP_GTEST_MAIN_TARGET GTest::gtest_main)
        elseif(TARGET gtest_main)
            set(PEEP_GTEST_MAIN_TARGET gtest_main)
        else()
            message(FATAL_ERROR "GoogleTest source tree did not provide a gtest_main target")
        endif()
    elseif(PEEP_FETCH_GTEST)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        v1.14.0
        )
        # Prevent GoogleTest from overriding our compiler/linker options
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        # Keep peep installs focused on peep, even from test-enabled builds.
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
        set(PEEP_GTEST_MAIN_TARGET GTest::gtest_main)
    else()
        message(FATAL_ERROR
            "GoogleTest was not found. Install GoogleTest, configure "
            "GTest_DIR, or re-run with -DPEEP_FETCH_GTEST=ON to allow "
            "CMake to download it with FetchContent."
        )
    endif()

    add_executable(peep_test tests/peep_test.cpp)
    target_link_libraries(peep_test PRIVATE peep ${PEEP_GTEST_MAIN_TARGET})
    target_compile_features(peep_test PRIVATE cxx_std_11)

    include(GoogleTest)
    gtest_discover_tests(peep_test)
endif()
