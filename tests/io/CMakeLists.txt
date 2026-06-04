add_executable(file_test file_test.cpp)
target_compile_definitions(file_test PRIVATE
    BOOST_NO_EXCEPTIONS 
)
target_include_directories(file_test PRIVATE
    .
    ${CMAKE_SOURCE_DIR}/include
)
target_compile_options(file_test PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,GNU>:${COMMON_CXX_OPTS}>
    $<$<COMPILE_LANG_AND_ID:C,GNU>:${COMMON_C_OPTS}>
)
target_link_libraries(file_test PRIVATE
    "gtest"
    "gtest_main"
    "gmock"
    "fmt::fmt"
    "Microsoft.GSL::GSL"
)
add_test(NAME file_test COMMAND file_test)
# enable_coverage(file_test)

# add_executable(pcap_test pcap_test.cpp)
# if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#     target_compile_definitions(pcap_test PRIVATE 
#         "-D__cpp_concepts=202002L"
#     )
# endif()
# target_include_directories(pcap_test PRIVATE
#     .
#     ${CMAKE_SOURCE_DIR}/inc
# )
# target_link_libraries(pcap_test PRIVATE 
#     "gtest"
#     "gtest_main"
#     "gmock"
#     "fmt::fmt"
#     "Microsoft.GSL::GSL"
#     "PkgConfig::PCAP"
# )
# add_test(NAME pcap_test COMMAND pcap_test)
