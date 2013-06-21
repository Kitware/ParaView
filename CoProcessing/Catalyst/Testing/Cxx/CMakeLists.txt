vtk_module_test_executable(SimpleDriver SimpleDriver.cxx)
vtk_module_test_executable(SimpleDriver2 SimpleDriver2.cxx)
vtk_module_test_executable(AdaptorDriver
  AdaptorDriver.cxx
  vtkCustomUnstructuredGridBuilder.cxx)

# the CoProcessingTestOutputs needs to be run with ${MPIEXEC} if
# the executable was built with MPI because certain machines only
# allow running MPI programs with the proper ${MPIEXEC}
if (NOT PARAVIEW_USE_MPI)
  vtk_module_test_executable(CoProcessingTestOutputs CoProcessingTestOutputs.cxx)
  add_test(NAME vtkPVCatalystCxx-CoProcessingTestOutputs
    COMMAND CoProcessingTestOutputs)
  set_tests_properties(vtkPVCatalystCxx-CoProcessingTestOutputs
    PROPERTIES LABELS "PARAVIEW;CATALYST")
else()
  vtk_add_test_mpi(CoProcessingTestOutputs)
  set_tests_properties(vtkPVCatalystCxx-MPI-CoProcessingTestOutputs
    PROPERTIES LABELS "PARAVIEW;CATALYST")
endif()
