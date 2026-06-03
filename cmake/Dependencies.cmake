# External dependency declarations. Tests may opt in to FetchContent when enabled.
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# When tests are enabled, the tests/CMakeLists.txt will bring gtest into the build.
