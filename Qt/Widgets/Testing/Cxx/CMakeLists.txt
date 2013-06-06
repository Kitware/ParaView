SET(MyTests
  CTKSliders
  FlatTreeView
  Animation
  )

QT4_WRAP_CPP(Tests_MOC_SOURCES
  SignalCatcher.h
)

INCLUDE_DIRECTORIES(${QtWidgets_SOURCE_DIR} ${QtWidgets_BINARY_DIR})
CREATE_TEST_SOURCELIST(Tests pqWidgetsTest.cxx ${MyTests}) 
vtk_module_test_executable(pqWidgetsTest QTestApp.cxx ${Tests} ${Tests_MOC_SOURCES})
FOREACH(test ${MyTests})
  ADD_TEST(
    NAME pqWidgets${test} 
    COMMAND pqWidgetsTest ${test} --exit)
  set_tests_properties(pqWidgets${test} PROPERTIES LABELS "PARAVIEW")
ENDFOREACH()
