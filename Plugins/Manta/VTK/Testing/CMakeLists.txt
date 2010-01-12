INCLUDE_DIRECTORIES(../)

# Locate VTK_DATA_ROOT as the directory of VTKData.
FIND_PATH( VTK_DATA_ROOT VTKData.readme $ENV{VTK_DATA_ROOT} )
IF ( NOT VTK_DATA_ROOT )
  FIND_PATH ( VTK_DATA_ROOT VTKData.readme ../VTKData DOC "VTK Data root directory" )
ENDIF ( NOT VTK_DATA_ROOT )

ADD_EXECUTABLE( colorMap MACOSX_BUNDLE colorMap.cxx )
TARGET_LINK_LIBRARIES( colorMap vtkManta vtkRendering )
ADD_TEST( ColorMapping ${EXECUTABLE_OUTPUT_PATH}/colorMap
          -T ${PROJECT_BINARY_DIR}/Testing/Temporary
          -V ${PROJECT_SOURCE_DIR}/examples/Baseline/ColorMapping.png )

ADD_EXECUTABLE( CreateOddWidthImages MACOSX_BUNDLE CreateOddWidthImages.cxx )
TARGET_LINK_LIBRARIES( CreateOddWidthImages vtkManta vtkRendering vtkHybrid )
ADD_TEST( CreateOddWidthImages ${EXECUTABLE_OUTPUT_PATH}/CreateOddWidthImages
          -T ${PROJECT_BINARY_DIR}/Testing/Temporary
          -V ${PROJECT_SOURCE_DIR}/examples/Baseline/CreateOddWidthImages.png )

ADD_EXECUTABLE( CreateDeleteObjects MACOSX_BUNDLE CreateDeleteObjects.cxx )
TARGET_LINK_LIBRARIES( CreateDeleteObjects vtkManta vtkRendering vtkHybrid )
ADD_TEST( CreateDeleteObjects ${EXECUTABLE_OUTPUT_PATH}/CreateDeleteObjects
          -T ${PROJECT_BINARY_DIR}/Testing/Temporary
          -V ${PROJECT_SOURCE_DIR}/examples/Baseline/CreateDeleteObjects.png )

ADD_EXECUTABLE( ToggleVisibility MACOSX_BUNDLE ToggleVisibility.cxx )
TARGET_LINK_LIBRARIES( ToggleVisibility vtkManta vtkRendering vtkHybrid)
ADD_TEST( ToggleVisibility ${EXECUTABLE_OUTPUT_PATH}/ToggleVisibility
          -T ${PROJECT_BINARY_DIR}/Testing/Temporary
          -V ${PROJECT_SOURCE_DIR}/examples/Baseline/ToggleVisibility.png )

ADD_EXECUTABLE( multiRens MACOSX_BUNDLE multiRens.cxx )
TARGET_LINK_LIBRARIES( multiRens vtkManta vtkRendering vtkHybrid )
ADD_TEST( MultiLayerRendering ${EXECUTABLE_OUTPUT_PATH}/multiRens
          ${PROJECT_SOURCE_DIR}/examples/multiRensEvents.log
          -T ${PROJECT_BINARY_DIR}/Testing/Temporary
          -V ${PROJECT_SOURCE_DIR}/examples/Baseline/MultiLayerRendering.png )

ADD_EXECUTABLE( marching.timing MACOSX_BUNDLE marching.timing.cxx )
TARGET_LINK_LIBRARIES( marching.timing vtkManta vtkHybrid vtkRendering )
ADD_TEST( MarchingCubeTiming ${EXECUTABLE_OUTPUT_PATH}/marching.timing 2
          -T ${PROJECT_BINARY_DIR}/Testing/Temporary
          -V ${PROJECT_SOURCE_DIR}/examples/Baseline/MarchingCubeTiming.png )

IF (VTK_USE_MPI)
  ADD_EXECUTABLE( TaskParallelism TaskParallelism )
  TARGET_LINK_LIBRARIES ( TaskParallelism vtkManta vtkRendering vtkParallel )
  ADD_TEST( ParallelImageCompositing mpirun -np 2 
            ${EXECUTABLE_OUTPUT_PATH}/TaskParallelism
            -T ${PROJECT_BINARY_DIR}/Testing/Temporary
            -V ${PROJECT_SOURCE_DIR}/examples/Baseline/ParallelImageCompositing.png )


  ADD_EXECUTABLE( MantaBenchmark MantaBenchmark.cxx )
  TARGET_LINK_LIBRARIES ( MantaBenchmark vtkManta vtkRendering vtkParallel )

ENDIF(VTK_USE_MPI)

