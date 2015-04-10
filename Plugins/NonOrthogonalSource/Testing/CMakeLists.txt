if (NOT PARAVIEW_BUILD_QT_GUI)
  return()
endif()

include(ParaViewTestingMacros)
set (xmls
  ${CMAKE_CURRENT_SOURCE_DIR}/NonOrthogonalSliceView.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/NonOrthographicOrthographicView.xml
)

set (NonOrthogonalSliceView_FORCE_SERIAL TRUE)

add_client_tests("pv"
  LOAD_PLUGIN "NonOrthogonalSource"
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS ${xmls}
)
add_client_server_tests("pvcs"
  LOAD_PLUGIN "NonOrthogonalSource"
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS ${xmls}
)
add_client_render_server_tests("pvcrs"
  LOAD_PLUGIN "NonOrthogonalSource"
  BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
  TEST_SCRIPTS ${xmls}
)
