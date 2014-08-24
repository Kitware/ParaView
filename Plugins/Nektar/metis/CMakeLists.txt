include_directories(
  ${metis_SOURCE_DIR}
  )

if(WIN32)
  add_definitions(-D_USE_MATH_DEFINES)
endif()

vtk_module_warnings_disable(C)

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

vtk_module_library(metis STATIC ${metis_sources})
if (CMAKE_VERSION VERSION_LESS "2.8.9")
  if (NOT MSVC)
    set_target_properties(metis
      PROPERTIES
        COMPILE_FLAGS "-fPIC")
  endif ()
else ()
  set_target_properties(metis
    PROPERTIES
      POSITION_INDEPENDENT_CODE TRUE)
endif ()
