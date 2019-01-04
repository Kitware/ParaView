set(classes
  vtkMooseXfemClip)

vtk_module_add_module(MooseXfemClip::vtkMooseXfemClip
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  MooseXfemClip.xml)
