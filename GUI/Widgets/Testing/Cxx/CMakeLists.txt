ADD_EXECUTABLE(TestRegistry TestRegistry.cxx)
ADD_TEST(TestRegistry ${CXX_TEST_PATH}/TestRegistry)
TARGET_LINK_LIBRARIES(TestRegistry KWWidgetsTCL)

ADD_EXECUTABLE(TestRandom TestRandom.cxx)
ADD_TEST(TestRandom ${CXX_TEST_PATH}/TestRandom)
TARGET_LINK_LIBRARIES(TestRandom KWWidgetsTCL)
