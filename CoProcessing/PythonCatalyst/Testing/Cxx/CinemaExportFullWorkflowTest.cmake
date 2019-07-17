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

file(REMOVE
  "${COPROCESSING_TEST_DIR}/cinema"
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

if("${TEST_NAME}" STREQUAL "ExportNow" )
  #don't need batch for this, but the rest of the infrastructure
  #is handy
  set(rv 0)
elseif("${TEST_NAME}" MATCHES "CinemaExport" )
  if("${TEST_NAME}" MATCHES "NoTimesteps" )
    if(WIN32)
    # prepping the output python script
    execute_process_with_echo(COMMAND
      @powershell -Command "(get-content ${COPROCESSING_TEST_DIR}/${CINEMA_BATCH_SCRIPT}).replace(\"'cube.vtu'\",\"'input'\") | set-content -Path ${COPROCESSING_TEST_DIR}/${CINEMA_BATCH_SCRIPT}"
      )
    else()
    # prepping the output python script
    execute_process_with_echo(COMMAND
      perl -i -pe "s/'cube.vtu'/'input'/g" ${COPROCESSING_TEST_DIR}/${CINEMA_BATCH_SCRIPT}
      )
    endif()
  else()
    if(WIN32)
    # prepping the output python script
    execute_process_with_echo(COMMAND
      @powershell -Command "(get-content ${COPROCESSING_TEST_DIR}/${CINEMA_BATCH_SCRIPT}).replace(\"'can.ex2'\",\"'input'\") | set-content -Path ${COPROCESSING_TEST_DIR}/${CINEMA_BATCH_SCRIPT}"
      )
    else()
    # prepping the output python script
    execute_process_with_echo(COMMAND
      perl -i -pe "s/'can.ex2'/'input'/g" ${COPROCESSING_TEST_DIR}/${CINEMA_BATCH_SCRIPT}
      )
    endif()
  endif()


  # run the batch script 
  execute_process_with_echo(COMMAND
    ${PVBATCH_EXECUTABLE} -sym -dr
    ${COPROCESSING_DRIVER_SCRIPT}
    ${CINEMA_INPUT_DATA}
    ${COPROCESSING_TEST_DIR}/${CINEMA_BATCH_SCRIPT}
    WORKING_DIRECTORY ${COPROCESSING_TEST_DIR}
    RESULT_VARIABLE rv)
endif()

if(rv)
  message(FATAL_ERROR "pvbatch return value was = '${rv}' ")
endif()

if("${TEST_NAME}" STREQUAL "CinemaExportGeometry")
  message("${CINEMA_DATABASE_TESTER}")
  execute_process_with_echo(COMMAND 
    ${CINEMA_DATABASE_TESTER} 
    --interactive ${COPROCESSING_TEST_DIR}/cinema/interactive/CinemaExportGeometry.cdb
    --batch ${COPROCESSING_TEST_DIR}/cinema/batch/CinemaExportGeometry.cdb
    RESULT_VARIABLE rv)
  if(rv)
    message(FATAL_ERROR "CoProcessingCompareImageTester second image return value was = '${rv}' ")
  endif()
elseif("${TEST_NAME}" STREQUAL "CinemaExportGeometryAndImages")
  message("${CINEMA_DATABASE_TESTER}")
  execute_process_with_echo(COMMAND 
    ${CINEMA_DATABASE_TESTER} 
    --interactive ${COPROCESSING_TEST_DIR}/cinema/interactive/CinemaExportGeometryAndImages.cdb
    --batch ${COPROCESSING_TEST_DIR}/cinema/batch/CinemaExportGeometryAndImages.cdb
    RESULT_VARIABLE rv)
  if(rv)
    message(FATAL_ERROR "CoProcessingCompareImageTester second image return value was = '${rv}' ")
  endif()
elseif("${TEST_NAME}" STREQUAL "CinemaExportNoTimesteps")
  message("${CINEMA_DATABASE_TESTER}")
  execute_process_with_echo(COMMAND 
    ${CINEMA_DATABASE_TESTER} 
    --interactive ${COPROCESSING_TEST_DIR}/cinema/interactive/CinemaExportNoTimesteps.cdb
    --batch ${COPROCESSING_TEST_DIR}/cinema/batch/CinemaExportNoTimesteps.cdb
    RESULT_VARIABLE rv)
  if(rv)
    message(FATAL_ERROR "CoProcessingCompareImageTester second image return value was = '${rv}' ")
  endif()
endif()

if("${TEST_NAME}" MATCHES "CinemaExport")
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/${TEST_NAME}.py")
    message(FATAL_ERROR "Catalyst Script did not export successfully")
  endif()
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/interactive/${TEST_NAME}.cdb/data.csv") 
    message(FATAL_ERROR "Cinema Spec-D Table did not export interactively (data.csv)")
  endif()
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/batch/${TEST_NAME}.cdb/data.csv") 
    message(FATAL_ERROR "Cinema Spec-D Table did not export during batch (data.csv)")
  endif()
endif()

if("${TEST_NAME}" MATCHES "CinemaExportGeometry")
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/interactive/${TEST_NAME}.cdb/can.ex2_0.vtm") 
    message(FATAL_ERROR "Geometry did not export during interactive (can.ex2_*.vtm)")
  endif()
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/batch/${TEST_NAME}.cdb/can.ex2_0.vtm") 
    message(FATAL_ERROR "Geometry did not export during batch (can.ex2_*.vtm)")
  endif()
  return()
elseif("${TEST_NAME}" MATCHES "CinemaExportNoTime")
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/interactive/${TEST_NAME}.cdb/cube.vtu_0.pvtu") 
    message(FATAL_ERROR "Geometry did not export during interactive (cube.vtu_*.pvtu)")
  endif()
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/batch/${TEST_NAME}.cdb/cube.vtu_0.pvtu") 
    message(FATAL_ERROR "Geometry did not export during batch (cube.vtu_*.pvtu)")
  endif()
  return()
endif()

# There is an extra cinema directory for the batch use-case
if("${TEST_NAME}" MATCHES "CinemaExportGeometryAndImages")
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/batch/${TEST_NAME}.cdb/cinema/RenderView1/info.json") 
    message(FATAL_ERROR "Cinema images did not export during batch")
  endif()
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/interactive/${TEST_NAME}.cdb/RenderView1/info.json") 
    message(FATAL_ERROR "Cinema images did not export during batch")
  endif()
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
