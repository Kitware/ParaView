#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${VTK_INCLUDE_DIRS}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/Resources)

# Plugins for ParaView that add support to export state for temporal parallelism
#------------------------------------------------------------------------------
SET (TP_SOURCES
  pqTPPluginManager.cxx
  pqTPPluginManager.h
  pqTPWritersMenuManager.cxx
  pqTPWritersMenuManager.h
  pqTPActionsGroup.cxx
  pqTPActionsGroup.h
  pqTPExportStateWizard.cxx
  pqTPExportStateWizard.h
)

SET (UI_RESOURCES
  Resources/ExportStateWizard.ui
  Resources/ImageOutputInfo.ui)

INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(MOC_SRCS
  pqTPPluginManager.h
  pqTPWritersMenuManager.h
  pqTPActionsGroup.h
  pqTPExportStateWizard.h
)

QT4_WRAP_UI(UI_SRCS
  ${UI_RESOURCES})

#------------------------------------------------------------------------------
SET (PARAVIEW_PLUGIN_IFACES)
SET (PARAVIEW_PLUGIN_SRCS)

SET (PARAVIEW_PLUGIN_IFACES2)
SET (PARAVIEW_PLUGIN_SRCS2)

# This is a macro for adding QActionGroup subclasses automatically as toolbars.
ADD_PARAVIEW_ACTION_GROUP(
  PARAVIEW_PLUGIN_IFACES
  PARAVIEW_PLUGIN_SRCS
  CLASS_NAME pqTPActionsGroup
  GROUP_NAME "MenuBar/TemporalParallelism")

ADD_PARAVIEW_AUTO_START(
  PARAVIEW_PLUGIN_IFACES2
  PARAVIEW_PLUGIN_SRCS2
  CLASS_NAME pqTPPluginManager)

ADD_CUSTOM_COMMAND(
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/tp_export_py.cxx"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tp_export.py"
  COMMAND vtkEncodeString
  ARGS "${CMAKE_CURRENT_BINARY_DIR}/tp_export_py.cxx"
       "${CMAKE_CURRENT_SOURCE_DIR}/tp_export.py"
       tp_export_py)

ADD_PARAVIEW_PLUGIN(
  TemporalParallelismScriptGenerator
  "0.1.0"
  SERVER_MANAGER_XML Resources/servermanager.xml
  GUI_INTERFACES
    ${PARAVIEW_PLUGIN_IFACES}
    ${PARAVIEW_PLUGIN_IFACES2}
  GUI_SOURCES
    ${TP_SOURCES}
    ${MOC_SRCS}
    ${UI_RESOURCES}
    ${PARAVIEW_PLUGIN_SRCS}
    ${PARAVIEW_PLUGIN_SRCS2}
    tp_export_py.cxx
)
