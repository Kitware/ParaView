set(classes
  vtkPVPythonAlgorithmPlugin
  vtkSIPythonSourceProxy)

vtk_module_add_module(ParaView::PythonAlgorithm
  CLASSES ${classes})
