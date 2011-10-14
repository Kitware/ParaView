
project(gs)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)


add_definitions(-DMPISRC -DSAFE)

#GS = bit_mask bss_malloc ivec error comm gs queue stack # ex

set(gslib_sources
  src/bit_mask.c
  src/bss_malloc.c
  src/comm.c
  src/error.c
  #src/ex.c
  src/gs.c
  src/ivec.c
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
