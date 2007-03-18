PROJECT(ServerManager)
INCLUDE_REGULAR_EXPRESSION("^vtk.*$")

SET(KIT PVServerManager)
SET(KIT_UPCASE "PVSERVERMANAGER")
IF(NOT WIN32)
  SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
  ADD_DEFINITIONS(-D_HPUX_SOURCE)
ENDIF(NOT WIN32)

SET(Kit_SRCS
  vtkSMProxyManager.cxx
  vtkSM3DWidgetProxy.cxx
  vtkSMAbstractDisplayProxy.cxx
  vtkSMAbstractViewModuleProxy.cxx
  vtkSMAnimationCueManipulatorProxy.cxx
  vtkSMAnimationCueProxy.cxx
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
  vtkSMAxesProxy.cxx
  vtkSMBooleanDomain.cxx
  vtkSMBooleanKeyFrameProxy.cxx
  vtkSMBoundsDomain.cxx
  vtkSMBoxProxy.cxx
  vtkSMBoxWidgetProxy.cxx
  vtkSMCameraKeyFrameProxy.cxx
  vtkSMCameraLink.cxx
  vtkSMCameraManipulatorProxy.cxx
  vtkSMCameraProxy.cxx
  vtkSMCaveRenderModuleProxy.cxx
  vtkSMClientServerRenderModuleProxy.cxx
  vtkSMComparativeVisProxy.cxx
  vtkSMCompositeDisplayProxy.cxx
  vtkSMCompositeKeyFrameProxy.cxx
  vtkSMCompositeRenderModuleProxy.cxx
  vtkSMCompoundProxy.cxx
  vtkSMCompoundProxyDefinitionLoader.cxx
  vtkSMConnectionCleanerProxy.cxx
  vtkSMConsumerDisplayProxy.cxx
  vtkSMDataObjectDisplayProxy.cxx
  vtkSMDataTypeDomain.cxx
  vtkSMDefaultStateLoader.cxx
  vtkSMDisplayProxy.cxx
  vtkSMDocumentation.cxx
  vtkSMDomain.cxx
  vtkSMDomainIterator.cxx
  vtkSMDoubleArrayInformationHelper.cxx
  vtkSMDoubleRangeDomain.cxx
  vtkSMDoubleVectorProperty.cxx
  vtkSMEnumerationDomain.cxx
  vtkSMExponentialKeyFrameProxy.cxx
  vtkSMExtentDomain.cxx
  vtkSMFieldDataDomain.cxx
  vtkSMFileListDomain.cxx
  vtkSMFixedTypeDomain.cxx
  vtkSMGenericViewDisplayProxy.cxx
  vtkSMIceTDesktopRenderModuleProxy.cxx
  vtkSMIceTMultiDisplayProxy.cxx
  vtkSMIceTRenderModuleProxy.cxx
  vtkSMIdTypeVectorProperty.cxx
  vtkSMImplicitPlaneProxy.cxx
  vtkSMImplicitPlaneWidgetProxy.cxx
  vtkSMImplicitPlaneRepresentationProxy.cxx
  vtkSMInformationHelper.cxx
  vtkSMInputArrayDomain.cxx
  vtkSMInputProperty.cxx
  vtkSMIntArrayInformationHelper.cxx
  vtkSMIntRangeDomain.cxx
  vtkSMIntVectorProperty.cxx
  vtkSMKeyFrameProxy.cxx
  vtkSMKeyFrameAnimationCueManipulatorProxy.cxx
  vtkSMLinearAnimationCueManipulatorProxy.cxx
  vtkSMLineWidgetProxy.cxx
  vtkSMLink.cxx
  vtkSMLookupTableProxy.cxx
  vtkSMLODDisplayProxy.cxx
  vtkSMLODRenderModuleProxy.cxx
  vtkSMMaterialLoaderProxy.cxx
  vtkSMMultiDisplayProxy.cxx
  vtkSMMultiViewRenderModuleProxy.cxx
  vtkSMNew3DWidgetProxy.cxx
  vtkSMNumberOfGroupsDomain.cxx
  vtkSMNumberOfPartsDomain.cxx
  vtkSMObject.cxx
  vtkSMOrderedPropertyIterator.cxx
  vtkSMPVDWriterProxy.cxx
  vtkSMPart.cxx
  vtkSMPickLineWidgetProxy.cxx
  vtkSMPickPointWidgetProxy.cxx
  vtkSMPluginLoader.cxx
  vtkSMPointWidgetProxy.cxx 
  vtkSMProperty.cxx
  vtkSMPropertyAdaptor.cxx
  vtkSMPropertyIterator.cxx
  vtkSMPropertyLink.cxx
  vtkSMPropertyModificationUndoElement.cxx
  vtkSMPropertyStatusManager.cxx
  vtkSMProxy.cxx
  vtkSMProxyDefinitionIterator.cxx
  vtkSMProxyGroupDomain.cxx
  vtkSMProxyIterator.cxx
  vtkSMProxyLink.cxx
  vtkSMProxyListDomain.cxx
  vtkSMProxyManagerReviver.cxx
  vtkSMProxyProperty.cxx
  vtkSMProxyRegisterUndoElement.cxx
  vtkSMProxyUnRegisterUndoElement.cxx
  vtkSMPQStateLoader.cxx
  vtkSMPVAnimationSceneProxy.cxx
  vtkSMPWriterProxy.cxx
  vtkSMRampKeyFrameProxy.cxx
  vtkSMRenderModuleProxy.cxx
  vtkSMScalarBarActorProxy.cxx
  vtkSMScalarBarWidgetProxy.cxx
  vtkSMSelectionHelper.cxx
#  vtkSMSelectionProxy.cxx
  vtkSMServerFileListingProxy.cxx
  vtkSMServerProxyManagerReviver.cxx
  vtkSMServerSideAnimationPlayer.cxx
  vtkSMShrunkContoursProxy.cxx
  vtkSMSimpleIntInformationHelper.cxx
  vtkSMSinusoidKeyFrameProxy.cxx
  vtkSMSimpleDoubleInformationHelper.cxx
  vtkSMSimpleRenderModuleProxy.cxx
  vtkSMSimpleStringInformationHelper.cxx
  vtkSMSourceProxy.cxx
  vtkSMSphereWidgetProxy.cxx
  vtkSMStateLoader.cxx
  vtkSMStringListDomain.cxx
  vtkSMStringListRangeDomain.cxx
  vtkSMStringVectorProperty.cxx
  vtkSMSubPropertyIterator.cxx
  vtkSMSummaryHelperProxy.cxx
  vtkSMTemporalXYPlotDisplayProxy.cxx
  vtkSMTesting.cxx
  vtkSMTimeStepsInformationHelper.cxx
  vtkSMTransformProxy.cxx
  vtkSMUndoElement.cxx
  vtkSMUndoRedoStateLoader.cxx
  vtkSMUndoStackBuilder.cxx
  vtkSMUndoStack.cxx
  vtkSMUpdateInformationUndoElement.cxx
  vtkSMUpdateSuppressorProxy.cxx
  vtkSMXDMFInformationHelper.cxx
  vtkSMXDMFPropertyDomain.cxx
  vtkSMVectorProperty.cxx
  vtkSMViewModuleProxy.cxx
  vtkSMWidgetRepresentationProxy.cxx
  vtkSMWriterProxy.cxx
  vtkSMCubeAxesDisplayProxy.cxx
  vtkSMPointLabelDisplayProxy.cxx
  vtkSMXMLPVAnimationWriterProxy.cxx
  vtkSMXYPlotActorProxy.cxx
  vtkSMXYPlotDisplayProxy.cxx
  )

