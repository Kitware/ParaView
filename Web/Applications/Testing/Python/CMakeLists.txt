###
### Add tests for the python web services
###

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

# An interactive test which uses selenium to run a browser and do an image compare
add_test(NAME pvweb-Visualizer-SeleniumConnect
  COMMAND $<TARGET_FILE:pvpython>
  ${pv_pymodule_build_dir}/paraview/web/pv_web_visualizer.py
  --content ${ParaView_BINARY_DIR}/www
  --port 8080
  --data-dir ${PARAVIEW_DATA_ROOT}/Data
  --run-test-script ${pv_pymodule_build_dir}/paraview/web/test/test_pv_web_visualizer_renderer_click.py
  --baseline-img-dir ${PARAVIEW_DATA_ROOT}/Baseline/WebTesting/ParaView)
set_tests_properties(pvweb-Visualizer-SeleniumConnect PROPERTIES LABELS "PARAVIEW")

# Another interactive test using selenium to load a paraview file
add_test(NAME pvweb-Visualizer-SeleniumFileOpen
  COMMAND $<TARGET_FILE:pvpython>
  ${pv_pymodule_build_dir}/paraview/web/pv_web_visualizer.py
  --content ${ParaView_BINARY_DIR}/www
  --port 8080
  --data-dir ${PARAVIEW_DATA_ROOT}/Data
  --run-test-script ${pv_pymodule_build_dir}/paraview/web/test/test_pv_web_visualizer_can_test.py
  --baseline-img-dir ${PARAVIEW_DATA_ROOT}/Baseline/WebTesting/ParaView)
set_tests_properties(pvweb-Visualizer-SeleniumFileOpen PROPERTIES LABELS "PARAVIEW")

###
### Add some tests which interact with the TestApp
###

# A basic test which just retrieves the index.html and reads it
add_test(NAME pvweb-TestApp-BasicConnect
  COMMAND $<TARGET_FILE:pvpython>
  ${pv_pymodule_build_dir}/paraview/web/pv_web_test_app.py
  --content ${ParaView_BINARY_DIR}/www
  --port 8080
  --data-dir ${PARAVIEW_DATA_ROOT}/Data
  --run-test-script ${pv_pymodule_build_dir}/paraview/web/test/test_pv_web_test_retrieve_file_check_includes.py)
set_tests_properties(pvweb-TestApp-BasicConnect PROPERTIES LABELS "PARAVIEW")
