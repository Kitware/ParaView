ADD_EXECUTABLE( Random Random.cxx )
ADD_TEST(Random ${CXX_TEST_PATH}/Random)

TARGET_LINK_LIBRARIES(Random vtkKWWidgetsTCL)
