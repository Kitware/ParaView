# Configure the vtkClientServer library.
SET(VTK_CLIENT_SERVER_BUILD_SHARED ${BUILD_SHARED_LIBS})
CONFIGURE_FILE(
  ${PARAVIEW_SOURCE_DIR}/ClientServer/vtkClientServerConfigure.h.in
  ${PARAVIEW_BINARY_DIR}/ClientServer/vtkClientServerConfigure.h
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
IF(BUILD_SHARED_LIBS)
  INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} vtkClientServer)
ENDIF(BUILD_SHARED_LIBS)

# Macro to create ClientServer wrappers classes in a single VTK kit.
MACRO(PV_WRAP_VTK_CS kit ukit deps)
  SET(vtk${kit}CS_HEADERS)
  INCLUDE(${VTK_KITS_DIR}/vtk${kit}Kit.cmake)
  FOREACH(class ${VTK_${ukit}_CLASSES})
    IF(NOT VTK_CLASS_WRAP_EXCLUDE_${class})
      IF(VTK_CLASS_ABSTRACT_${class})
        SET_SOURCE_FILES_PROPERTIES(${VTK_${ukit}_HEADER_DIR}/${class}.h
          PROPERTIES ABSTRACT 1)
      ENDIF(VTK_CLASS_ABSTRACT_${class})
        SET(vtk${kit}CS_HEADERS ${vtk${kit}CS_HEADERS}
          ${VTK_${ukit}_HEADER_DIR}/${class}.h)
    ENDIF(NOT VTK_CLASS_WRAP_EXCLUDE_${class})
  ENDFOREACH(class)
  VTK_WRAP_ClientServer(vtk${kit}CS vtk${kit}CS_SRCS ${vtk${kit}CS_HEADERS})
  ADD_LIBRARY(vtk${kit}CS ${vtk${kit}CS_SRCS})
  TARGET_LINK_LIBRARIES(vtk${kit}CS vtkClientServer vtk${kit})
  FOREACH(dep ${deps})
    TARGET_LINK_LIBRARIES(vtk${kit}CS vtk${dep}CS)
  ENDFOREACH(dep)
  IF(BUILD_SHARED_LIBS)
    INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} vtk${kit}CS)
  ENDIF(BUILD_SHARED_LIBS)
ENDMACRO(PV_WRAP_VTK_CS)

# Build the vtkClientServer wrapping utility first.
SUBDIRS(Wrapping)

# Build vtkClientServer wrappers for VTK kits.
SUBDIRS(Common Filtering Imaging Graphics IO Rendering Hybrid Parallel)
IF(VTK_USE_PATENTED)
  SUBDIRS(Patented)
ENDIF(VTK_USE_PATENTED)
