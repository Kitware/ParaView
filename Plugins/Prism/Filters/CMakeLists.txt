set(classes
  vtkPrismGeometryConverter
  vtkSimulationToPrismFilter
  )

vtk_module_add_module(Prism::vtkPrismFilters
  CLASSES ${classes}
  )
