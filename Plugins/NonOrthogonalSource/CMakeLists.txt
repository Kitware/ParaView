set(interfaces)
set(sources)
if (PARAVIEW_USE_QT)
  list(APPEND sources
    pqNonOrthogonalAutoStart.cxx
    pqNonOrthogonalAutoStart.h
    pqSetupModelTransformBehavior.cxx
    pqSetupModelTransformBehavior.h)
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
  MODULES       NonOrthogonalSource::vtkNonOrthogonalSources
  MODULE_FILES  "${CMAKE_CURRENT_SOURCE_DIR}/Sources/vtk.module")

if (PARAVIEW_USE_QT)
  target_link_libraries(NonOrthogonalSource
    PRIVATE
      ParaView::pqApplicationComponents)
endif ()

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
