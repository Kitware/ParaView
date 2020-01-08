# This plugin implements scripting that resembles commands from
# the SEACAS blot program.

set(python_modules
  async_io_helper.py
  blot_common.py
  blotish.py
  number_list_parser.py
  pvblot.py
  timestep_selection.py
  tplot.py)

set(interfaces)
set(ui_files)
set(qrc_files)
set(sources)

if (PARAVIEW_USE_QT)
  list(APPEND ui_files
    pqBlotDialog.ui)
  list(APPEND qrc_files
    pvblot.qrc)
  list(APPEND sources
    PVBlotPluginActions.cxx
    PVBlotPluginActions.h
    pqBlotDialog.cxx
    pqBlotDialog.h
    pqBlotDialog.ui
    pqBlotShell.cxx
    pqBlotShell.h)

  paraview_plugin_add_action_group(
    CLASS_NAME "PVBlotPluginActions"
    GROUP_NAME "MenuBar/Tools"
    INTERFACES group_interfaces
    SOURCES group_sources)
  list(APPEND interfaces
    ${group_interfaces})
  list(APPEND sources
    ${group_sources})
endif ()

paraview_add_plugin(pvblot
  REQUIRED_ON_CLIENT
  VERSION "1.0"
  PYTHON_MODULES ${python_modules}
  UI_INTERFACES ${interfaces}
  UI_RESOURCES ${qrc_files}
  UI_FILES ${ui_files}
  SOURCES ${sources})

if (PARAVIEW_USE_QT)
  target_link_libraries(pvblot
    PRIVATE
      ParaView::RemotingCore
      VTK::GUISupportQt
      VTK::Python)
endif ()

set(ext "")
if (WIN32 AND NOT CYGWIN)
  set(ext ".bat")
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pvblot${ext}.in.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pvblot${ext}.in"
  @ONLY)

get_property(is_multi_config GLOBAL
  PROPERTY GENERATOR_IS_MULTI_CONFIG)
set(config_subdir "")
if (is_multi_config)
  set(config_subdir "/$<CONFIGURATION>")
endif ()

file(GENERATE
  OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}${config_subdir}/pvblot${ext}"
  INPUT  "${CMAKE_CURRENT_BINARY_DIR}/pvblot${ext}.in")

# TODO: Install the pvblot script. This requires the python modules to be
# installed as well.
