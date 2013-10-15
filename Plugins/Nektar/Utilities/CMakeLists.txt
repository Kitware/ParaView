

# These are the module names:

#ALL =   nek2vel_PV_interface gen_utils io \
#        prepost mlevel bwoptim comm_PV_NEKTAR \
#        dgalerkin  ScaLapackNektar_new test_partition_connectivity \
#        sgs

if(NOT MPI_INCLUDE_PATH)
  find_package(MPI REQUIRED)
endif()

include_directories(
  ${MPI_INCLUDE_PATH}
  ${Nektar_SOURCE_DIR}/include
  )

if(NOT WIN32)
# need some try compiles
  set(CMAKE_CXX_FLAGS "-fPIC -ffast-math -funroll-loops -fstrict-aliasing -w")
endif()

add_definitions(-DPARALLEL -DMETIS -DCSGSLIB -DNDEBUG -DNOSCALAPACK)

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

if(BUILD_TEST_EXE)
  FIND_LIBRARY(SCALAPACK_LIBRARY REQUIRED NAMES scalapack scalapack-pvm scalapack-mpi scalapack-mpich scalapack-mpich2 scalapack-openmpi scalapack-lam
    PATHS /usr/lib64 /usr/lib /usr/local/lib64 /usr/local/lib)

  # the BLACS library may be included in some newer builds of scalapack
  FIND_LIBRARY(BLACS_LIBRARY REQUIRED NAMES blacs blacs-pvm blacs-mpi blacs-mpich blacs-mpich2 blacs-openmpi blacs-lam
    PATHS /usr/lib64 /usr/lib /usr/local/lib64 /usr/local/lib)

  # Report the found libraries, quit with fatal error if any required library has not been found.
  INCLUDE(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(SCALAPACK DEFAULT_MSG SCALAPACK_LIBRARY BLACS_LIBRARY)
endif()

SET(SCALAPACK_LIBRARIES ${SCALAPACK_LIBRARY} ${BLACS_LIBRARY})

vtk_module_library(nektar STATIC ${nektar_sources})
target_link_libraries(nektar
  LINK_PUBLIC
  ${SCALAPACK_LIBRARIES}
  ${LAPACK_lapack_LIBRARY}
  ${BLAS_LIBRARIES}
  ${MPI_LIBRARY}
)