PROJECT(PVServerImplementation)

INCLUDE_REGULAR_EXPRESSION("^vtk.*$")

# Any build configured headers should be added to this
# variable so that they also get installed.
SET (dynamicHeaders)


# needed by vtkExportKit.cmake
SET(VTK_INSTALL_PACKAGE_DIR_CM24 ${PV_INSTALL_LIB_DIR})
SET(VTK_INSTALL_PACKAGE_DIR ${PV_INSTALL_LIB_DIR})

SET(KIT PVServerImplementation)
SET(KIT_UPCASE "PVSERVERIMPLEMENTATION")
IF(NOT WIN32)
  SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
  ADD_DEFINITIONS(-D_HPUX_SOURCE)
ENDIF(NOT WIN32)

SET (Kit_SRCS
  vtkPVProxyDefinitionIterator.cxx
  vtkPVSessionBase.cxx
  vtkPVSessionCore.cxx
  vtkPVSessionCoreInterpreterHelper.cxx
  vtkPVSessionServer.cxx
  vtkSIAMRVolumeRepresentationProxy.cxx
  vtkSIArraySelectionProperty.cxx
  vtkSIChartRepresentationProxy.cxx
  vtkSICollaborationManager.cxx
  vtkSICompoundSourceProxy.cxx
  vtkSIContextArraysProperty.cxx
  vtkSIDataArrayProperty.cxx
  vtkSIDoubleVectorProperty.cxx
  vtkSIFileSeriesReaderProxy.cxx
  vtkSIIdTypeVectorProperty.cxx
  vtkSIImageTextureProxy.cxx
  vtkSIInputProperty.cxx
  vtkSIIntVectorProperty.cxx
  vtkSIObject.cxx
  vtkSIProperty.cxx
  vtkSIProxy.cxx
  vtkSIProxyDefinitionManager.cxx
  vtkSIProxyProperty.cxx
  vtkSIPVRepresentationProxy.cxx
  vtkSIScalarBarActorProxy.cxx
  vtkSISelectionRepresentationProxy.cxx
  vtkSISILProperty.cxx
  vtkSISourceProxy.cxx
  vtkSIStringVectorProperty.cxx
  vtkSITextSourceRepresentationProxy.cxx
  vtkSITimeRangeProperty.cxx
  vtkSITimeStepsProperty.cxx
  vtkSIUniformGridVolumeRepresentationProxy.cxx
  vtkSIUnstructuredGridVolumeRepresentationProxy.cxx
  vtkSIVectorProperty.cxx
  vtkSIWriterProxy.cxx
  vtkSIXMLAnimationWriterRepresentationProperty.cxx
  vtkSMMessage.cxx
)

SET_SOURCE_FILES_PROPERTIES(
  vtkSMMessage.cxx
  WRAP_EXCLUDE
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkPVProxyDefinitionIterator.cxx
  vtkSIVectorProperty.cxx
  vtkPVSessionBase.cxx
  vtkSMMessage.cxx
  ABSTRACT)

INCLUDE_DIRECTORIES(
  ${PVCommon_SOURCE_DIR}
  ${PVCommon_BINARY_DIR}
  ${PVVTKExtensions_SOURCE_DIR}
  ${PVVTKExtensions_BINARY_DIR}
  ${PVClientServerCore_SOURCE_DIR}
  ${PVClientServerCore_BINARY_DIR}
  ${PVServerImplementation_SOURCE_DIR}
  ${PVServerImplementation_BINARY_DIR}
  ${protobuf_SOURCE_DIR}/src
  ${VTK_INCLUDE_DIR}
  )

#------------------------------------------------------------------------------
# Compile protobuf resources.
# run it from a script so we can set necessary paths.
get_target_property(PROTOC_LOCATION protoc_compiler LOCATION)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkPVMessage.pb.h
           ${CMAKE_CURRENT_BINARY_DIR}/vtkPVMessage.pb.cc
    
    COMMAND protoc_compiler
           "--cpp_out=dllexport_decl=VTK_PROTOBUF_EXPORT:${CMAKE_CURRENT_BINARY_DIR}"
           --proto_path "@CMAKE_CURRENT_SOURCE_DIR@/Resources"
           "${CMAKE_CURRENT_SOURCE_DIR}/Resources/vtkPVMessage.proto"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Resources/vtkPVMessage.proto protoc_compiler
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(
  protobuf_code_generation
  DEPENDS  ${CMAKE_CURRENT_BINARY_DIR}/vtkPVMessage.pb.h ${CMAKE_CURRENT_BINARY_DIR}/vtkPVMessage.pb.cc
)

