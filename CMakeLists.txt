PROJECT(himedia)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#SET(CPU_TYPE	"hi3518e")
INCLUDE(${CMAKE_HOME_DIRECTORY}/config/toolchain-cross-hi3518e.cmake)

#-Wall 
SET(CMAKE_C_FLAGS "-O2 -pthread -ffunction-sections -lm -fPIC")







####################################################################

SET (NL_DIRS ${CMAKE_HOME_DIRECTORY}/nl/netlink/libnl-3.5.0)
INCLUDE_DIRECTORIES(${NL_DIRS}/include ${NL_DIRS}/)
LINK_DIRECTORIES(${NL_DIRS}/lib/.libs)


SET(NL_TEST_LIST "nl/test/nl-test.c")
FILE(GLOB_RECURSE NL_CLI_LIST "nl/test/lib/*.c")

ADD_EXECUTABLE(nl-test ${NL_TEST_LIST} ${NL_CLI_LIST})
TARGET_LINK_LIBRARIES(nl-test ${NL_DIRS}/lib/.libs/libnl-3.a 
								${NL_DIRS}/lib/.libs/libnl-route-3.a
								${NL_DIRS}/lib/.libs/libnl-nf-3.a
								)


