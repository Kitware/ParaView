set(classes
  vtkPVRenderViewWithEDL)

vtk_module_add_module(EyeDomeLighting::EyeDomeLightingView
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  RenderPassViews.xml)
