PROJECT(PVServerManager)

# needed by vtkExportKit.cmake
SET(VTK_INSTALL_PACKAGE_DIR_CM24 ${PV_INSTALL_LIB_DIR})
#SET(VTK_INSTALL_PACKAGE_DIR ${PV_INSTALL_LIB_DIR})

# Any build configured headers should be added to this
# variable so that they also get installed.
SET (dynamicHeaders)


INCLUDE_REGULAR_EXPRESSION("^vtk.*$")

SET(KIT PVServerManager)
SET(KIT_UPCASE "PVSERVERMANAGER")
IF(NOT WIN32)
  SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
  ADD_DEFINITIONS(-D_HPUX_SOURCE)
ENDIF(NOT WIN32)

SET (Kit_SRCS
  vtkInitializationHelper.cxx
  vtkPVComparativeAnimationCue.cxx
  vtkPVComparativeView.cxx
  vtkPVKeyFrameAnimationCueForProxies.cxx
  vtkPVRepresentationAnimationHelper.cxx
  vtkSMAnimationScene.cxx
  vtkSMAnimationSceneGeometryWriter.cxx
  vtkSMAnimationSceneImageWriter.cxx
  vtkSMAnimationSceneProxy.cxx
  vtkSMAnimationSceneWriter.cxx
  vtkSMArrayListDomain.cxx
  vtkSMArrayRangeDomain.cxx
  vtkSMArraySelectionDomain.cxx
  vtkSMBooleanDomain.cxx
  vtkSMBoundsDomain.cxx
  vtkSMBoxRepresentationProxy.cxx
  vtkSMCacheBasedProxyLocator.cxx
  vtkSMCameraConfigurationReader.cxx
  vtkSMCameraConfigurationWriter.cxx
  vtkSMCameraLink.cxx
  vtkSMCameraProxy.cxx
  vtkSMChartRepresentationProxy.cxx
  vtkSMComparativeAnimationCueProxy.cxx
  vtkSMComparativeAnimationCueUndoElement.cxx
  vtkSMComparativeViewProxy.cxx
  vtkSMCompositeTreeDomain.cxx
  vtkSMCompoundProxyDefinitionLoader.cxx
  vtkSMCompoundSourceProxy.cxx
  vtkSMCompoundSourceProxyDefinitionBuilder.cxx
  vtkSMContextNamedOptionsProxy.cxx
  vtkSMContextViewProxy.cxx
  vtkSMCSVExporterProxy.cxx
  vtkSMDataTypeDomain.cxx
  vtkSMDeserializer.cxx
  vtkSMDimensionsDomain.cxx
  vtkSMDirectoryProxy.cxx
  vtkSMDocumentation.cxx
  vtkSMDomain.cxx
  vtkSMDomainIterator.cxx
  vtkSMDoubleRangeDomain.cxx
  vtkSMDoubleVectorProperty.cxx
  vtkSMEnumerationDomain.cxx
  vtkSMExporterProxy.cxx
  vtkSMExtentDomain.cxx
  vtkSMFieldDataDomain.cxx
  vtkSMFileListDomain.cxx
  vtkSMFixedTypeDomain.cxx
  vtkSMGlobalPropertiesLinkUndoElement.cxx
  vtkSMGlobalPropertiesManager.cxx
  vtkSMIdTypeVectorProperty.cxx
  vtkSMImplicitPlaneRepresentationProxy.cxx
  vtkSMInputArrayDomain.cxx
  vtkSMInputProperty.cxx
  vtkSMInteractionUndoStackBuilder.cxx
  vtkSMIntRangeDomain.cxx
  vtkSMIntVectorProperty.cxx
  vtkSMLink.cxx
  vtkSMNamedPropertyIterator.cxx
  vtkSMNewWidgetRepresentationProxy.cxx
  vtkSMNullProxy.cxx
  vtkSMNumberOfComponentsDomain.cxx
  vtkSMNumberOfGroupsDomain.cxx
  vtkSMObject.cxx
  vtkSMOrderedPropertyIterator.cxx
  vtkSMOutputPort.cxx
  vtkSMParallelCoordinatesRepresentationProxy.cxx
  vtkSMPipelineState.cxx
  vtkSMPluginLoaderProxy.cxx
  vtkSMPluginManager.cxx
  vtkSMProperty.cxx
  vtkSMPropertyAdaptor.cxx
  vtkSMPropertyHelper.cxx
  vtkSMPropertyIterator.cxx
  vtkSMPropertyLink.cxx
  vtkSMPropertyModificationUndoElement.cxx
  vtkSMProxy.cxx
  vtkSMProxyConfigurationReader.cxx
  vtkSMProxyConfigurationWriter.cxx
  vtkSMProxyDefinitionManager.cxx
  vtkSMProxyGroupDomain.cxx
  vtkSMProxyIterator.cxx
  vtkSMProxyLink.cxx
  vtkSMProxyListDomain.cxx
  vtkSMProxyLocator.cxx
  vtkSMProxyManager.cxx
  vtkSMProxyProperty.cxx
  vtkSMProxySelectionModel.cxx
  vtkSMPSWriterProxy.cxx
  vtkSMPVRepresentationProxy.cxx
  vtkSMPWriterProxy.cxx
  vtkSMPythonTraceObserver.cxx
  vtkSMReaderFactory.cxx
  vtkSMRemoteObject.cxx
  vtkSMRemoteObjectUpdateUndoElement.cxx
  vtkSMRenderViewExporterProxy.cxx
  vtkSMRenderViewProxy.cxx
  vtkSMRepresentationProxy.cxx
  vtkSMScalarBarWidgetRepresentationProxy.cxx
  vtkSMSelectionHelper.cxx
  vtkSMSession.cxx
  vtkSMSessionClient.cxx
  vtkSMSILDomain.cxx
  vtkSMSILModel.cxx
  vtkSMSourceProxy.cxx
  vtkSMSpreadSheetRepresentationProxy.cxx
  vtkSMStateLoader.cxx
  vtkSMStateLocator.cxx
  vtkSMStateVersionController.cxx
  vtkSMStateVersionControllerBase.cxx
  vtkSMStringListDomain.cxx
  vtkSMStringListRangeDomain.cxx
  vtkSMStringVectorProperty.cxx
  vtkSMTesting.cxx
  vtkSMTextWidgetRepresentationProxy.cxx
  vtkSMTimeKeeper.cxx
  vtkSMTimeKeeperProxy.cxx
  vtkSMTwoDRenderViewProxy.cxx
  vtkSMUndoElement.cxx
  vtkSMUndoStack.cxx
  vtkSMUndoStackBuilder.cxx
  vtkSMUtilities.cxx
  vtkSMVectorProperty.cxx
  vtkSMViewProxy.cxx
  vtkSMWidgetRepresentationProxy.cxx
  vtkSMWriterFactory.cxx
  vtkSMWriterProxy.cxx
)

