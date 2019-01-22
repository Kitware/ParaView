set(classes
  vtkPMaterialClusterAnalysisFilter
  vtkPMaterialClusterExplodeFilter)

vtk_module_add_module(DigitalRockPhysics::DigitalRocksFilters
  CLASSES ${classes})

paraview_add_server_manager_xmls(
    XMLS  DigitalRockPhysics.xml)
