PROJECT(spcth C)

SET(srcs
  spcth_interface.c
  spcth_interface.h
  spy_file.c
  spy_file.h
  stm_types.h
  structured_mesh.h
  )

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/spcthConfig.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/spcthConfig.h")
INCLUDE_DIRECTORIES(
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_BINARY_DIR}")

SET(SPCTHSTATIC)
IF(NOT BUILD_SHARED_LIBS)
  SET(SPCTHSTATIC 1)
ENDIF(NOT BUILD_SHARED_LIBS)

ADD_LIBRARY(SPCTH ${srcs})
