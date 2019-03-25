# This plugin contains tools developed for the Stanford Linear Accelerator
# Center to make it easier to visualize finite element simulation results.

vtk_module_scan(
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/Filters/vtk.module"
  REQUEST_MODULES   SLACTools::vtkSLACFilters
  PROVIDES_MODULES  slac_modules
  REQUIRES_MODULES  required_modules
  HIDE_MODULES_FROM_CACHE ON)

if (required_modules)
  foreach (required_module IN LISTS required_modules)
    if (NOT TARGET "${required_module}")
      message(FATAL_ERROR
        "Failed to find the required module ${required_module}.")
    endif ()
  endforeach ()
endif ()

vtk_module_build(
  MODULES             ${slac_modules}
  PACKAGE             SLACTools
  INSTALL_HEADERS     OFF
  LIBRARY_NAME_SUFFIX "pv${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}"
  VERSION             "${PARAVIEW_VERSION}"
  SOVERSION           "1")

set(ui_files)
set(qrc_files)
set(sources)
set(interfaces)
if (PARAVIEW_BUILD_QT_GUI)
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
  MODULES SLACTools::vtkSLACFilters)

if (PARAVIEW_BUILD_QT_GUI)
  target_link_libraries(SLACTools
    PRIVATE
      ParaView::ClientServerCoreCore
      ParaView::ServerManagerCore
      ParaView::ServerManagerRendering
      VTK::CommonCore
      VTK::CommonDataModel
      VTK::CommonExecutionModel)
endif ()

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
