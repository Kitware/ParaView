SET(MyTests
  CTKSliders
  FlatTreeView
  Animation
  )

IF(PARAVIEW_QT_VERSION VERSION_GREATER "4")
  QT5_WRAP_CPP(Tests_MOC_SOURCES
    SignalCatcher.h
  )
ELSE()
  QT4_WRAP_CPP(Tests_MOC_SOURCES
    SignalCatcher.h
  )
ENDIF()

INCLUDE_DIRECTORIES(${QtWidgets_SOURCE_DIR} ${QtWidgets_BINARY_DIR})
CREATE_TEST_SOURCELIST(Tests pqWidgetsTest.cxx ${MyTests})
vtk_module_test_executable(pqWidgetsTest QTestApp.cxx ${Tests} ${Tests_MOC_SOURCES})
IF(PARAVIEW_QT_VERSION VERSION_GREATER "4")
  SET_TARGET_PROPERTIES(pqWidgetsTest PROPERTIES
    COMPILE_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
ENDIF()
FOREACH(test ${MyTests})
  ADD_TEST(
    NAME pqWidgets${test}
    COMMAND pqWidgetsTest ${test} --exit)
  set_tests_properties(pqWidgets${test} PROPERTIES LABELS "PARAVIEW")
ENDFOREACH()
