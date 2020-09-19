set(classes
  vtkPVLookingGlassSettings)

vtk_module_add_module(LookingGlass::Settings
  CLASSES ${classes}
)

vtk_module_link(LookingGlass::Settings
  PRIVATE
    ParaView::pqApplicationComponents
    ParaView::RemotingCore
    ParaView::RemotingServerManager
    ParaView::RemotingViews
    VTK::CommonCore
)

paraview_add_server_manager_xmls(
  XMLS LookingGlass.xml
)
