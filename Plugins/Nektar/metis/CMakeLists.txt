
project(metis)

include_directories(
  ${metis_SOURCE_DIR}
  )

if(WIN32)
  add_definitions(-D_USE_MATH_DEFINES)
else()
# need some try compiles
  set(CMAKE_C_FLAGS "-fPIC -O2 -ansi -w")
endif()


#OBJS = coarsen.o fm.o initpart.o match.o ccgraph.o memory.o \
#       pmetis.o pqueue.o refine.o util.o timing.o debug.o \
#       bucketsort.o graph.o stat.o kmetis.o kwayrefine.o \
#       kwayfm.o balance.o ometis.o srefine.o sfm.o separator.o \
#       mincover.o mmd.o mesh.o meshpart.o frename.o fortran.o \
#       myqsort.o compress.o parmetis.o estmem.o \
#       mpmetis.o mcoarsen.o mmatch.o minitpart.o mbalance.o \
#       mrefine.o mutil.o mfm.o mkmetis.o mkwayrefine.o mkwayfmh.o \
#       mrefine2.o minitpart2.o mbalance2.o mfm2.o \
#       kvmetis.o kwayvolrefine.o kwayvolfm.o subdomains.o

set(metis_sources
  coarsen.c
  fm.c
  initpart.c
  match.c
  ccgraph.c
  memory.c
  pmetis.c
  pqueue.c
  refine.c
  util.c
  timing.c
  debug.c
  bucketsort.c
  graph.c
  stat.c
  kmetis.c
  kwayrefine.c
  kwayfm.c
  balance.c
  ometis.c
  srefine.c
  sfm.c
  separator.c
  mincover.c
  mmd.c
  mesh.c
  meshpart.c
  frename.c
  fortran.c
  myqsort.c
  compress.c
  parmetis.c
  estmem.c
  mpmetis.c
  mcoarsen.c
  mmatch.c
  minitpart.c
  mbalance.c
  mrefine.c
  mutil.c
  mfm.c
  mkmetis.c
  mkwayrefine.c
  mkwayfmh.c
  mrefine2.c
  minitpart2.c
  mbalance2.c
  mfm2.c
  kvmetis.c
  kwayvolrefine.c
  kwayvolfm.c
  subdomains.c
  )

add_library(metis STATIC ${metis_sources})
set_target_properties(metis PROPERTIES LINKER_LANGUAGE CXX)
