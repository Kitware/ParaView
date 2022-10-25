if (TARGET VTK::IOCGNSReader AND TARGET VTK::IOIOSS)

  vtk_add_test_cxx(vtkPVVTKExtensionsIOCGNSWriterCxxTests tests
    TestPartitionedDataSetCollection2.cxx,NO_VALID
  )
endif()

if (TARGET VTK::IOCGNSReader)
  vtk_add_test_cxx(vtkPVVTKExtensionsIOCGNSWriterCxxTests tests
    TestStructuredGrid.cxx,NO_VALID,NO_DATA
    TestUnstructuredGrid.cxx,NO_VALID,NO_DATA
    TestPartitionedDataSet.cxx,NO_VALID,NO_DATA
    TestPartitionedDataSetCollection.cxx,NO_VALID,NO_DATA
    TestPolydata.cxx,NO_VALID,NO_DATA
    TestPolyhedral.cxx,NO_VALID,NO_DATA
    TestCellAndPointData.cxx,NO_VALID,NO_DATA
    TestMultiBlockDataSet.cxx,NO_VALID,NO_DATA
    TestMappedUnstructuredGrid.cxx,NO_VALID,NO_DATA
    TestTimeWriting.cxx,NO_VALID,NO_DATA
  )
  vtk_test_cxx_executable(vtkPVVTKExtensionsIOCGNSWriterCxxTests tests)
endif()
