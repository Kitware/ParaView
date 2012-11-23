INCLUDE(ParaViewTestingMacros)

set(GMV_READER_TESTS
  ${CMAKE_CURRENT_SOURCE_DIR}/GMVReaderBasicRead.xml
)

set(GMV_READER_SERVER_TESTS
  ${CMAKE_CURRENT_SOURCE_DIR}/serverGMVReaderBasicRead.xml
)

set(GMV_READER_STATIC_TESTS
  ${CMAKE_CURRENT_SOURCE_DIR}/GMVReaderBasicRead-static.xml
)

set(GMVReaderBasicRead_BREAK TRUE)
set(GMVReaderBasicRead_USE_NEW_PANELS TRUE)

set(serverGMVReaderBasicRead_BREAK TRUE)
set(serverGMVReaderBasicRead_USE_NEW_PANELS TRUE)

set(GMVReaderBasicRead-static_BREAK TRUE)
set(GMVReaderBasicRead-static_USE_NEW_PANELS TRUE)

IF (PARAVIEW_DATA_ROOT AND PARAVIEW_BUILD_QT_GUI)
  IF (BUILD_SHARED_LIBS)
    add_client_tests("pv"
      BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
      TEST_SCRIPTS ${GMV_READER_TESTS}
    )

    add_client_server_tests("pvcs"
      BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
      TEST_SCRIPTS ${GMV_READER_SERVER_TESTS}
    )

  ELSE(BUILD_SHARED_LIBS)
    # Static build no need to load plugins
    add_client_tests("pv"
      BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
      TEST_SCRIPTS ${GMV_READER_STATIC_TESTS}
    )

    add_client_server_tests("pvcs"
      BASELINE_DIR ${PARAVIEW_DATA_ROOT}/Baseline
      TEST_SCRIPTS ${GMV_READER_STATIC_TESTS}
    )

  ENDIF(BUILD_SHARED_LIBS)
ENDIF ()
