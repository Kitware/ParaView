# The plugins in this example do not require compiling; the test loads them
# directly from this directory.
if (TARGET ParaView::paraview)
  set(TestCustomMenus_USES_DIRECT_DATA ON)
  paraview_add_client_tests(
    LOAD_PLUGINS AnalysisTools AnalysisToolsPython
    PLUGIN_PATH "${CMAKE_CURRENT_SOURCE_DIR}"
    TEST_SCRIPTS TestCustomMenus.xml)
endif ()
