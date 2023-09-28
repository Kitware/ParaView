# pvweb tests require asyncio.
if (Python3_VERSION VERSION_LESS "3.7")
  return ()
endif ()

include(ParaViewFindPythonModules)
find_python_module(wslink wslink_found)

# Make sure we can start the DataProber server
if (wslink_found)
  add_test(NAME pvweb-StartTest
    COMMAND
      "$<TARGET_FILE:ParaView::pvpython>"
        -m paraview.web.test_server
        --port 9739
        --host 0.0.0.0
        --timeout 1)
endif ()
