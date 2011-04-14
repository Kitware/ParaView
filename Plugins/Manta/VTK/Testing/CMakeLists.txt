#TODO: wrap these all together under one "kit" test executable to reduce clutter

INCLUDE_DIRECTORIES(../)

# Locate VTK_DATA_ROOT as the directory of VTKData.
FIND_PATH( VTK_DATA_ROOT VTKData.readme $ENV{VTK_DATA_ROOT} )
IF ( NOT VTK_DATA_ROOT )
  FIND_PATH ( VTK_DATA_ROOT VTKData.readme ../VTKData DOC "VTK Data root directory" )
ENDIF ( NOT VTK_DATA_ROOT )

ADD_EXECUTABLE( ColorMap ColorMap.cxx )
TARGET_LINK_LIBRARIES( ColorMap vtkManta vtkRendering vtkPVClientServerCore )
ADD_TEST( Manta-TestColorMap ${EXECUTABLE_OUTPUT_PATH}/ColorMap
          -T ${PROJECT_BINARY_DIR}/Testing/Temporary
          -V ${PROJECT_SOURCE_DIR}/examples/Baseline/ColorMapping.png )

ADD_EXECUTABLE( TriangleStrips TriangleStrips.cxx )
TARGET_LINK_LIBRARIES( TriangleStrips vtkManta vtkRendering vtkHybrid vtkPVClientServerCore )
#disabling this until sort out the shutdown segfault
#ADD_TEST( Manta-TestTriangleStrips ${EXECUTABLE_OUTPUT_PATH}/TriangleStrips
#          -T ${PROJECT_BINARY_DIR}/Testing/Temporary
#          -V ${PROJECT_SOURCE_DIR}/examples/Baseline/TriangleStrips.png )

ADD_EXECUTABLE( OddWidthImages OddWidthImages.cxx )
TARGET_LINK_LIBRARIES( OddWidthImages vtkManta vtkRendering vtkHybrid vtkPVClientServerCore )
ADD_TEST( Manta-TestOddWidthImages ${EXECUTABLE_OUTPUT_PATH}/OddWidthImages
          -T ${PROJECT_BINARY_DIR}/Testing/Temporary
          -V ${PROJECT_SOURCE_DIR}/examples/Baseline/CreateOddWidthImages.png )

ADD_EXECUTABLE( CreateDeleteObjects CreateDeleteObjects.cxx )
TARGET_LINK_LIBRARIES( CreateDeleteObjects vtkManta vtkRendering vtkHybrid vtkPVClientServerCore )
ADD_TEST( Manta-TestCreateDeleteObjects ${EXECUTABLE_OUTPUT_PATH}/CreateDeleteObjects
          -T ${PROJECT_BINARY_DIR}/Testing/Temporary
          -V ${PROJECT_SOURCE_DIR}/examples/Baseline/CreateDeleteObjects.png )

ADD_EXECUTABLE( ToggleVisibility ToggleVisibility.cxx )
TARGET_LINK_LIBRARIES( ToggleVisibility vtkManta vtkRendering vtkHybrid vtkPVClientServerCore )
ADD_TEST( Manta-TestToggleVisibility ${EXECUTABLE_OUTPUT_PATH}/ToggleVisibility
          -T ${PROJECT_BINARY_DIR}/Testing/Temporary
          -V ${PROJECT_SOURCE_DIR}/examples/Baseline/ToggleVisibility.png )

ADD_EXECUTABLE( MultipleLayers MultipleLayers.cxx )
TARGET_LINK_LIBRARIES( MultipleLayers
  vtkManta vtkRendering vtkHybrid vtkPVClientServerCore )
#disabling this until sort out the shutdown segfault
#ADD_TEST( Manta-TestMultipleLayers ${EXECUTABLE_OUTPUT_PATH}/MultipleLayers
#          ${PROJECT_SOURCE_DIR}/examples/multiRensEvents.log
#          -T ${PROJECT_BINARY_DIR}/Testing/Temporary
#          -V ${PROJECT_SOURCE_DIR}/examples/Baseline/MultiLayerRendering.png )


IF ( VTK_USE_MPI )

  ADD_EXECUTABLE( MantaBenchmark MantaBenchmark.cxx )
#  TARGET_LINK_LIBRARIES( MantaBenchmark vtkManta vtkHybrid vtkRendering vtkPVClientServerCore )
  TARGET_LINK_LIBRARIES( MantaBenchmark vtkManta vtkHybrid vtkRendering vtkParallel )
  ADD_TEST( Manta-TestBenchmark ${EXECUTABLE_OUTPUT_PATH}/MantaBenchmark
    -T ${PROJECT_BINARY_DIR}/Testing/Temporary
    -V ${PROJECT_SOURCE_DIR}/examples/Baseline/MantaBenchmark.png )

ENDIF (VTK_USE_MPI)
