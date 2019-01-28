set(sources
  Box.F90
  ConjugateGradient.F90
  FEDriver.F90
  PoissonDiscretization.F90
  SparseMatrix.F90)

if (USE_CATALYST)
  # We don't create a separate library for the adaptor here
  # since FEFortranAdaptor.F90 depends on Box.F90
  list(APPEND sources
    FECxxAdaptor.cxx
    FEFortranAdaptor.F90)
endif ()

add_executable(FortranPoissonSolver ${sources})
target_link_libraries(FortranPoissonSolver
  PRIVATE
    MPI::MPI_Fortran)
if (USE_CATALYST)
  target_link_libraries(FortranPoissonSolver
    PRIVATE
      ParaView::PythonCatalyst
      VTK::ParallelMPI)
endif ()
set_property(TARGET FortranPoissonSolver
  PROPERTY
    LINKER_LANGUAGE Fortran)

if (BUILD_TESTING)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/SampleScripts/coproc.py
    ${CMAKE_CURRENT_BINARY_DIR}/coproc.py
    COPYONLY)
  add_test(NAME FortranPoissonSolverTest COMMAND FortranPoissonSolver coproc.py)
endif()
