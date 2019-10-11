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
  execute_process(${ARGV} WORKING_DIRECTORY ${COPROCESSING_TEST_DIR})
endmacro()

file(REMOVE_RECURSE
  "${COPROCESSING_TEST_DIR}/cptest.py"
  "${COPROCESSING_TEST_DIR}/cinema"
  "${COPROCESSING_TEST_DIR}/image_0.png"
  "${COPROCESSING_TEST_DIR}/image_0_0.png"
  "${COPROCESSING_TEST_DIR}/image_1_0.png"
  "${COPROCESSING_TEST_DIR}/filename_0.pvtp"
  "${COPROCESSING_TEST_DIR}/filename_0_0.vtp"
  "${COPROCESSING_TEST_DIR}/file_00.pvtp"
  "${COPROCESSING_TEST_DIR}/file_00_0.vtp"
  "${COPROCESSING_TEST_DIR}/Slice1_0"
  "${COPROCESSING_TEST_DIR}/Slice1_0.pvtp"
  "${COPROCESSING_TEST_DIR}/Slice1_4"
  "${COPROCESSING_TEST_DIR}/Slice1_4.pvtp"
  "${COPROCESSING_TEST_DIR}/RenderView1_4.png"
  "${COPROCESSING_TEST_DIR}/CinD"
  )

if (NOT EXISTS "${PARAVIEW_EXECUTABLE}")
  message(FATAL_ERROR "Could not file ParaView '${PARAVIEW_EXECUTABLE}'")
endif()

# On windows, execute_process runs PARAVIEW_EXECUTABLE in background.
# We prepend "cmd /c" to force paraview's window to be shown to ensure proper
# mouse interactions with the GUI.
if(WIN32)
  set(PARAVIEW_EXECUTABLE cmd /c ${PARAVIEW_EXECUTABLE})
endif()

# First run ParaView GUI and generate a catalyst script.
execute_process_with_echo(COMMAND
    ${PARAVIEW_EXECUTABLE} -dr
    --data-directory=${COPROCESSING_DATA_DIR}
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

# Then run a simulated Catalyzed simulation that does something and then runs the generated script.
if("${TEST_NAME}" STREQUAL "TemporalScriptFullWorkflow")
  execute_process_with_echo(COMMAND
    ${PVBATCH_EXECUTABLE} -sym -dr
    ${COPROCESSING_DRIVER_SCRIPT}
    WORKING_DIRECTORY ${COPROCESSING_TEST_DIR}
    RESULT_VARIABLE rv)
else()
  if("${TEST_NAME}" STREQUAL "ExportNow")
    #don't need batch for this, but the rest of the infrastructure
    #is handy
    set(rv 0)
  else()
    execute_process_with_echo(COMMAND
      ${PVBATCH_EXECUTABLE} -sym -dr
      ${COPROCESSING_DRIVER_SCRIPT}
      ${COPROCESSING_TEST_DIR}/cptest.py 1
      WORKING_DIRECTORY ${COPROCESSING_TEST_DIR}
      RESULT_VARIABLE rv)
  endif()
endif()

if(rv)
  message(FATAL_ERROR "pvbatch return value was = '${rv}' ")
endif()

if(COPROCESSING_IMAGE_TESTER AND NOT EXISTS "${COPROCESSING_IMAGE_TESTER}")
  message(FATAL_ERROR "Image tester executable: '${COPROCESSING_IMAGE_TESTER}' does not exist")
endif()

# Now validate that the output produced by the simulation matches what we expect.
# We expect different outputs from each test.
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

if("${TEST_NAME}" STREQUAL "CoProcessingFullWorkflowCinema")
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/image/info.json" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/image/0.000000e+00/-180/-90.png" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/image/0.000000e+00/-180/30.png" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/image/0.000000e+00/60/-90.png" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/image/0.000000e+00/60/30.png")
    message(FATAL_ERROR "Catalyst did not generate a spec A cinema store.")
  endif()
  return()
endif()

if("${TEST_NAME}" STREQUAL "CoProcessingFullWorkflowCinemaFilters")
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/cinema_filters/info.json" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/cinema_filters/0.000000e+00/-6.66.png" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/cinema_filters/0.000000e+00/0.png" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/cinema_filters/0.000000e+00/6.66.png")
    message(FATAL_ERROR "Catalyst did not generate a spec A cinema store.")
  endif()
  return()
