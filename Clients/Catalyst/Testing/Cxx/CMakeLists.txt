vtk_add_test_cxx(vtkPVCatalystCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  SimpleDriver.cxx
  SimpleDriver2.cxx
  AdaptorDriver.cxx
  )

vtk_add_test_cxx(vtkPVCatalystCxxTests tests
  NO_VALID
  CPXMLPWriterPipeline.cxx
  )
# the CoProcessingTestOutputs needs to be run with ${MPIEXEC} if
# the executable was built with MPI because certain machines only
# allow running MPI programs with the proper ${MPIEXEC}
if (PARAVIEW_USE_MPI)
  vtk_add_test_mpi(vtkPVCatalystCxx-MPI mpi_tests
    NO_VALID
    CoProcessingTestOutputs.cxx
    SubController.cxx
    )
  vtk_test_cxx_executable(vtkPVCatalystCxx-MPI mpi_tests)
else ()
  vtk_add_test_cxx(vtkPVCatalystCxxTests tests
    NO_DATA NO_VALID NO_OUTPUT
    CoProcessingTestOutputs.cxx)
endif ()

vtk_test_cxx_executable(vtkPVCatalystCxxTests tests
  vtkCustomUnstructuredGridBuilder.cxx)
