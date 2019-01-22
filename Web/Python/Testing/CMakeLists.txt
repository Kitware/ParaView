# Make sure we can start the DataProber server
add_test(NAME pvweb-StartTest
  COMMAND
    "$<TARGET_FILE:ParaView::pvpython>"
    -m paraview.web.test_server
    --port 9739
    --timeout 1)
set_tests_properties(pvweb-StartTest PROPERTIES LABELS "PARAVIEW")
