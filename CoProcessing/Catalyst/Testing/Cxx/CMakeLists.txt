vtk_module_test_executable(SimpleDriver SimpleDriver.cxx)
vtk_module_test_executable(SimpleDriver2 SimpleDriver2.cxx)
vtk_module_test_executable(AdaptorDriver
  AdaptorDriver.cxx
  vtkCustomUnstructuredGridBuilder.cxx)

# the CoProcessingTestOutputs needs to be run with ${MPIEXEC} if
# the executable was built with MPI because certain machines only
# allow running MPI programs with the proper ${MPIEXEC}
if (NOT PARAVIEW_USE_MPI)
  vtk_add_test_cxx(${vtk-module}CxxTests tests
    CoProcessingTestOutputs.cxx)
  vtk_test_cxx_executable(${vtk-module}CxxTests tests)
else()
  vtk_add_test_mpi(${vtk-module}CxxTests-MPI tests
    CoProcessingTestOutputs.cxx)
  vtk_test_mpi_executable(${vtk-module}CxxTests-MPI tests)
endif()
