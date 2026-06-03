cmake_minimum_required(VERSION 3.24)
project(cpp_template VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/Options.cmake)
include(cmake/Dependencies.cmake)

add_library(dsp STATIC
    src/dsp/Filters.cpp
    src/dsp/Benchmark.cpp
)
target_include_directories(dsp PUBLIC include)
target_compile_features(dsp PUBLIC cxx_std_23)
target_compile_options(dsp PRIVATE ${CPP_TEMPLATE_WARNINGS})

add_executable(cpp_template_app src/main.cpp)
target_link_libraries(cpp_template_app PRIVATE dsp)

option(BUILD_PERFORMANCE_RUNNER "Build the performance runner application" ON)
if(BUILD_PERFORMANCE_RUNNER)
    add_executable(dsp_perf src/dsp/PerfRunner.cpp)
    target_link_libraries(dsp_perf PRIVATE dsp)
    target_compile_options(dsp_perf PRIVATE ${CPP_TEMPLATE_WARNINGS})
endif()

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Generic")
    enable_testing()
    add_subdirectory(tests)
else()
    message(STATUS "Skipping host-based tests for bare-metal targets")
endif()
