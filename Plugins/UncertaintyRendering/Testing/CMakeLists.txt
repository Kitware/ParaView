include(ParaViewTestingMacros)

set(UNCERTAINTY_RENDERING_TESTS
  ${CMAKE_CURRENT_SOURCE_DIR}/UncertaintyRendering.xml
)

if(PARAVIEW_BUILD_QT_GUI AND BUILD_SHARED_LIBS)
  add_client_tests("pv"
    LOAD_PLUGIN "UncertaintyRendering"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${UNCERTAINTY_RENDERING_TESTS}
  )

  add_client_server_tests("pvcs"
    LOAD_PLUGIN "UncertaintyRendering"
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS ${UNCERTAINTY_RENDERING_TESTS}
  )

  if(PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
    add_client_render_server_tests("pvcrs"
      LOAD_PLUGIN "UncertaintyRendering"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${UNCERTAINTY_RENDERING_TESTS}
    )
  endif()
endif()
