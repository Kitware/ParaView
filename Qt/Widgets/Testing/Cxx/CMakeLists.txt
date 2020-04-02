SET(MyTests
  Animation
  FlatTreeView
  HeaderViewCheckState
  TreeViewSelectionAndCheckState
  )

set(TestsNoArgs
  DoubleLineEdit
  HierarchicalGridLayout)

SET(MocSources
  DoubleLineEdit.h
  HierarchicalGridLayout.h
  LogViewerWidget.h
  SignalCatcher.h)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  # QtTest fails to compile on ICC. Hence we don't add this test on
  # Intel compilers.
  list(APPEND TestsNoArgs pqTextEditTest)
  list(APPEND MocSources pqTextEditTest.h)
endif()

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Test)
set(CMAKE_AUTOMOC 1)

CREATE_TEST_SOURCELIST(Tests pqWidgetsTest.cxx ${MyTests} ${TestsNoArgs})

vtk_module_test_executable(pqWidgetsTest QTestApp.cxx ${Tests} ${Tests_MOC_SOURCES})
target_link_libraries(pqWidgetsTest PRIVATE Qt5::Core Qt5::Widgets Qt5::Test)

FOREACH(test ${MyTests})
  ADD_TEST(
    NAME pqWidgets${test}
    COMMAND pqWidgetsTest ${test} --exit)
ENDFOREACH()

foreach(test IN LISTS TestsNoArgs)
  add_test(
    NAME pqWidgets${test}
    COMMAND pqWidgetsTest ${test})
endforeach()

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  # since serial since this relies on focus.
  set_tests_properties(pqWidgetspqTextEditTest PROPERTIES RUN_SERIAL ON)
endif()
