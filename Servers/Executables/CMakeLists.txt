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
  ADD_EXECUTABLE(${pvexe}${PV_EXE_SUFFIX} ${pvexe}.cxx)
  TARGET_LINK_LIBRARIES(${pvexe}${PV_EXE_SUFFIX} ${LIBS})
  # Install exe
  IF(NOT PV_INSTALL_NO_RUNTIME)
      INSTALL(TARGETS ${pvexe}${PV_EXE_SUFFIX} 
        DESTINATION ${PV_EXE_INSTALL}
        COMPONENT Runtime)
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

ADD_EXECUTABLE(smTestDriver${PV_EXE_SUFFIX} vtkSMTestDriver.cxx)
TARGET_LINK_LIBRARIES(smTestDriver${PV_EXE_SUFFIX} vtksys)
IF (NOT PV_INSTALL_NO_RUNTIME)
  INSTALL(TARGETS smTestDriver${PV_EXE_SUFFIX} 
    DESTINATION ${PV_EXE_INSTALL}
    COMPONENT Runtime)
ENDIF (NOT PV_INSTALL_NO_RUNTIME)
SET (PV_EXE_LIST ${PV_EXE_LIST} smTestDriver)

# Add shared link forwarding executables if necessary.
IF(PV_NEED_SHARED_FORWARD)
  FOREACH(pvexe ${PV_EXE_LIST})
    SET(PV_FORWARD_EXE ${pvexe}${PV_EXE_SUFFIX})
    CONFIGURE_FILE(
      ${ParaView_SOURCE_DIR}/Servers/Executables/pv-forward.c.in
      ${CMAKE_CURRENT_BINARY_DIR}/${pvexe}-forward.c
      @ONLY IMMEDIATE)
    ADD_EXECUTABLE(${pvexe} ${CMAKE_CURRENT_BINARY_DIR}/${pvexe}-forward.c)
    ADD_DEPENDENCIES(${pvexe} ${pvexe}${PV_EXE_SUFFIX})
    IF(NOT PV_INSTALL_NO_RUNTIME)
        # Install
        INSTALL(TARGETS ${pvexe}
          DESTINATION ${PV_INSTALL_BIN_DIR}
          COMPONENT Runtime)
    ENDIF(NOT PV_INSTALL_NO_RUNTIME)
  ENDFOREACH(pvexe)
ENDIF(PV_NEED_SHARED_FORWARD)
