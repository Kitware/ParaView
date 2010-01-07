INCLUDE_DIRECTORIES(../)

#TODO: Can these all be one unix style app instead of N bundled apps?
#TODO: Many of these are direct copies from VTK Examples, just use them directly

ADD_EXECUTABLE(Cone MACOSX_BUNDLE Cone.cpp)
TARGET_LINK_LIBRARIES(Cone vtkManta vtkRendering)

ADD_EXECUTABLE(Cylinder MACOSX_BUNDLE Cylinder.cxx)
TARGET_LINK_LIBRARIES(Cylinder vtkManta vtkRendering)

ADD_EXECUTABLE(marching MACOSX_BUNDLE marching.cxx)
TARGET_LINK_LIBRARIES(marching vtkManta vtkRendering vtkHybrid)

ADD_EXECUTABLE(Cube MACOSX_BUNDLE Cube.cxx)
TARGET_LINK_LIBRARIES(Cube vtkManta vtkRendering)

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

ADD_EXECUTABLE(TriangleStripTest MACOSX_BUNDLE TriangleStripTest.cxx)
TARGET_LINK_LIBRARIES(TriangleStripTest vtkManta vtkRendering)

IF (VTK_USE_MPI)
  ADD_EXECUTABLE(timing_new timing_new.cxx)
  TARGET_LINK_LIBRARIES (timing_new vtkManta vtkRendering)

  ADD_EXECUTABLE(ParallelRendering ParallelRendering)
  TARGET_LINK_LIBRARIES (ParallelRendering vtkManta vtkRendering vtkParallel)
  
  ADD_EXECUTABLE(timing timing.cxx)
  TARGET_LINK_LIBRARIES (timing vtkManta vtkRendering vtkParallel)
ENDIF(VTK_USE_MPI)

