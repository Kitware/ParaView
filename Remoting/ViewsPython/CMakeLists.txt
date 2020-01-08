set(classes
  vtkPythonRepresentation
  vtkPythonView
  vtkSMPythonViewProxy)

vtk_module_add_module(ParaView::RemotingViewsPython
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS Resources/rendering_remotingpython.xml)
