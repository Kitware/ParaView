vtk_add_test_cxx(vtkPVVTKExtensionsMiscCxxTests tests
  NO_VALID NO_OUTPUT
  TestMergeTablesMultiBlock.cxx
  TestExtractHistogram.cxx,NO_DATA)
vtk_test_cxx_executable(vtkPVVTKExtensionsMiscCxxTests tests)
