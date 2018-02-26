###
### Add tests for the python web services
###

include(ParaViewTestingMacros)

# Make sure we can start the DataProber server
add_test(NAME pvweb-StartTest
  COMMAND $<TARGET_FILE:pvpython>
  ${VTK_BUILD_PYTHON_MODULES_DIR}/paraview/web/test_server.py
  --port 9739
  --timeout 1)
set_tests_properties(pvweb-StartTest PROPERTIES LABELS "PARAVIEW")
