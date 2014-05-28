include(ParaViewTestingMacros)

paraview_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID
  ColorMap.cxx
  OddWidthImages.cxx
  CreateDeleteObjects.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)

#TARGET_LINK_LIBRARIES( MantaCxxTests vtkManta vtkRenderingOpenGL)

#disabling this until I sort out the shutdown segfault
#ADD_EXECUTABLE( TriangleStrips TriangleStrips.cxx )
#TARGET_LINK_LIBRARIES( TriangleStrips vtkManta vtkRendering vtkHybrid)
#ADD_TEST( Manta-TestTriangleStrips ${EXECUTABLE_OUTPUT_PATH}/TriangleStrips
#          -T ${PARAVIEW_TEST_OUTPUT_DIR}
#          -V DATA{${PARAVIEW_TEST_BASELINE_DIR}/TriangleStrips.png} )

#disabling this until I sort out the shutdown segfault
#ADD_EXECUTABLE( MultipleLayers MultipleLayers.cxx )
#TARGET_LINK_LIBRARIES( MultipleLayers vtkManta vtkRendering vtkHybrid )
#ADD_TEST( Manta-TestMultipleLayers ${EXECUTABLE_OUTPUT_PATH}/MultipleLayers
#          ${PROJECT_SOURCE_DIR}/examples/multiRensEvents.log
#          -T ${PARAVIEW_TEST_OUTPUT_DIR}
#          -V DATA{${PARAVIEW_TEST_BASELINE_DIR}/MultiLayerRendering.png} )


if (PARAVIEW_USE_MPI)
  include(vtkMPI)
  include_directories(${MPI_C_INCLUDE_PATH})
  paraview_add_test_cxx(${vtk-module}Cxx-MPI mpi_tests
    NO_DATA NO_VALID
    MantaBenchmark.cxx
    )
  vtk_test_cxx_executable(${vtk-module}Cxx-MPI mpi_tests)
  target_link_libraries(${vtk-module}Cxx-MPI LINK_PRIVATE vtkParallelMPI)
  vtk_mpi_link(${vtk-module}Cxx-MPI)
endif()
