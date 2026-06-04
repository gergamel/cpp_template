add_executable(dsp_test
    ${CMAKE_SOURCE_DIR}/src/dsp/Filters.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/Benchmark.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/PerfRunner.cpp
    FiltersTest.cpp
    PerformanceTest.cpp
)
target_include_directories(dsp_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dsp_test PRIVATE
    "gtest"
    "gtest_main"
    "gmock"
    "fmt::fmt"
    "Microsoft.GSL::GSL"
    Threads::Threads
)
target_compile_options(dsp_test PRIVATE ${CPP_TEMPLATE_WARNINGS})
if(NOT CMAKE_CROSSCOMPILING)
    gtest_discover_tests(dsp_test)
endif()