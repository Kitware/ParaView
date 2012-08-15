SET(MyTests
  FlatTreeView
  Animation
  )

INCLUDE_DIRECTORIES(${QtWidgets_SOURCE_DIR} ${QtWidgets_BINARY_DIR})
CREATE_TEST_SOURCELIST(Tests pqWidgetsTest.cxx ${MyTests}) 
vtk_module_test_executable(pqWidgetsTest QTestApp.cxx ${Tests})
FOREACH(test ${MyTests})
  ADD_TEST(
    NAME pqWidgets${test} 
    COMMAND pqWidgetsTest ${test} --exit)
  set_tests_properties(pqWidgets${test} PROPERTIES LABELS "PARAVIEW")
ENDFOREACH()
