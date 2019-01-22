vtk_add_test_cxx(vtkPVClientServerCoreDefaultCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  ParaViewCoreClientServerCorePrintSelf.cxx
  TestPVArrayInformation.cxx
  TestPartialArraysInformation.cxx
  TestSpecialDirectories.cxx
  TestSystemCaps.cxx
  )
if (PARAVIEW_USE_MPI)
  vtk_add_test_mpi(vtkPVClientServerCoreDefaultCxxTests mpi_tests
    NO_DATA NO_VALID NO_OUTPUT
    TestMPI.cxx)
  list(APPEND tests
    ${mpi_tests})
else ()
  vtk_add_test_cxx(vtkPVClientServerCoreDefaultCxxTests no_mpi_tests
    NO_DATA NO_VALID NO_OUTPUT
    TestMPI.cxx)
  list(APPEND tests
    ${no_mpi_tests})
endif()

vtk_test_cxx_executable(vtkPVClientServerCoreDefaultCxxTests tests)
