PROJECT(PVServers)
INCLUDE_DIRECTORIES(
  ${ParaView_BINARY_DIR}
  ${ParaView_BINARY_DIR}/GUI/Client
  ${KWSYS_INCLUDE_DIR}
  ${PVSERVERCOMMON_INCLUDE_DIR}
  ${PVFILTERS_INCLUDE_DIR}
  ${PVSERVERMANAGER_INCLUDE_DIR}
  ${VTKCLIENTSERVER_INCLUDE_DIR}
  ${VTK_INCLUDE_DIR}
  ${XDMF_INCLUDE_DIRS}
  ${TCL_INCLUDE_PATH}
  )
SET(LIBS vtkPVCommon vtkPVFilters vtkPVServerManager)
IF(VTK_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF(VTK_USE_MPI)

# Add main server executables.
SET(PV_EXE_LIST pvserver pvrenderserver pvdataserver)
FOREACH(pvexe ${PV_EXE_LIST})
  ADD_EXECUTABLE(${pvexe}${PV_EXE_SUFFIX} ${pvexe}.cxx)
  TARGET_LINK_LIBRARIES(${pvexe}${PV_EXE_SUFFIX} ${LIBS})
  INSTALL_TARGETS(${PV_EXE_INSTALL} ${pvexe}${PV_EXE_SUFFIX})
ENDFOREACH(pvexe)

# Optionally add pvbatch executable.
IF (PARAVIEW_BUILD_GUI)
  ADD_EXECUTABLE(pvbatch${PV_EXE_SUFFIX} pvbatch.cxx)
  TARGET_LINK_LIBRARIES(pvbatch${PV_EXE_SUFFIX} ${LIBS} 
    vtkPVServerManagerTCL 
    vtkPVServerCommonTCL
    vtkCommonTCL)

  INSTALL_TARGETS(${PV_EXE_INSTALL} pvbatch${PV_EXE_SUFFIX})
  SET(PV_EXE_LIST ${PV_EXE_LIST} pvbatch)
ENDIF (PARAVIEW_BUILD_GUI)

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
    INSTALL_TARGETS(${PV_INSTALL_BIN_DIR} ${pvexe})
  ENDFOREACH(pvexe)
ENDIF(PV_NEED_SHARED_FORWARD)
