PROJECT(ServerManager)

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

SET(Kit_SRCS
  vtkDummyProcessModuleHelper.cxx
  vtkInitializationHelper.cxx
  vtkSMAnimationCueManipulatorProxy.cxx
  vtkSMAnimationCueProxy.cxx
  vtkSMAnimationPlayerProxy.cxx
  vtkSMAnimationSceneGeometryWriter.cxx
  vtkSMAnimationSceneImageWriter.cxx
  vtkSMAnimationSceneProxy.cxx
  vtkSMAnimationSceneWriter.cxx
  vtkSMApplication.cxx
  vtkSMArrayListDomain.cxx
  vtkSMArrayListInformationHelper.cxx
  vtkSMArrayRangeDomain.cxx
  vtkSMArraySelectionDomain.cxx
  vtkSMArraySelectionInformationHelper.cxx
  vtkSMBooleanDomain.cxx
  vtkSMBooleanKeyFrameProxy.cxx
  vtkSMBoundsDomain.cxx
  vtkSMBoxProxy.cxx
  vtkSMBoxRepresentationProxy.cxx
  vtkSMCameraConfigurationReader.cxx
  vtkSMCameraConfigurationWriter.cxx
  vtkSMCameraKeyFrameProxy.cxx
  vtkSMCameraLink.cxx
  vtkSMCameraManipulatorProxy.cxx
  vtkSMCameraProxy.cxx
  #vtkSMCaveRenderModuleProxy.cxx
#  vtkSMCaveRenderViewProxy.cxx
  vtkSMChartRepresentationProxy.cxx
  vtkSMComparativeAnimationCueProxy.cxx
  vtkSMComparativeViewProxy.cxx
  vtkSMCompositeKeyFrameProxy.cxx
  vtkSMCompositeTreeDomain.cxx
  vtkSMCompoundSourceProxy.cxx
  vtkSMCompoundProxyDefinitionLoader.cxx
  vtkSMConnectionCleanerProxy.cxx
  vtkSMContextArraysInformationHelper.cxx
  vtkSMContextNamedOptionsProxy.cxx
  vtkSMContextViewProxy.cxx
  vtkSMCSVExporterProxy.cxx
  vtkSMDataTypeDomain.cxx
  vtkSMDeserializer.cxx
  vtkSMDimensionsDomain.cxx
  vtkSMDistanceRepresentation2DProxy.cxx
  vtkSMDocumentation.cxx
  vtkSMDomain.cxx
  vtkSMDomainIterator.cxx
  vtkSMDoubleArrayInformationHelper.cxx
  vtkSMDoubleRangeDomain.cxx
  vtkSMDoubleVectorProperty.cxx
  vtkSMEnumerationDomain.cxx
  vtkSMExponentialKeyFrameProxy.cxx
  vtkSMExporterProxy.cxx
  vtkSMExtentDomain.cxx
  vtkSMFetchDataProxy.cxx
  vtkSMFieldDataDomain.cxx
  vtkSMFileListDomain.cxx
  vtkSMFileSeriesReaderProxy.cxx
  vtkSMFixedTypeDomain.cxx
  vtkSMGlobalPropertiesManager.cxx
  vtkSMGlobalPropertiesLinkUndoElement.cxx
  vtkSMHardwareSelector.cxx
  vtkSMIdBasedProxyLocator.cxx
  vtkSMIdTypeVectorProperty.cxx
  vtkSMImageTextureProxy.cxx
  vtkSMImplicitPlaneProxy.cxx
  vtkSMImplicitPlaneRepresentationProxy.cxx
  vtkSMInformationHelper.cxx
  vtkSMInputArrayDomain.cxx
  vtkSMInputProperty.cxx
  vtkSMIdTypeArrayInformationHelper.cxx
  vtkSMIntArrayInformationHelper.cxx
  vtkSMInteractionUndoStackBuilder.cxx
  vtkSMIntRangeDomain.cxx
  vtkSMIntVectorProperty.cxx
  vtkSMKeyFrameAnimationCueManipulatorProxy.cxx
  vtkSMKeyFrameProxy.cxx
  vtkSMLinearAnimationCueManipulatorProxy.cxx
  vtkSMLink.cxx
  vtkSMLookupTableProxy.cxx
  vtkSMMaterialLoaderProxy.cxx
  vtkSMNewWidgetRepresentationProxy.cxx
  vtkSMNullProxy.cxx
  vtkSMNumberOfComponentsDomain.cxx
  vtkSMNumberOfGroupsDomain.cxx
  vtkSMObject.cxx
  vtkSMOrderedPropertyIterator.cxx
  vtkSMOutputPort.cxx
  vtkSMNetworkImageSourceProxy.cxx
  vtkSMParallelCoordinatesRepresentationProxy.cxx
  vtkSMPQStateLoader.cxx
  vtkSMPSWriterProxy.cxx
  vtkSMPluginManager.cxx
  vtkSMPluginProxy.cxx
#  vtkSMPluginInformationHelper.cxx
  vtkSMPropertyAdaptor.cxx
  vtkSMProperty.cxx
  vtkSMPropertyHelper.cxx
  vtkSMPropertyIterator.cxx
  vtkSMNamedPropertyIterator.cxx
  vtkSMPropertyLink.cxx
  vtkSMPropertyModificationUndoElement.cxx
  vtkSMProxy.cxx
  vtkSMProxyConfigurationReader.cxx
  vtkSMProxyConfigurationWriter.cxx
  vtkSMProxyDefinitionIterator.cxx
  vtkSMProxyGroupDomain.cxx
  vtkSMProxyIterator.cxx
  vtkSMProxyLink.cxx
  vtkSMProxyListDomain.cxx
  vtkSMProxyLocator.cxx
  vtkSMProxyManager.cxx
  vtkSMProxyManagerReviver.cxx
  vtkSMProxyProperty.cxx
  vtkSMProxyRegisterUndoElement.cxx
  vtkSMProxySelectionModel.cxx
  vtkSMProxyStateChangedUndoElement.cxx
  vtkSMProxyUnRegisterUndoElement.cxx
  vtkSMPythonTraceObserver.cxx
  vtkSMPVLookupTableProxy.cxx
  vtkSMPVRepresentationProxy.cxx
  vtkSMPWriterProxy.cxx
  vtkSMRampKeyFrameProxy.cxx
  vtkSMReaderFactory.cxx
  vtkSMRenderViewProxy.cxx
  vtkSMRenderViewExporterProxy.cxx
  vtkSMRepresentationAnimationHelperProxy.cxx
  vtkSMRepresentationProxy.cxx
  vtkSMScalarBarActorProxy.cxx
  vtkSMScalarBarWidgetRepresentationProxy.cxx
#  vtkSMScatterPlotArraysInformationHelper.cxx
#  vtkSMScatterPlotViewProxy.cxx
#  vtkSMSelectionDeliveryRepresentationProxy.cxx
  vtkSMSelectionHelper.cxx
  vtkSMSelectionRepresentationProxy.cxx
  vtkSMServerFileListingProxy.cxx
  vtkSMServerProxyManagerReviver.cxx
  vtkSMServerSideAnimationPlayer.cxx
  vtkSMSILDomain.cxx
  vtkSMSILInformationHelper.cxx
  vtkSMSILModel.cxx
  vtkSMSimpleDoubleInformationHelper.cxx
  vtkSMSimpleIdTypeInformationHelper.cxx
  vtkSMSimpleIntInformationHelper.cxx
  vtkSMSimpleStringInformationHelper.cxx
  vtkSMSinusoidKeyFrameProxy.cxx
  vtkSMSourceProxy.cxx
  vtkSMSpreadSheetRepresentationProxy.cxx
  vtkSMStateLoader.cxx
  vtkSMStateVersionControllerBase.cxx
  vtkSMStateVersionController.cxx
  vtkSMStringArrayHelper.cxx
  vtkSMStringListDomain.cxx
  vtkSMStringListRangeDomain.cxx
  vtkSMStringVectorProperty.cxx
  vtkSMSubPropertyIterator.cxx
#  vtkSMTemporalInputDomain.cxx
  vtkSMTesting.cxx
  vtkSMTextSourceRepresentationProxy.cxx
  vtkSMTextWidgetRepresentationProxy.cxx
  vtkSMTimeAnimationCueProxy.cxx
  vtkSMTimeKeeperProxy.cxx
  vtkSMTimeRangeInformationHelper.cxx
  vtkSMTimeStepsInformationHelper.cxx
  vtkSMTransformProxy.cxx
  vtkSMTwoDRenderViewProxy.cxx
  vtkSMUndoElement.cxx
  vtkSMUndoRedoStateLoader.cxx
  vtkSMUndoStackBuilder.cxx
  vtkSMUndoStack.cxx
  vtkSMUniformGridVolumeRepresentationProxy.cxx
  vtkSMUnstructuredGridVolumeRepresentationProxy.cxx
  vtkSMUpdateInformationUndoElement.cxx
  vtkSMUtilities.cxx
  vtkSMVectorProperty.cxx
  vtkSMViewProxy.cxx
  vtkSMWidgetRepresentationProxy.cxx
  vtkSMWriterFactory.cxx
  vtkSMWriterProxy.cxx
  vtkSMXMLPVAnimationWriterProxy.cxx
  vtkSMDataSourceProxy.cxx
  )

