find_package(Qt5 REQUIRED COMPONENTS Widgets)

set(interfaces)
set(sources
  ExampleDockPanel.cxx
  ExampleDockPanel.h)

paraview_plugin_add_dock_window(
  CLASS_NAME ExampleDockPanel
  DOCK_AREA Right
  INTERFACES dock_interfaces
  SOURCES dock_sources)
list(APPEND interfaces
  ${dock_interfaces})
list(APPEND sources
  ${dock_sources})

# If UI files are not in the current source or binary directory, they may be
# searched for by adding its directory to `CMAKE_AUTOUIC_SEARCH_PATHS`.
list(APPEND CMAKE_AUTOUIC_SEARCH_PATHS
  "${CMAKE_CURRENT_SOURCE_DIR}/UIs")

paraview_add_plugin(ExampleDockPanel
  VERSION "1.0"
  UI_FILES UIs/ExampleDockPanel.ui
  UI_INTERFACES ${interfaces}
  SOURCES ${sources})

target_link_libraries(ExampleDockPanel
  PRIVATE
    Qt5::Widgets)
