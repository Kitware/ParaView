set(classes
  vtkPTemporalRanges
  vtkSamplePlaneSource
  vtkSLACPlaneGlyphs
  vtkTemporalRanges)

vtk_module_add_module(SLACTools::vtkSLACFilters
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  SLACToolsSM.xml)
