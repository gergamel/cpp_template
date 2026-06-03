add_executable(popen_test
    ../TestMain.cpp
    popen_test.cpp
    ${CMAKE_SOURCE_DIR}/src/process/Popen.cpp
)
target_include_directories(popen_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(popen_test PRIVATE gtest_main dsp Threads::Threads)
target_compile_options(popen_test PRIVATE ${CPP_TEMPLATE_WARNINGS})
if(NOT CMAKE_CROSSCOMPILING)
    gtest_discover_tests(popen_test)
endif()
