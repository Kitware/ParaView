# Make sure we can start the DataProber server
add_test(NAME pvweb-StartTest
  COMMAND
    "$<TARGET_FILE:ParaView::pvpython>"
    "${CMAKE_BINARY_DIR}/${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}/paraview/web/test_server.py"
    --port 9739
    --timeout 1)
