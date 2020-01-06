SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)
SET(TOOLCHAIN_DIR /opt/hisi-linux/x86-arm/arm-hisiv300-linux/target/bin) #+++

SET(CMAKE_C_COMPILER   ${TOOLCHAIN_DIR}/arm-hisiv300-linux-gcc)
SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/arm-hisiv300-linux-g++)

SET(CMAKE_FIND_ROOT_PATH  ${TOOLCHAIN_DIR})
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)


SET(MPP_PATH	/home/lroyd/haisi/Hi3518E_SDK_V1.0.3.0/mpp)	#+++
SET(COMMON_DIR  ${MPP_PATH}/sample/common)
FILE(GLOB_RECURSE CHIP_LIST ${MPP_PATH}/sample/common/*.c)

SET(CHIP_INC_DIR ${COMMON_DIR} ${MPP_PATH}/include ${MPP_PATH}/component/acodec)

#SET(CHIP_CFLAGS "-DCHIP_ID=CHIP_HI3518E_V200 -DHI_ACODEC_TYPE_INNER -DSENSOR_TYPE=APTINA_AR0130_DC_720P_30FPS")
SET(CHIP_CFLAGS	"-DCHIP_ID=CHIP_HI3518E_V200 -DHI_ACODEC_TYPE_INNER -DSENSOR_TYPE=OMNIVISION_OV9732_DC_720P_30FPS")

SET(LIB_AUDIO	"-lVoiceEngine -lupvqe -ldnvqe ")
#SET(LIB_SENSOR	"-lisp -l_iniparser -l_hiae -l_hiawb -l_hiaf -l_hidefog -lsns_ar0130")
SET(LIB_SENSOR	"-lisp -l_iniparser -l_hiae -l_hiawb -l_hiaf -l_hidefog -lsns_ov9732")
SET(LIB_MPI		"-lmpi -live -lmd")

SET(CHIP_LDFLAGS "${LIB_AUDIO} ${LIB_SENSOR} ${LIB_MPI}")

SET(CHIP_LIBS_DIR ${MPP_PATH}/lib)
