PROJECT(VTKCS)

# Support building this project outside of ParaView.
IF(NOT PARAVIEW_SOURCE_DIR)
  FIND_PACKAGE(VTK)

  IF(NOT VTK_FOUND)
    MESSAGE(FATAL_ERROR "VTK is required.  Please set VTK_DIR.")
  ENDIF(NOT VTK_FOUND)

  INCLUDE(${VTK_USE_FILE})
  INCLUDE_DIRECTORIES(${VTKCS_SOURCE_DIR})
  INCLUDE_DIRECTORIES(${VTKCS_BINARY_DIR})

  # load the ClientServer command
  IF (COMMAND VTK_WRAP_ClientServer)
  ELSE (COMMAND VTK_WRAP_ClientServer)
    TRY_COMPILE(COMPILE_OK
      ${VTKCS_BINARY_DIR}/CMake
      ${VTKCS_SOURCE_DIR}/CMake
      ClientServer_LOADED_COMMANDS OUTPUT_VARIABLE TRYOUT)
    IF (COMPILE_OK)
      LOAD_COMMAND(VTK_WRAP_ClientServer
        ${VTKCS_BINARY_DIR}/CMake
        ${VTKCS_BINARY_DIR}/CMake/Debug)
    ELSE (COMPILE_OK)
      MESSAGE("failed to compile ParaView ClientServer extensions to CMake:\n${TRYOUT}")
    ENDIF (COMPILE_OK)
  ENDIF (COMMAND VTK_WRAP_ClientServer)
  UTILITY_SOURCE(VTK_WRAP_ClientServer_EXE vtkWrapClientServer Wrapping vtkWrapClientServer.c)
  SET(LIBRARY_OUTPUT_PATH ${VTKCS_BINARY_DIR}/bin CACHE PATH "Single output path for libraries")
  SET(EXECUTABLE_OUTPUT_PATH ${VTKCS_BINARY_DIR}/bin CACHE PATH "Single output path for executable")
  SET(BUILD_SHARED_LIBS ${VTK_BUILD_SHARED_LIBS})
  MARK_AS_ADVANCED(VTK_WRAP_ClientServer_EXE)
ENDIF(NOT PARAVIEW_SOURCE_DIR)

# Configure the vtkClientServer library.
SET(VTK_CLIENT_SERVER_BUILD_SHARED ${BUILD_SHARED_LIBS})
CONFIGURE_FILE(
  ${VTKCS_SOURCE_DIR}/vtkClientServerConfigure.h.in
  ${VTKCS_BINARY_DIR}/vtkClientServerConfigure.h
  @ONLY IMMEDIATE
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

INCLUDE(${VTKCS_SOURCE_DIR}/vtkClientServer.cmake)

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
