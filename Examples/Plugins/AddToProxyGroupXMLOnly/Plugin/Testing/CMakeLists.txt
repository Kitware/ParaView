if (TARGET ParaView::paraview)
  set (TestAddToProxyGroupXMLOnlyState_USES_DIRECT_DATA ON)
  paraview_add_client_tests(
    LOAD_PLUGIN AddToProxyGroupXMLOnly 
    PLUGIN_PATH $<TARGET_FILE_DIR:AddToProxyGroupXMLOnly>
    TEST_SCRIPTS TestAddToProxyGroupXMLOnlyState.xml)
endif()
