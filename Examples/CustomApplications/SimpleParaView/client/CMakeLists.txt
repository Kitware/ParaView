set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTOUIC 1)

set(sources
  myMainWindow.cxx
  myMainWindow.h)
set(ui_files
  myMainWindow.ui)
set(xml_files
  ParaViewFilters.xml)

# The main cmake macro to create a paraview based application
paraview_client_add(
  NAME simple_paraview
  VERSION "1.0.1"
  SOURCES ${sources} ${ui_files}
  PLUGINS_TARGETS simple_paraview_plugins
  REQUIRED_PLUGINS SimpleParaViewCustomFilters
  APPLICATION_XMLS ${xml_files}
  MAIN_WINDOW_CLASS myMainWindow
  ORGANIZATION "Kitware SAS."
  TITLE "Simple ParaView")
