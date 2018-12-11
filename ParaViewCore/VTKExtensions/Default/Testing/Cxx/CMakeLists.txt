vtk_add_test_cxx(vtkPVVTKExtensionsDefaultCxxTests tests
  NO_VALID NO_OUTPUT NO_DATA
  TestFileSequenceParser.cxx
  )
vtk_add_test_cxx(vtkPVVTKExtensionsDefaultCxxTests tests
  NO_VALID NO_OUTPUT
  TestPVDArraySelection.cxx
  )
vtk_test_cxx_executable(vtkPVVTKExtensionsDefaultCxxTests tests)