SET( ServerManagerCS_SRCS
  vtkPickPointWidget.cxx
  vtkPickLineWidget.cxx
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
  vtkSM3DWidgetProxy.cxx
  vtkSMAbstractDisplayProxy.cxx
  vtkSMAbstractViewModuleProxy.cxx
  vtkSMAnimationCueManipulatorProxy.cxx
  vtkSMAnimationSceneWriter.cxx
  vtkSMCompositeRenderModuleProxy.cxx
  vtkSMConsumerDisplayProxy.cxx
  vtkSMDomain.cxx
  vtkSMInformationHelper.cxx
  vtkSMLink.cxx
  vtkSMProxyManagerReviver.cxx
  vtkSMRenderModuleProxy.cxx
  vtkSMUndoElement.cxx
  vtkSMVectorProperty.cxx
  ABSTRACT
  )


INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${TCL_INCLUDE_PATH}
  ${ParaView_SOURCE_DIR}/Servers/Filters
  ${ParaView_SOURCE_DIR}/Servers/Common
  ${ParaView_SOURCE_DIR}/Servers/ServerManager
  ${ParaView_BINARY_DIR}/Servers/ServerManager
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

# Wrap PVServerCommon
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
  ${KWCommon_LIBRARIES})

# Create Tcl wrappers for classes.
IF (PARAVIEW_BUILD_GUI)
  SET(VTK_WRAP_TCL_EXE ${PARAVIEW_VTK_WRAP_TCL_EXE})
  SET(VTK_WRAP_TCL_INIT_EXE ${PARAVIEW_VTK_WRAP_TCL_INIT_EXE})
  INCLUDE("${VTK_CMAKE_DIR}/vtkWrapTcl.cmake")
  VTK_WRAP_TCL3(
    vtk${KIT}TCL 
    KitTCL_SRCS 
    "${Kit_SRCS}"
    ""
    )
  CONFIGURE_FILE(
    "${CMAKE_CURRENT_SOURCE_DIR}/vtkPVProcessModuleBatchHelperConfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/vtkPVProcessModuleBatchHelperConfig.h"
    @ONLY IMMEDIATE)
  ADD_LIBRARY(vtk${KIT}TCL ${KitTCL_SRCS}
    vtkPVProcessModuleBatchHelper.cxx
    )
  TARGET_LINK_LIBRARIES (vtk${KIT}TCL vtk${KIT} vtkCommonTCL)
  TARGET_LINK_LIBRARIES (vtk${KIT}TCL vtkPVServerCommonTCL)
