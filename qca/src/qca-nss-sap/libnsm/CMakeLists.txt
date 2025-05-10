project(qca-nsm-lib)
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

add_library(nsm SHARED sal_nsm.c)
set(LIBS nl-3 nl-genl-3)
target_link_libraries(nsm ${LIBS} ${LIBNL_INCLUDE_DIR})

install(TARGETS nsm LIBRARY DESTINATION /usr/lib)
