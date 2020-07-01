include(FindPythonModules)
find_python_module(twisted twisted_found)

# Make sure we can start the DataProber server
if (twisted_found)
    add_test(NAME pvweb-StartTest
      COMMAND
        "$<TARGET_FILE:ParaView::pvpython>"
        -m paraview.web.test_server
        --port 9739
        --timeout 1)
endif()