ENDIF (PARAVIEW_BUILD_GUI)

IF (PARAVIEW_ENABLE_PYTHON)
  SET(VTK_WRAP_PYTHON_EXE ${PARAVIEW_VTK_WRAP_PYTHON_EXE})
  SET(VTK_WRAP_PYTHON_INIT_EXE ${PARAVIEW_VTK_WRAP_PYTHON_INIT_EXE})
  SET(Kit_PYTHON_EXTRA_SRCS
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
  SET(VTK_INSTALL_LIB_DIR "${VTKPythonWrapping_INSTALL_LIB_DIR}")
  SET(VTK_INSTALL_BIN_DIR "${VTKPythonWrapping_INSTALL_BIN_DIR}")
  STRING(REGEX REPLACE "^/" "" VTK_INSTALL_LIB_DIR_CM24 "${VTK_INSTALL_LIB_DIR}")
  STRING(REGEX REPLACE "^/" "" VTK_INSTALL_BIN_DIR_CM24 "${VTK_INSTALL_BIN_DIR}")

  # include python wrapping from VTK
  INCLUDE(KitCommonPythonWrapBlock)
ENDIF (PARAVIEW_ENABLE_PYTHON)

SET(CFG_INIT "/${CMAKE_CFG_INTDIR}")
IF(MAKE_SYSTEM OR UNIX)
  SET(CFG_INIT "")
ENDIF(MAKE_SYSTEM OR UNIX)
SET(CMD ${EXECUTABLE_OUTPUT_PATH}${CFG_INIT}/kwProcessXML${EXE_EXT})

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
  kwProcessXML
  COMMAND ${CMD}
  ARGS ${CMAKE_CURRENT_BINARY_DIR}/vtkSMGeneratedModules.h
  vtkSMDefaultModules Interface GetInterfaces
  ${resourceFiles})

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

SET_SOURCE_FILES_PROPERTIES(vtkSMApplication.cxx PROPERTIES OBJECT_DEPENDS 
  ${CMAKE_CURRENT_BINARY_DIR}/vtkSMGeneratedModules.h)

IF(NOT PV_INSTALL_NO_LIBRARIES)
  SET (targets_to_install)
  IF (PARAVIEW_BUILD_GUI)
    SET (targets_to_install
      vtk${KIT}TCL)
  ENDIF (PARAVIEW_BUILD_GUI)
  
  IF(PARAVIEW_ENABLE_PYTHON)
    SET (targets_to_install ${targets_to_install} 
      vtk${KIT}Python)
  ENDIF(PARAVIEW_ENABLE_PYTHON)
    
  IF(PV_INSTALL_HAS_CMAKE_24)
    INSTALL(TARGETS vtk${KIT} ${targets_to_install}
      RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime
      LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime
      ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Development)
  ELSE(PV_INSTALL_HAS_CMAKE_24)
    INSTALL_TARGETS(${PV_INSTALL_LIB_DIR}
      vtk${KIT}
      ${targets_to_install}
      )
  ENDIF(PV_INSTALL_HAS_CMAKE_24)

ENDIF(NOT PV_INSTALL_NO_LIBRARIES)
IF(NOT PV_INSTALL_NO_RUNTIME)
  INSTALL_FILES(${PV_INSTALL_LIB_DIR}/ServerManager/Resources FILES
    ${resourceFiles}
    )
ENDIF(NOT PV_INSTALL_NO_RUNTIME)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)
