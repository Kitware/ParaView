ADD_EXECUTABLE(TestRegistry TestRegistry.cxx)
ADD_TEST(TestRegistry ${EXECUTABLE_OUTPUT_PATH}/TestRegistry)
TARGET_LINK_LIBRARIES(TestRegistry ${KWWidgets_LIBRARIES})

ADD_EXECUTABLE(TestRandom TestRandom.cxx)
ADD_TEST(TestRandom ${EXECUTABLE_OUTPUT_PATH}/TestRandom)
TARGET_LINK_LIBRARIES(TestRandom ${KWWidgets_LIBRARIES})
