ADD_EXECUTABLE(TestRandom TestRandom.cxx)
ADD_TEST(TestRandom ${CXX_TEST_PATH}/TestRandom)

TARGET_LINK_LIBRARIES(TestRandom vtkKWWidgetsTCL)
