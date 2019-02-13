vtk_module_scan(
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/Filters/vtk.module"
  REQUEST_MODULES   LagrangianParticleTracker::vtkLagrangianParticleTracker
  PROVIDES_MODULES  lagrangian_modules
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
  MODULES             ${lagrangian_modules}
  PACKAGE             LagrangianParticleTracker
  INSTALL_HEADERS     OFF
  LIBRARY_NAME_SUFFIX "pv${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}"
  VERSION             "${PARAVIEW_VERSION}"
  SOVERSION           "1")

set(interfaces)
set(sources
  pqIntegrationModelHelperWidget.cxx
  pqIntegrationModelHelperWidget.h
  pqIntegrationModelSeedHelperWidget.cxx
  pqIntegrationModelSeedHelperWidget.h
  pqIntegrationModelSurfaceHelperWidget.cxx
  pqIntegrationModelSurfaceHelperWidget.h)

paraview_plugin_add_property_widget(
  KIND WIDGET
  TYPE "integration_model_seed_helper_widget"
  CLASS_NAME pqIntegrationModelSeedHelperWidget
  INTERFACES pwi_interfaces
  SOURCES pwi_sources)
list(APPEND interfaces
  ${pwi_interfaces})
list(APPEND sources
  ${pwi_sources})

paraview_plugin_add_property_widget(
  KIND WIDGET
  TYPE "integration_model_surface_helper_widget"
  CLASS_NAME pqIntegrationModelSurfaceHelperWidget
  INTERFACES pwi_interfaces
  SOURCES pwi_sources)
list(APPEND interfaces
  ${pwi_interfaces})
list(APPEND sources
  ${pwi_sources})

paraview_add_plugin(LagrangianParticleTracker
  REQUIRED_ON_SERVER
  REQUIRED_ON_CLIENT
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources}
  MODULES LagrangianParticleTracker::vtkLagrangianParticleTracker)

target_link_libraries(LagrangianParticleTracker
  PRIVATE
    ParaView::ClientServerCoreCore
    ParaView::ServerManagerCore
    VTK::CommonCore
    VTK::GUISupportQt)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
