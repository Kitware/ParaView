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

ADD_EXECUTABLE(pvserver pvserver.cxx)
TARGET_LINK_LIBRARIES(pvserver ${LIBS})

ADD_EXECUTABLE(pvrenderserver pvrenderserver.cxx)
TARGET_LINK_LIBRARIES(pvrenderserver ${LIBS})

ADD_EXECUTABLE(pvdataserver pvdataserver.cxx)
TARGET_LINK_LIBRARIES(pvdataserver ${LIBS})

ADD_EXECUTABLE(pvbatch pvbatch.cxx)
TARGET_LINK_LIBRARIES(pvbatch ${LIBS} vtkPVServerManagerTCL vtkCommonTCL)

INSTALL_TARGETS(/bin pvserver pvrenderserver pvdataserver pvbatch)