SET( ServerManagerNonWrapped_SRCS
  vtkPVBatchOptions.cxx
  vtkSMXMLParser.cxx
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
  vtkSMAnimationCueManipulatorProxy.cxx
  vtkSMAnimationSceneWriter.cxx
  vtkSMClientServerRenderSyncManagerHelper.cxx
  vtkSMDomain.cxx
  vtkSMInformationHelper.cxx
  vtkSMLink.cxx
#  vtkSMMultiProcessRenderView.cxx
  vtkSMProxyManagerReviver.cxx
  vtkSMStateVersionControllerBase.cxx
  vtkSMStateVersionControllerBase.cxx
  vtkSMUndoElement.cxx
  vtkSMVectorProperty.cxx
  vtkSMPropertyHelper.cxx
  vtkSMExporterProxy.cxx
  ABSTRACT
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkSMPropertyHelper.cxx
  WRAP_EXCLUDE)


INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${ParaView_SOURCE_DIR}/Servers/Filters
  ${ParaView_SOURCE_DIR}/Servers/Common
  ${ParaView_SOURCE_DIR}/Servers/ServerManager
  ${ParaView_BINARY_DIR}/Servers/Common
  ${ParaView_BINARY_DIR}/Servers/Filters
  ${ParaView_BINARY_DIR}/Servers/ServerManager
  ${ParaView_BINARY_DIR}/Servers/Common
  ${ParaView_BINARY_DIR}/Servers/Filters
  ${KWCommon_INCLUDE_PATH}

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
ADD_LIBRARY(vtk${KIT}
  ${ServerManager_SRCS}
  ${MIN_INIT_SRC}
  ${CMInstantiator_SRCS}
  ${vtkPVServerManagerCS_SRCS}
  ${KitInstantiator_SRCS}
  )
