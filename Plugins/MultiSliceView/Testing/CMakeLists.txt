INCLUDE(ParaViewTestingMacros)

set(MULTI_SLICE_TESTS
  ${CMAKE_CURRENT_SOURCE_DIR}/MultiSliceMultiBlock.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/MultiSliceWavelet.xml
)

set(MULTI_SLICE_SERVER_TESTS
  ${CMAKE_CURRENT_SOURCE_DIR}/serverMultiSliceMultiBlock.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/serverMultiSliceWavelet.xml
)

set(MultiSliceWavelet_BREAK TRUE)
set(MultiSliceWavelet_USE_NEW_PANELS TRUE)
set(MultiSliceMultiBlock_BREAK TRUE)
set(MultiSliceMultiBlock_USE_NEW_PANELS TRUE)

set(serverMultiSliceWavelet_BREAK TRUE)
set(serverMultiSliceWavelet_USE_NEW_PANELS TRUE)
set(serverMultiSliceMultiBlock_BREAK TRUE)
set(serverMultiSliceMultiBlock_USE_NEW_PANELS TRUE)

IF (PARAVIEW_DATA_ROOT)

  add_client_tests("pv"
    BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
    TEST_SCRIPTS ${MULTI_SLICE_TESTS}
  )

  add_client_server_tests("pvcs"
    BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
    TEST_SCRIPTS ${MULTI_SLICE_SERVER_TESTS}
  )

  # No selection are available on pvcrs
  #IF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
  #  add_client_render_server_tests("pvcrs"
  #    BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
  #    TEST_SCRIPTS ${MULTI_SLICE_SERVER_TESTS}
  #  )
  #ENDIF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)

ENDIF (PARAVIEW_DATA_ROOT)
