INCLUDE(ParaViewTestingMacros)

set(MULTI_SLICE_TESTS
  ${CMAKE_CURRENT_SOURCE_DIR}/MultiSliceMultiBlock.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/MultiSliceWavelet.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/QuadViewSliceMultiBlock.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/QuadViewSliceWavelet.xml
)

set(MULTI_SLICE_SERVER_TESTS
  ${CMAKE_CURRENT_SOURCE_DIR}/serverMultiSliceMultiBlock.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/serverMultiSliceWavelet.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/serverQuadViewSliceMultiBlock.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/serverQuadViewSliceWavelet.xml
)

set(MultiSliceWavelet_BREAK TRUE)
set(MultiSliceWavelet_USE_NEW_PANELS TRUE)
set(MultiSliceMultiBlock_BREAK TRUE)
set(MultiSliceMultiBlock_USE_NEW_PANELS TRUE)
set(QuadViewSliceMultiBlock_BREAK TRUE)
set(QuadViewSliceMultiBlock_USE_NEW_PANELS TRUE)
set(QuadViewSliceWavelet_BREAK TRUE)
set(QuadViewSliceWavelet_USE_NEW_PANELS TRUE)

set(serverMultiSliceWavelet_BREAK TRUE)
set(serverMultiSliceWavelet_USE_NEW_PANELS TRUE)
set(serverMultiSliceMultiBlock_BREAK TRUE)
set(serverMultiSliceMultiBlock_USE_NEW_PANELS TRUE)
set(serverQuadViewSliceMultiBlock_BREAK TRUE)
set(serverQuadViewSliceMultiBlock_USE_NEW_PANELS TRUE)
set(serverQuadViewSliceWavelet_BREAK TRUE)
set(serverQuadViewSliceWavelet_USE_NEW_PANELS TRUE)

IF (PARAVIEW_DATA_ROOT AND PARAVIEW_BUILD_QT_GUI)

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

ENDIF ()
