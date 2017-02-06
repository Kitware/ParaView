paraview_test_load_data(""
genericio/m000.499.allparticles
)

paraview_add_test_cxx(${vtk-module}CxxTests tests
  TestHaloFinder.cxx # test of particles output
  TestHaloFinderSummaryInfo.cxx # test of summary information output
  TestHaloFinderSubhaloFinding.cxx # test of subhalo finding option
  TestSubhaloFinder.cxx # test of subhalo finding filter
)

vtk_test_mpi_executable(${vtk-module}CxxTests tests
HaloFinderTestHelpers.h
)
