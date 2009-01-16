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
  vtkSMAxesRepresentationProxy.cxx
  vtkSMBlockDeliveryRepresentationProxy.cxx
  vtkSMBlockDeliveryStrategy.cxx
  vtkSMBooleanDomain.cxx
  vtkSMBooleanKeyFrameProxy.cxx
  vtkSMBoundsDomain.cxx
  vtkSMBoxProxy.cxx
  vtkSMBoxRepresentationProxy.cxx
  vtkSMCameraKeyFrameProxy.cxx
  vtkSMCameraLink.cxx
  vtkSMCameraManipulatorProxy.cxx
  vtkSMCameraProxy.cxx
  vtkSMChartRepresentationProxy.cxx
  vtkSMClientDeliveryRepresentationProxy.cxx
  vtkSMClientDeliveryStrategyProxy.cxx
  vtkSMClientServerRenderSyncManagerHelper.cxx
  vtkSMClientServerRenderViewProxy.cxx
  vtkSMComparativeViewProxy.cxx
  vtkSMCompositeKeyFrameProxy.cxx
  vtkSMCompositeTreeDomain.cxx
  vtkSMCompoundSourceProxy.cxx
  vtkSMCompoundProxyDefinitionLoader.cxx
  vtkSMConnectionCleanerProxy.cxx
  vtkSMCSVExporterProxy.cxx
  vtkSMCubeAxesRepresentationProxy.cxx
  vtkSMDataLabelRepresentationProxy.cxx
  vtkSMDataRepresentationProxy.cxx
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
  vtkSMFieldDataDomain.cxx
  vtkSMFileListDomain.cxx
  vtkSMFileSeriesReaderProxy.cxx
  vtkSMFixedTypeDomain.cxx
  vtkSMHardwareSelector.cxx
  vtkSMIceTCompositeViewProxy.cxx
  vtkSMIceTDesktopRenderViewProxy.cxx
  vtkSMIceTMultiDisplayRenderViewProxy.cxx
  vtkSMIdBasedProxyLocator.cxx
  vtkSMIdTypeVectorProperty.cxx
  vtkSMImageDataParallelStrategy.cxx
  vtkSMImageSliceRepresentationProxy.cxx
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
  vtkSMMultiProcessRenderView.cxx
  vtkSMNewWidgetRepresentationProxy.cxx
  vtkSMNullProxy.cxx
  vtkSMNumberOfComponentsDomain.cxx
  vtkSMNumberOfGroupsDomain.cxx
  vtkSMNumberOfPartsDomain.cxx
  vtkSMObject.cxx
  vtkSMOrderedPropertyIterator.cxx
  vtkSMOutlineRepresentationProxy.cxx
  vtkSMOutputPort.cxx
  vtkSMNetworkImageSourceProxy.cxx
  vtkSMPQStateLoader.cxx
  vtkSMPSWriterProxy.cxx
  vtkSMPropertyAdaptor.cxx
  vtkSMProperty.cxx
  vtkSMPropertyHelper.cxx
  vtkSMPropertyIterator.cxx
  vtkSMPropertyLink.cxx
  vtkSMPropertyModificationUndoElement.cxx
  vtkSMPropertyStatusManager.cxx
  vtkSMPropRepresentationProxy.cxx
  vtkSMProxy.cxx
  vtkSMProxyDefinitionIterator.cxx
  vtkSMProxyGroupDomain.cxx
  vtkSMProxyIterator.cxx
  vtkSMProxyLink.cxx
  vtkSMProxyListDomain.cxx
  vtkSMProxyLocator.cxx
  vtkSMProxyManager.cxx
  vtkSMProxyManagerExtension.cxx
  vtkSMProxyManagerReviver.cxx
  vtkSMProxyProperty.cxx
  vtkSMProxyRegisterUndoElement.cxx
  vtkSMProxySelectionModel.cxx
  vtkSMProxyUnRegisterUndoElement.cxx
  vtkSMPVDWriterProxy.cxx
  vtkSMPVLookupTableProxy.cxx
  vtkSMPVRepresentationProxy.cxx
  vtkSMPWriterProxy.cxx
  vtkSMRampKeyFrameProxy.cxx
  vtkSMRenderViewHelper.cxx
  vtkSMRenderViewProxy.cxx
  vtkSMRenderViewExporterProxy.cxx
  vtkSMRepresentationAnimationHelperProxy.cxx
  vtkSMRepresentationProxy.cxx
  vtkSMRepresentationStrategy.cxx
  vtkSMScalarBarActorProxy.cxx
  vtkSMScalarBarWidgetRepresentationProxy.cxx
  vtkSMSelectionDeliveryRepresentationProxy.cxx
  vtkSMSelectionHelper.cxx
  vtkSMSelectionLink.cxx
  vtkSMSelectionLinkProxy.cxx
  vtkSMSelectionRepresentationProxy.cxx
  vtkSMServerFileListingProxy.cxx
  vtkSMServerProxyManagerReviver.cxx
  vtkSMServerSideAnimationPlayer.cxx
  vtkSMSimpleDoubleInformationHelper.cxx
  vtkSMSimpleIdTypeInformationHelper.cxx
  vtkSMSimpleIntInformationHelper.cxx
  vtkSMSimpleParallelStrategy.cxx
  vtkSMSimpleStrategy.cxx
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
  vtkSMSummaryHelperProxy.cxx
  vtkSMSurfaceRepresentationProxy.cxx
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
  vtkSMUniformGridParallelStrategy.cxx
  vtkSMUniformGridVolumeRepresentationProxy.cxx
  vtkSMUnstructuredDataParallelStrategy.cxx
  vtkSMUnstructuredGridParallelStrategy.cxx
  vtkSMUnstructuredGridVolumeRepresentationProxy.cxx
  vtkSMUtilities.cxx
  vtkSMUpdateInformationUndoElement.cxx
  vtkSMUpdateSuppressorProxy.cxx
  vtkSMVectorProperty.cxx
  vtkSMViewProxy.cxx
  vtkSMWidgetRepresentationProxy.cxx
  vtkSMWriterProxy.cxx
  vtkSMXDMFInformationHelper.cxx
  vtkSMXDMFPropertyDomain.cxx
  vtkSMXMLPVAnimationWriterProxy.cxx
  vtkSMXYPlotRepresentationProxy.cxx
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
  vtkSMDataRepresentationProxy.cxx
  vtkSMDomain.cxx
  vtkSMInformationHelper.cxx
  vtkSMLink.cxx
  vtkSMMultiProcessRenderView.cxx
  vtkSMMultiProcessRenderView.cxx
  vtkSMProxyManagerReviver.cxx
  vtkSMRepresentationProxy.cxx
  vtkSMRepresentationStrategy.cxx
  vtkSMStateVersionControllerBase.cxx
  vtkSMStateVersionControllerBase.cxx
  vtkSMUndoElement.cxx
  vtkSMVectorProperty.cxx
  vtkSMPropertyHelper.cxx
  vtkSMProxyManagerExtension.cxx
  vtkSMExporterProxy.cxx
  ABSTRACT
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkSMRenderViewHelper.cxx
  vtkSMPropertyHelper.cxx
  vtkSMProxyManagerExtension.cxx
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

