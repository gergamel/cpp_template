# Tests are only enabled for host-like targets.
find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

enable_testing()

add_subdirectory(atomic)
add_subdirectory(dsp)
add_subdirectory(net)
add_subdirectory(process)

add_executable(args_test
    TestMain.cpp
    args_test.cpp
)
target_include_directories(args_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(args_test PRIVATE gtest_main dsp Threads::Threads)
target_compile_options(args_test PRIVATE ${CPP_TEMPLATE_WARNINGS})
gtest_discover_tests(args_test)

add_executable(thread_test
    TestMain.cpp
    thread_test.cpp
)
target_include_directories(thread_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(thread_test PRIVATE gtest_main dsp Threads::Threads)
target_compile_options(thread_test PRIVATE ${CPP_TEMPLATE_WARNINGS})
gtest_discover_tests(thread_test)

