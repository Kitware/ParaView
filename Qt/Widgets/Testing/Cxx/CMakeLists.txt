SET(MyTests
  FlatTreeView.cxx
  HeaderViewCheckState.cxx
  TreeViewSelectionAndCheckState.cxx
  )

set(TestsNoArgs
  DoubleLineEdit.cxx
  HierarchicalGridLayout.cxx)

SET(MocSources
  DoubleLineEdit.h
  HierarchicalGridLayout.h
  LogViewerWidget.h
  SignalCatcher.h)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  # QtTest fails to compile on ICC. Hence we don't add this test on
  # Intel compilers.
  list(APPEND TestsNoArgs pqTextEditTest.cxx)
  list(APPEND MocSources pqTextEditTest.h)
endif()

find_package("Qt${PARAVIEW_QT_MAJOR_VERSION}" REQUIRED COMPONENTS Core Widgets Test)
set(CMAKE_AUTOMOC 1)

CREATE_TEST_SOURCELIST(Tests pqWidgetsTest.cxx ${MyTests} ${TestsNoArgs})

vtk_module_test_executable(pqWidgetsTest QTestApp.cxx ${Tests} ${Tests_MOC_SOURCES})
target_link_libraries(pqWidgetsTest PRIVATE "Qt${PARAVIEW_QT_MAJOR_VERSION}::Core" "Qt${PARAVIEW_QT_MAJOR_VERSION}::Widgets" "Qt${PARAVIEW_QT_MAJOR_VERSION}::Test")

foreach(fname IN LISTS MyTests)
  get_filename_component(tname "${fname}" NAME_WE)
  add_test(
    NAME pqWidgets${tname}
    COMMAND pqWidgetsTest ${tname} --exit)
endforeach()

foreach(fname IN LISTS TestsNoArgs)
  get_filename_component(tname "${fname}" NAME_WE)
  add_test(
    NAME pqWidgets${tname}
    COMMAND pqWidgetsTest ${tname})
endforeach()

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  # since serial since this relies on focus.
  set_tests_properties(pqWidgetspqTextEditTest PROPERTIES RUN_SERIAL ON)
endif()