# Create the library.
ADD_LIBRARY(vtk${KIT}
  ${ServerManager_SRCS} 
  ${CMInstantiator_SRCS} 
  ${vtkPVServerManagerCS_SRCS} 
  ${KitInstantiator_SRCS}
  )
TARGET_LINK_LIBRARIES(vtk${KIT}
  vtkPVFiltersCS 
  vtkClientServer
  ${KWCommon_LIBRARIES}
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
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/writers.xml
  ${PARAVIEW_EXTRA_SERVERMANAGER_RESOURCES}
  )
IF(PARAVIEW_ENABLE_PYTHON)
  SET(resourceFiles
      ${resourceFiles} 
      ${CMAKE_CURRENT_SOURCE_DIR}/Resources/pythonfilter.xml
      ) 
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

SET(GUI_CONFIG_FILES
  ParaViewSources
  ParaViewFilters
  ParaViewReaders
  ParaViewWriters
)

IF(NOT CMAKE_CROSSCOMPILING)

  ADD_EXECUTABLE(vtkSMExtractDocumentation${PV_EXE_SUFFIX} vtkSMExtractDocumentation.cxx)
  EXPORT(TARGETS vtkSMExtractDocumentation${PV_EXE_SUFFIX} FILE ${EXPORT_EXECUTABLES_FILE} NAMESPACE "${EXPORT_EXECUTABLES_NAMESPACE}" APPEND) 

  # Add shared link forwarding executables if necessary.
  IF(PV_NEED_SHARED_FORWARD)
    SET(edexe vtkSMExtractDocumentation)
    SET(PV_FORWARD_EXE ${edexe}${PV_EXE_SUFFIX})
    CONFIGURE_FILE(
      ${ParaView_SOURCE_DIR}/Servers/Executables/pv-forward.c.in
      ${CMAKE_CURRENT_BINARY_DIR}/${edexe}-forward.c
      @ONLY IMMEDIATE)
    ADD_EXECUTABLE(${edexe} ${CMAKE_CURRENT_BINARY_DIR}/${edexe}-forward.c)
    ADD_DEPENDENCIES(${edexe} ${edexe}${PV_EXE_SUFFIX})
    INSTALL(TARGETS ${edexe} DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime)
  ENDIF(PV_NEED_SHARED_FORWARD)

  TARGET_LINK_LIBRARIES(vtkSMExtractDocumentation${PV_EXE_SUFFIX} vtkPVServerManager)
ENDIF(NOT CMAKE_CROSSCOMPILING)

IF(CMAKE_CROSSCOMPILING)
  SET(DOCUMENTATION_EXE vtkSMExtractDocumentation)
ELSE(CMAKE_CROSSCOMPILING)
  GET_TARGET_PROPERTY(DOCUMENTATION_EXE vtkSMExtractDocumentation LOCATION)
ENDIF(CMAKE_CROSSCOMPILING)

SET(adpFile ${ParaView_SOURCE_DIR}/Documentation/pqClient.adp)
FOREACH(gcf ${GUI_CONFIG_FILES})
  SET(fullXMLFile ${ParaView_SOURCE_DIR}/Qt/Components/Resources/XML/${gcf}.xml)
  SET(fullHTMLFile ${ParaView_BINARY_DIR}/Documentation/${gcf}.html)
  ADD_CUSTOM_COMMAND(
    OUTPUT ${fullHTMLFile}
    DEPENDS ${fullXMLFile} ${resourceFiles} ${adpFile} vtkSMExtractDocumentation
    COMMAND ${DOCUMENTATION_EXE}
    ARGS ${ParaView_BINARY_DIR}/Documentation ${fullXMLFile}
    COMMENT "Creating Documentation for ${gcf}"
    )

  SET(HTML_RESULTS ${HTML_RESULTS} ${fullHTMLFile})
ENDFOREACH(gcf)

ADD_CUSTOM_TARGET(HTMLDocumentation
  DEPENDS ${HTML_RESULTS}
)

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


