set(classes
  vtkPythonRepresentation
  vtkPythonView
  vtkSMPythonViewProxy)

vtk_module_add_module(ParaView::RemotingViewsPython
  CLASSES ${classes})
