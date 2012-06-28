INCLUDE_DIRECTORIES(${VTKCS_SOURCE_DIR}/ClientServer)

ADD_EXECUTABLE(vtkClientServerTests coverClientServer.cxx)
TARGET_LINK_LIBRARIES(vtkClientServerTests vtkClientServer)

ADD_TEST(vtkClientServerCoverage
  ${EXECUTABLE_OUTPUT_PATH}/vtkClientServerTests
  )
