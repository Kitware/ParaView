ADD_EXECUTABLE( TestRegistery TestRegistery.cxx )
ADD_EXECUTABLE( TestMemory TestMemory.cxx )
ADD_EXECUTABLE( CommandLineArguments CommandLineArguments.cxx )
ADD_EXECUTABLE( RemoteProcess RemoteProcess.cxx )

ADD_EXECUTABLE( ProcessFiles ProcessFiles.cxx )
TARGET_LINK_LIBRARIES(ProcessFiles vtkKWCommonTCL)

ADD_TEST(TestRegistery ${CXX_TEST_PATH}/TestRegistery)
ADD_TEST(TestMemory ${CXX_TEST_PATH}/TestMemory)
ADD_TEST(CommandLineArguments ${CXX_TEST_PATH}/CommandLineArguments -A -B asdklj -C=jkaskhd --long1 --long2 kjasdh --long3=dsljf -Dkdsfj -DONE --long4akhdkj -D lkdjlkjf -B)

TARGET_LINK_LIBRARIES(TestRegistery vtkKWCommonTCL)
TARGET_LINK_LIBRARIES(TestMemory vtkKWCommonTCL)
TARGET_LINK_LIBRARIES(CommandLineArguments vtkKWCommonTCL)
TARGET_LINK_LIBRARIES(RemoteProcess vtkKWCommonTCL)
