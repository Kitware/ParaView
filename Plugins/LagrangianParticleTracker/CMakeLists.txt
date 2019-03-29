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
  MODULES LagrangianParticleTracker::vtkLagrangianParticleTracker
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Filters/vtk.module")

target_link_libraries(LagrangianParticleTracker
  PRIVATE
    ParaView::ClientServerCoreCore
    ParaView::ServerManagerCore
    VTK::CommonCore
    VTK::GUISupportQt)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
