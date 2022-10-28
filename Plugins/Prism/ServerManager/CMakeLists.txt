set(classes
  vtkSMPrismIsSimulationDataDomain
  vtkSMPrismTableArraysDomain
  vtkSMPrismThresholdRangeDomain
  )

vtk_module_add_module(Prism::vtkPrismServerManager
  CLASSES ${classes}
  )
