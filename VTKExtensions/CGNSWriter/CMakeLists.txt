set(classes
  vtkCGNSWriter)

vtk_module_add_module(ParaView::VTKExtensionsCGNSWriter
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  resources/CGNSWriter.xml)
