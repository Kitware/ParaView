execute_process(
  COMMAND ${PARAVIEW_EXECUTABLE} -dr
          --test-directory=${PARAVIEW_TEST_OUTPUT_DIR}
          --test-script=${TEST_SCRIPT}
          --exit
  RESULT_VARIABLE rv
)
if (NOT rv EQUAL 0)
  message(FATAL_ERROR "ParaView return value was ${rv}")
endif()

execute_process(
  COMMAND ${PVPYTHON_EXECUTABLE} -dr
  ${TEST_VERIFIER}
  -T ${PARAVIEW_TEST_OUTPUT_DIR}
  -N ${TEST_NAME}
  RESULT_VARIABLE rv
)
if (NOT rv EQUAL 0)
  message(FATAL_ERROR "pvpython return value was ${rv}")
endif()
