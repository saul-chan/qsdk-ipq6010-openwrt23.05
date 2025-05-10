project(salqostest)
cmake_minimum_required(VERSION 2.8.9)
set(CMAKE_VERBOSE_MAKEFILE ON)

LINK_DIRECTORIES("-I${STAGING_DIR}/usr/lib")
add_executable(salqostest sal_qos_test.c)
target_link_libraries(salqostest salqos)
install(TARGETS salqostest RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
