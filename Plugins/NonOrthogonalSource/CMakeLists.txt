vtk_module_scan(
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/Sources/vtk.module"
  REQUEST_MODULES   NonOrthogonalSource::vtkNonOrthogonalSources
  PROVIDES_MODULES  sources_modules
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
  MODULES             ${sources_modules}
  PACKAGE             NonOrthogonalSource
  INSTALL_HEADERS     OFF
  LIBRARY_NAME_SUFFIX "pv${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}"
  VERSION             "${PARAVIEW_VERSION}"
  SOVERSION           "1")

set(interfaces)
set(sources
  pqNonOrthogonalAutoStart.cxx
  pqNonOrthogonalAutoStart.h
  pqSetupModelTransformBehavior.cxx
  pqSetupModelTransformBehavior.h)
if (PARAVIEW_BUILD_QT_GUI)
  paraview_plugin_add_auto_start(
    CLASS_NAME pqNonOrthogonalAutoStart
    INTERFACES autostart_interface
    SOURCES autostart_sources)
  list(APPEND interfaces
    ${autostart_interface})
  list(APPEND sources
    ${autostart_sources})
endif ()

paraview_add_plugin(NonOrthogonalSource
  VERSION       "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES       ${sources}
  MODULES       NonOrthogonalSource::vtkNonOrthogonalSources)

if (PARAVIEW_BUILD_QT_GUI)
  target_link_libraries(NonOrthogonalSource
    PRIVATE
      ParaView::pqApplicationComponents)
endif ()

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
