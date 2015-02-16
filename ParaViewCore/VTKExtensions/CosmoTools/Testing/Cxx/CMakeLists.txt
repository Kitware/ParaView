paraview_test_load_data(""
genericio/m000.499.allparticles
)

paraview_add_test_cxx(${vtk-module}CxxTests tests
  NO_OUTPUT
  TestHaloFinder.cxx
)

vtk_test_mpi_executable(${vtk-module}CxxTests tests
HaloFinderTestHelpers.h
)
