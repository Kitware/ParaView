###
### Add tests for the python web services
###

include(ParaViewTestingMacros)

set (pv_pymodule_build_dir ${VTK_BUILD_PYTHON_MODULE_DIR})

# Make sure we can start the DataProber server
add_test(NAME pvweb-StartTest
  COMMAND $<TARGET_FILE:pvpython>
  ${pv_pymodule_build_dir}/paraview/web/test_server.py
  --port 9739
  --timeout 1)
set_tests_properties(pvweb-StartTest PROPERTIES LABELS "PARAVIEW")
