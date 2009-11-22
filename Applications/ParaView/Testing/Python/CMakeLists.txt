SET (TESTS_WITHOUT_BASELINES
  ${CMAKE_CURRENT_SOURCE_DIR}/TestContourWidget.py
  )

SET (TESTS_WITH_BASELINES
  ${CMAKE_CURRENT_SOURCE_DIR}/DisconnectAndSaveAnimation.py
  ${CMAKE_CURRENT_SOURCE_DIR}/FileSeries.py
  ${CMAKE_CURRENT_SOURCE_DIR}/LoadStateMultiView.py
  ${CMAKE_CURRENT_SOURCE_DIR}/SaveLargeScreenshot.py
  )

# Disable client test, only for remote connections.
SET (DisconnectAndSaveAnimation_DISABLE_C TRUE)
SET (DisconnectAndSaveAnimation_DISABLE_CRS TRUE)

# Set image threshold overrides for the tests.
SET(SaveLargeScreenshot_THRESHOLD 10000)

add_client_tests(
  TEST_SCRIPTS ${TESTS_WITHOUT_BASELINES})

IF (PARAVIEW_DATA_ROOT)
  add_client_tests("pypv"
    TEST_SCRIPTS ${TESTS_WITH_BASELINES}
  )

  add_client_server_tests("pypvcs"
    TEST_SCRIPTS ${TESTS_WITH_BASELINES}
  )

  add_client_render_server_tests("pypvcrs"
    TEST_SCRIPTS ${TESTS_WITH_BASELINES}
  )

# TODO: remote rendering tests and reverse connect tests.

ENDIF (PARAVIEW_DATA_ROOT)

