include(ParaViewTestingMacros)

paraview_test_load_data(""
  dualSphereAnimation.pvd)
paraview_test_load_data_dirs(""
  dualSphereAnimation)

paraview_add_test_cxx(${vtk-module}CxxTests tests
  NO_VALID NO_OUTPUT
  TestFileSequenceParser.cxx,NO_DATA
  TestPVDArraySelection.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
