###
### Add tests for the python web services
###

find_package(PythonInterp 2.7)
if(NOT ${PythonInterp_FOUND})
  message(STATUS "Sufficient Python version not found, disabling pvweb tests")
  return()
endif()

include(ParaViewTestingMacros)
include(FindPythonModules)

set (pv_pymodule_build_dir ${VTK_BUILD_PYTHON_MODULE_DIR})

# Make sure we can start the DataProber server
add_test(NAME pvweb-DataProber-StartTest
  COMMAND $<TARGET_FILE:pvpython>
  ${pv_pymodule_build_dir}/paraview/web/pv_web_data_prober.py
  #--data-dir ${VTK_TEST_DATA_DIR}
  --data-dir ${PARAVIEW_DATA_ROOT}
  --timeout 1)
set_tests_properties(pvweb-DataProber-StartTest PROPERTIES LABELS "PARAVIEW")

# Make sure we can start the Visualizer server
add_test(NAME pvweb-Visualizer-StartTest
  COMMAND $<TARGET_FILE:pvpython>
  ${pv_pymodule_build_dir}/paraview/web/pv_web_visualizer.py
  --timeout 1)
set_tests_properties(pvweb-Visualizer-StartTest PROPERTIES LABELS "PARAVIEW")

# Make sure we can start the FileLoader server
add_test(NAME pvweb-FileViewer-StartTest
  COMMAND $<TARGET_FILE:pvpython>
  ${pv_pymodule_build_dir}/paraview/web/pv_web_file_loader.py
  --timeout 1)
set_tests_properties(pvweb-FileViewer-StartTest PROPERTIES LABELS "PARAVIEW")

###
### Add some tests which interact with the WebVisualizer
###

set(WEB_TESTS_WITH_CHROME
  ${pv_pymodule_build_dir}/paraview/web/test/test_pv_web_visualizer_renderer_click.py
  ${pv_pymodule_build_dir}/paraview/web/test/test_pv_web_visualizer_can_test.py
  )

add_pvweb_tests("pvweb"
  APP          Visualizer
  COMMAND      $<TARGET_FILE:pvpython>
  SERVER       ${pv_pymodule_build_dir}/paraview/web/pv_web_visualizer.py
  DEPEND_MODS  Image
               selenium
  BROWSER      chrome
               firefox
               internet_explorer
               safari
  BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline/WebTesting/ParaView
  TEST_SCRIPTS ${WEB_TESTS_WITH_CHROME}
  )

###
### Add some tests which interact with the TestApp
###

set(WEB_TESTS_WITH_REQUESTS
  ${pv_pymodule_build_dir}/paraview/web/test/test_pv_web_testapp_retrieve_file_check_includes.py
  )

add_pvweb_tests("pvweb"
  APP          TestApp
  COMMAND      $<TARGET_FILE:pvpython>
  SERVER       ${pv_pymodule_build_dir}/paraview/web/pv_web_test_app.py
  ARGS         --echo "test string to be retrieved by app"
  DEPEND_MODS  requests
  TEST_SCRIPTS ${WEB_TESTS_WITH_REQUESTS}
  )

set(TESTAPP_WEB_TESTS_WITH_CHROME
  ${pv_pymodule_build_dir}/paraview/web/test/test_pv_web_testapp_all.py
  )

add_pvweb_tests("pvweb"
  APP          TestApp
  COMMAND      $<TARGET_FILE:pvpython>
  SERVER       ${pv_pymodule_build_dir}/paraview/web/pv_web_test_app.py
  DEPEND_MODS  selenium
  BROWSER      chrome
               firefox
               internet_explorer
               safari
  TEST_SCRIPTS ${TESTAPP_WEB_TESTS_WITH_CHROME}
  )
