
ADD_EXECUTABLE(pqCoreBasicApp
  MACOSX_BUNDLE
  BasicApp.cxx)
TARGET_LINK_LIBRARIES(pqCoreBasicApp
  pqCore
  QtTesting
  ${QT_LIBRARIES}
  )

IF(APPLE)
  SET(TEST_BINARY 
    ${EXECUTABLE_OUTPUT_PATH}/pqCoreBasicApp.app/Contents/MacOS/pqCoreBasicApp
      --test-directory=${PARAVIEW_TEST_DIR})
ELSE(APPLE)
  SET(TEST_BINARY ${EXECUTABLE_OUTPUT_PATH}/pqCoreBasicApp
      --test-directory=${PARAVIEW_TEST_DIR})
ENDIF(APPLE)

SET(TEST_IMAGE ${PARAVIEW_DATA_ROOT}/Baseline)

ADD_TEST(pqCoreBasicApp ${TEST_BINARY}
         --compare-view=${TEST_IMAGE}/pqCoreBasicApp.png
         --exit)

