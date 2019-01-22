# Plugins for ParaView that add support to export state for spatio-temporal
# parallelism
#------------------------------------------------------------------------------
set(sources
  pqTPActionsGroup.cxx
  pqTPActionsGroup.h
  pqTPExportStateWizard.cxx
  pqTPExportStateWizard.h
  pqTPPluginManager.cxx
  pqTPPluginManager.h)

set(interfaces)
paraview_plugin_add_action_group(
  CLASS_NAME "pqTPActionsGroup"
  GROUP_NAME "MenuBar/SpatioTemporal"
  INTERFACES group_interfaces
  SOURCES group_sources)
list(APPEND interfaces
  ${group_interfaces})
list(APPEND sources
  ${group_sources})

paraview_plugin_add_auto_start(
  CLASS_NAME "pqTPPluginManager"
  INTERFACES auto_start_interfaces
  SOURCES auto_start_sources)
list(APPEND interfaces
  ${auto_start_interfaces})
list(APPEND sources
  ${auto_start_sources})

vtk_encode_string(
  INPUT "tp_export.py"
  NAME  "tp_export_py"
  HEADER_OUTPUT export_header
  SOURCE_OUTPUT export_source)
list(APPEND sources
  "${export_header}"
  "${export_source}")

paraview_add_plugin(TemporalParallelismScriptGenerator
  VERSION "0.1.0"
  SERVER_MANAGER_XML Resources/servermanagertp.xml
  UI_INTERFACES ${interfaces}
  SOURCES ${sources})
target_link_libraries(TemporalParallelismScriptGenerator
  PRIVATE
    ParaView::Core
    ParaView::ServerManagerCore
    ParaView::ServerManagerRendering)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
