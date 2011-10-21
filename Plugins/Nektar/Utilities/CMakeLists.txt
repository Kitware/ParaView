

# These are the module names:

#ALL =   nek2vel_PV_interface gen_utils io \
#        prepost mlevel bwoptim comm_PV_NEKTAR \
#        dgalerkin  ScaLapackNektar_new test_partition_connectivity \
#        sgs

project(nektar)

include_directories(
  ${Nektar_SOURCE_DIR}/include
  ${MPI_INCLUDE_PATH}
  )

add_definitions(-DPARALLEL -DMETIS)

set(nektar_sources
  SRC_PV_NEK/nek2vel_PV_interface.C
  SRC_PV_NEK/gen_utils.C
  SRC_PV_NEK/io.C
  SRC_PV_NEK/prepost.C
  SRC_PV_NEK/mlevel.C
  SRC_PV_NEK/bwoptim.C
  SRC_PV_NEK/comm_PV_NEKTAR.C
  SRC_PV_NEK/dgalerkin.C
  SRC_PV_NEK/ScaLapackNektar_new.C
  SRC_PV_NEK/test_partition_connectivity.C
  SRC_PV_NEK/sgs.C
  )

add_library(nektar STATIC ${nektar_sources})
target_link_libraries(nektar 
  metis
  gs
  ${BLACS_C_init_LIBRARY}
  ${BLACS_LIBRARY}
  ${SCALAPACK_LIBRARY}
  )
