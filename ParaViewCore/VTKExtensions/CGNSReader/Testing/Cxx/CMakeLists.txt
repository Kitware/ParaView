include(ParaViewTestingMacros)

# Tell ExternalData to fetch test input at build time.
paraview_test_load_data(""
  "Example_mixed.cgns"
  "Example_nface_n.cgns"
  "channelBump_solution.cgns"
  "test_node_and_cell.cgns"
  )

paraview_add_test_cxx(
  ${vtk-module}CxxTests tests
  NO_VALID NO_OUTPUT
  TestCGNSReader.cxx
  TestReadCGNSSolution.cxx
  TestCGNSNoFlowSolutionPointers.cxx)
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
