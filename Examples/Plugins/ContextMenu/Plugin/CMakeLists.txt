set(interfaces
  ExampleContextMenu
)
set(sources
  ExampleContextMenu.cxx
  ExampleContextMenu.h
)

paraview_add_plugin(ContextMenu
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources}
)
