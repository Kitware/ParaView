# CoProcessing test expects the following arguments to be passed to cmake using
# -DFoo=BAR arguments.
# PARAVIEW_EXECUTABLE -- path to paraview
# COPROCESSING_TEST_DIR    -- path to temporary dir
# PARAVIEW_TEST_XML -- xml to run
# PVPYTHON_EXECUTABLE -- path to pvpython
# COPROCESSING_DRIVER_SCRIPT -- driver py script
# COPROCESSING_IMAGE_TESTER -- path to CoProcessingCompareImagesTester
# COPROCESSING_DATA_DIR     -- path to data dir for baselines
# COPROCESSING_OUTPUTCHECK_SCRIPT -- path to outputcheck.py
# DO_CINEMA_TEST -- a flag that makes this expect cinema files

macro(execute_process_with_echo)
  set (_cmd)
  foreach (arg ${ARGV})
    set (_cmd "${_cmd} ${arg}")
  endforeach()
  message(STATUS "Executing command: \n    ${_cmd}")
  execute_process(${ARGV})
endmacro()

file(REMOVE
  "${COPROCESSING_TEST_DIR}/cptest.py"
  "${COPROCESSING_TEST_DIR}/image_0.png"
  "${COPROCESSING_TEST_DIR}/filename_0.pvtp"
  "${COPROCESSING_TEST_DIR}/filename_0_0.vtp"
  "${COPROCESSING_TEST_DIR}/cinema")

if (NOT EXISTS "${PARAVIEW_EXECUTABLE}")
  message(FATAL_ERROR "Could not file ParaView '${PARAVIEW_EXECUTABLE}'")
endif()

execute_process_with_echo(COMMAND
    ${PARAVIEW_EXECUTABLE} -dr
    --test-plugin=CatalystScriptGeneratorPlugin
    --test-directory=${COPROCESSING_TEST_DIR}
    --test-script=${PARAVIEW_TEST_XML}
    --exit
  RESULT_VARIABLE rv)
if(rv)
  message(FATAL_ERROR "ParaView return value was ${rv}")
endif()

if(NOT EXISTS "${PVPYTHON_EXECUTABLE}")
  message(FATAL_ERROR "'${PVPYTHON_EXECUTABLE}' does not exist")
endif()


message("Running pvpython")
execute_process_with_echo(COMMAND
  ${PVPYTHON_EXECUTABLE} -dr
  ${COPROCESSING_DRIVER_SCRIPT}
  ${COPROCESSING_TEST_DIR}/cptest.py 1
  WORKING_DIRECTORY ${COPROCESSING_TEST_DIR}
  RESULT_VARIABLE rv)
if(rv)
  message(FATAL_ERROR "pvpython return value was = '${rv}' ")
endif()

if(DO_CINEMA_TEST)
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/image/info.json" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/image/0.000000e+00/-180/-180.png" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/image/0.000000e+00/-180/60.png" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/image/0.000000e+00/60/-180.png" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/image/0.000000e+00/60/60.png")
    message(FATAL_ERROR "Catalyst did not generate a cinema store")
  endif()
  return()
endif()

if(NOT EXISTS "${COPROCESSING_IMAGE_TESTER}")
  message(FATAL_ERROR "'${COPROCESSING_IMAGE_TESTER}' does not exist")
endif()

message("${COPROCESSING_IMAGE_TESTER} ${COPROCESSING_TEST_DIR}/image_0.png -V
  ${COPROCESSING_DATA_DIR}/CPFullWorkflow.png -T
  ${COPROCESSING_TEST_DIR}")
execute_process_with_echo(COMMAND
  ${COPROCESSING_IMAGE_TESTER} ${COPROCESSING_TEST_DIR}/image_0.png 20 -V ${COPROCESSING_DATA_DIR}/CPFullWorkflow.png -T ${COPROCESSING_TEST_DIR}
  RESULT_VARIABLE rv)
if(rv)
  message(FATAL_ERROR "CoProcessingCompareImageTester return value was = '${rv}' ")
endif()

execute_process_with_echo(COMMAND
  ${PVPYTHON_EXECUTABLE} -dr
  ${COPROCESSING_OUTPUTCHECK_SCRIPT}
  ${COPROCESSING_TEST_DIR}/filename_0.pvtp
  RESULT_VARIABLE rv)
if(rv)
  message(FATAL_ERROR "vtkpython return value was = '${rv}' ")
endif()
