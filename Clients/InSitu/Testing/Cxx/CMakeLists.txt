vtk_add_test_cxx(vtkPVInSituCatalystCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestDataObjectToConduit.cxx)

vtk_test_cxx_executable(vtkPVInSituCatalystCxxTests tests)
