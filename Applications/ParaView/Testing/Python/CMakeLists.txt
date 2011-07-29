SET (TESTS_WITHOUT_BASELINES
  ${CMAKE_CURRENT_SOURCE_DIR}/TestContourWidget.py
  )

SET (TESTS_WITH_BASELINES
#  ${CMAKE_CURRENT_SOURCE_DIR}/DisconnectAndSaveAnimation.py
  ${CMAKE_CURRENT_SOURCE_DIR}/FileSeries.py
  ${CMAKE_CURRENT_SOURCE_DIR}/LoadStateMultiView.py
  ${CMAKE_CURRENT_SOURCE_DIR}/NonlinearSubdivisionDisplay.py
  ${CMAKE_CURRENT_SOURCE_DIR}/RenderNan.py
  ${CMAKE_CURRENT_SOURCE_DIR}/SaveLargeScreenshot.py
  ${CMAKE_CURRENT_SOURCE_DIR}/SLACTools.py
  ${CMAKE_CURRENT_SOURCE_DIR}/TestIsoVolume.py
  )

IF(PARAVIEW_BUILD_PLUGIN_StreamingView)
  SET(TESTS_WITH_BASELINES ${TESTS_WITH_BASELINES}
    ${CMAKE_CURRENT_SOURCE_DIR}/StreamingTest.py
    ${CMAKE_CURRENT_SOURCE_DIR}/StreamingClientSideTest.py
    ${CMAKE_CURRENT_SOURCE_DIR}/StreamingServerSideTest.py
    )
ENDIF(PARAVIEW_BUILD_PLUGIN_StreamingView)

IF(PARAVIEW_BUILD_PLUGIN_Manta)
  SET(TESTS_WITH_BASELINES ${TESTS_WITH_BASELINES}
    ${CMAKE_CURRENT_SOURCE_DIR}/MantaTest.py
    ${CMAKE_CURRENT_SOURCE_DIR}/MantaServerSideTest.py
    )
ENDIF(PARAVIEW_BUILD_PLUGIN_Manta)

# Disable client test, only for remote connections.
SET (DisconnectAndSaveAnimation_DISABLE_C TRUE)
SET (DisconnectAndSaveAnimation_DISABLE_CRS TRUE)
set (FileSeries_BREAK TRUE)
set (DisconnectAndSaveAnimation_BREAK TRUE)
set (LoadStateMultiView_BREAK TRUE)
set (NonlinearSubdivisionDisplay_DISABLE_CRS TRUE)
set (TestIsoVolume_BREAK TRUE)
SET (SLACTools_DISABLE_CS TRUE)
SET (SLACTools_DISABLE_CRS TRUE)
SET (SLACTools_BREAK TRUE)
set (RenderNan_BREAK TRUE)
set (MantaServerSideTest_DISABLE_C TRUE)
set (MantaServerSideTest_DISABLE_CRS TRUE)
SET (MantaTest_BREAK TRUE)

# Don't test in cli/serv since number of pieces is function of number of processors.
set (StreamingTest_DISABLE_CS TRUE)
set (StreamingTest_DISABLE_CRS TRUE)
#serializing streaming tests to better tell what breaks where
SET (StreamingTest_BREAK TRUE)

# This tests verifies that cli/serv mode works
set (StreamingClientSideTest_DISABLE_CS TRUE) #just to save time
set (StreamingClientSideTest_DISABLE_CRS TRUE)
SET (StreamingClientSideTest_BREAK TRUE)
#loosen tolerance since viewpoint and thus images differ dependent on num processes
SET (StreamingClientSideTest_THRESHOLD 1000)

# This test verifies that serv side rendering works
set (StreamingServerSideTest_DISABLE_C TRUE)
set (StreamingServerSideTest_DISABLE_CRS TRUE)
SET (StreamingServerSideTest_BREAK TRUE)

# Set image threshold overrides for the tests.
SET(SaveLargeScreenshot_THRESHOLD 10000)
SET(SLACTools_THRESHOLD 20)

add_client_tests(
  TEST_SCRIPTS ${TESTS_WITHOUT_BASELINES}
  PARALLEL)

IF (PARAVIEW_DATA_ROOT)
  add_client_tests("pypv"
    TEST_SCRIPTS ${TESTS_WITH_BASELINES}
    PARALLEL
  )

  add_client_server_tests("pypvcs"
    TEST_SCRIPTS ${TESTS_WITH_BASELINES}
  )

  IF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)
    add_client_render_server_tests("pypvcrs"
      TEST_SCRIPTS ${TESTS_WITH_BASELINES}
    )
  ENDIF (PARAVIEW_CLIENT_RENDER_SERVER_TESTS)

# TODO: remote rendering tests and reverse connect tests.

ENDIF (PARAVIEW_DATA_ROOT)
