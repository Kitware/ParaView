if (PARAVIEW_BUILD_QT_GUI AND BUILD_SHARED_LIBS)
  ExternalData_Expand_Arguments("ParaViewData" _
    "DATA{Data/LagrangianParticleTracker/,REGEX:.*}"
    "DATA{Data/LagrangianParticleTracker/flow/,REGEX:.*}"
    "DATA{Data/LagrangianParticleTracker/surface/,REGEX:.*}")

  paraview_add_client_tests(
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS LagrangianSeedHelperCustom.xml)
  paraview_add_client_tests(
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS LagrangianSeedHelper.xml)
  paraview_add_client_tests(
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS LagrangianSurfaceHelper.xml)
  paraview_add_client_tests(
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS LagrangianSurfaceHelperNone.xml)
  paraview_add_client_tests(
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_DATA_TARGET ParaViewData
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS LagrangianSurfaceHelperComposite.xml)
  paraview_add_client_tests(
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_DATA_TARGET ParaViewData
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS LagrangianParticleTracker.xml)
  paraview_add_client_tests(
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_DATA_TARGET ParaViewData
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS LagrangianParticleTrackerReseeding.xml)

  if (PARAVIEW_USE_MPI)
    paraview_add_client_server_tests(
      LOAD_PLUGIN "LagrangianParticleTracker"
      TEST_DATA_TARGET ParaViewData
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerParallel.xml)
    paraview_add_client_server_render_tests(
      LOAD_PLUGIN "LagrangianParticleTracker"
      TEST_DATA_TARGET ParaViewData
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerParallel.xml)
    paraview_add_client_server_tests(
      LOAD_PLUGIN "LagrangianParticleTracker"
      TEST_DATA_TARGET ParaViewData
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerParallelDistributed.xml)
    paraview_add_client_server_render_tests(
      LOAD_PLUGIN "LagrangianParticleTracker"
      TEST_DATA_TARGET ParaViewData
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerParallelDistributed.xml)
    paraview_add_client_server_tests(
      LOAD_PLUGIN "LagrangianParticleTracker"
      TEST_DATA_TARGET ParaViewData
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerParallelSurfDistributed.xml)
    paraview_add_client_server_render_tests(
      LOAD_PLUGIN "LagrangianParticleTracker"
      TEST_DATA_TARGET ParaViewData
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerParallelSurfDistributed.xml)
  endif ()

  paraview_add_client_server_tests(
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_DATA_TARGET ParaViewData
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerReseeding.xml)
  paraview_add_client_server_render_tests(
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_DATA_TARGET ParaViewData
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerReseeding.xml)
  paraview_add_client_server_tests(
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_DATA_TARGET ParaViewData
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTracker.xml)
  paraview_add_client_server_render_tests(
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_DATA_TARGET ParaViewData
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTracker.xml)
endif ()
