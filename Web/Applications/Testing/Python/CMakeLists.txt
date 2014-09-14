###
### Add tests for the python web services
###

find_package(PythonInterp 2.7)
if(NOT ${PYTHONINTERP_FOUND})
  message(STATUS "Sufficient Python version not found, disabling pvweb tests")
  return()
endif()

include(ParaViewTestingMacros)
#include(FindPythonModules)

# Ensure data exists for the listFiles tests. These files are selected for
# being tiny since their names are the only required part for the test.
paraview_test_load_data_dirs(""
  GMV # 24K
  )
paraview_test_load_data(""
  sonic.pht # 279 bytes
  can.ex2   # ~16.8 MB
  )

set (pv_pymodule_build_dir ${VTK_BUILD_PYTHON_MODULE_DIR})

# Make sure we can start the DataProber server
add_test(NAME pvweb-nobrowser.DataProber-StartTest
  COMMAND $<TARGET_FILE:pvpython>
  ${pv_pymodule_build_dir}/paraview/web/pv_web_data_prober.py
  --data-dir ${PARAVIEW_DATA_ROOT}
  --port 8077
  --timeout 1)
set_tests_properties(pvweb-nobrowser.DataProber-StartTest PROPERTIES LABELS "PARAVIEW")

# Make sure we can start the Visualizer server
add_test(NAME pvweb-nobrowser.Visualizer-StartTest
  COMMAND $<TARGET_FILE:pvpython>
  ${pv_pymodule_build_dir}/paraview/web/pv_web_visualizer.py
  --port 8078
  --timeout 1)
set_tests_properties(pvweb-nobrowser.Visualizer-StartTest PROPERTIES LABELS "PARAVIEW")

# Make sure we can start the FileLoader server
add_test(NAME pvweb-nobrowser.FileViewer-StartTest
  COMMAND $<TARGET_FILE:pvpython>
  ${pv_pymodule_build_dir}/paraview/web/pv_web_file_loader.py
  --port 8079
  --timeout 1)
set_tests_properties(pvweb-nobrowser.FileViewer-StartTest PROPERTIES LABELS "PARAVIEW")

###
### Add some tests which interact with the WebVisualizer
###

# COMMENT:
#    => New Web Visualizer tests are directly in the test web app
#
# set(WEB_TESTS_WITH_CHROME
#   ${pv_pymodule_build_dir}/paraview/web/test/test_xxx_.py
#   )
#
# paraview_test_load_baselines_dirs(""
#   WebTesting/ParaView
#   )
#
# add_pvweb_tests("pvweb"
#   APP          Visualizer
#   COMMAND      $<TARGET_FILE:pvpython>
#   SERVER       ${pv_pymodule_build_dir}/paraview/web/pv_web_visualizer.py
#   BROWSER      chrome
#                firefox
#                internet_explorer
#                safari
#   BASELINE_DIR ${PARAVIEW_TEST_OUTPUT_BASELINE_DIR}/WebTesting/ParaView
#   TEST_SCRIPTS ${WEB_TESTS_WITH_CHROME}
#   )

###
### Add some tests which interact with the TestApp
###

set(TESTAPP_WEB_TESTS_WITH_CHROME
  ${pv_pymodule_build_dir}/paraview/web/test/test_pv_web_testapp_all.py
  )

add_pvweb_tests("pvweb"
  APP          TestApp
  COMMAND      $<TARGET_FILE:pvpython>
  SERVER       ${pv_pymodule_build_dir}/paraview/web/pv_web_test_app.py
  BROWSER      firefox
               chrome
               internet_explorer
               safari
  TEST_SCRIPTS ${TESTAPP_WEB_TESTS_WITH_CHROME}
  )
