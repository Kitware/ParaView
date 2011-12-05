
project(gs)

include_directories(
  ${gs_SOURCE_DIR}/include
  ${MPI_INCLUDE_PATH}
  )

if(NOT WIN32)
# need some try compiles
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -ffast-math -funroll-loops -fstrict-aliasing -w")
endif()

add_definitions(-Dr8 -DMPISRC -DSAFE)

#GS = bit_mask bss_malloc ivec error comm gs queue stack # ex

set(gslib_sources
  src/bit_mask.c
  src/bss_malloc.c
  src/ivec.c
  src/error.c
  src/comm.c
  src/gs.c
  src/queue.c
  src/stack.c
  )

set(gslib_headers
  include/bit_mask.h
  include/bss_malloc.h
  include/comm.h
  include/const.h
  include/error.h
  include/gs.h
  include/ivec.h
  include/queue.h
  include/stack.h
  include/types.h
  )

add_library(gs STATIC ${gslib_sources} ${gslib_headers})
