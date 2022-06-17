set(interfaces)
set(sources
  pqMyToolBar.cxx
  pqMyToolBar.h)

paraview_plugin_add_toolbar(
  CLASS_NAME pqMyToolBar
  INTERFACES toolbar_interfaces
  SOURCES toolbar_sources)
list(APPEND interfaces
  ${toolbar_interfaces})
list(APPEND sources
  ${toolbar_sources})

paraview_add_plugin(ComplexGUIMyToolBar
  REQUIRED_ON_CLIENT
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources})

target_link_libraries(ComplexGUIMyToolBar PRIVATE ComplexPluginArchitecture::Shared)
