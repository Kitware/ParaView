###
### Add tests for the python web services
###

include(ParaViewTestingMacros)
include(FindPythonModules)

set (pv_pymodule_build_dir ${VTK_BUILD_PYTHON_MODULE_DIR})

# Make sure we can start the DataProber server
add_test(NAME pvweb-DataProber
  COMMAND $<TARGET_FILE:pvpython>
  ${pv_pymodule_build_dir}/paraview/web/pv_web_data_prober.py
  #--data-dir ${VTK_TEST_DATA_DIR}
  --data-dir ${PARAVIEW_DATA_ROOT}
  --timeout 1)
set_tests_properties(pvweb-DataProber PROPERTIES LABELS "PARAVIEW")

# Make sure we can start the Visualizer server
add_test(NAME pvweb-Visualizer-StartTest
  COMMAND $<TARGET_FILE:pvpython>
  ${pv_pymodule_build_dir}/paraview/web/pv_web_visualizer.py
  --timeout 1)
set_tests_properties(pvweb-Visualizer-StartTest PROPERTIES LABELS "PARAVIEW")

# Make sure we can start the FileLoader server
add_test(NAME pvweb-FileLoader
  COMMAND $<TARGET_FILE:pvpython>
  ${pv_pymodule_build_dir}/paraview/web/pv_web_file_loader.py
  --timeout 1)
set_tests_properties(pvweb-FileLoader PROPERTIES LABELS "PARAVIEW")

# Make sure we can start the Parallel server
add_test(NAME pvweb-ParallelViz
  COMMAND $<TARGET_FILE:pvpython>
  ${pv_pymodule_build_dir}/paraview/web/pv_web_parallel.py
  --timeout 1)
set_tests_properties(pvweb-ParallelViz PROPERTIES LABELS "PARAVIEW")


###
### Add some tests which interact with the WebVisualizer
###

set(WEB_TESTS_WITH_CHROME
  ${pv_pymodule_build_dir}/paraview/web/test/test_pv_web_visualizer_renderer_click.py
  ${pv_pymodule_build_dir}/paraview/web/test/test_pv_web_visualizer_can_test.py
  )

add_pvweb_tests("pvweb"
  APP          WebVisualizer
  COMMAND      $<TARGET_FILE:pvpython>
  SERVER       ${pv_pymodule_build_dir}/paraview/web/pv_web_visualizer.py
  DEPEND_MODS  Image
               selenium
  BROWSER      chrome
               firefox
               ie
               safari
  BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline/WebTesting/ParaView
  TEST_SCRIPTS ${WEB_TESTS_WITH_CHROME}
  )

###
### Add some tests which interact with the TestApp
###

set(WEB_TESTS_WITH_REQUESTS
  ${pv_pymodule_build_dir}/paraview/web/test/test_pv_web_test_retrieve_file_check_includes.py
  )

add_pvweb_tests("pvweb"
  APP          TestApp
  COMMAND      $<TARGET_FILE:pvpython>
  SERVER       ${pv_pymodule_build_dir}/paraview/web/pv_web_test_app.py
  ARGS         --echo "test string to be retrieved by app"
  DEPEND_MODS  requests
  TEST_SCRIPTS ${WEB_TESTS_WITH_REQUESTS}
  )
