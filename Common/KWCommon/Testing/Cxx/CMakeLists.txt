ADD_EXECUTABLE(TestRegistery TestRegistery.cxx)
ADD_EXECUTABLE(TestMemory TestMemory.cxx)
ADD_EXECUTABLE(TestCommandLineArguments TestCommandLineArguments.cxx)
ADD_EXECUTABLE(TestRemoteProcess TestRemoteProcess.cxx)

ADD_TEST(TestRegistery ${CXX_TEST_PATH}/TestRegistery)
ADD_TEST(TestMemory ${CXX_TEST_PATH}/TestMemory)
ADD_TEST(TestCommandLineArguments ${CXX_TEST_PATH}/TestCommandLineArguments -A -B asdklj -C=jkaskhd --long1 --long2 kjasdh --long3=dsljf -Dkdsfj -DONE --long4akhdkj -D lkdjlkjf -B)

TARGET_LINK_LIBRARIES(TestRegistery vtkKWCommonTCL)
TARGET_LINK_LIBRARIES(TestMemory vtkKWCommonTCL)
TARGET_LINK_LIBRARIES(TestCommandLineArguments vtkKWCommonTCL)
TARGET_LINK_LIBRARIES(TestRemoteProcess vtkKWCommonTCL)
