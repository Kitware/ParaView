PROJECT(VTKCS)

SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")

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

# OpenGL include directories.
IF(APPLE)
  IF(VTK_USE_X)
    SET(PVWRAP_OPEN_GL_DIRS ${PVWRAP_OPEN_GL_DIRS}
      ${OPENGL_INCLUDE_DIR})
  ENDIF(VTK_USE_X)
ELSE(APPLE)
  SET(PVWRAP_OPEN_GL_DIRS ${PVWRAP_OPEN_GL_DIRS}
    ${OPENGL_INCLUDE_DIR})
ENDIF(APPLE)

IF(VTK_USE_X)
  # X include directories.
  SET(PVWRAP_OPEN_GL_DIRS ${PVWRAP_OPEN_GL_DIRS}
    ${CMAKE_Xlib_INCLUDE_PATH} ${CMAKE_Xutil_INCLUDE_PATH})
ENDIF(VTK_USE_X)

IF(VTK_HAVE_VP1000)
  # VolumePro VP 1000 include directory.
  SET(PVWRAP_OPEN_GL_DIRS ${PVWRAP_OPEN_GL_DIRS}
    ${VLI_INCLUDE_PATH_FOR_VP1000})
ENDIF(VTK_HAVE_VP1000)

IF(VTK_USE_MANGLED_MESA)
  # Mangled Mesa include directory.
  SET(PVWRAP_OPEN_GL_DIRS ${PVWRAP_OPEN_GL_DIRS}
    ${MESA_INCLUDE_PATH})
ELSE(VTK_USE_MANGLED_MESA)
  # Off-Screen Mesa include directory.
  IF(VTK_OPENGL_HAS_OSMESA)
    IF(OSMESA_INCLUDE_DIR)
      SET(PVWRAP_OPEN_GL_DIRS ${PVWRAP_OPEN_GL_DIRS}
        ${OSMESA_INCLUDE_DIR})
    ENDIF(OSMESA_INCLUDE_DIR)
  ENDIF(VTK_OPENGL_HAS_OSMESA)
ENDIF(VTK_USE_MANGLED_MESA)

INCLUDE_DIRECTORIES(${PVWRAP_OPEN_GL_DIRS})

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
SUBDIRS(Common Filtering GenericFiltering Imaging Graphics IO Rendering Hybrid Parallel)
IF(VTK_USE_PATENTED)
  SUBDIRS(Patented)
ENDIF(VTK_USE_PATENTED)

# Build vtkClientServer wrappers for Xdmf if it is available.
IF(PARAVIEW_SOURCE_DIR OR ParaView_SOURCE_DIR)
  IF(PARAVIEW_LINK_XDMF)
    SUBDIRS(Xdmf)
  ENDIF(PARAVIEW_LINK_XDMF)
ENDIF(PARAVIEW_SOURCE_DIR OR ParaView_SOURCE_DIR)

IF(BUILD_SHARED_LIBS)
  INSTALL_TARGETS(/lib vtkClientServer)
ENDIF(BUILD_SHARED_LIBS)
