set(classes
  vtkSharedUtils
  vtkMyOtherElevationFilter)

vtk_module_add_module(ComplexPluginArchitecture::Shared
  CLASSES ${classes})
