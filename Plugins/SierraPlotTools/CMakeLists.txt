# This plugin contains tools developed for
# The Computational Solid Mechanics & Dynamics group(s) at Sandia National Labortories
# to make it easier to plot variables from Sierra-based finite
# element simulations.
# Author:  Jon Goldman
# Created:  Aug 2009 -to- Jan 2010

SET(PLUGIN_NAME SierraPlotTools)
SET(PLUGIN_VERSION "1.0")

#
# This macro changes the Visual Studio warning levels from 3 to 4,
#  to catch things such as "unreferenced local variable"
#
MACRO(VS_SET_VS_WARNING_4)
  IF(CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake)")
    #MESSAGE(WARNING "CMAKE_BUILD_TOOL MATCHES msdev|devenv|nmake")

    add_definitions(-DDISABLE_NOISY_COMPILE_WARNINGS)

    # Use the highest warning level for visual studio.
    SET(CMAKE_CXX_WARNING_LEVEL 4)
    IF(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
     STRING(REGEX REPLACE "/W[0-4]" "/W4"
            CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    ELSE()
     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    ENDIF()
  ENDIF()
ENDMACRO()

# Comment out VS_SET_VS_WARNING_4 to keep default Visual Studio Warning level 3
# unComment VS_SET_VS_WARNING_4 to change Visual Studio Warning level to 4
#VS_SET_VS_WARNING_4()

INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIRS}
  )

SET(UI_FILES
  # pqSierraPlotToolsActionHolder.ui -- container for plot tools actions
  pqSierraPlotToolsActionHolder.ui
  
  pqSierraPlotToolsDataLoadManager.ui
  pqVariablePlot.ui
  
  # pqSierraToolsRichTextDocs.ui -- container for Rich Text documents
  pqSierraToolsRichTextDocs.ui
  )

SET(MOC_HEADERS
  pqElementPlotter.h
  pqHoverLabel.h
  pqGlobalPlotter.h
  pqNodePlotter.h
  pqPlotVariablesDialog.h
  pqPlotter.h
  pqResizingScrollArea.h
  pqSierraPlotToolsActionGroup.h
  pqSierraPlotToolsDataLoadManager.h
  pqSierraPlotToolsManager.h
  pqVariableVariablePlotter.h
  )

SET(HEADERS
  ${MOC_HEADERS}
  pqSierraPlotToolsUtils.h
  warningState.h
  )

SET(CLIENT_SRCS
  pqElementPlotter.cxx
  pqGlobalPlotter.cxx
  pqHoverLabel.cxx
  pqNodePlotter.cxx
  pqPlotVariablesDialog.cxx
  pqPlotter.cxx
  pqResizingScrollArea.cxx
  pqSierraPlotToolsActionGroup.cxx
  pqSierraPlotToolsDataLoadManager.cxx
  pqSierraPlotToolsManager.cxx
  pqSierraPlotToolsUtils.cxx
  pqVariableVariablePlotter.cxx
  )

IF (PARAVIEW_BUILD_QT_GUI)
  INCLUDE (ParaViewQt)
  pv_find_package_qt(qt_targets REQUIRED QT4_COMPONENTS QtGui QT5_COMPONENTS Widgets)
  pv_qt_wrap_cpp(MOC_SRCS ${MOC_HEADERS})
  pv_qt_wrap_ui(UI_SRCS ${UI_FILES})
  ADD_PARAVIEW_ACTION_GROUP(SIERRA_PLOT_TOOLS_TOOLBAR_IFACE SIERRA_PLOT_TOOLS_TOOLBAR_IFACE_SRCS
    CLASS_NAME pqSierraPlotToolsActionGroup
    GROUP_NAME "ToolBar/SierraTools"
    )
  ADD_PARAVIEW_PLUGIN(${PLUGIN_NAME} ${PLUGIN_VERSION}
    GUI_INTERFACES ${SIERRA_PLOT_TOOLS_TOOLBAR_IFACE}
    GUI_RESOURCES SierraPlotTools.qrc
    SOURCES  ${HEADERS}  ${MOC_SRCS} ${UI_SRCS} ${SIERRA_PLOT_TOOLS_TOOLBAR_IFACE_SRCS} ${CLIENT_SRCS}
    )
  target_link_libraries(${PLUGIN_NAME} LINK_PRIVATE vtkPVVTKExtensionsDefault ${qt_targets})
ELSE ()
ENDIF ()
