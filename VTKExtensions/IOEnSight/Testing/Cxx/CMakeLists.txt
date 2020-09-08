if (PARAVIEW_USE_MPI)
  vtk_add_test_mpi(vtkPVVTKExtensionsIOEnSightTests tests
    TESTING_DATA NO_VALID
    TestPEnSightBinaryGoldReader.cxx)
  vtk_test_cxx_executable(vtkPVVTKExtensionsIOEnSightTests tests)
endif ()
