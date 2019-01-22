set(classes
  vtkShearedWaveletSource)

vtk_module_add_module(NonOrthogonalSource::vtkNonOrthogonalSources
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  vtkNonOrthogonalSource.xml)
