ADD_EXECUTABLE(TestRegistery TestRegistery.cxx)
ADD_TEST(TestRegistery ${CXX_TEST_PATH}/TestRegistery)
TARGET_LINK_LIBRARIES(TestRegistery KWWidgetsTCL)

ADD_EXECUTABLE(TestRandom TestRandom.cxx)
ADD_TEST(TestRandom ${CXX_TEST_PATH}/TestRandom)
TARGET_LINK_LIBRARIES(TestRandom KWWidgetsTCL)
