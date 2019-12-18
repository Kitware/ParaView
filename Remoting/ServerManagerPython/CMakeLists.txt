set(classes
  vtkPVPythonAlgorithmPlugin
  vtkSIPythonSourceProxy)

vtk_module_add_module(ParaView::RemotingServerManagerPython
  CLASSES ${classes})
