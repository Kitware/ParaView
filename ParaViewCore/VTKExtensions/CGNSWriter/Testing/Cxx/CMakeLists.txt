include(ParaViewTestingMacros)

paraview_add_test_cxx(
  ${vtk-module}CxxTests tests
  NO_VALID NO_DATA
  TestStructuredGrid.cxx
  TestUnstructuredGrid.cxx
  TestPolydata.cxx
  TestPolyhedral.cxx
  TestMultiBlockDataSet.cxx
)
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
