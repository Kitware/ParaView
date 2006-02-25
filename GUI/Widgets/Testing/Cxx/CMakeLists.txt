INCLUDE("${KWWidgets_CMAKE_DIR}/KWWidgetsTestingMacros.cmake")

ADD_EXECUTABLE(TestRegistry TestRegistry.cxx)
TARGET_LINK_LIBRARIES(TestRegistry ${KWWidgets_LIBRARIES})
KWWidgets_ADD_TEST_WITH_LAUNCHER(TestRegistry TestRegistry)

ADD_EXECUTABLE(TestRandom TestRandom.cxx)
TARGET_LINK_LIBRARIES(TestRandom ${KWWidgets_LIBRARIES})
KWWidgets_ADD_TEST_WITH_LAUNCHER(TestRandom TestRandom)
