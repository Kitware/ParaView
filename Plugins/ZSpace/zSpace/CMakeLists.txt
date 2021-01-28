set(classes
  vtkPVZSpaceView
  vtkZSpaceRayActor
  vtkZSpaceSDKManager
  vtkZSpaceInteractorStyle
  vtkZSpaceCamera)

vtk_module_find_package(
  PACKAGE zSpace)

vtk_module_add_module(zSpace::vtkZSpaceView
  CLASSES ${classes})

vtk_module_link(zSpace::vtkZSpaceView
  PUBLIC
  zSpace::zSpace)
    
paraview_add_server_manager_xmls(
  XMLS ZSpaceView.xml)
