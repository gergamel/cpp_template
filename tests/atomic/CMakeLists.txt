add_executable(atomic_test
    ../TestMain.cpp
    atomic_test.cpp
)
target_include_directories(atomic_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(atomic_test PRIVATE gtest_main dsp Threads::Threads)
target_compile_options(atomic_test PRIVATE ${CPP_TEMPLATE_WARNINGS})
gtest_discover_tests(atomic_test)
