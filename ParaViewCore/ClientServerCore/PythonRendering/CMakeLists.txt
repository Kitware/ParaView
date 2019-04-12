set(classes
  vtkPythonRepresentation
  vtkPythonView)

vtk_module_add_module(ParaView::ClientServerCorePythonRendering
  CLASSES ${classes})
