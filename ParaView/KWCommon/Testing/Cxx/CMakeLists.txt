ADD_EXECUTABLE( TestRegistery TestRegistery.cxx )
ADD_EXECUTABLE( TestMemory TestMemory.cxx )

ADD_TEST(TestRegistery ${CXX_TEST_PATH}/TestRegistery)
ADD_TEST(TestMemory ${CXX_TEST_PATH}/TestMemory)

TARGET_LINK_LIBRARIES(TestRegistery vtkKWCommonTCL)
TARGET_LINK_LIBRARIES(TestMemory vtkKWCommonTCL)
