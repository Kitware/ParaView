find_package(Qt5 REQUIRED COMPONENTS Widgets Network)

set(interfaces)
set(sources
  pvOpenVRDockPanel.cxx
  pvOpenVRDockPanel.h
  vtkPVOpenVRHelper.cxx
  vtkPVOpenVRHelper.h)

paraview_plugin_add_dock_window(
  CLASS_NAME pvOpenVRDockPanel
  DOCK_AREA Left
  INTERFACES dock_interfaces
  SOURCES dock_sources)
list(APPEND interfaces
  ${dock_interfaces})
list(APPEND sources
  ${dock_sources})

paraview_add_plugin(OpenVR
  VERSION "1.1"
  UI_FILES pvOpenVRDockPanel.ui
  UI_INTERFACES ${interfaces}
  SOURCES ${sources})

target_link_libraries(OpenVR
  PRIVATE
    ParaView::ClientServerCoreRendering
    ParaView::Core
    ParaView::ServerManagerCore
    ParaView::ServerManagerRendering
    ParaView::VTKExtensionsRendering
    VTK::CommonCore
    VTK::CommonDataModel
    VTK::IOCore
    VTK::IOImage
    VTK::IOXML
    VTK::IOXMLParser
    VTK::InteractionWidgets
    VTK::RenderingCore
    VTK::RenderingOpenGL2
    VTK::RenderingOpenVR
    VTK::ViewsCore)
