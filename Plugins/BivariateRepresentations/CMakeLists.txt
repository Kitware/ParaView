set(interfaces)
set(sources)

if (PARAVIEW_USE_QT)
  list(APPEND sources
    pqBivariateManager.cxx
    pqBivariateManager.h
    vtkPVBivariatePluginLocation.cxx
    vtkPVBivariatePluginLocation.h)

  paraview_plugin_add_auto_start(
    CLASS_NAME "pqBivariateManager"
    STARTUP onStartup
    SHUTDOWN onShutdown
    INTERFACES autostart_interface
    SOURCES autostart_sources)
  list(APPEND interfaces
    ${autostart_interface})
  list(APPEND sources
    ${autostart_sources})

  paraview_plugin_add_location(
    CLASS_NAME vtkPVBivariatePluginLocation
    INTERFACES location_interfaces
    SOURCES location_sources)

  list(APPEND interfaces
    ${location_interfaces})
  list(APPEND sources
    ${location_sources})

  set(resources
    Bremm.png
    Schumann.png
    Steiger.png
    Teulingfig2.png)

  foreach(inputfile IN LISTS resources)
    configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/Resources/${inputfile}
      "${CMAKE_BINARY_DIR}/${_paraview_build_plugin_directory}/Resources/${inputfile}"
      COPYONLY)
  endforeach()

  install(
    DIRECTORY Resources
    DESTINATION "${_paraview_build_plugin_directory}"
    COMPONENT   "${_paraview_build_PLUGINS_COMPONENT}")
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

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif()
