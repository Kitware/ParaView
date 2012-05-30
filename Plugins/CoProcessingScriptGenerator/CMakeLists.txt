project(CoProcessingScriptGeneratorPlugin)


#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    ${PARAVIEW_KWSYS_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIR}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)
 
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/Resources)

# Plugins for ParaView that add support to export state for CoProcessing etc.
#------------------------------------------------------------------------------
SET (CP_SOURCES
  pqCPPluginManager.cxx
  pqCPPluginManager.h
  pqCPWritersMenuManager.cxx
  pqCPWritersMenuManager.h
  pqCPActionsGroup.cxx
  pqCPActionsGroup.h
  pqCPExportStateWizard.cxx
  pqCPExportStateWizard.h
)

SET (UI_RESOURCES
  Resources/ExportStateWizard.ui
  Resources/ImageOutputInfo.ui)

INCLUDE(${QT_USE_FILE})

QT4_WRAP_CPP(MOC_SRCS
  pqCPPluginManager.h
  pqCPWritersMenuManager.h
  pqCPActionsGroup.h
  pqCPExportStateWizard.h
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
  CLASS_NAME pqCPActionsGroup
  GROUP_NAME "MenuBar/CoProcessing")

ADD_PARAVIEW_AUTO_START(
  PARAVIEW_PLUGIN_IFACES2
  PARAVIEW_PLUGIN_SRCS2
  CLASS_NAME pqCPPluginManager)


ADD_CUSTOM_COMMAND(
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/cp_export_py.cxx"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cp_export.py"
  COMMAND ${VTK_ENCODESTRING_EXE}
  ARGS "${CMAKE_CURRENT_BINARY_DIR}/cp_export_py.cxx"
       "${CMAKE_CURRENT_SOURCE_DIR}/cp_export.py"
       cp_export_py)

ADD_PARAVIEW_PLUGIN(
  CoProcessingPlugin
  #"${CoProcessing_VERSION_MAJOR}.${CoProcessing_VERSION_MINOR}.${CoProcessing_VERSION_PATCH}"
  "0.1.0"
  SERVER_MANAGER_XML Resources/servermanager.xml
  GUI_INTERFACES
    ${PARAVIEW_PLUGIN_IFACES}
    ${PARAVIEW_PLUGIN_IFACES2}
  GUI_SOURCES
    ${CP_SOURCES}
    ${MOC_SRCS}
    ${UI_RESOURCES}
    ${PARAVIEW_PLUGIN_SRCS}
    ${PARAVIEW_PLUGIN_SRCS2}
    cp_export_py.cxx
)

IF (BUILD_TESTING)
  include(ParaViewTestingMacros)
  set(CoProcessingGUI_USE_NEW_PANELS TRUE)
  # this test is redundant with the CoProcessingFullWorkflow test
  # in the coprocessing library but that may not be built so we
  # add it here as well. this test though is a subset of that test.
  if(Q_WS_MAC)
    add_client_tests("pv"
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/Testing/CoProcessingGUI-Mac.xml )
    SET_TESTS_PROPERTIES(pv.CoProcessingGUI-Mac PROPERTIES RUN_SERIAL ON)
  else(Q_WS_MAC)
    add_client_tests("pv"
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/Testing/CoProcessingGUI.xml )
    SET_TESTS_PROPERTIES(pv.CoProcessingGUI PROPERTIES RUN_SERIAL ON)
  endif(Q_WS_MAC)
ENDIF (BUILD_TESTING)
