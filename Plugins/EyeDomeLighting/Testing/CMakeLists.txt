if (PARAVIEW_BUILD_QT_GUI)
  set(tests
    EyeDomeLighting.xml)

  paraview_add_client_tests(
    LOAD_PLUGIN "EyeDomeLighting"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${tests}
  )
  paraview_add_client_server_tests(
   LOAD_PLUGIN "EyeDomeLighting"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${tests}
  )
  paraview_add_client_server_render_tests(
   LOAD_PLUGIN "EyeDomeLighting"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${tests}
  )
endif ()

if (PARAVIEW_BUILD_QT_GUI AND PARAVIEW_USE_MPI)
  set(tests
    EDLWithSubsampling.xml)

  paraview_add_client_server_tests(
   LOAD_PLUGIN "EyeDomeLighting"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${tests}
  )
  paraview_add_client_server_render_tests(
   LOAD_PLUGIN "EyeDomeLighting"
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS ${tests}
  )
endif()

if (TARGET ParaView::pvpython)
  # this test is not specific to the EyeDomeLighting plugin, it is meant
  # to test that we can reload a plugin multiple times without a crash.
  # for complex Python scripts we may not know that a plugin has already
  # been loaded.
  add_test(NAME ReloadPlugin
    COMMAND $<TARGET_FILE:ParaView::pvpython>
            "${CMAKE_CURRENT_SOURCE_DIR}/ReloadPlugin.py")
  set_tests_properties(ReloadPlugin PROPERTIES LABELS EyeDomeLighting)
endif ()
