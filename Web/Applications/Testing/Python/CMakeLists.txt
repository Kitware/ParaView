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
add_test(NAME pvweb-Visualizer
  COMMAND $<TARGET_FILE:pvpython>
  ${pv_pymodule_build_dir}/paraview/web/pv_web_visualizer.py
  --timeout 1)
set_tests_properties(pvweb-Visualizer PROPERTIES LABELS "PARAVIEW")

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
