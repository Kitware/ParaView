set(classes
  vtkPrismGeometryConverter
  vtkSimulationPointCloudFilter
  vtkSimulationToPrismFilter
  )

vtk_module_add_module(Prism::vtkPrismFilters
  CLASSES ${classes}
  )
