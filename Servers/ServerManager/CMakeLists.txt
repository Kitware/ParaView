PROJECT(ServerManager)

SET( ServerManagerCS_SRCS
  vtkPickBoxWidget.cxx
  vtkPickPointWidget.cxx
  vtkPickLineWidget.cxx
  vtkPickSphereWidget.cxx
  )
 

SET( ServerManagerCommon_SRCS
  vtkSM3DWidgetProxy.cxx
  vtkSMAnimationCueManipulatorProxy.cxx
  vtkSMAnimationCueProxy.cxx
  vtkSMAnimationSceneProxy.cxx
  vtkSMApplication.cxx
  vtkSMArrayListDomain.cxx
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
  vtkSMCameraManipulatorProxy.cxx
  vtkSMCaveRenderModuleProxy.cxx
  vtkSMComparativeVisProxy.cxx
  vtkSMCompositeDisplayProxy.cxx
  vtkSMCompositeRenderModuleProxy.cxx
  vtkSMConsumerDisplayProxy.cxx
  vtkSMDataObjectDisplayProxy.cxx
  vtkSMDataTypeDomain.cxx
  vtkSMDisplayProxy.cxx
  vtkSMDomain.cxx
  vtkSMDomainIterator.cxx
  vtkSMDoubleRangeDomain.cxx
  vtkSMDoubleVectorProperty.cxx
  vtkSMEnumerationDomain.cxx
  vtkSMExponentialKeyFrameProxy.cxx
  vtkSMExtentDomain.cxx
  vtkSMFieldDataDomain.cxx
  vtkSMFixedTypeDomain.cxx
  vtkSMIceTDesktopRenderModuleProxy.cxx
  vtkSMIceTMultiDisplayProxy.cxx
  vtkSMIceTRenderModuleProxy.cxx
  vtkSMIdTypeVectorProperty.cxx
  vtkSMImplicitPlaneProxy.cxx
  vtkSMImplicitPlaneWidgetProxy.cxx
  vtkSMInformationHelper.cxx
  vtkSMInputArrayDomain.cxx
  vtkSMInputProperty.cxx
  vtkSMIntRangeDomain.cxx
  vtkSMIntVectorProperty.cxx
  vtkSMKeyFrameProxy.cxx
  vtkSMKeyFrameAnimationCueManipulatorProxy.cxx
  vtkSMLinearAnimationCueManipulatorProxy.cxx
  vtkSMLineWidgetProxy.cxx
  vtkSMLookupTableProxy.cxx
  vtkSMLODDisplayProxy.cxx
  vtkSMLODRenderModuleProxy.cxx
  vtkSMMultiDisplayProxy.cxx
  vtkSMMultiDisplayRenderModuleProxy.cxx
  vtkSMNumberOfGroupsDomain.cxx
  vtkSMNumberOfPartsDomain.cxx
  vtkSMObject.cxx
  vtkSMPVDWriterProxy.cxx
  vtkSMPart.cxx
  vtkSMPickBoxWidgetProxy.cxx
  vtkSMPickLineWidgetProxy.cxx
  vtkSMPickPointWidgetProxy.cxx
  vtkSMPickSphereWidgetProxy.cxx
  vtkSMPointWidgetProxy.cxx 
  vtkSMProperty.cxx
  vtkSMPropertyAdaptor.cxx
  vtkSMPropertyIterator.cxx
  vtkSMPropertyStatusManager.cxx
  vtkSMProxy.cxx
  vtkSMProxyGroupDomain.cxx
  vtkSMProxyIterator.cxx
  vtkSMProxyManager.cxx
  vtkSMProxyProperty.cxx
  vtkSMRampKeyFrameProxy.cxx
  vtkSMRenderModuleProxy.cxx
  vtkSMServerFileListingProxy.cxx
  vtkSMScalarBarActorProxy.cxx
  vtkSMScalarBarWidgetProxy.cxx
  vtkSMShrunkContoursProxy.cxx
  vtkSMSimpleIntInformationHelper.cxx
  vtkSMSinusoidKeyFrameProxy.cxx
  vtkSMSimpleDoubleInformationHelper.cxx
  vtkSMSimpleProxyInformationHelper.cxx
  vtkSMSimpleRenderModuleProxy.cxx
  vtkSMSimpleStringInformationHelper.cxx
  vtkSMMPIRenderModuleProxy.cxx
  vtkSMSourceProxy.cxx
  vtkSMSphereWidgetProxy.cxx
  vtkSMStringListDomain.cxx
  vtkSMStringListRangeDomain.cxx
  vtkSMStringVectorProperty.cxx
  vtkSMSubPropertyIterator.cxx
  vtkSMSummaryHelperProxy.cxx
  vtkSMTemporalXYPlotDisplayProxy.cxx
  vtkSMTimeStepsInformationHelper.cxx
  vtkSMTransformProxy.cxx
  vtkSMXDMFInformationHelper.cxx
  vtkSMXDMFPropertyDomain.cxx
  vtkSMVectorProperty.cxx
  vtkSMWriterProxy.cxx
  vtkSMCubeAxesDisplayProxy.cxx
  vtkSMPointLabelDisplayProxy.cxx
  vtkSMXMLPVAnimationWriterProxy.cxx
  vtkSMXYPlotActorProxy.cxx
  vtkSMXYPlotDisplayProxy.cxx
  )


SET( ServerManagerNonWrapped_SRCS
  vtkPVBatchOptions.cxx
  vtkSMXMLParser.cxx
  )