TARGET_LINK_LIBRARIES(vtk${KIT}
  vtkPVFiltersCS
  vtkClientServer
  ${KWCommon_LIBRARIES}
  vtkCharts
  vtkXdmfCS vtkXdmf)

IF (PARAVIEW_ENABLE_PYTHON)
  SET(Kit_PYTHON_EXTRA_SRCS)
  SET (Kit_SRCS
      ${Kit_SRCS}
      vtkSMXMLParser.cxx
    )
  SET(KIT_PYTHON_LIBS
    vtkPVServerCommonPythonD vtk${KIT}
    )
  # Tell vtkWrapPython.cmake to set VTK_PYTHON_LIBRARIES for us.
  SET(VTK_WRAP_PYTHON_FIND_LIBS 1)
  SET(VTK_WRAP_INCLUDE_DIRS ${VTK_INCLUDE_DIR})
  INCLUDE("${VTK_CMAKE_DIR}/vtkWrapPython.cmake")
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
  SET(KIT_PYTHON_DEPS)
  SET(VTK_INSTALL_NO_LIBRARIES 1)
  IF(VTKPythonWrapping_INSTALL_BIN_DIR)
    SET(VTK_INSTALL_NO_LIBRARIES)
  ENDIF(VTKPythonWrapping_INSTALL_BIN_DIR)

