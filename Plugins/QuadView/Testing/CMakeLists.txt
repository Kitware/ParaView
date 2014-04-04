INCLUDE(ParaViewTestingMacros)

set(MULTI_SLICE_TESTS
  ${CMAKE_CURRENT_SOURCE_DIR}/QuadViewSliceMultiBlock.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/QuadViewSliceWavelet.xml
)

set(QuadViewSliceMultiBlock_BREAK TRUE)
set(QuadViewSliceWavelet_BREAK TRUE)

IF (PARAVIEW_BUILD_QT_GUI)
    add_client_tests("pv"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${MULTI_SLICE_TESTS}
      LOAD_PLUGIN "QuadView"
    )

    add_client_server_tests("pvcs"
      BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
      TEST_SCRIPTS ${MULTI_SLICE_TESTS}
      LOAD_PLUGIN "QuadView"
    )

    # No selection are available on pvcrs

    add_tile_display_tests("pvcs-tile-display" 2 1
       BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
       TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/Tile2x1-QuadViewSliceWavelet.xml
       LOAD_PLUGIN "QuadView"
    )

    add_tile_display_tests("pvcs-tile-display" 1 2
       BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
       TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/Tile1x2-QuadViewSliceWavelet.xml
       LOAD_PLUGIN "QuadView"
    )

    add_tile_display_tests("pvcs-tile-display" 2 2
       BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
       TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/Tile2x2-QuadViewSliceWavelet.xml
       LOAD_PLUGIN "QuadView"
    )
ENDIF ()