SET( ServerManager_SRCS
  ${ServerManagerCS_SRCS}
  ${Kit_SRCS}
  ${ServerManagerNonWrapped_SRCS}
  ${ExtraParaViewServerManagerNonWrapped_SRCS}
  )

# Abstract and Wrap exclude

SET_SOURCE_FILES_PROPERTIES(
  vtkInitializationHelper.cxx
  vtkSMAnimationSceneWriter.cxx
  vtkSMDomain.cxx
  vtkSMExporterProxy.cxx
  vtkSMLink.cxx
  vtkSMPropertyHelper.cxx
  vtkSMRemoteObject.cxx
  vtkSMSession.cxx
  vtkSMStateVersionControllerBase.cxx
  vtkSMUndoElement.cxx
  vtkSMVectorProperty.cxx
  ABSTRACT
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkSMPropertyHelper.cxx
  WRAP_EXCLUDE)

INCLUDE_DIRECTORIES(
  ${PVCommon_SOURCE_DIR}
  ${PVCommon_BINARY_DIR}
  ${PVVTKExtensions_SOURCE_DIR}
  ${PVVTKExtensions_BINARY_DIR}
  ${PVClientServerCore_SOURCE_DIR}
  ${PVClientServerCore_BINARY_DIR}
  ${PVServerImplementation_SOURCE_DIR}
  ${PVServerImplementation_BINARY_DIR}
  ${PVServerManager_SOURCE_DIR}
  ${PVServerManager_BINARY_DIR}
  ${protobuf_SOURCE_DIR}/src
  ${VTK_INCLUDE_DIR}
  )

