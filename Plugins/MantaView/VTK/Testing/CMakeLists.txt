INCLUDE_DIRECTORIES(../)

create_test_sourcelist(MANTATESTS
  MantaCxxTests.cxx
  ColorMap.cxx
  OddWidthImages.cxx
  CreateDeleteObjects.cxx
)

ADD_EXECUTABLE( MantaCxxTests ${MANTATESTS} )
set (TestsToRun ${MANTATESTS})
remove (TestsToRun MantaCxxTests.cxx)
foreach (test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  add_test(Manta-Test${TName} ${EXECUTABLE_OUTPUT_PATH}/MantaCxxTests ${TName}
    -T ${EXECUTABLE_OUTPUT_PATH}/../Testing/Temporary
    -V ${PARAVIEW_DATA_ROOT}/Baseline/MantaTest${TName}.png
    )
endforeach()
TARGET_LINK_LIBRARIES( MantaCxxTests vtkManta vtkRenderingOpenGL)

#disabling this until I sort out the shutdown segfault
#ADD_EXECUTABLE( TriangleStrips TriangleStrips.cxx )
#TARGET_LINK_LIBRARIES( TriangleStrips vtkManta vtkRendering vtkHybrid)
#ADD_TEST( Manta-TestTriangleStrips ${EXECUTABLE_OUTPUT_PATH}/TriangleStrips
#          -T ${EXECUTABLE_OUTPUT_PATH}/../Testing/Temporary
#          -V ${PARAVIEW_DATA_ROOT}/Baseline/TriangleStrips.png )

#disabling this until I sort out the shutdown segfault
#ADD_EXECUTABLE( MultipleLayers MultipleLayers.cxx )
#TARGET_LINK_LIBRARIES( MultipleLayers vtkManta vtkRendering vtkHybrid )
#ADD_TEST( Manta-TestMultipleLayers ${EXECUTABLE_OUTPUT_PATH}/MultipleLayers
#          ${PROJECT_SOURCE_DIR}/examples/multiRensEvents.log
#          -T ${EXECUTABLE_OUTPUT_PATH}/../Testing/Temporary
#          -V ${PARAVIEW_DATA_ROOT}/Baseline/MultiLayerRendering.png )


IF ( PARAVIEW_USE_MPI )

  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
  ADD_EXECUTABLE( MantaBenchmark MantaBenchmark.cxx )
  TARGET_LINK_LIBRARIES( MantaBenchmark vtkManta vtkFiltersHybrid vtkRenderingOpenGL vtkParallelMPI )
  ADD_TEST( Manta-TestBenchmark ${EXECUTABLE_OUTPUT_PATH}/MantaBenchmark
    -T ${EXECUTABLE_OUTPUT_PATH}/../Testing/Temporary
    -V ${PARAVIEW_DATA_ROOT}/Baseline/MantaBenchmark.png )

ENDIF (PARAVIEW_USE_MPI)
