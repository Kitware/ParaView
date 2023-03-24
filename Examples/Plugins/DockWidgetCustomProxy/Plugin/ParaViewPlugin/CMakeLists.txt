find_package("Qt${PARAVIEW_QT_MAJOR_VERSION}" REQUIRED COMPONENTS Widgets)

set(interfaces)
set(sources
  pqWriterDockPanel.cxx
  pqWriterDockPanel.h)

paraview_plugin_add_dock_window(
  CLASS_NAME pqWriterDockPanel
  DOCK_AREA Right
  INTERFACES dock_interfaces
  SOURCES dock_sources)
list(APPEND interfaces
  ${dock_interfaces})
list(APPEND sources
  ${dock_sources})

paraview_add_plugin(DockWidgetCustomProxy
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  SERVER_MANAGER_XML string_writer.xml
  MODULES
    DockWidgetCustomProxy::Writer
  MODULE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/../Writer/vtk.module"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources})

target_link_libraries(DockWidgetCustomProxy
  PRIVATE
    DockWidgetCustomProxy::Writer
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Widgets")
