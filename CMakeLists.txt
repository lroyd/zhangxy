PROJECT(himedia)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(CPU_TYPE	"hi3518e")
INCLUDE(${CMAKE_HOME_DIRECTORY}/config/toolchain-cross-hi3518e.cmake)

#-Wall 
SET(CMAKE_C_FLAGS "-O2 -pthread -ffunction-sections -lm -fPIC -lz -lssl -lcrypto")


SET(CMAKE_CXX_FLAGS " -lm -fPIC -pthread")

FILE(GLOB_RECURSE HEADER_LIST "*.h")


###################################################################
#第三方库
SET (RTSP_DIRS ${CMAKE_HOME_DIRECTORY}/rtsp_lib)
SET (RTMP_DIRS ${CMAKE_HOME_DIRECTORY}/rtmp_lib)
SET (RTMP_CFLAGS "")




SET (INCLUDE_DIRS "")

FOREACH(_HEADERFILE ${HEADER_LIST})
    GET_FILENAME_COMPONENT(_DIR ${_HEADERFILE} PATH)
    LIST(APPEND INCLUDE_DIRS ${_DIR})
ENDFOREACH()

LIST(REMOVE_DUPLICATES INCLUDE_DIRS)

INCLUDE_DIRECTORIES(${INCLUDE_DIRS} ${RTSP_DIRS})





###################################################################


###################################################################
FILE(GLOB_RECURSE IOQ_LIST "ioqueue/src/*.c")
SET (IOQ_TEST_LIST  "ioqueue/test/test.c")
#ADD_EXECUTABLE(ioqueue ${IOQ_TEST_LIST} ${IOQ_LIST} ${TIMS_LIST})


###################################################################
FILE(GLOB_RECURSE OS_LIST "oslib/src/*.c")
FILE(GLOB_RECURSE OS_TEST_LIST "oslib/test/*.c")
#ADD_EXECUTABLE(os_test ${OS_TEST_LIST} ${OS_LIST})

###################################################################
FILE(GLOB_RECURSE PORT_LIST "port/src/*.c")
SET (PORT_TEST_LIST  "port/test/port.c")
#ADD_EXECUTABLE(port ${PORT_TEST_LIST} ${PORT_LIST} ${OS_LIST})


###################################################################
SET (RTC_TEST_LIST  "rtc/test/rtc_time.c")
#ADD_EXECUTABLE(rtc ${RTC_TEST_LIST} ${OS_LIST})




###################################################################
FILE(GLOB_RECURSE INI_SRC_LIST "iniparse/*.c")

INCLUDE_DIRECTORIES(${CHIP_INC_DIR})
LINK_DIRECTORIES(${CHIP_LIBS_DIR})

FILE(GLOB_RECURSE AMIO_LIST "amio/src/*.c")
FILE(GLOB_RECURSE AMIO_DEV_LIST "amio/src/dev/*.c")
FILE(GLOB_RECURSE AMIO_TEST_LIST "amio/test/*.c")

SET(CHIP_LD_FLAGS "-Wl,-gc-sections")

ADD_EXECUTABLE(amio ${AMIO_TEST_LIST} ${AMIO_LIST} ${AMIO_DEV_LIST} ${OS_LIST} ${INI_SRC_LIST} ${CHIP_LIST})
SET_TARGET_PROPERTIES(amio PROPERTIES COMPILE_FLAGS "${CHIP_CFLAGS} ${HI_CC_TYPE}" LINK_FLAGS "${CHIP_LDFLAGS} ${CHIP_LD_FLAGS}")


TARGET_LINK_LIBRARIES(amio ${RTSP_DIRS}/librtsp.a ${RTMP_DIRS}/librtmp.a)


