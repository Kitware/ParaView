# this cmake script tests python state file saving and loading. it
# first launches paraview, runs an xml test, and save the resulting
# state file. it then launches pvpython with a small test driver
# which loads the state file and then compares the resulting image.

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
  ${PARAVIEW_TEST_OUTPUT_DIR}/StateFile.py
  -T ${PARAVIEW_TEST_OUTPUT_DIR}
  -V ${PARAVIEW_TEST_OUTPUT_BASELINE_DIR}/SavePythonState.png
  RESULT_VARIABLE rv)
if(NOT rv EQUAL 0)
  message(FATAL_ERROR "PVPython return value was ${rv}")
endif()
