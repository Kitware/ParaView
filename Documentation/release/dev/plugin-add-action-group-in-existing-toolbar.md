# Add action group in existing toolbar with plugins

You can now use `paraview_plugin_add_action_group` CMake macro to add action groups to existing toolbars as it is already possible with menus.

For example, the following code will add an action in ParaView main control toolbar instead of a new one:
```cmake
paraview_plugin_add_action_group(
  CLASS_NAME MyToolBarActions
  GROUP_NAME "ToolBar/MainControlsToolbar"
  INTERFACES action_interfaces
  SOURCES action_sources)
```
