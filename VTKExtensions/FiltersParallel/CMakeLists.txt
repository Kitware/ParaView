set(classes
  vtkPVGenerateProcessIds
  vtkPVRemoveGhosts)

vtk_module_add_module(ParaView::VTKExtensionsFiltersParallel
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  Resources/parallel_filters.xml)
