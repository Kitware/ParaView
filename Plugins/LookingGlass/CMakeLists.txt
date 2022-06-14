set(interfaces)
set(sources
  pqLookingGlassDockPanel.cxx
  pqLookingGlassDockPanel.h
)

paraview_plugin_add_dock_window(
  CLASS_NAME pqLookingGlassDockPanel
  DOCK_AREA Right
  INTERFACES dock_interfaces
  SOURCES dock_sources
)
list(APPEND interfaces
  ${dock_interfaces}
)
list(APPEND sources
  ${dock_sources}
)

paraview_add_plugin(LookingGlass
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.3"
  EULA "EULA.txt"
  UI_FILES
    pqLookingGlassDockPanel.ui
  UI_INTERFACES ${interfaces}
  SOURCES ${sources}
  MODULES LookingGlass::Settings
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Settings/vtk.module"
)

target_compile_definitions(LookingGlass
  PRIVATE
    QT_NO_KEYWORDS)
target_link_libraries(LookingGlass
  PRIVATE
    ParaView::pqApplicationComponents
    ParaView::RemotingCore
    ParaView::RemotingServerManager
    ParaView::RemotingViews
    VTK::CommonCore
    VTK::CommonDataModel
    VTK::CommonSystem
    VTK::IOCore
    VTK::IOImage
    VTK::IOXML
    VTK::IOXMLParser
    VTK::InteractionWidgets
    VTK::RenderingCore
    VTK::RenderingLookingGlass
    VTK::RenderingOpenGL2
    VTK::ViewsCore
)
