if (TARGET ParaView::paraview)
  set(TestMultiOuputPortPointPicker_USES_DIRECT_DATA ON)
  paraview_add_client_tests(
    LOAD_PLUGIN MultiOutputPortReader
    PLUGIN_PATH $<TARGET_FILE_DIR:MultiOutputPortReader>
    BASELINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline
    TEST_SCRIPTS "TestMultiOuputPortPointPicker.xml")

  set_tests_properties(pv.TestMultiOuputPortPointPicker
    PROPERTIES ENVIRONMENT "PARAVIEW_DATA_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/Data")
endif()

if (PARAVIEW_USE_PYTHON)
  file(GENERATE
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/TestMultiOutputPortReader.py
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/TestMultiOutputPortReader.py.in)

  set(_vtk_build_TEST_FILE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

  paraview_add_test_python(
    NO_RT DIRECT_DATA
    TestMultiOutputPortReader.py)

  set_tests_properties(Python-TestMultiOutputPortReader
    PROPERTIES ENVIRONMENT "PARAVIEW_DATA_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/Data")
endif()
