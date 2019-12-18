vtk_add_test_cxx(vtkPVVTKExtensionsCGNSWriterCxxTests tests
  NO_VALID NO_DATA
  TestStructuredGrid.cxx
  TestUnstructuredGrid.cxx
  TestPolydata.cxx
  TestPolyhedral.cxx
  TestMultiBlockDataSet.cxx
)
vtk_test_cxx_executable(vtkPVVTKExtensionsCGNSWriterCxxTests tests)
