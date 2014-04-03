include(ParaViewTestingMacros)

paraview_add_test_cxx(${vtk-module}CxxTests tests
  NO_VALID NO_OUTPUT
  TestDataEncoder.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
