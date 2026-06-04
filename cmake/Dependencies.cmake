# External dependency declarations. Tests may opt in to FetchContent when enabled.
include(FetchContent)

#####################################################################
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# Required on MSVC so gtest uses /MD (shared CRT) matching the project default.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# When tests are enabled, the tests/CMakeLists.txt will bring gtest into the build.
#####################################################################
FetchContent_Declare(
    GSL
    GIT_REPOSITORY "https://github.com/microsoft/GSL"
    GIT_TAG        "v4.2.0"
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(GSL)
#####################################################################
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        407c905
)
set(FMT_LOCALE OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(fmt)
set(LIBFMT_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/build/_deps/fmt-src/include" CACHE PATH "")
if (MSVC)
target_compile_definitions(fmt PRIVATE 
    _WIN32
)
else()
# target_compile_definitions(fmt PRIVATE
#     FMT_MODULE
# )
target_compile_options(fmt PRIVATE 
    -Wno-error
    -Wno-conversion
    -Wno-nan-infinity-disabled
)
# target_link_options(fmt PRIVATE -std=c++23-stdlib=libc++)
endif()
#####################################################################
# 1. Configure Benchmark options (must be set before declaring/populating)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.9.5
)
FetchContent_MakeAvailable(benchmark)
if (MSVC)
    target_compile_definitions(benchmark PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
else()
    target_compile_options(benchmark PRIVATE 
        -Wno-error
        -Wno-null-dereference
        -Wno-psabi # Suppress warning for "parameter passing for argument of type ... changed in GCC 7.1"
    )
endif()
#####################################################################