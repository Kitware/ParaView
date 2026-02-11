set(interfaces)
set(sources
  MyToolBar.cxx
  MyToolBar.h
  MyToolBarActions.cxx
  MyToolBarActions.h)

# If the toolbar name already exists, this macro will append the actions to it.
paraview_plugin_add_action_group(
  CLASS_NAME MyToolBarActions
  GROUP_NAME "ToolBar/MainControlsToolbar"
  INTERFACES action_interfaces
  SOURCES action_sources)
list(APPEND interfaces
  ${action_interfaces})
list(APPEND sources
  ${action_sources})

# Another way of adding a toolbar is to simply add a new QToolBar subclass.
paraview_plugin_add_toolbar(
  CLASS_NAME MyToolBar
  INTERFACES toolbar_interfaces
  SOURCES toolbar_sources)
list(APPEND interfaces
  ${toolbar_interfaces})
list(APPEND sources
  ${toolbar_sources})

paraview_add_plugin(GUIMyToolBar
  REQUIRED_ON_CLIENT
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources})

option(BUILD_TESTING "Build Plugin Testing" OFF)
if (BUILD_TESTING AND BUILD_SHARED_LIBS)
  add_subdirectory(Testing)
endif()
