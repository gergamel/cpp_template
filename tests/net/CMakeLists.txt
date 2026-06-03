add_executable(udp_test
    ../TestMain.cpp
    udp_test.cpp
    ${CMAKE_SOURCE_DIR}/src/net/UdpSocket.cpp
)
target_include_directories(udp_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(udp_test PRIVATE gtest_main dsp Threads::Threads)
target_compile_options(udp_test PRIVATE ${CPP_TEMPLATE_WARNINGS})
if(NOT CMAKE_CROSSCOMPILING)
    gtest_discover_tests(udp_test)
endif()
