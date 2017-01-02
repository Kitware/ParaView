include(ParaViewTestingMacros)

if (PARAVIEW_BUILD_QT_GUI AND BUILD_SHARED_LIBS)
  paraview_test_load_data_dirs(LagrangianParticleTracker
    LagrangianParticleTracker
    LagrangianParticleTracker/flow
    LagrangianParticleTracker/surface)
  
  add_client_tests("pv"
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianSeedHelperCustom.xml)
  add_client_tests("pv"
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianSeedHelper.xml)
  add_client_tests("pv"
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianSurfaceHelper.xml)
  add_client_tests("pv"
    LOAD_PLUGIN "LagrangianParticleTracker"
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianSurfaceHelperNone.xml)
  if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
    add_client_tests("pv"
      LOAD_PLUGIN "LagrangianParticleTracker"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianSurfaceHelperComposite.xml)
  endif()
  add_client_tests("pv"
    LOAD_PLUGIN "LagrangianParticleTracker"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTracker.xml)
  add_client_tests("pv"
    LOAD_PLUGIN "LagrangianParticleTracker"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerReseeding.xml)

  if (PARAVIEW_USE_MPI)
    add_client_server_tests("pvcs"
      LOAD_PLUGIN "LagrangianParticleTracker"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerParallel.xml)
    add_client_render_server_tests("pvcrs"
      LOAD_PLUGIN "LagrangianParticleTracker"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerParallel.xml)
    add_client_server_tests("pvcs"
      LOAD_PLUGIN "LagrangianParticleTracker"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerParallelDistributed.xml)
    add_client_render_server_tests("pvcrs"
      LOAD_PLUGIN "LagrangianParticleTracker"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerParallelDistributed.xml)
    add_client_server_tests("pvcs"
      LOAD_PLUGIN "LagrangianParticleTracker"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerParallelSurfDistributed.xml)
    add_client_render_server_tests("pvcrs"
      LOAD_PLUGIN "LagrangianParticleTracker"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerParallelSurfDistributed.xml)
  else()
    add_client_server_tests("pvcs"
      LOAD_PLUGIN "LagrangianParticleTracker"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerReseeding.xml)
    add_client_render_server_tests("pvcrs"
      LOAD_PLUGIN "LagrangianParticleTracker"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTrackerReseeding.xml)
    add_client_server_tests("pvcs"
      LOAD_PLUGIN "LagrangianParticleTracker"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTracker.xml)
    add_client_render_server_tests("pvcrs"
      LOAD_PLUGIN "LagrangianParticleTracker"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/LagrangianParticleTracker.xml)
  endif()
endif()
