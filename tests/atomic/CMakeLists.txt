add_executable(atomic_test
    atomic_test.cpp
)
target_include_directories(atomic_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(atomic_test PRIVATE
    "gtest"
    "gtest_main"
    "Threads::Threads"
    "fmt::fmt"
    "Microsoft.GSL::GSL"
)
target_compile_options(atomic_test PRIVATE ${CPP_TEMPLATE_WARNINGS})
if(NOT CMAKE_CROSSCOMPILING)
    gtest_discover_tests(atomic_test)
endif()
