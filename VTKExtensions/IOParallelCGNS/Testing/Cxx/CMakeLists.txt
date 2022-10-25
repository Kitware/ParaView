if (PARAVIEW_USE_MPI AND TARGET VTK::IOCGNSReader)
  set(vtkPVVTKExtensionsIOParallelCGNSWriterCxxTests_NUMPROCS 4)
  vtk_add_test_mpi(
    vtkPVVTKExtensionsIOParallelCGNSWriterCxxTests mpi_tests
    NO_VALID TESTING_DATA
    TestMultiBlockData.cxx
    TestUnstructuredGrid.cxx
    TestPartialData.cxx
    TestPartitionedDataSet.cxx
    TestPartitionedDataSetCollection.cxx
    TestPolyData.cxx
    TestPolygonalData.cxx
    TestPolyhedralGrid.cxx
    )

  vtk_test_cxx_executable(vtkPVVTKExtensionsIOParallelCGNSWriterCxxTests mpi_tests
    TestFunctions.cxx
    TestFunctions.h
    )
endif()
