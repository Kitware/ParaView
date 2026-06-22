if (TARGET ParaView::paraview)
  set(PythonAlgorithmPluginProxies_USES_DIRECT_DATA ON)

  paraview_add_client_tests(
    LOAD_PLUGIN PythonAlgorithm
    PLUGIN_PATH $<TARGET_FILE_DIR:PythonAlgorithm>
    TEST_SCRIPTS PythonAlgorithmPluginProxies.xml)
endif()
