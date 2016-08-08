project(STPScriptGeneratorPlugin)

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
INCLUDE_DIRECTORIES(${VTK_INCLUDE_DIRS})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/Resources)
INCLUDE_DIRECTORIES(${ParaView_SOURCE_DIR}/Applications/ParaView)
INCLUDE_DIRECTORIES(${ParaView_BINARY_DIR}/Applications/ParaView)

# Plugins for ParaView that add support to export state for spatio-temporal
# parallelism
#------------------------------------------------------------------------------
SET (TP_SOURCES
  pqTPActionsGroup.cxx
  pqTPActionsGroup.h
  pqTPExportStateWizard.cxx
  pqTPExportStateWizard.h
  pqTPPluginManager.cxx
  pqTPPluginManager.h
)

SET (TP_MOC_SOURCES
  pqTPExportStateWizard.h
  pqTPActionsGroup.h
  pqTPPluginManager.h
  )
include(ParaViewQt)
pv_find_package_qt(qt_targets REQUIRED QT4_COMPONENTS QtGui QT5_COMPONENTS Widgets)
pv_qt_wrap_cpp(MOC_SRCS ${TP_MOC_SOURCES})
pv_qt_wrap_ui(UI_SRCS ${UI_RESOURCES})

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
  SERVER_MANAGER_XML Resources/servermanagertp.xml
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
TARGET_LINK_LIBRARIES(TemporalParallelismScriptGenerator LINK_PRIVATE ${qt_targets})

IF (BUILD_TESTING AND BUILD_SHARED_LIBS)
  include(ParaViewTestingMacros)
  add_client_tests("pv"
    LOAD_PLUGIN "TemporalParallelismScriptGenerator"
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/Testing/TemporalParallelism.xml )
ENDIF ()
