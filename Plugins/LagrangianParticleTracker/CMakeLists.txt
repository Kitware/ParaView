include_directories(${VTK_INCLUDE_DIRS})

if (PARAVIEW_BUILD_QT_GUI)
  pv_find_package_qt(qt_targets REQUIRED QUIET
    QT4_COMPONENTS QtGui
    QT5_COMPONENTS Widgets)

  pv_qt_wrap_cpp(MOC_SRCS
    pqIntegrationModelHelperWidget.h
    pqIntegrationModelSeedHelperWidget.h
    pqIntegrationModelSurfaceHelperWidget.h)

  set(OUTIFACES0)
  set(OUTSRCS0)
  add_paraview_property_widget(OUTIFACES0 OUTSRCS0
    TYPE "integration_model_seed_helper_widget"
    CLASS_NAME pqIntegrationModelSeedHelperWidget)

  set(OUTIFACES1)
  set(OUTSRCS1)
  add_paraview_property_widget(OUTIFACES1 OUTSRCS1
    TYPE "integration_model_surface_helper_widget"
    CLASS_NAME pqIntegrationModelSurfaceHelperWidget)

  add_paraview_plugin(LagrangianParticleTracker "1.0"
    REQUIRED_ON_SERVER
    REQUIRED_ON_CLIENT
    SERVER_MANAGER_XML
      LagrangianParticleTracker.xml
    SERVER_MANAGER_SOURCES
      vtkLagrangianHelperBase.cxx
      vtkLagrangianSeedHelper.cxx
      vtkLagrangianSurfaceHelper.cxx
    GUI_INTERFACES
      ${OUTIFACES0} ${OUTIFACES1}
    GUI_SOURCES
      ${MOC_SRCS} ${OUTSRCS0} ${OUTSRCS1}
      pqIntegrationModelHelperWidget.cxx
      pqIntegrationModelSeedHelperWidget.cxx
      pqIntegrationModelSurfaceHelperWidget.cxx)

  target_link_libraries(LagrangianParticleTracker
                        LINK_PRIVATE vtkFiltersFlowPaths)

  if (BUILD_TESTING)
    add_subdirectory(Testing)
  endif()
endif()
