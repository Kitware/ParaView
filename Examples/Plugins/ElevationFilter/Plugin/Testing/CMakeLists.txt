if (TARGET ParaView::paraview)
  # This tests shows how to use either
  # inline compares and baseline compares
  set(DATA_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Data)
  set (TestXML_USES_DIRECT_DATA ON)
  paraview_add_client_tests(
    LOAD_PLUGIN ElevationFilter
    PLUGIN_PATH $<TARGET_FILE_DIR:ElevationFilter>
    BASELINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline
    TEST_SCRIPTS TestXML.xml)
  set_tests_properties(pv.TestXML PROPERTIES ENVIRONMENT "PARAVIEW_DATA_ROOT=${DATA_ROOT}")

  # This is a test specific to delayed_load mechanism
  set(_plugin_config_file "${CMAKE_BINARY_DIR}/")
  if (WIN32)
    string(APPEND _plugin_config_file "${CMAKE_INSTALL_BINDIR}/")
  else ()
    string(APPEND _plugin_config_file "${CMAKE_INSTALL_LIBDIR}/")
  endif ()
  string(APPEND _plugin_config_file "${PARAVIEW_PLUGIN_SUBDIR}/elev.plugins.xml")
  configure_file(TestDelayedLoad.xml.in ${CMAKE_CURRENT_BINARY_DIR}/TestDelayedLoad.xml)
  set (TestDelayedLoad_USES_DIRECT_DATA ON)
  paraview_add_client_tests(
    BASELINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline
    TEST_SCRIPTS ${CMAKE_CURRENT_BINARY_DIR}/TestDelayedLoad.xml)
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
