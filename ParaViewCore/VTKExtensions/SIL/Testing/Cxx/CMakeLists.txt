vtk_add_test_cxx(
  vtkPVVTKExtensionsSILCxxTests tests
  NO_VALID NO_OUTPUT
  TestSubsetInclusionLattice.cxx)
vtk_test_cxx_executable(vtkPVVTKExtensionsSILCxxTests tests)
