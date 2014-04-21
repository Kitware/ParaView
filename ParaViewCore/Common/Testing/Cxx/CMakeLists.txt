include(ParaViewTestingMacros)
# Download all top-level baselines here.
paraview_test_load_baselines_dirs(""
  "/"
  TestCTHFragmentConnect
  )

paraview_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  ParaViewCoreCommonPrintSelf.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
