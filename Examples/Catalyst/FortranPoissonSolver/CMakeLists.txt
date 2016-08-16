cmake_minimum_required(VERSION 3.3)
project(CatalystFortranPoissonSolver CXX Fortran)

set(SOURCES
  FEDriver.F90
  Box.F90
  SparseMatrix.F90
  ConjugateGradient.F90
  PoissonDiscretization.F90
)

set(USE_CATALYST ON CACHE BOOL "Link the simulator with Catalyst")
if(USE_CATALYST)
  find_package(ParaView 4.1 REQUIRED COMPONENTS vtkPVPythonCatalyst)
  include("${PARAVIEW_USE_FILE}")
  # we don't create a separate library for the adaptor here
  # since FEFortranAdaptor.F90 depends on Box.F90
  list(APPEND SOURCES FEFortranAdaptor.F90 FECxxAdaptor.cxx)
  add_definitions("-DUSE_CATALYST")
  if(NOT PARAVIEW_USE_MPI)
    message(SEND_ERROR "ParaView must be built with MPI enabled")
  endif()
else()
  find_package(MPI REQUIRED)
  include_directories(${MPI_Fortran_INCLUDE_PATH})
endif()

add_executable(FortranPoissonSolver ${SOURCES})
target_link_libraries(FortranPoissonSolver ${MPI_Fortran_LIBRARIES})
if(USE_CATALYST)
  target_link_libraries(FortranPoissonSolver vtkPVPythonCatalyst vtkParallelMPI)
endif()
set_target_properties(FortranPoissonSolver PROPERTIES
  LINKER_LANGUAGE Fortran)

option(BUILD_TESTING "Build Testing" OFF)
# Setup testing.
if (BUILD_TESTING)
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/coproc.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  include(CTest)
  add_test(NAME FortranPoissonSolverTest COMMAND FortranPoissonSolver coproc.py)
  set_tests_properties(FortranPoissonSolverTest PROPERTIES LABELS "PARAVIEW;CATALYST")
endif()
