if (TARGET ParaView::paraview)
  set (TestHelix_USES_DIRECT_DATA ON)
  paraview_add_client_tests(
    LOAD_PLUGIN helix
    PLUGIN_PATH ${CMAKE_CURRENT_SOURCE_DIR}# $<TARGET_FILE_DIR:AddToProxyGroupXMLOnly>
    TEST_SCRIPTS TestHelix.xml)
endif()
