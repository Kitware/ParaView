# This plugin contains tools developed for
# The Computational Solid Mechanics & Dynamics group(s) at Sandia National Labortories
# to make it easier to plot variables from Sierra-based finite
# element simulations.
# Author:  Jon Goldman
# Created:  Aug 2009 -to- Jan 2010

set(ui_files
  # container for plot tools actions
  pqSierraPlotToolsActionHolder.ui
  pqSierraPlotToolsDataLoadManager.ui
  # container for Rich Text documents
  pqSierraToolsRichTextDocs.ui
  pqVariablePlot.ui)

set(sources
  pqElementPlotter.cxx
  pqElementPlotter.h
  pqGlobalPlotter.cxx
  pqGlobalPlotter.h
  pqHoverLabel.cxx
  pqHoverLabel.h
  pqNodePlotter.cxx
  pqNodePlotter.h
  pqPlotter.cxx
  pqPlotter.h
  pqPlotVariablesDialog.cxx
  pqPlotVariablesDialog.h
  pqResizingScrollArea.cxx
  pqResizingScrollArea.h
  pqSierraPlotToolsActionGroup.cxx
  pqSierraPlotToolsActionGroup.h
  pqSierraPlotToolsDataLoadManager.cxx
  pqSierraPlotToolsDataLoadManager.h
  pqSierraPlotToolsManager.cxx
  pqSierraPlotToolsManager.h
  pqSierraPlotToolsUtils.cxx
  pqSierraPlotToolsUtils.h
  pqVariableVariablePlotter.cxx
  pqVariableVariablePlotter.h
  warningState.h)

set(qrc_files
  SierraPlotTools.qrc)

set(interfaces)
paraview_plugin_add_action_group(
  CLASS_NAME pqSierraPlotToolsActionGroup
  GROUP_NAME "ToolBar/SierraTools"
  INTERFACES action_interfaces
  SOURCES action_sources)
list(APPEND interfaces
  ${action_interfaces})
list(APPEND sources
  ${action_sources})

paraview_add_plugin(SierraPlotTools
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  UI_RESOURCES ${qrc_files}
  UI_FILES ${ui_files}
  SOURCES ${sources})

target_link_libraries(SierraPlotTools
  PRIVATE
    ParaView::ClientServerCoreCore
    ParaView::ServerManagerCore
    ParaView::ServerManagerRendering
    ParaView::VTKExtensionsDefault
    VTK::CommonCore
    VTK::CommonDataModel
    VTK::IOExodus)
