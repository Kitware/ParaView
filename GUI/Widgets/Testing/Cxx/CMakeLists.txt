INCLUDE("${KWWidgets_CMAKE_DIR}/KWWidgetsTestingMacros.cmake")

SET(TESTS 
  Random
  Registry
  StateMachine
  )

FOREACH(TEST ${TESTS})
  SET(EXE_NAME KWWidgetsTest${TEST})
  ADD_EXECUTABLE(${EXE_NAME} Test${TEST}.cxx)
  TARGET_LINK_LIBRARIES(${EXE_NAME} ${KWWidgets_LIBRARIES})
  KWWidgets_ADD_TEST_WITH_LAUNCHER(KWWidgets-${TEST} ${EXE_NAME})
ENDFOREACH(TEST)
