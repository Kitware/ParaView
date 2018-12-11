set(classes
  vtkBoundedPlaneSource
  vtkBoundedVolumeSource)

vtk_module_add_module(ParaView::VTKExtensionsPoints
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  points.xml)
