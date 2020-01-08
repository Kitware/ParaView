# FIXME: these tests are not designed for parallel MPI runs!
# we need to fix them before we can run them on more than 1 rank.
# see paraview/paraview#18813
set (vtkPVVTKExtensionsCosmoToolsCxxTests_NUMPROCS 1)

vtk_add_test_mpi(vtkPVVTKExtensionsCosmoToolsCxxTests tests
  TESTING_DATA
  TestHaloFinder.cxx # test of particles output
  TestHaloFinderSummaryInfo.cxx # test of summary information output
  TestHaloFinderSubhaloFinding.cxx # test of subhalo finding option
  TestSubhaloFinder.cxx # test of subhalo finding filter
)

vtk_test_cxx_executable(vtkPVVTKExtensionsCosmoToolsCxxTests tests
HaloFinderTestHelpers.h
)
