PROJECT(VTKCS)

INCLUDE(${VTKCS_SOURCE_DIR}/vtkClientServer.cmake)
IF(PARAVIEW_BINARY_DIR)
ELSE(PARAVIEW_BINARY_DIR)
  SET(VTK_KITS_DIR "${ParaView_BINARY_DIR}/VTK/Utilities")
ENDIF(PARAVIEW_BINARY_DIR)
CS_INITIALIZE_WRAP()

# Configure the vtkClientServer library.
SET(VTK_CLIENT_SERVER_BUILD_SHARED ${BUILD_SHARED_LIBS})
CONFIGURE_FILE(
  ${VTKCS_SOURCE_DIR}/vtkClientServerConfigure.h.in
  ${VTKCS_BINARY_DIR}/vtkClientServerConfigure.h
  @ONLY IMMEDIATE
  )

INCLUDE_DIRECTORIES(
  ${VTKCS_SOURCE_DIR}
  ${VTKCS_BINARY_DIR}
  ${VTK_INCLUDE_DIR}
  )

# Create vtkClientServer library.
ADD_LIBRARY(vtkClientServer
  vtkClientServerID.h
  vtkClientServerInterpreter.cxx
  vtkClientServerInterpreter.h
  vtkClientServerStream.cxx
  vtkClientServerStream.h
  vtkType.h
  vtkTypeFromNative.h
  )
TARGET_LINK_LIBRARIES(vtkClientServer vtkCommon)

IF(PARAVIEW_SOURCE_DIR)
  INSTALL_FILES(${KW_INSTALL_INCLUDE_DIR} FILES
    ${VTKCS_BINARY_DIR}/vtkClientServerConfigure.h
    ${VTKCS_SOURCE_DIR}/vtkClientServerID.h
    ${VTKCS_SOURCE_DIR}/vtkClientServerInterpreter.h
    ${VTKCS_SOURCE_DIR}/vtkClientServerStream.h
    ${VTKCS_SOURCE_DIR}/vtkType.h
    ${VTKCS_SOURCE_DIR}/vtkTypeFromNative.h
    )
  INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} vtkClientServer)
ENDIF(PARAVIEW_SOURCE_DIR)


# Build the vtkClientServer wrapping utility first.
SUBDIRS(Wrapping)

# Build vtkClientServer wrappers for VTK kits.
SUBDIRS(Common Filtering Imaging Graphics IO Rendering Hybrid Parallel)
IF(VTK_USE_PATENTED)
  SUBDIRS(Patented)
ENDIF(VTK_USE_PATENTED)

# Build vtkClientServer wrappers for Xdmf if it is available.
IF(PARAVIEW_SOURCE_DIR)
  IF(PARAVIEW_LINK_XDMF)
    SUBDIRS(Xdmf)
  ENDIF(PARAVIEW_LINK_XDMF)
ENDIF(PARAVIEW_SOURCE_DIR)
