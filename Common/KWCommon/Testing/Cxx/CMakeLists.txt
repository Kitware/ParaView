ADD_EXECUTABLE(TestMemory TestMemory.cxx)
ADD_EXECUTABLE(TestRemoteProcess TestRemoteProcess.cxx)
ADD_EXECUTABLE(CommonKWCommonPrintSelf CommonKWCommonPrintSelf.cxx)

ADD_TEST(TestMemory ${CXX_TEST_PATH}/TestMemory)
# disable this one for now
#ADD_TEST(TestRemoteProcess ${CXX_TEST_PATH}/TestRemoteProcess)
ADD_TEST(CommonKWCommonPrintSelf ${CXX_TEST_PATH}/CommonKWCommonPrintSelf)


TARGET_LINK_LIBRARIES(TestMemory KWCommon)
TARGET_LINK_LIBRARIES(TestRemoteProcess KWCommon)
TARGET_LINK_LIBRARIES(CommonKWCommonPrintSelf KWCommon)

