project(salqos)
cmake_minimum_required(VERSION 2.8.9)
set(CMAKE_VERBOSE_MAKEFILE ON)
LINK_DIRECTORIES("-I${STAGING_DIR}/usr/lib")

if (UNIX)
    SET(FEATURES "-D_GNU_SOURCE")
endif()

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 ${WARNFLGS} ${COMMONCFLGS} ${FEATURES}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98 ${WARNFLGS} ${COMMONCFLGS}")
add_library(salqos SHARED sal_qos.c)
install(TARGETS salqos LIBRARY DESTINATION /usr/lib)