#------------------------------------------------------------------------------

SET(resourceFiles
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/filters.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/sources.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/readers.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/utilities.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/rendering.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/views_and_representations.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/3d_widgets.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/internal_writers.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/writers.xml
  ${PARAVIEW_EXTRA_SERVERMANAGER_RESOURCES}
  )

IF(PARAVIEW_ENABLE_PYTHON)
  OPTION(PARAVIEW_ENABLE_PYTHON_FILTERS "Enable Python calculator and programmable filters." ON)
  IF(PARAVIEW_ENABLE_PYTHON_FILTERS)
    SET(resourceFiles
      ${resourceFiles}
      ${CMAKE_CURRENT_SOURCE_DIR}/Resources/pythonfilter.xml
      )
  ENDIF(PARAVIEW_ENABLE_PYTHON_FILTERS)
ENDIF(PARAVIEW_ENABLE_PYTHON)

SET (PARAVIEW_SERVERMANAGER_XMLS ${resourceFiles}
    CACHE INTERNAL "Server Manager XMLs")

#needs to depend on vtkParaViewIncludeModulesToSMApplication so
#we can regenerate when python filters or VisItBridge are toggled on/off
SET(CMD_DEP kwProcessXML)
IF(CMAKE_CROSSCOMPILING)
  SET(CMD kwProcessXML)
ELSE(CMAKE_CROSSCOMPILING)
  GET_TARGET_PROPERTY(CMD kwProcessXML LOCATION)
ENDIF(CMAKE_CROSSCOMPILING)
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkSMGeneratedModules.h
  DEPENDS ${resourceFiles}
  ${CMD_DEP}
  "${CMAKE_CURRENT_BINARY_DIR}/vtkParaViewIncludeModulesToSMApplication.h"
  COMMAND ${CMD}
  ARGS ${CMAKE_CURRENT_BINARY_DIR}/vtkSMGeneratedModules.h
  vtkSMDefaultModules Interface GetInterfaces
  ${resourceFiles}
  COMMENT "generating modules")

SET(PARAVIEW_INCLUDE_MODULES_TO_SMAPPLICATION "")
FOREACH(rf ${resourceFiles})
  STRING(REGEX REPLACE "^.*/(.*).(xml|pvsm)$" "\\1" moduleName "${rf}")
  SET(oneModule "  init_string =  vtkSMDefaultModules${moduleName}GetInterfaces();\n")
  SET(oneModule "${oneModule}  parser_ret = parser->Parse(init_string);\n")
  SET(oneModule "${oneModule}  assert(parser_ret != 0);\n")
  SET(oneModule "${oneModule} this->LoadConfigurationXML(parser->GetRootElement());\n")
  SET(oneModule "${oneModule}  delete[] init_string;\n")
  SET(PARAVIEW_INCLUDE_MODULES_TO_SMAPPLICATION
    "${PARAVIEW_INCLUDE_MODULES_TO_SMAPPLICATION}\n${oneModule}")
ENDFOREACH(rf)


CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkParaViewIncludeModulesToSMApplication.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkParaViewIncludeModulesToSMApplication.h"
  @ONLY IMMEDIATE)
SET (dynamicHeaders
    "${dynamicHeaders};${CMAKE_CURRENT_BINARY_DIR}/vtkParaViewIncludeModulesToSMApplication.h")


SET_SOURCE_FILES_PROPERTIES(vtkSIProxyDefinitionManager.cxx PROPERTIES OBJECT_DEPENDS
  ${CMAKE_CURRENT_BINARY_DIR}/vtkSMGeneratedModules.h)


# Setup vtkInstantiator registration for this library's classes.
INCLUDE(${ParaView_SOURCE_DIR}/VTK/CMake/vtkMakeInstantiator.cmake)
VTK_MAKE_INSTANTIATOR3(vtk${KIT}Instantiator KitInstantiator_SRCS
  "${Kit_SRCS}"
  VTK_EXPORT
  "${CMAKE_CURRENT_BINARY_DIR}" "")

