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
  IF(BUILD_SHARED_LIBS)
    INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} vtkClientServer)
  ENDIF(BUILD_SHARED_LIBS)
ENDIF(PARAVIEW_SOURCE_DIR)

# Macro to create ClientServer wrappers classes in a single VTK kit.
MACRO(PV_WRAP_VTK_CS kit ukit deps)
  SET(vtk${kit}CS_HEADERS)
  INCLUDE(${VTK_KITS_DIR}/vtk${kit}Kit.cmake)
  FOREACH(class ${VTK_${ukit}_CLASSES})
    SET(full_name "${VTK_${ukit}_HEADER_DIR}/${class}.h")
    IF("${class}" MATCHES "^(\\/|.\\/|.\\\\|.:\\/|.:\\\\)")
      # handle full paths
      SET(full_name "${class}.h")
    ENDIF("${class}" MATCHES "^(\\/|.\\/|.\\\\|.:\\/|.:\\\\)")
    IF(NOT VTK_CLASS_WRAP_EXCLUDE_${class})
      IF(VTK_CLASS_ABSTRACT_${class})
        SET_SOURCE_FILES_PROPERTIES(${full_name}
          PROPERTIES ABSTRACT 1)
      ENDIF(VTK_CLASS_ABSTRACT_${class})
        SET(vtk${kit}CS_HEADERS ${vtk${kit}CS_HEADERS}
          ${full_name})
    ENDIF(NOT VTK_CLASS_WRAP_EXCLUDE_${class})
  ENDFOREACH(class)
  VTK_WRAP_ClientServer(vtk${kit}CS vtk${kit}CS_SRCS ${vtk${kit}CS_HEADERS})
  ADD_LIBRARY(vtk${kit}CS ${vtk${kit}CS_SRCS})
  TARGET_LINK_LIBRARIES(vtk${kit}CS vtkClientServer vtk${kit})
  FOREACH(dep ${deps})
    TARGET_LINK_LIBRARIES(vtk${kit}CS vtk${dep}CS)
  ENDFOREACH(dep)
  IF(PARAVIEW_SOURCE_DIR)
    IF(BUILD_SHARED_LIBS)
      INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} vtk${kit}CS)
    ENDIF(BUILD_SHARED_LIBS)
  ENDIF(PARAVIEW_SOURCE_DIR)
ENDMACRO(PV_WRAP_VTK_CS)

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
