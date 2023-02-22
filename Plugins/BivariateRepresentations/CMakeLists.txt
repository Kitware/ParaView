if (PARAVIEW_USE_QT)
  list(APPEND sources
    pqBivariateAnimationManager.cxx
    pqBivariateAnimationManager.h)

  paraview_plugin_add_auto_start(
    CLASS_NAME "pqBivariateAnimationManager"
    STARTUP onStartup
    SHUTDOWN onShutdown
    INTERFACES autostart_interface
    SOURCES autostart_sources)
  list(APPEND interfaces
    ${autostart_interface})
  list(APPEND sources
    ${autostart_sources})
endif ()

paraview_add_plugin(BivariateRepresentations
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources}
  MODULES BivariateRepresentations::vtkBivariateRepresentations
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Representations/vtk.module"
  XML_DOCUMENTATION OFF)
