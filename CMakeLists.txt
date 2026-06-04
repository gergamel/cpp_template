cmake_minimum_required(VERSION 3.24)
project(cpp_template VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(cmake/Options.cmake)
include(cmake/Dependencies.cmake)

function(enable_debug_symbols TARGET_NAME)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${TARGET_NAME} PRIVATE -g)
        target_link_options(${TARGET_NAME} PRIVATE -g)
    elseif (MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /Zi)
        target_link_options(${TARGET_NAME} PRIVATE /DEBUG)
    endif()
endfunction()

if (MSVC)
    # warning level 4
    add_compile_options(/W4)
endif()

# add_library(dsp STATIC
#     src/dsp/Filters.cpp
#     src/dsp/Benchmark.cpp
# )
# target_include_directories(dsp PUBLIC include)
# target_compile_features(dsp PUBLIC cxx_std_23)
# target_compile_options(dsp PRIVATE ${CPP_TEMPLATE_WARNINGS})
# add_executable(cpp_template_app src/main.cpp)
# target_link_libraries(cpp_template_app PRIVATE dsp)

# option(BUILD_PERFORMANCE_RUNNER "Build the performance runner application" ON)
# if(BUILD_PERFORMANCE_RUNNER)
#     add_executable(dsp_perf src/dsp/PerfRunner.cpp)
#     target_link_libraries(dsp_perf PRIVATE dsp)
#     target_compile_options(dsp_perf PRIVATE ${CPP_TEMPLATE_WARNINGS})
# endif()

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Generic")
    enable_testing()
    add_subdirectory(tests)
else()
    message(STATUS "Skipping host-based tests for bare-metal targets")
endif()
