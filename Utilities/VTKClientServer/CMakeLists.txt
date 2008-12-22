PROJECT(VTKCS)

SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")

INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/vtkClientServer.cmake)

IF(NOT PARAVIEW_BINARY_DIR)
  SET(VTK_KITS_DIR "${ParaView_BINARY_DIR}/VTK/Utilities")
ENDIF(NOT PARAVIEW_BINARY_DIR)

# Configure the vtkClientServer library.
SET(VTK_CLIENT_SERVER_BUILD_SHARED ${BUILD_SHARED_LIBS})
CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkClientServerConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/vtkClientServerConfigure.h
  @ONLY IMMEDIATE
  )

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
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

# Build the vtkClientServer wrapping utility first.
ADD_SUBDIRECTORY(Wrapping)

CS_INITIALIZE_WRAP()

# Create vtkClientServer library.
ADD_LIBRARY(vtkClientServer
  vtkClientServerID.cxx
  vtkClientServerID.h
  vtkClientServerInterpreter.cxx
  vtkClientServerInterpreter.h
  vtkClientServerStream.cxx
  vtkClientServerStream.h
  )
TARGET_LINK_LIBRARIES(vtkClientServer vtkCommon)

IF(NOT PV_INSTALL_NO_DEVELOPMENT)
  # Headers etc...
  SET(sourceFiles
     "${CMAKE_CURRENT_SOURCE_DIR}/vtkClientServerID.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/vtkClientServerInterpreter.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/vtkClientServerStream.h"
     "${CMAKE_CURRENT_BINARY_DIR}/vtkClientServerConfigure.h")
  INSTALL(
      FILES ${sourceFiles}
      DESTINATION ${PV_INSTALL_INCLUDE_DIR}
      COMPONENT Development)
  # CMake files
  SET(configFiles
      "${CMAKE_CURRENT_SOURCE_DIR}/vtkClientServer.cmake"
      "${CMAKE_CURRENT_SOURCE_DIR}/CMake/vtkWrapClientServer.cmake"
      "${CMAKE_CURRENT_SOURCE_DIR}/CMake/vtkWrapClientServer.cxx.in")
  INSTALL(
      FILES ${configFiles}
      DESTINATION ${PV_INSTALL_LIB_DIR}
      COMPONENT Development)
  # Wrapper exec
  INSTALL(
      FILES "${ParaView_BINARY_DIR}/bin/vtkWrapClientServer"
      DESTINATION ${PV_INSTALL_BIN_DIR}
      COMPONENT Development)
ENDIF(NOT PV_INSTALL_NO_DEVELOPMENT)

# Build vtkClientServer wrappers for VTK kits.
ADD_SUBDIRECTORY(Common)
ADD_SUBDIRECTORY(Filtering)
ADD_SUBDIRECTORY(GenericFiltering)
ADD_SUBDIRECTORY(Imaging)
ADD_SUBDIRECTORY(Graphics)
ADD_SUBDIRECTORY(IO)
ADD_SUBDIRECTORY(Rendering)
ADD_SUBDIRECTORY(Hybrid)
ADD_SUBDIRECTORY(Parallel)
ADD_SUBDIRECTORY(VolumeRendering)
ADD_SUBDIRECTORY(Widgets)

# Build vtkClientServer wrappers for Xdmf if it is available.
IF(PARAVIEW_SOURCE_DIR OR ParaView_SOURCE_DIR)
  ADD_SUBDIRECTORY(Xdmf2)
ENDIF(PARAVIEW_SOURCE_DIR OR ParaView_SOURCE_DIR)

# Build vtkClientServer wrappers for Infovis if it is enabled
IF(VTK_USE_INFOVIS)
  ADD_SUBDIRECTORY(Infovis)
ENDIF(VTK_USE_INFOVIS)

# Build vtkClientServer wrappers for Views if they are enabled
IF(VTK_USE_VIEWS)
  ADD_SUBDIRECTORY(Views)
ENDIF(VTK_USE_VIEWS)

IF(NOT PV_INSTALL_NO_LIBRARIES)
  INSTALL(TARGETS vtkClientServer
    RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
    LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
    ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development)
ENDIF(NOT PV_INSTALL_NO_LIBRARIES)

IF(BUILD_TESTING)
  ADD_SUBDIRECTORY(Testing)
ENDIF(BUILD_TESTING)