endif()

if("${TEST_NAME}" STREQUAL "CoProcessingFullWorkflowCinemaComposite")
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/composite/info.json" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/composite/time=0/vis=0/Slice1=0")
    message(FATAL_ERROR "Cinema did not generate a good composite directory.")
  endif()
  file(GLOB rfiles LIST_DIRECTORIES false "${COPROCESSING_TEST_DIR}/cinema/composite/time=0/vis=0/Slice1=0/*.png")
  list(LENGTH rfiles lenrf)
  if(NOT lenrf EQUAL 1)
    message(FATAL_ERROR "Cinema did not generate an RGB image.")
  endif()
  file(GLOB rfiles LIST_DIRECTORIES false "${COPROCESSING_TEST_DIR}/cinema/composite/time=0/vis=0/Slice1=0/*.Z")
  list(LENGTH rfiles lenrf)
  if(NOT lenrf EQUAL 1)
    message(FATAL_ERROR "Cinema did not generate a depth image.")
  endif()
  return()
endif()

if("${TEST_NAME}" STREQUAL "CoProcessingFullWorkflowCinemaRecolorable")
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/recolorable/info.json" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/recolorable/time=0/vis=0")
    message(FATAL_ERROR "Cinema did not generate a good composite directory.")
  endif()
  file(GLOB rfiles LIST_DIRECTORIES false "${COPROCESSING_TEST_DIR}/cinema/recolorable/time=0/vis=0/*.png")
  list(LENGTH rfiles lenrf)
  if(NOT lenrf EQUAL 1)
    message(FATAL_ERROR "Cinema did not generate a luminance image.")
  endif()
  file(GLOB rfiles LIST_DIRECTORIES false "${COPROCESSING_TEST_DIR}/cinema/recolorable/time=0/vis=0/*.Z")
  list(LENGTH rfiles lenrf)
  if(NOT lenrf EQUAL 2)
    message(FATAL_ERROR "Cinema did not generate a depth or value image.")
  endif()
  return()
endif()

if("${TEST_NAME}" STREQUAL "CoProcessingFullWorkflowCinemaPose")
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/pose/info.json" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/pose/pose=0" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/pose/pose=1" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/pose/pose=2" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/pose/pose=3" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/pose/pose=4" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/pose/pose=5" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/pose/pose=6" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/pose/pose=7/time=0/vis=0")
    message(FATAL_ERROR "Cinema did not generate a good composite directory.")
  endif()
  file(GLOB rfiles LIST_DIRECTORIES false "${COPROCESSING_TEST_DIR}/cinema/pose/pose=0/time=0/vis=0/*.Z")
  list(LENGTH rfiles lenrf)
  if(NOT lenrf EQUAL 1)
    message(FATAL_ERROR "Cinema did not generate a depth image.")
  endif()
  return()
endif()

if("${TEST_NAME}" STREQUAL "TemporalScriptFullWorkflow")
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/Slice1_4.pvtp" OR
     NOT EXISTS "${COPROCESSING_TEST_DIR}/Slice1_4/Slice1_4_0.vtp")
    message(FATAL_ERROR "TemporalScript did not generate a data extract!")
  endif()
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/RenderView1_4.png")
    message(FATAL_ERROR "TemporalScript did not generate a screen capture!")
  endif()
  return()
endif()

if("${TEST_NAME}" STREQUAL "ExportNow")
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/CinD/Slice1_0.vtp" AND
     NOT EXISTS "${COPROCESSING_TEST_DIR}/CinD/Slice1_0.pvtp")
    message(FATAL_ERROR "ExportNow did not generate a data extract!")
  endif()
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/CinD/RenderView1_0.png")
    message(FATAL_ERROR "ExportNow did not generate a screen capture!")
  endif()
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/CinD/data.csv")
    message(FATAL_ERROR "ExportNow did not generate a CinemaD index!")
  endif()
  return()
endif()
