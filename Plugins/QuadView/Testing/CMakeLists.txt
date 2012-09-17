INCLUDE(ParaViewTestingMacros)

set(MULTI_SLICE_TESTS
  ${CMAKE_CURRENT_SOURCE_DIR}/QuadViewSliceMultiBlock.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/QuadViewSliceWavelet.xml
)

set(MULTI_SLICE_SERVER_TESTS
  ${CMAKE_CURRENT_SOURCE_DIR}/serverQuadViewSliceMultiBlock.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/serverQuadViewSliceWavelet.xml
)

set(QuadViewSliceMultiBlock_BREAK TRUE)
set(QuadViewSliceMultiBlock_USE_NEW_PANELS TRUE)
set(QuadViewSliceWavelet_BREAK TRUE)
set(QuadViewSliceWavelet_USE_NEW_PANELS TRUE)

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


  add_tile_display_tests("pvcs-tile-display" 2 1
     BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
     TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/Tile2x1-QuadViewSliceWavelet.xml
  )

  add_tile_display_tests("pvcs-tile-display" 1 2
     BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
     TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/Tile1x2-QuadViewSliceWavelet.xml
  )

  add_tile_display_tests("pvcs-tile-display" 2 2
     BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
     TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/Tile2x2-QuadViewSliceWavelet.xml
  )

ENDIF ()