#   SET(VTK_INSTALL_LIB_DIR      "${VTKPythonWrapping_INSTALL_LIB_DIR}")
#   SET(VTK_INSTALL_BIN_DIR      "${VTKPythonWrapping_INSTALL_BIN_DIR}")
  SET(VTK_INSTALL_LIB_DIR_CM24 "${VTKPythonWrapping_INSTALL_LIB_DIR}")
  SET(VTK_INSTALL_BIN_DIR_CM24 "${VTKPythonWrapping_INSTALL_BIN_DIR}")

  # include python wrapping from VTK
  INCLUDE(KitCommonPythonWrapBlock)
ENDIF (PARAVIEW_ENABLE_PYTHON)

SET(CMD_DEP kwProcessXML)
IF(CMAKE_CROSSCOMPILING)
  SET(CMD kwProcessXML)
ELSE(CMAKE_CROSSCOMPILING)
  GET_TARGET_PROPERTY(CMD kwProcessXML LOCATION)
ENDIF(CMAKE_CROSSCOMPILING)


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

ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkSMGeneratedModules.h
  DEPENDS ${resourceFiles}
  ${CMD_DEP}
  COMMAND ${CMD}
  ARGS ${CMAKE_CURRENT_BINARY_DIR}/vtkSMGeneratedModules.h
  vtkSMDefaultModules Interface GetInterfaces
  ${resourceFiles}
  COMMENT "generating modules")

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

SET(PARAVIEW_INCLUDE_MODULES_TO_SMAPPLICATION "")
FOREACH(rf ${resourceFiles})
  STRING(REGEX REPLACE "^.*/(.*).(xml|pvsm)$" "\\1" moduleName "${rf}")
  SET(oneModule "  init_string =  vtkSMDefaultModules${moduleName}GetInterfaces();\n")
  SET(oneModule "${oneModule}  parser->Parse(init_string);\n")
  SET(oneModule "${oneModule}  parser->ProcessConfiguration(proxyM);\n")
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


SET_SOURCE_FILES_PROPERTIES(vtkSMApplication.cxx PROPERTIES OBJECT_DEPENDS
  ${CMAKE_CURRENT_BINARY_DIR}/vtkSMGeneratedModules.h)

# including MINI_INIT -nix
# SET_SOURCE_FILES_PROPERTIES(vtkSMApplication.cxx PROPERTIES OBJECT_DEPENDS
#   ${CMAKE_CURRENT_BINARY_DIR}/${MINI_INIT})

# Install
#----------------------------------------------------------------------------------------------------
# Libraries
IF(NOT PV_INSTALL_NO_LIBRARIES)
  SET (targets_to_install)

  IF(PARAVIEW_ENABLE_PYTHON)
    SET (targets_to_install ${targets_to_install}
      vtk${KIT}Python)
  ENDIF(PARAVIEW_ENABLE_PYTHON)

  INSTALL(TARGETS vtk${KIT} ${targets_to_install}
      RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
      ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development)
ENDIF(NOT PV_INSTALL_NO_LIBRARIES)
#
# Resource files
IF(NOT PV_INSTALL_NO_RUNTIME)
  INSTALL(
    FILES ${resourceFiles}
    DESTINATION ${PV_INSTALL_LIB_DIR}/ServerManager/Resources)
ENDIF(NOT PV_INSTALL_NO_RUNTIME)
#
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
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)


