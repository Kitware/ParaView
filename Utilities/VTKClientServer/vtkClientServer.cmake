MACRO(CS_INITIALIZE_WRAP)
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
  UTILITY_SOURCE(VTK_WRAP_ClientServer_EXE vtkWrapClientServer 
    Wrapping vtkWrapClientServer.c)
  SET(LIBRARY_OUTPUT_PATH ${VTKCS_BINARY_DIR}/bin CACHE PATH 
    "Single output path for libraries")
  SET(EXECUTABLE_OUTPUT_PATH ${VTKCS_BINARY_DIR}/bin CACHE PATH 
    "Single output path for executable")
  SET(BUILD_SHARED_LIBS ${VTK_BUILD_SHARED_LIBS})
  MARK_AS_ADVANCED(VTK_WRAP_ClientServer_EXE)
ENDMACRO(CS_INITIALIZE_WRAP)

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
    #MESSAGE("Link vtk${kit}CS to vtk${dep}CS")
    TARGET_LINK_LIBRARIES(vtk${kit}CS vtk${dep}CS)
  ENDFOREACH(dep)
  IF(PARAVIEW_SOURCE_DIR)
    IF(BUILD_SHARED_LIBS)
      INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} vtk${kit}CS)
    ENDIF(BUILD_SHARED_LIBS)
  ENDIF(PARAVIEW_SOURCE_DIR)
ENDMACRO(PV_WRAP_VTK_CS)
