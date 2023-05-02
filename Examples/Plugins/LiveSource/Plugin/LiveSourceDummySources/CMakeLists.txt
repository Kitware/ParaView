set(classes vtkLiveSourceDummy)
vtk_module_add_module(LiveSourceDummySources CLASSES ${classes})
paraview_add_server_manager_xmls(XMLS  LiveSourceDummy.xml)
