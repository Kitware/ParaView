# This cmake script tests python state file saving and loading. It
# first launches paraview, runs an XML test, and saves the resulting
# state file. It then launches pvpython with a small test driver
# that loads the state file and then compares the resulting image to the
# baseline.

# run paraview to setup and save the python state file
execute_process(
  COMMAND ${PARAVIEW_EXECUTABLE} -dr
          --test-directory=${PARAVIEW_TEST_OUTPUT_DIR}
          --test-script=${TEST_SCRIPT}
          --exit
  RESULT_VARIABLE rv)
if(NOT rv EQUAL 0)
  message(FATAL_ERROR "ParaView return value was ${rv}")
endif()

# run pvpython to load the state file and verify the result
execute_process(
  COMMAND ${PVPYTHON_EXECUTABLE} -dr
  ${TEST_DRIVER}
  ${PARAVIEW_TEST_OUTPUT_DIR}/${PYTHON_STATE_TEST_NAME}-StateFile.py
  -T ${PARAVIEW_TEST_OUTPUT_DIR}
  -V ${PARAVIEW_TEST_OUTPUT_BASELINE_DIR}/${PYTHON_STATE_TEST_NAME}.png
  RESULT_VARIABLE rv)
if(NOT rv EQUAL 0)
  message(FATAL_ERROR "PVPython return value was ${rv}")
endif()
