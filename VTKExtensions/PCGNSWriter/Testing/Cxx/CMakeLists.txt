if (PARAVIEW_USE_MPI)
      
      set(vtkPVVTKExtensionsPCGNSWriterCxxTests_NUMPROCS 4)
      vtk_add_test_mpi(
        vtkPVVTKExtensionsPCGNSWriterCxxTests mpi_tests
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
        
      vtk_test_cxx_executable(vtkPVVTKExtensionsPCGNSWriterCxxTests mpi_tests
        TestFunctions.cxx
        TestFunctions.h
        )  

endif(PARAVIEW_USE_MPI)
