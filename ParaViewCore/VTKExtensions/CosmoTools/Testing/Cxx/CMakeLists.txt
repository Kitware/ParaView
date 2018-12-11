paraview_test_load_data(""
genericio/m000.499.allparticles
)

vtk_add_test_cxx(vtkPVVTKExtensionsCosmoToolsCxxTests tests
  TestHaloFinder.cxx # test of particles output
  TestHaloFinderSummaryInfo.cxx # test of summary information output
  TestHaloFinderSubhaloFinding.cxx # test of subhalo finding option
  TestSubhaloFinder.cxx # test of subhalo finding filter
)

vtk_test_mpi_executable(vtkPVVTKExtensionsCosmoToolsCxxTests tests
HaloFinderTestHelpers.h
)
