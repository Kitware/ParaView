vtk_add_test_mpi(vtkPVVTKExtensionsCosmoToolsCxxTests tests
  TestHaloFinder.cxx # test of particles output
  TestHaloFinderSummaryInfo.cxx # test of summary information output
  TestHaloFinderSubhaloFinding.cxx # test of subhalo finding option
  TestSubhaloFinder.cxx # test of subhalo finding filter
)

vtk_test_cxx_executable(vtkPVVTKExtensionsCosmoToolsCxxTests tests
HaloFinderTestHelpers.h
)
