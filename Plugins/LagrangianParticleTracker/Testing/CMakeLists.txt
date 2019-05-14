if (PARAVIEW_BUILD_QT_GUI AND BUILD_SHARED_LIBS)
  ExternalData_Expand_Arguments("ParaViewData" _
    "DATA{Data/LagrangianParticleTracker/,REGEX:.*}"
    "DATA{Data/LagrangianParticleTracker/flow/,REGEX:.*}"
    "DATA{Data/LagrangianParticleTracker/surface/,REGEX:.*}"

    "DATA{Data/Baseline/LagrangianParticleTracker_InterA.png}"
    "DATA{Data/Baseline/LagrangianParticleTracker_InterB.png}"
    "DATA{Data/Baseline/LagrangianParticleTracker_InterF.png}"
    "DATA{Data/Baseline/LagrangianParticleTracker_InterG.png}"
    "DATA{Data/Baseline/LagrangianParticleTracker_InterI.png}"
    "DATA{Data/Baseline/LagrangianParticleTracker_InterJ.png}"

    "DATA{Data/Baseline/LagrangianParticleTrackerParallel_InterA.png}"
    "DATA{Data/Baseline/LagrangianParticleTrackerParallel_InterB.png}"

    "DATA{Data/Baseline/LagrangianParticleTrackerParallelDistributed_InterA.png}")

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
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_SCRIPTS LagrangianSurfaceHelperComposite.xml)
  paraview_add_client_tests(
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_DATA_TARGET ParaViewData
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_SCRIPTS LagrangianParticleTracker.xml)
  paraview_add_client_tests(
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_DATA_TARGET ParaViewData
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_SCRIPTS LagrangianParticleTrackerReseeding.xml)

  if (PARAVIEW_USE_MPI)
    paraview_add_client_server_tests(
      LOAD_PLUGIN "LagrangianParticleTracker"
      TEST_DATA_TARGET ParaViewData
      BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerParallel.xml)
    paraview_add_client_server_render_tests(
      LOAD_PLUGIN "LagrangianParticleTracker"
      TEST_DATA_TARGET ParaViewData
      BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerParallel.xml)
    paraview_add_client_server_tests(
      LOAD_PLUGIN "LagrangianParticleTracker"
      TEST_DATA_TARGET ParaViewData
      BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerParallelDistributed.xml)
    paraview_add_client_server_render_tests(
      LOAD_PLUGIN "LagrangianParticleTracker"
      TEST_DATA_TARGET ParaViewData
      BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerParallelDistributed.xml)
    paraview_add_client_server_tests(
      LOAD_PLUGIN "LagrangianParticleTracker"
      TEST_DATA_TARGET ParaViewData
      BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerParallelSurfDistributed.xml)
    paraview_add_client_server_render_tests(
      LOAD_PLUGIN "LagrangianParticleTracker"
      TEST_DATA_TARGET ParaViewData
      BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerParallelSurfDistributed.xml)
  endif ()

  paraview_add_client_server_tests(
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_DATA_TARGET ParaViewData
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerReseeding.xml)
  paraview_add_client_server_render_tests(
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_DATA_TARGET ParaViewData
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerReseeding.xml)
  paraview_add_client_server_tests(
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_DATA_TARGET ParaViewData
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTracker.xml)
  paraview_add_client_server_render_tests(
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_DATA_TARGET ParaViewData
    BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTracker.xml)
endif ()
