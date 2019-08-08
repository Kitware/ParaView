# CoProcessing test expects the following arguments to be passed to cmake using
# -DFoo=BAR arguments.
# PARAVIEW_EXECUTABLE -- path to paraview
# COPROCESSING_TEST_DIR    -- path to temporary dir
# PARAVIEW_TEST_XML -- xml to run
# PVBATCH_EXECUTABLE -- path to pvbatch
# PVPYTHON_EXECUTABLE -- path to pvpython
# COPROCESSING_DRIVER_SCRIPT -- driver py script
# COPROCESSING_IMAGE_TESTER -- path to CoProcessingCompareImagesTester
# COPROCESSING_DATA_DIR     -- path to data dir for baselines
# COPROCESSING_OUTPUTCHECK_SCRIPT -- path to outputcheck.py
# TEST_NAME -- a string to specify which results to test

# USE_MPI
# MPIEXEC
# MPIEXEC_NUMPROC_FLAG
# MPIEXEC_NUMPROCS
# MPIEXEC_PREFLAGS

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
  if("${TEST_NAME}" MATCHES "NoTimesteps" OR "${TEST_NAME}" MATCHES "FloatFiles")
    file(READ "${COPROCESSING_TEST_DIR}/${CINEMA_BATCH_SCRIPT}" batch_script_data)
    string(REGEX REPLACE "'cube.vtu'" "'input'" batch_script_data "${batch_script_data}")
    file(WRITE "${COPROCESSING_TEST_DIR}/${CINEMA_BATCH_SCRIPT}" "${batch_script_data}")
  else()
    file(READ "${COPROCESSING_TEST_DIR}/${CINEMA_BATCH_SCRIPT}" batch_script_data)
    string(REGEX REPLACE "'can.ex2'" "'input'" batch_script_data "${batch_script_data}")
    file(WRITE "${COPROCESSING_TEST_DIR}/${CINEMA_BATCH_SCRIPT}" "${batch_script_data}")
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

if(NOT EXISTS "${PVPYTHON_EXECUTABLE}")
  message(FATAL_ERROR "'${PVPYTHON_EXECUTABLE}' does not exist")
endif()

if(USE_MPI)
  message("${CINEMA_DATABASE_TESTER}")
  execute_process_with_echo(COMMAND
    ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_NUMPROCS} ${MPIEXEC_PREFLAGS}
    ${PVBATCH_EXECUTABLE}
    ${CINEMA_DATABASE_TESTER} 
    --interactive ${COPROCESSING_TEST_DIR}/cinema/interactive/${TEST_NAME}.cdb
    --batch ${COPROCESSING_TEST_DIR}/cinema/batch/${TEST_NAME}.cdb
    RESULT_VARIABLE rv)
  if(rv)
    message(FATAL_ERROR "CoProcessingCompareImageTester second image return value was = '${rv}' ")
  endif()
else()
  execute_process_with_echo(COMMAND
    ${PVBATCH_EXECUTABLE}
    ${CINEMA_DATABASE_TESTER} 
    --interactive ${COPROCESSING_TEST_DIR}/cinema/interactive/${TEST_NAME}.cdb
    --batch ${COPROCESSING_TEST_DIR}/cinema/batch/${TEST_NAME}.cdb
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
    message(FATAL_ERROR "Geometry did not export interactively (can.ex2_*.vtm)")
  endif()
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/batch/${TEST_NAME}.cdb/can.ex2_0.vtm") 
    message(FATAL_ERROR "Geometry did not export during batch (can.ex2_*.vtm)")
  endif()
  if(NOT "${TEST_NAME}" MATCHES "CinemaExportGeometryAndImages")
    return()
  endif()
elseif("${TEST_NAME}" MATCHES "CinemaExportNoTime")
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/interactive/${TEST_NAME}.cdb/cube.vtu_0.pvtu") 
    message(FATAL_ERROR "Geometry did not export interactively (cube.vtu_*.pvtu)")
  endif()
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/batch/${TEST_NAME}.cdb/cube.vtu_0.pvtu") 
    message(FATAL_ERROR "Geometry did not export during batch (cube.vtu_*.pvtu)")
  endif()
endif()

if("${TEST_NAME}" MATCHES "CinemaExportNoTime" OR "${TEST_NAME}" MATCHES "CinemaExportGeometryAnd")
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/interactive/${TEST_NAME}.cdb/RenderView1/info.json") 
    message(FATAL_ERROR "Image database did not export interactively (RenderView*)")
  endif()
  # There is an extra cinema directory for the batch use-case
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/batch/${TEST_NAME}.cdb/cinema/RenderView1/info.json")
    message(FATAL_ERROR "Image database did not export during batch (RenderView*)")
  endif()
  return()
endif()

if("${TEST_NAME}" MATCHES "CinemaExportFloatFiles")
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/interactive/${TEST_NAME}.cdb/RenderView1/time=0/vis=0/colorcube.vtu=0.Z") 
    message(FATAL_ERROR "Float files did not export interactively (*.Z)")
  endif()
  # There is an extra cinema directory for the batch use-case
  # colorinput instead of colorcube.vtu because of the way filedriver.py works
  if(NOT EXISTS "${COPROCESSING_TEST_DIR}/cinema/batch/${TEST_NAME}.cdb/cinema/RenderView1/time=0/vis=0/colorinput=0.Z") 
    message(FATAL_ERROR "Float files did not export during batch (*.Z)")
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
