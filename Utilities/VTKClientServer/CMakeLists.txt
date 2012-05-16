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

# Off-Screen Mesa include directory.
IF(VTK_OPENGL_HAS_OSMESA)
  IF(OSMESA_INCLUDE_DIR)
    SET(PVWRAP_OPEN_GL_DIRS ${PVWRAP_OPEN_GL_DIRS}
      ${OSMESA_INCLUDE_DIR})
  ENDIF(OSMESA_INCLUDE_DIR)
ENDIF(VTK_OPENGL_HAS_OSMESA)

INCLUDE_DIRECTORIES(${PVWRAP_OPEN_GL_DIRS})

# Build the vtkClientServer wrapping utility first.
ADD_SUBDIRECTORY(Wrapping)

CS_INITIALIZE_WRAP()

# Create vtkClientServer library.
PVVTK_ADD_LIBRARY(vtkClientServer
  vtkClientServerID.cxx
  vtkClientServerID.h
  vtkClientServerInterpreter.cxx
  vtkClientServerInterpreter.h
  vtkClientServerStream.cxx
  vtkClientServerStream.h)


TARGET_LINK_LIBRARIES(vtkClientServer vtkCommonCore)

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
      DESTINATION ${PV_INSTALL_CMAKE_DIR}
      COMPONENT Development)
ENDIF(NOT PV_INSTALL_NO_DEVELOPMENT)

# Build vtkClientServer wrappers for Xdmf if it is available.
IF(PARAVIEW_SOURCE_DIR OR ParaView_SOURCE_DIR)
  ADD_SUBDIRECTORY(Xdmf2)
ENDIF(PARAVIEW_SOURCE_DIR OR ParaView_SOURCE_DIR)

foreach(_vtk_module ${VTK_MODULES_ENABLED})
  if(NOT ${_vtk_module}_EXCLUDE_FROM_WRAPPING) 
    pv_wrap_vtk_mod_cs(${_vtk_module})
  endif()
endforeach()

IF(PARAVIEW_USE_VISITBRIDGE)
  ADD_SUBDIRECTORY(VisItBridge)
ENDIF(PARAVIEW_USE_VISITBRIDGE)

IF(NOT PV_INSTALL_NO_LIBRARIES)
  INSTALL(TARGETS vtkClientServer
    EXPORT ${PV_INSTALL_EXPORT_NAME}
    RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
    LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
    ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development)
ENDIF(NOT PV_INSTALL_NO_LIBRARIES)

IF(BUILD_TESTING)
  ADD_SUBDIRECTORY(Testing)
ENDIF(BUILD_TESTING)