SET(VTK_WRAP_HINTS ${CMAKE_CURRENT_SOURCE_DIR}/hints)

# Setup vtkInstantiator registration for this library's classes.
INCLUDE(${ParaView_SOURCE_DIR}/VTK/CMake/vtkMakeInstantiator.cmake)
VTK_MAKE_INSTANTIATOR3(vtk${KIT}Instantiator KitInstantiator_SRCS
  "${ServerManager_SRCS}"
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
INCLUDE(${VTK_CMAKE_DIR}/vtkExportKit.cmake)
VTK_EXPORT_KIT("${KIT}" "${KIT_UPCASE}" "${Kit_SRCS};${ServerManagerCS_SRCS};${ExtraParaViewServerManagerNonWrapped_SRCS}")
SET(VTK_BINARY_DIR "${_VTK_BINARY_DIR}")

# Wrap PVServerManager
SET(KIT_WRAP_DEPS "")
SET(VTK_KITS_DIR "${CMAKE_CURRENT_BINARY_DIR}/Utilities")
PV_PRE_WRAP_VTK_CS("vtk${KIT}" "${KIT}" "${KIT_UPCASE}" "${KIT_WRAP_DEPS}")

# The client server minimal initializer
SET(MIN_INIT_SRC)
IF(PARAVIEW_MINIMAL_BUILD)
  SET(MIN_INIT_SRC "${CMAKE_CURRENT_BINARY_DIR}/vtkParaviewMinInit.cxx")
ENDIF(PARAVIEW_MINIMAL_BUILD)

# Create the library.
VTK_ADD_LIBRARY(vtk${KIT}
  ${ServerManager_SRCS}
  ${MIN_INIT_SRC}
  ${CMInstantiator_SRCS}
  ${vtkPVServerManagerCS_SRCS}
  ${KitInstantiator_SRCS}
  )
TARGET_LINK_LIBRARIES(vtk${KIT}
  vtkPVServerImplementationCS
  vtkPVServerImplementation
  )

IF(PARAVIEW_USE_VISITBRIDGE)
  TARGET_LINK_LIBRARIES(vtk${KIT}
  vtkVisItDatabasesCS
  vtkVisItDatabases
  )
ENDIF(PARAVIEW_USE_VISITBRIDGE)

IF (PARAVIEW_ENABLE_PYTHON)
  SET(Kit_PYTHON_EXTRA_SRCS)
  SET (Kit_SRCS
      ${Kit_SRCS}
    )
  SET(KIT_PYTHON_LIBS
    vtkPVServerImplementationPythonD
    )
  # Tell vtkWrapPython.cmake to set VTK_PYTHON_LIBRARIES for us.
  SET(VTK_WRAP_PYTHON_FIND_LIBS 1)
  SET(VTK_WRAP_INCLUDE_DIRS ${VTK_INCLUDE_DIR})
  INCLUDE("${VTK_CMAKE_DIR}/vtkWrapPython.cmake")
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
  SET(KIT_PYTHON_DEPS)

#   SET(VTK_INSTALL_LIB_DIR      "${VTKPythonWrapping_INSTALL_LIB_DIR}")
#   SET(VTK_INSTALL_BIN_DIR      "${VTKPythonWrapping_INSTALL_BIN_DIR}")
  SET(VTK_INSTALL_LIB_DIR_CM24 "${VTKPythonWrapping_INSTALL_LIB_DIR}")
  SET(VTK_INSTALL_BIN_DIR_CM24 "${VTKPythonWrapping_INSTALL_BIN_DIR}")

  # include python wrapping from VTK
  INCLUDE(KitCommonPythonWrapBlock)
ENDIF (PARAVIEW_ENABLE_PYTHON)

# Custom command for generating the minimal initializer, requires python interpreter
IF(PARAVIEW_MINIMAL_BUILD)
SET(min_init_writer
    "${ParaView_SOURCE_DIR}/Utilities/VTKClientServer/MinimalBuildTools/WriteMinInit.py")
ADD_CUSTOM_COMMAND(
  OUTPUT ${MIN_INIT_SRC}
  DEPENDS ${PARAVIEW_MINIMAL_BUILD_CLASS_FILE}
  ${min_init_writer}
  COMMAND ${PYTHON_EXECUTABLE}
  ARGS ${min_init_writer} ${PARAVIEW_MINIMAL_BUILD_CLASS_FILE} ${MIN_INIT_SRC}
  COMMENT "generating ${MIN_INIT_SRC}")
ENDIF(PARAVIEW_MINIMAL_BUILD)

SET(GUI_CONFIG_FILES
  ParaViewSources
  ParaViewFilters
  ParaViewReaders
  ParaViewWriters
)

IF(NOT CMAKE_CROSSCOMPILING)
  add_executable_with_forwarding(PV_EXE_SUFFIX
    vtkSMExtractDocumentation vtkSMExtractDocumentation.cxx)
  TARGET_LINK_LIBRARIES(vtkSMExtractDocumentation${PV_EXE_SUFFIX} vtkPVServerManager)
  EXPORT(TARGETS vtkSMExtractDocumentation${PV_EXE_SUFFIX} FILE ${EXPORT_EXECUTABLES_FILE} NAMESPACE "${EXPORT_EXECUTABLES_NAMESPACE}" APPEND)
ENDIF(NOT CMAKE_CROSSCOMPILING)

# FIXME: DUDE! Dependency on Qt code in ServerManager!!!!

CONFIGURE_FILE(${ParaView_CMAKE_DIR}/ExecuteSMExtractDocumentation.cmake.in
  ${ParaView_BINARY_DIR}/CMake/ExecuteSMExtractDocumentation.cmake
  @ONLY)

FOREACH(gcf ${GUI_CONFIG_FILES})
  SET(fullXMLFile ${ParaView_SOURCE_DIR}/Applications/ParaView/${gcf}.xml)
  SET(fullHTMLFile ${ParaView_BINARY_DIR}/Documentation/${gcf}.html)

  # run the following cmake script so that we can set the runtime path for Qt when executing
  # SMExtractDocumentation during the build.
  ADD_CUSTOM_COMMAND(
    OUTPUT ${fullHTMLFile}
    DEPENDS ${fullXMLFile} ${resourceFiles} vtkSMExtractDocumentation
    COMMAND ${CMAKE_COMMAND}
    ARGS -DfullXMLFile=${fullXMLFile} -DOutDir=${CMAKE_CFG_INTDIR} -P ${ParaView_BINARY_DIR}/CMake/ExecuteSMExtractDocumentation.cmake
    COMMENT "Creating Documentation for ${gcf}"
    )

  SET(HTML_RESULTS ${HTML_RESULTS} ${fullHTMLFile})
ENDFOREACH(gcf)

ADD_CUSTOM_TARGET(HTMLDocumentation
  DEPENDS ${HTML_RESULTS}
)
SET (HTMLDocumentation_FILES ${HTML_RESULTS} CACHE INTERNAL
  "HTMLDocumentation files generated from ServerManager")

# including MINI_INIT -nix
# SET_SOURCE_FILES_PROPERTIES(vtkSMApplication.cxx PROPERTIES OBJECT_DEPENDS
#   ${CMAKE_CURRENT_BINARY_DIR}/${MINI_INIT})

# Install
#----------------------------------------------------------------------------------------------------
# Libraries
IF(NOT PV_INSTALL_NO_LIBRARIES)
  SET (targets_to_install)

  INSTALL(TARGETS vtk${KIT} ${targets_to_install}
      EXPORT ${PV_INSTALL_EXPORT_NAME}
      RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
      ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development)
ENDIF(NOT PV_INSTALL_NO_LIBRARIES)

# Development
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

# Testing
#----------------------------------------------------------------------------------------------------
IF(BUILD_TESTING)
  ADD_SUBDIRECTORY(Testing)
ENDIF(BUILD_TESTING)


