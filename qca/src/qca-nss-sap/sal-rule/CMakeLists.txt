project(salrule)
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
set(LIBS  nl-3 nl-genl-3)

add_library(salrule SHARED sal_rule.c)
target_link_libraries(salrule ${LIBS} ${LIBNL_INCLUDE_DIR})
install(TARGETS salrule LIBRARY DESTINATION /usr/lib)