SET( ServerManager_SRCS
  ${ServerManagerCS_SRCS}
  ${ServerManagerCommon_SRCS}
  ${ServerManagerNonWrapped_SRCS}
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkSM3DWidgetProxy.cxx
  vtkSMAnimationCueManipulatorProxy.cxx
  vtkSMConsumerDisplayProxy.cxx
  vtkSMDomain.cxx
  vtkSMInformationHelper.cxx
  vtkSMRenderModuleProxy.cxx
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
VTK_MAKE_INSTANTIATOR3(
  vtkSMInstantiator 
  SMInstantiator_SRCS
  "${ServerManager_SRCS}"
  VTK_EXPORT
  ${ParaView_BINARY_DIR}/Servers/ServerManager "")

# load the ClientServer command
INCLUDE(${ParaView_SOURCE_DIR}/Utilities/VTKClientServer/vtkClientServer.cmake)
CS_INITIALIZE_WRAP()
INCLUDE_DIRECTORIES(
  ${ParaView_SOURCE_DIR}/Utilities/VTKClientServer
  ${ParaView_BINARY_DIR}/Utilities/VTKClientServer
  )

# Wrap PVServer
# Create ClientServer wrappers for ParaView classes.
INCLUDE(${VTK_SOURCE_DIR}/CMake/vtkExportKit.cmake)
SET(VTK_BINARY_DIR "${ServerManager_BINARY_DIR}")
VTK_EXPORT_KIT("PVServerManager" "PVSERVERMANAGER" "${ServerManagerCommon_SRCS};${ServerManagerCS_SRCS}")

SET(KIT_WRAP_DEPS "")
SET(VTK_KITS_DIR "${ParaView_BINARY_DIR}/Servers/ServerManager/Utilities")
PV_PRE_WRAP_VTK_CS(vtkPVServerManager PVServerManager PVSERVERMANAGER "${KIT_WRAP_DEPS}")

ADD_LIBRARY(vtkPVServerManager 
  ${ServerManager_SRCS} 
  ${CMInstantiator_SRCS} 
  ${vtkPVServerManagerCS_SRCS} 
  ${SMInstantiator_SRCS}
  ) 
TARGET_LINK_LIBRARIES (vtkPVServerManager 
  vtkPVFiltersCS 
  vtkClientServer
  ${KWCommon_LIBRARIES})

# Create Tcl wrappers for SM classes.
IF (PARAVIEW_BUILD_GUI)
  SET(VTK_WRAP_TCL_EXE ${PARAVIEW_VTK_WRAP_TCL_EXE})
  SET(VTK_WRAP_TCL_INIT_EXE ${PARAVIEW_VTK_WRAP_TCL_INIT_EXE})
  SET(VTK_WRAP_TCL3_INIT_DIR "${VTK_SOURCE_DIR}/Wrapping")
  INCLUDE("${VTK_SOURCE_DIR}/CMake/vtkWrapTcl.cmake")
  VTK_WRAP_TCL3(
    vtkPVServerManagerTCL 
    ServerManagerTCL_SRCS 
    "${ServerManagerCommon_SRCS};${SMInstantiator_SRCS}"
    ""
    )
  IF(NOT WIN32)
    SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
    ADD_DEFINITIONS(-D_HPUX_SOURCE)
  ENDIF(NOT WIN32)
  CONFIGURE_FILE(
    "${CMAKE_CURRENT_SOURCE_DIR}/vtkPVProcessModuleBatchHelperConfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/vtkPVProcessModuleBatchHelperConfig.h"
  @ONLY IMMEDIATE)
  ADD_LIBRARY(vtkPVServerManagerTCL ${ServerManagerTCL_SRCS}
    vtkPVProcessModuleBatchHelper.cxx)
  TARGET_LINK_LIBRARIES (vtkPVServerManagerTCL 
    vtkPVServerCommonTCL vtkPVServerManager vtkCommonTCL)
ENDIF (PARAVIEW_BUILD_GUI)

IF (PARAVIEW_WRAP_PYTHON)
  SET(VTK_WRAP_PYTHON_EXE ${PARAVIEW_VTK_WRAP_PYTHON_EXE})
  SET(VTK_WRAP_PYTHON_INIT_EXE ${PARAVIEW_VTK_WRAP_PYTHON_INIT_EXE})
  SET(VTK_WRAP_PYTHON3_INIT_DIR "${VTK_SOURCE_DIR}/Wrapping")
  INCLUDE("${VTK_SOURCE_DIR}/CMake/vtkWrapPython.cmake")
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
  VTK_WRAP_PYTHON3(
    vtkPVServerManagerPython 
    ServerManagerPython_SRCS 
    "${ServerManagerCommon_SRCS};${SMInstantiator_SRCS}"
    ""
    )
  IF(NOT WIN32)
    SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
    ADD_DEFINITIONS(-D_HPUX_SOURCE)
  ENDIF(NOT WIN32)
  ADD_LIBRARY(vtkPVServerManagerPython ${ServerManagerPython_SRCS})
  TARGET_LINK_LIBRARIES (vtkPVServerManagerPython 
    vtkPVServerCommonPython vtkPVServerManager vtkCommonPython)
ENDIF (PARAVIEW_WRAP_PYTHON)

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
  STRING(REGEX REPLACE "^.*/(.*).xml$" "\\1" moduleName "${rf}")
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
  INSTALL_TARGETS(${PV_INSTALL_LIB_DIR}
    vtkPVServerManagerTCL
    vtkPVServerManager)
ENDIF(NOT PV_INSTALL_NO_LIBRARIES)
IF(NOT PV_INSTALL_NO_RUNTIME)
  INSTALL_FILES(${PV_INSTALL_LIB_DIR}/ServerManager/Resources FILES
    ${resourceFiles}
    )
ENDIF(NOT PV_INSTALL_NO_RUNTIME)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)