# load the ClientServer command
INCLUDE(${ParaView_SOURCE_DIR}/Utilities/VTKClientServer/vtkClientServer.cmake)
CS_INITIALIZE_WRAP()
INCLUDE_DIRECTORIES(${VTKCLIENTSERVER_INCLUDE_DIR})

# Wrap PVServer
# Create ClientServer wrappers for ParaView classes.
SET(_VTK_BINARY_DIR "${VTK_BINARY_DIR}")
SET(VTK_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
INCLUDE(${ParaView_CMAKE_DIR}/VTKMono/vtkExportKit.cmake)
VTK_EXPORT_KIT("${KIT}" "${KIT_UPCASE}" "${Kit_SRCS}")
SET(VTK_BINARY_DIR "${_VTK_BINARY_DIR}")

# Wrap PVServerCommon
SET(KIT_WRAP_DEPS PVClientServerCore)
SET(VTK_KITS_DIR "${CMAKE_CURRENT_BINARY_DIR}/Utilities")
PV_WRAP_VTK_CS("${KIT}" "${KIT_UPCASE}" "${KIT_WRAP_DEPS}")

# Wrap for python
IF (PARAVIEW_ENABLE_PYTHON)
  SET(Kit_PYTHON_EXTRA_SRCS)
  SET(KIT_PYTHON_LIBS
    vtkPVClientServerCorePythonD
    )
  # Tell vtkWrapPython.cmake to set VTK_PYTHON_LIBRARIES for us.
  SET(VTK_WRAP_PYTHON_FIND_LIBS 1)
  SET(VTK_WRAP_INCLUDE_DIRS ${VTK_INCLUDE_DIR})
  INCLUDE("${VTK_CMAKE_DIR}/vtkWrapPython.cmake")
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
  SET(KIT_PYTHON_DEPS)

  SET(VTK_INSTALL_LIB_DIR "${VTKPythonWrapping_INSTALL_LIB_DIR}")
  SET(VTK_INSTALL_BIN_DIR "${VTKPythonWrapping_INSTALL_BIN_DIR}")
  SET(VTK_INSTALL_LIB_DIR_CM24 "${VTKPythonWrapping_INSTALL_LIB_DIR}")
  SET(VTK_INSTALL_BIN_DIR_CM24 "${VTKPythonWrapping_INSTALL_BIN_DIR}")

  # include python wrapping from VTK
  INCLUDE(KitCommonPythonWrapBlock)
ENDIF (PARAVIEW_ENABLE_PYTHON)


# Create the library.
PVVTK_ADD_LIBRARY(vtk${KIT}
  ${Kit_SRCS}
  ${KitInstantiator_SRCS}
  )
TARGET_LINK_LIBRARIES(vtk${KIT}
  vtkPVClientServerCore
  vtkPVClientServerCoreCS
  ${KIT_LIBS}
  protobuf
)
add_dependencies(vtk${KIT} protobuf_code_generation)

IF(NOT PV_INSTALL_NO_LIBRARIES)
  SET(temp_TO_INSTALL
    vtk${KIT})

  INSTALL(TARGETS ${temp_TO_INSTALL}
    EXPORT ${PV_INSTALL_EXPORT_NAME}
    RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
    LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
    ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development)
ENDIF(NOT PV_INSTALL_NO_LIBRARIES)

# Resource files
IF(NOT PV_INSTALL_NO_RUNTIME)
  INSTALL(
    FILES ${resourceFiles}
    DESTINATION ${PV_INSTALL_LIB_DIR}/ParaViewCore/ServerImplementation/Resources)
ENDIF(NOT PV_INSTALL_NO_RUNTIME)

# Development files install
IF (NOT PV_INSTALL_NO_DEVELOPMENT)
  GLOB_INSTALL_DEVELOPMENT(
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${PV_INSTALL_INCLUDE_DIR}
      "*.h;*.hxx;*.txx")
  INSTALL(
      FILES ${dynamicHeaders}
      DESTINATION ${PV_INSTALL_INCLUDE_DIR}
      COMPONENT Development)
ENDIF (NOT PV_INSTALL_NO_DEVELOPMENT)

IF (BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF (BUILD_TESTING)
