set(TEST_ROOT ${CMAKE_BINARY_DIR}/Testing/Temporary)

configure_file(
  TestButtonPythonQt.py
  ${TEST_ROOT}/TestButtonPythonQt.py COPYONLY)
configure_file(
  ResetCameraPythonQt.py
  ${TEST_ROOT}/ResetCameraPythonQt.py COPYONLY)

paraview_add_client_tests(
  LOAD_PLUGIN "PythonQtPlugin"
  BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_SCRIPTS "TestPythonQt.xml"
)
