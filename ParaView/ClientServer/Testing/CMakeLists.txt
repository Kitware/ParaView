INCLUDE_DIRECTORIES(${PARAVIEW_SOURCE_DIR}/ClientServer)

ADD_EXECUTABLE(TestSimple TestSimple.cxx)
TARGET_LINK_LIBRARIES(TestSimple vtkClientServer vtkParaViewCSWrapped)

