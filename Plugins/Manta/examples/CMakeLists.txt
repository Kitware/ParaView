INCLUDE_DIRECTORIES(../)

IF (VTK_DATA_ROOT)
#  ADD_EXECUTABLE(CADPart MACOSX_BUNDLE CADPart.cxx)
#  TARGET_LINK_LIBRARIES(CADPart vtkManta vtkRendering)
ENDIF (VTK_DATA_ROOT)

ADD_EXECUTABLE(Cone MACOSX_BUNDLE Cone.cpp)
TARGET_LINK_LIBRARIES(Cone vtkManta vtkRendering)

ADD_EXECUTABLE(Cylinder MACOSX_BUNDLE Cylinder.cxx)
TARGET_LINK_LIBRARIES(Cylinder vtkManta vtkRendering)

ADD_EXECUTABLE( colorMap MACOSX_BUNDLE colorMap.cxx )
TARGET_LINK_LIBRARIES( colorMap vtkManta vtkRendering )
ADD_TEST( ColorMapping ${EXECUTABLE_OUTPUT_PATH}/colorMap
          -T ${PROJECT_BINARY_DIR}/Testing/Temporary
          -V ${PROJECT_SOURCE_DIR}/examples/Baseline/ColorMapping.png )

ADD_EXECUTABLE(marching MACOSX_BUNDLE marching.cxx)
TARGET_LINK_LIBRARIES(marching vtkManta vtkRendering vtkHybrid)

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

ADD_EXECUTABLE(Cube MACOSX_BUNDLE Cube.cxx)
TARGET_LINK_LIBRARIES(Cube vtkManta vtkRendering)

IF (VTK_DATA_ROOT)
#   ADD_EXECUTABLE(rainbow MACOSX_BUNDLE rainbow.cxx)
#   TARGET_LINK_LIBRARIES(rainbow vtkManta vtkRendering)
ENDIF (VTK_DATA_ROOT)

IF (VTK_DATA_ROOT)
#   ADD_EXECUTABLE(TPlane MACOSX_BUNDLE TPlane.cxx)
#   TARGET_LINK_LIBRARIES(TPlane vtkManta vtkRendering)
ENDIF (VTK_DATA_ROOT)

ADD_EXECUTABLE(mace MACOSX_BUNDLE mace.cxx)
TARGET_LINK_LIBRARIES(mace vtkManta vtkRendering)

ADD_EXECUTABLE(isosurface MACOSX_BUNDLE isosurface.cpp)
TARGET_LINK_LIBRARIES(isosurface vtkManta vtkRendering vtkGraphics)

ADD_EXECUTABLE(water MACOSX_BUNDLE water.cpp)
TARGET_LINK_LIBRARIES(water vtkManta vtkRendering)

ADD_EXECUTABLE(SaveImage MACOSX_BUNDLE SaveImage.cpp)
TARGET_LINK_LIBRARIES(SaveImage vtkManta vtkRendering)

ADD_EXECUTABLE(screenshot MACOSX_BUNDLE screenshot.cxx)
TARGET_LINK_LIBRARIES(screenshot vtkManta vtkRendering)

ADD_EXECUTABLE( marching.timing MACOSX_BUNDLE marching.timing.cxx )
TARGET_LINK_LIBRARIES( marching.timing vtkManta vtkHybrid vtkRendering )
ADD_TEST( MarchingCubeTiming ${EXECUTABLE_OUTPUT_PATH}/marching.timing 2
          -T ${PROJECT_BINARY_DIR}/Testing/Temporary
          -V ${PROJECT_SOURCE_DIR}/examples/Baseline/MarchingCubeTiming.png )

ADD_EXECUTABLE(TriangleStripTest MACOSX_BUNDLE TriangleStripTest.cxx)
TARGET_LINK_LIBRARIES(TriangleStripTest vtkManta vtkRendering)

ADD_EXECUTABLE(timing_new timing_new.cxx)
TARGET_LINK_LIBRARIES (timing_new vtkManta vtkRendering)

IF (VTK_USE_MPI)
  ADD_EXECUTABLE(ParallelRendering ParallelRendering)
  TARGET_LINK_LIBRARIES (ParallelRendering vtkManta vtkRendering vtkParallel)

  ADD_EXECUTABLE( TaskParallelism TaskParallelism )
  TARGET_LINK_LIBRARIES ( TaskParallelism vtkManta vtkRendering vtkParallel )
  ADD_TEST( ParallelImageCompositing mpirun -np 2 
            ${EXECUTABLE_OUTPUT_PATH}/TaskParallelism
            -T ${PROJECT_BINARY_DIR}/Testing/Temporary
            -V ${PROJECT_SOURCE_DIR}/examples/Baseline/ParallelImageCompositing.png )
  
  ADD_EXECUTABLE(timing timing.cxx)
  TARGET_LINK_LIBRARIES (timing vtkManta vtkRendering vtkParallel)
ENDIF(VTK_USE_MPI)

