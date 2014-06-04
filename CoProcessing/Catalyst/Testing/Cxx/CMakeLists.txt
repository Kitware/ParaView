include(ParaViewTestingMacros)

paraview_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  SimpleDriver.cxx
  SimpleDriver2.cxx
  AdaptorDriver.cxx
  )

# the CoProcessingTestOutputs needs to be run with ${MPIEXEC} if
# the executable was built with MPI because certain machines only
# allow running MPI programs with the proper ${MPIEXEC}
if (NOT PARAVIEW_USE_MPI)
  vtk_add_test_cxx(${vtk-module}CxxTests extra_tests
    NO_DATA NO_VALID NO_OUTPUT
    CoProcessingTestOutputs.cxx)
  list(APPEND tests
    ${extra_tests})
else()
  paraview_add_test_mpi(${vtk-module}Cxx-MPI mpi_tests
    NO_DATA NO_VALID NO_OUTPUT
    CoProcessingTestOutputs.cxx
    SubController.cxx
    )
  vtk_test_mpi_executable(${vtk-module}Cxx-MPI mpi_tests)
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests tests
  vtkCustomUnstructuredGridBuilder.cxx)
