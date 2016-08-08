project(CatalystScriptGeneratorPlugin)

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
INCLUDE_DIRECTORIES(${VTK_INCLUDE_DIRS})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/Resources)
INCLUDE_DIRECTORIES(${ParaView_SOURCE_DIR}/Applications/ParaView)
INCLUDE_DIRECTORIES(${ParaView_BINARY_DIR}/Applications/ParaView)

# Plugins for ParaView that add support to export state for Catalyst etc.
#------------------------------------------------------------------------------
SET (CP_SOURCES
  pqCPActionsGroup.cxx
  pqCPActionsGroup.h
  pqCPExportStateWizard.cxx
  pqCPExportStateWizard.h
  pqCPPluginManager.cxx
  pqCPPluginManager.h
)
include(ParaViewQt)
pv_find_package_qt(qt_targets REQUIRED QT4_COMPONENTS QtGui QT5_COMPONENTS Widgets)

pv_qt_wrap_cpp(MOC_SRCS
    pqCPExportStateWizard.h
    pqCPActionsGroup.h
    pqCPPluginManager.h
  )
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
  CLASS_NAME pqCPActionsGroup
  GROUP_NAME "MenuBar/CoProcessing")

ADD_PARAVIEW_AUTO_START(
  PARAVIEW_PLUGIN_IFACES2
  PARAVIEW_PLUGIN_SRCS2
  CLASS_NAME pqCPPluginManager)


if(PARAVIEW_USE_MPI)
  include(vtkMPI)
endif()

ADD_PARAVIEW_PLUGIN(
  CatalystScriptGeneratorPlugin
  #"${Catalyst_VERSION_MAJOR}.${Catalyst_VERSION_MINOR}.${Catalyst_VERSION_PATCH}"
  "0.1.0"
  REQUIRED_ON_CLIENT
  GUI_INTERFACES
    ${PARAVIEW_PLUGIN_IFACES}
    ${PARAVIEW_PLUGIN_IFACES2}
  GUI_SOURCES
    ${CP_SOURCES}
    ${MOC_SRCS}
    ${UI_RESOURCES}
    ${PARAVIEW_PLUGIN_SRCS}
    ${PARAVIEW_PLUGIN_SRCS2}
)
target_link_libraries(CatalystScriptGeneratorPlugin LINK_PRIVATE ${qt_targets})

IF (BUILD_TESTING)
  include(ParaViewTestingMacros)
  # this test is redundant with the CatalystFullWorkflow test
  # in the coprocessing library but that may not be built so we
  # add it here as well. this test though is a subset of that test.
  add_client_tests("pv"
    LOAD_PLUGIN "CatalystScriptGeneratorPlugin"
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/Testing/CatalystGUI.xml )
  SET_TESTS_PROPERTIES(pv.CatalystGUI PROPERTIES RUN_SERIAL ON LABELS "PARAVIEW;CATALYST")
  add_client_tests("pv"
    LOAD_PLUGIN "CatalystScriptGeneratorPlugin"
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/Testing/CatalystCinemaGUI.xml )
  SET_TESTS_PROPERTIES(pv.CatalystCinemaGUI PROPERTIES RUN_SERIAL ON LABELS "PARAVIEW;CATALYST")
ENDIF ()

if(PARAVIEW_USE_MPI)
  vtk_mpi_link(CatalystScriptGeneratorPlugin)
endif()

target_link_libraries (CatalystScriptGeneratorPlugin
  LINK_PRIVATE
    vtkPVServerManagerRendering
)
