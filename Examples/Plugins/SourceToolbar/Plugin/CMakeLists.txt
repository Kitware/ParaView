set(interfaces)
set(sources
  SourceToolbarActions.cxx
  SourceToolbarActions.h)

paraview_plugin_add_action_group(
  CLASS_NAME SourceToolbarActions
  GROUP_NAME "ToolBar/SourceToolbar"
  INTERFACES action_interfaces
  SOURCES action_sources)
list(APPEND interfaces
  ${action_interfaces})
list(APPEND sources
  ${action_sources})

paraview_add_plugin(SourceToolbar
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources})
