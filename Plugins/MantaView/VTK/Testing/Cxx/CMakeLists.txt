set (MyTests
  ColorMap.cxx
  OddWidthImages.cxx
  CreateDeleteObjects.cxx
)

## Create test source list
create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
    ${MyTests}
    EXTRA_INCLUDE vtkTestDriver.h
    )
vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

set(TestsToRun ${Tests})
remove(TestsToRun ${vtk-module}CxxTests.cxx)

## Add all executables
if (PARAVIEW_TEST_DIR AND PARAVIEW_DATA_ROOT)
  foreach(test ${TestsToRun})
    get_filename_component(t ${test} NAME_WE)
    add_test(NAME ${vtk-module}Cxx-${t}
      COMMAND ${vtk-module}CxxTests ${t}
              -T {PARAVIEW_TEST_DIR}
              -V ${PARAVIEW_DATA_ROOT}/Baseline/MyTests${t}.png)
    set_tests_properties(${vtk-module}Cxx-${t}
      PROPERTIES LABELS "${${vtk-module}_TEST_LABELS}")
  endforeach()
endif()

#TARGET_LINK_LIBRARIES( MantaCxxTests vtkManta vtkRenderingOpenGL)

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


if (PARAVIEW_USE_MPI)
  include(vtkMPI)
  include_directories(${MPI_C_INCLUDE_PATH})
  vtk_module_test_executable(MantaBenchmark MantaBenchmark.cxx)
  target_link_libraries(MantaBenchmark LINK_PRIVATE vtkParallelMPI)
  vtk_mpi_link(MantaBenchmark)
  add_test(NAME Manta-TestBenchmark
    COMMAND MantaBenchmark
           -T ${EXECUTABLE_OUTPUT_PATH}/../Testing/Temporary
           -V ${PARAVIEW_DATA_ROOT}/Baseline/MantaBenchmark.png)
  set_tests_properties(Manta-TestBenchmark
    PROPERTIES LABELS "${${vtk-module}_TEST_LABELS}")
endif()
