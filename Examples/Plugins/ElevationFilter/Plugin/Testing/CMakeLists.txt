if (PARAVIEW_USE_QT)
  set (TestXML_USES_DIRECT_DATA ON)
  paraview_add_client_tests(
    LOAD_PLUGIN ElevationFilter
    PLUGIN_PATH $<TARGET_FILE_DIR:ElevationFilter>
    BASELINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline
    TEST_SCRIPTS TestXML.xml)
endif()

if (PARAVIEW_USE_PYTHON)
  file(GENERATE
       OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/TestPython.py
       INPUT ${CMAKE_CURRENT_SOURCE_DIR}/TestPython.py.in)
  set(_vtk_build_TEST_FILE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  paraview_add_test_python(
    NO_RT DIRECT_DATA
    TestPython.py)
endif()
