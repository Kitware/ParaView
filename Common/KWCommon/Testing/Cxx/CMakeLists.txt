ADD_EXECUTABLE(TestRegistery TestRegistery.cxx)
ADD_EXECUTABLE(TestMemory TestMemory.cxx)
ADD_EXECUTABLE(TestCommandLineArguments TestCommandLineArguments.cxx)
ADD_EXECUTABLE(TestRemoteProcess TestRemoteProcess.cxx)
ADD_EXECUTABLE(CommonKWCommonPrintSelf CommonKWCommonPrintSelf.cxx)

ADD_TEST(TestRegistery ${CXX_TEST_PATH}/TestRegistery)
ADD_TEST(TestMemory ${CXX_TEST_PATH}/TestMemory)
ADD_TEST(TestCommandLineArguments
  ${CXX_TEST_PATH}/TestCommandLineArguments
  -A -B asdklj -C=jkaskhd --long1 --long2 kjasdh --long3=dsljf -Dkdsfj -DONE
  --long4akhdkj 
  --some-int-variable 5
  --some-double-variable1543.3
  --some-string-variable=HelloWorld
  -D lkdjlkjf -B)
# disable this one for now
#ADD_TEST(TestRemoteProcess ${CXX_TEST_PATH}/TestRemoteProcess)
ADD_TEST(CommonKWCommonPrintSelf ${CXX_TEST_PATH}/CommonKWCommonPrintSelf)


TARGET_LINK_LIBRARIES(TestRegistery vtkKWCommon)
TARGET_LINK_LIBRARIES(TestMemory vtkKWCommon)
TARGET_LINK_LIBRARIES(TestCommandLineArguments vtkKWCommon)
TARGET_LINK_LIBRARIES(TestRemoteProcess vtkKWCommon)
TARGET_LINK_LIBRARIES(CommonKWCommonPrintSelf vtkKWCommon)

