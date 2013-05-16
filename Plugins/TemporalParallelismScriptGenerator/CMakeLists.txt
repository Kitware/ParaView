project(STPScriptGeneratorPlugin)

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
INCLUDE_DIRECTORIES(${VTK_INCLUDE_DIRS})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/Resources)
INCLUDE_DIRECTORIES(${ParaView_SOURCE_DIR}/Applications/ParaView)
INCLUDE_DIRECTORIES(${ParaView_BINARY_DIR}/Applications/ParaView)

# Plugins for ParaView that add support to export state for CoProcessing etc.
#------------------------------------------------------------------------------
SET (TP_SOURCES
  pqTPActionsGroup.cxx
  pqTPActionsGroup.h
  pqTPExportStateWizard.cxx
  pqTPExportStateWizard.h
  pqTPPluginManager.cxx
  pqTPPluginManager.h
)

INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(MOC_SRCS
  pqTPExportStateWizard.h
  pqTPActionsGroup.h
  pqTPPluginManager.h
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
  GROUP_NAME "MenuBar/SpatioTemporal")

ADD_PARAVIEW_AUTO_START(
  PARAVIEW_PLUGIN_IFACES2
  PARAVIEW_PLUGIN_SRCS2
  CLASS_NAME pqTPPluginManager)


ADD_PARAVIEW_PLUGIN(
  TemporalParallelismScriptGenerator
  "0.1.0"
  SERVER_MANAGER_XML Resources/servermanagercp.xml
  GUI_INTERFACES
    ${PARAVIEW_PLUGIN_IFACES}
    ${PARAVIEW_PLUGIN_IFACES2}
  GUI_SOURCES
    ${TP_SOURCES}
    ${MOC_SRCS}
    ${UI_RESOURCES}
    ${PARAVIEW_PLUGIN_SRCS}
    ${PARAVIEW_PLUGIN_SRCS2}
)
