include(ParaViewTestingMacros)
paraview_add_test_cxx(${vtk-module}CxxTests tests
  NO_VALID NO_OUTPUT NO_DATA
  TestFileSequenceParser.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
