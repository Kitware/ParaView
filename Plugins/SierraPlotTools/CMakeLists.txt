# This plugin contains tools developed for
# The Computational Solid Mechanics & Dynamics group(s) at Sandia National Labortories
# to make it easier to plot variables from Sierra-based finite
# element simulations.
# Author:  Jon Goldman
# Created:  Aug 2009 -to- Jan 2009

SET(PLUGIN_NAME SierraPlotTools)
SET(PLUGIN_VERSION "1.0")

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${VTK_INCLUDE_DIR}
  )

SET(UI_FILES
  pqSierraPlotToolsActionHolder.ui
  pqSierraPlotToolsDataLoadManager.ui
  pqVariablePlot.ui
  )

SET(HEADERS
  pqPlotter.h
  )

SET(MOC_HEADERS
  pqSierraPlotToolsActionGroup.h
  pqSierraPlotToolsDataLoadManager.h
  pqSierraPlotToolsManager.h
  pqPlotVariablesDialog.h
  pqPlotter.h
  pqNodePlotter.h
  pqGlobalPlotter.h
  pqElementPlotter.h
  pqVariableVariablePlotter.h
  )

SET(HEADERS
  ${MOC_HEADERS}
  pqSierraPlotToolsUtils.h
  )

SET(CLIENT_SRCS
  pqSierraPlotToolsActionGroup.cxx
  pqSierraPlotToolsDataLoadManager.cxx
  pqSierraPlotToolsManager.cxx
  pqPlotVariablesDialog.cxx
  pqPlotter.cxx
  pqNodePlotter.cxx
  pqGlobalPlotter.cxx
  pqElementPlotter.cxx
  pqSierraPlotToolsUtils.cxx
  pqVariableVariablePlotter.cxx
  )

IF (PARAVIEW_BUILD_QT_GUI)

  INCLUDE(${QT_USE_FILE})

  INCLUDE_DIRECTORIES(
    ${VTK_INCLUDE_DIR}
    ${PARAVIEW_INCLUDE_DIRS}
    ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt
    ${pqCore_SOURCE_DIR}
    ${pqCore_BINARY_DIR}
    ${pqComponents_SOURCE_DIR}
    ${pqComponents_BINARY_DIR}
    ${QtWidgets_SOURCE_DIR}
    ${QtWidgets_BINARY_DIR}
    )

  QT4_WRAP_CPP(MOC_SRCS ${MOC_HEADERS})
  QT4_WRAP_UI(UI_SRCS ${UI_FILES})

  ADD_PARAVIEW_ACTION_GROUP(SIERRA_PLOT_TOOLS_TOOLBAR_IFACE SIERRA_PLOT_TOOLS_TOOLBAR_IFACE_SRCS
    CLASS_NAME pqSierraPlotToolsActionGroup
    GROUP_NAME "ToolBar/SierraTools"
    )

  ADD_PARAVIEW_PLUGIN(${PLUGIN_NAME} ${PLUGIN_VERSION}
    GUI_INTERFACES ${SIERRA_PLOT_TOOLS_TOOLBAR_IFACE}
    GUI_RESOURCES SierraPlotTools.qrc
    SOURCES  ${HEADERS}  ${MOC_SRCS} ${UI_SRCS} ${SIERRA_PLOT_TOOLS_TOOLBAR_IFACE_SRCS} ${CLIENT_SRCS}
    )

ELSE (PARAVIEW_BUILD_QT_GUI)
  ADD_PARAVIEW_PLUGIN(${PLUGIN_NAME} ${PLUGIN_VERSION})
ENDIF (PARAVIEW_BUILD_QT_GUI)
