# CoProcessing test expects the following arguments to be passed to cmake using
# -DFoo=BAR arguments.
# PARAVIEW_EXECUTABLE -- path to paraview
# COPROCESSING_TEST_DIR    -- path to temporary dir
# PARAVIEW_TEST_XML -- xml to run
# PVBATCH_EXECUTABLE -- path to pvbatch
# COPROCESSING_DRIVER_SCRIPT -- driver py script
# COPROCESSING_IMAGE_TESTER -- path to CoProcessingCompareImagesTester
# COPROCESSING_DATA_DIR     -- path to data dir for baselines
# COPROCESSING_OUTPUTCHECK_SCRIPT -- path to outputcheck.py
# TEST_NAME -- a string to specify which results to test

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
  "${COPROCESSING_TEST_DIR}/cpplottest.py"
  "${COPROCESSING_TEST_DIR}/image_0.png"
  "${COPROCESSING_TEST_DIR}/image_0_0.png"
  "${COPROCESSING_TEST_DIR}/image_1_0.png"
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

if(NOT EXISTS "${PVBATCH_EXECUTABLE}")
  message(FATAL_ERROR "'${PVBATCH_EXECUTABLE}' does not exist")
endif()

message("Running pvbatch")
execute_process_with_echo(COMMAND
  ${PVBATCH_EXECUTABLE} -sym -dr
  ${COPROCESSING_DRIVER_SCRIPT}
  ${COPROCESSING_TEST_DIR}/cptest.py 1
  WORKING_DIRECTORY ${COPROCESSING_TEST_DIR}
  RESULT_VARIABLE rv)
if(rv)
  message(FATAL_ERROR "pvbatch return value was = '${rv}' ")
endif()

if("${TEST_NAME}" STREQUAL "CoProcessingFullWorkflowCinema")
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/image/info.json" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/image/0.000000e+00/-180/-90.png" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/image/0.000000e+00/-180/30.png" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/image/0.000000e+00/60/-90.png" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/image/0.000000e+00/60/30.png")
    message(FATAL_ERROR "Catalyst did not generate a cinema store")
  endif()
  return()
endif()

if("${TEST_NAME}" STREQUAL "CoProcessingFullWorkflowCinemaComposite")
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/composite_image/info.json" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/composite_image/phi=0/theta=0/time=0/vis=0/Slice1=0/colorSlice1=0.Z" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/composite_image/phi=0/theta=0/time=0/vis=0/Slice1=0/colorSlice1=1.png" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/composite_image/phi=0/theta=0/time=0/vis=0/Slice1=0/colorSlice1=2.Z")
    message(FATAL_ERROR "Catalyst did not generate a composite cinema store!")
  endif()
  return()
endif()

if("${TEST_NAME}" STREQUAL "CoProcessingFullWorkflowCinemaCompositeFloat")
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/composite_fl_image/info.json" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/composite_fl_image/phi=0/theta=0/time=0/vis=0/Slice1=0/colorSlice1=0.Z" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/composite_fl_image/phi=0/theta=0/time=0/vis=0/Slice1=0/colorSlice1=1.png" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/composite_fl_image/phi=0/theta=0/time=0/vis=0/Slice1=0/colorSlice1=2.Z")
    message(FATAL_ERROR "Catalyst did not generate a composite cinema store (float value images)!")
  endif()
  return()
endif()

if(NOT EXISTS "${COPROCESSING_IMAGE_TESTER}")
  message(FATAL_ERROR "'${COPROCESSING_IMAGE_TESTER}' does not exist")
endif()

if("${TEST_NAME}" STREQUAL "CoProcessingFullWorkflow")
  message("${COPROCESSING_IMAGE_TESTER} ${COPROCESSING_TEST_DIR}/image_0.png -V
  ${COPROCESSING_DATA_DIR}/CPFullWorkflow.png -T
  ${COPROCESSING_TEST_DIR}")
  execute_process_with_echo(COMMAND
    ${COPROCESSING_IMAGE_TESTER} ${COPROCESSING_TEST_DIR}/image_0.png 40 -V ${COPROCESSING_DATA_DIR}/CPFullWorkflow.png -T ${COPROCESSING_TEST_DIR}
    RESULT_VARIABLE rv)
  if(rv)
    message(FATAL_ERROR "CoProcessingCompareImageTester return value was = '${rv}' ")
  endif()

  execute_process_with_echo(COMMAND
    ${PVBATCH_EXECUTABLE} -dr
    ${COPROCESSING_OUTPUTCHECK_SCRIPT}
    ${COPROCESSING_TEST_DIR}/filename_0.pvtp
    RESULT_VARIABLE rv)
  if(rv)
    message(FATAL_ERROR "vtkpython return value was = '${rv}' ")
  endif()
endif()

if("${TEST_NAME}" STREQUAL "CoProcessingFullWorkflowWithPlots")
  message("${COPROCESSING_IMAGE_TESTER} ${COPROCESSING_TEST_DIR}/image_0_0.png -V
  ${COPROCESSING_DATA_DIR}/CPFullWorkflowPlot1.png -T
  ${COPROCESSING_TEST_DIR}")
  execute_process_with_echo(COMMAND
    ${COPROCESSING_IMAGE_TESTER} ${COPROCESSING_TEST_DIR}/image_0_0.png 20 -V ${COPROCESSING_DATA_DIR}/CPFullWorkflowPlot1.png -T ${COPROCESSING_TEST_DIR}
    RESULT_VARIABLE rv)
  if(rv)
    message(FATAL_ERROR "CoProcessingCompareImageTester first image return value was = '${rv}' ")
  endif()

  message("${COPROCESSING_IMAGE_TESTER} ${COPROCESSING_TEST_DIR}/image_0_1.png -V
  ${COPROCESSING_DATA_DIR}/CPFullWorkflowPlot2.png -T
  ${COPROCESSING_TEST_DIR}")
  execute_process_with_echo(COMMAND
    ${COPROCESSING_IMAGE_TESTER} ${COPROCESSING_TEST_DIR}/image_1_0.png 40 -V ${COPROCESSING_DATA_DIR}/CPFullWorkflowPlot2.png -T ${COPROCESSING_TEST_DIR}
    RESULT_VARIABLE rv)
  if(rv)
    message(FATAL_ERROR "CoProcessingCompareImageTester second image return value was = '${rv}' ")
  endif()
endif()

if("${TEST_NAME}" STREQUAL "CoProcessingFullWorkflowWithOnlyPlots")
  message("${COPROCESSING_IMAGE_TESTER} ${COPROCESSING_TEST_DIR}/image_0.png -V
  ${COPROCESSING_DATA_DIR}/CPFullWorkflowPlot2.png -T
  ${COPROCESSING_TEST_DIR}")
  execute_process_with_echo(COMMAND
    ${COPROCESSING_IMAGE_TESTER} ${COPROCESSING_TEST_DIR}/image_0.png 40 -V ${COPROCESSING_DATA_DIR}/CPFullWorkflowPlot2.png -T ${COPROCESSING_TEST_DIR}
    RESULT_VARIABLE rv)
  if(rv)
    message(FATAL_ERROR "CoProcessingCompareImageTester second image return value was = '${rv}' ")
  endif()
endif()
