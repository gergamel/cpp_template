add_executable(dsp_test
    ../TestMain.cpp
    FiltersTest.cpp
    PerformanceTest.cpp
)
target_include_directories(dsp_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dsp_test PRIVATE gtest_main dsp Threads::Threads)
target_compile_options(dsp_test PRIVATE ${CPP_TEMPLATE_WARNINGS})
gtest_discover_tests(dsp_test)