set(classes
  vtkSMPrismTableArraysDomain
  )

vtk_module_add_module(Prism::vtkPrismServerManager
  CLASSES ${classes}
  )
