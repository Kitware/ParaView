PROJECT(PVServers)
INCLUDE_DIRECTORIES(
  ${ParaView_BINARY_DIR}
  ${PVSERVERCOMMON_INCLUDE_DIR}
  ${PVFILTERS_INCLUDE_DIR}
  ${PVSERVERMANAGER_INCLUDE_DIR}
  ${VTKCLIENTSERVER_INCLUDE_DIR}
  ${VTK_INCLUDE_DIR}
  ${XDMF_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  )
SET(LIBS vtkPVServerCommon vtkPVFilters vtkPVServerManager)
IF(VTK_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF(VTK_USE_MPI)

IF(VTK_WRAP_TCL)
  INCLUDE_DIRECTORIES(${TCL_INCLUDE_PATH})
ENDIF(VTK_WRAP_TCL)

# Add main server executables.
SET(PV_EXE_LIST pvserver pvrenderserver pvdataserver)
FOREACH(pvexe ${PV_EXE_LIST})
  add_executable_with_forwarding(pv_exe_suffix
    ${pvexe} ${pvexe}.cxx)
  TARGET_LINK_LIBRARIES(${pvexe}${pv_exe_suffix} ${LIBS})
  # Install exe
  IF(NOT PV_INSTALL_NO_RUNTIME)
    INSTALL(TARGETS ${pvexe}
            DESTINATION ${PV_INSTALL_BIN_DIR}
            COMPONENT Runtime)
    IF (pv_exe_suffix)
      # Shared forwarding enabled.
      INSTALL(TARGETS ${pvexe}${pv_exe_suffix}
              DESTINATION ${PV_INSTALL_LIB_DIR}
              COMPONENT Runtime)
    ENDIF (pv_exe_suffix)
  ENDIF(NOT PV_INSTALL_NO_RUNTIME)

ENDFOREACH(pvexe)

SET(PV_TEST_INIT_COMMAND "" CACHE STRING
  "Command to run before a test begins. Multiple commands are separated by ';'.")
MARK_AS_ADVANCED(PV_TEST_INIT_COMMAND)

SET(PV_TEST_CLEAN_COMMAND "" CACHE STRING
  "Command to run after a failed test to cleanup processes.  Example: \"killall -9 rsh paraview\"")
MARK_AS_ADVANCED(PV_TEST_CLEAN_COMMAND)

SET (PV_TEST_CLIENT "localhost" CACHE STRING
  "Node which serves as the client node, used to connect from the server side in reverse connection mode.")
MARK_AS_ADVANCED(PV_TEST_CLIENT)

# Add the test driver executable. This is not done in the above loop to avoid
# adding dependencies ${LIBS} since we only need vtksys for this executable.
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/vtkSMTestDriverConfig.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkSMTestDriverConfig.h @ONLY ESCAPE_QUOTES)

add_executable_with_forwarding(pv_exe_suffix smTestDriver vtkSMTestDriver.cxx)
TARGET_LINK_LIBRARIES(smTestDriver${pv_exe_suffix} vtksys)

IF (NOT PV_INSTALL_NO_RUNTIME)
  INSTALL(TARGETS smTestDriver
          DESTINATION ${PV_INSTALL_BIN_DIR}
          COMPONENT Runtime)
  IF (pv_exe_suffix)
    # Shared forwarding enabled.
    INSTALL(TARGETS smTestDriver${pv_exe_suffix} 
            DESTINATION ${PV_INSTALL_LIB_DIR}
            COMPONENT Runtime)
  ENDIF (pv_exe_suffix)
ENDIF (NOT PV_INSTALL_NO_RUNTIME)

