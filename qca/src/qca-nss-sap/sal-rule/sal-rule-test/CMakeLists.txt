project(salruletest)
cmake_minimum_required(VERSION 2.8.9)
set(CMAKE_VERBOSE_MAKEFILE ON)

find_library (LIBNL_LIBRARY nl-3)
find_path (LIBNL_INCLUDE_DIR
  NAMES
  netlink/netlink.h
  PATH_SUFFIXES
  libnl3
)

INCLUDE_DIRECTORIES(
        ${LIBNL_INCLUDE_DIR}
)

LINK_DIRECTORIES("-I${STAGING_DIR}/usr/lib")
add_executable(salruletest sal_rule_test.c)
set(LIBS nl-3 nl-genl-3)
target_link_libraries(salruletest ${LIBS} ${LIBNL_INCLUDE_DIR} salrule)
install(TARGETS salruletest RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
