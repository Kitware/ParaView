# This plugin contains tools developed for the Stanford Linear Accelerator
# Center to make it easier to visualize finite element simulation results.

set(ui_files)
set(qrc_files)
set(sources)
set(interfaces)
if (PARAVIEW_USE_QT)
  list(APPEND ui_files
    pqSLACActionHolder.ui
    pqSLACDataLoadManager.ui)

  list(APPEND qrc_files
    SLACTools.qrc)

  list(APPEND sources
    pqSLACActionGroup.cxx
    pqSLACActionGroup.h
    pqSLACDataLoadManager.cxx
    pqSLACDataLoadManager.h
    pqSLACManager.cxx
    pqSLACManager.h)

  paraview_plugin_add_action_group(
    CLASS_NAME pqSLACActionGroup
    GROUP_NAME "ToolBar/SLAC"
    INTERFACES action_interfaces
    SOURCES action_sources)
  list(APPEND interfaces
    ${action_interfaces})
  list(APPEND sources
    ${action_sources})
endif ()

paraview_add_plugin(SLACTools
  VERSION "1.1"
  UI_FILES ${ui_files}
  UI_INTERFACES ${interfaces}
  UI_RESOURCES ${qrc_files}
  SOURCES ${sources}
  MODULES SLACTools::vtkSLACFilters
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Filters/vtk.module")

if (PARAVIEW_USE_QT)
  target_link_libraries(SLACTools
    PRIVATE
      ParaView::RemotingViews
      VTK::CommonCore
      VTK::CommonDataModel
      VTK::CommonExecutionModel)
endif ()

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
