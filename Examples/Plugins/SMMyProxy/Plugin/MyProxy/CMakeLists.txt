set(classes
  vtkSMMyBoundsDomain
  vtkSMMyElevationProxy)

vtk_module_add_module(MyProxy
  FORCE_STATIC # Using FORCE_STATIC build the vtk module statically into the plugin library, to avoid confusion when loading                      
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  MyProxy.xml)
