PROJECT(ServerManager)

SET( ServerManagerCommon_SRCS
  vtkSMInteractorObserverProxy.cxx
  vtkSM3DWidgetProxy.cxx
  vtkSMBoxWidgetProxy.cxx
  vtkSMImplicitPlaneWidgetProxy.cxx
  vtkSMLineWidgetProxy.cxx
  vtkSMPointWidgetProxy.cxx 
  vtkSMSphereWidgetProxy.cxx
  vtkSMKeyFrameProxy.cxx
  vtkSMBooleanKeyFrameProxy.cxx
  vtkSMRampKeyFrameProxy.cxx
  vtkSMExponentialKeyFrameProxy.cxx
  vtkSMSinusoidKeyFrameProxy.cxx
  vtkSMAnimationCueManipulatorProxy.cxx
  vtkSMLinearAnimationCueManipulatorProxy.cxx
  vtkSMKeyFrameAnimationCueManipulatorProxy.cxx
  vtkSMAnimationCueProxy.cxx
  vtkSMAnimationSceneProxy.cxx
  vtkSMApplication.cxx
  vtkSMArrayListDomain.cxx
  vtkSMArrayRangeDomain.cxx
  vtkSMArraySelectionDomain.cxx
  vtkSMArraySelectionInformationHelper.cxx
  vtkSMAxesProxy.cxx
  vtkSMBooleanDomain.cxx
  vtkSMBoundsDomain.cxx
  vtkSMBoxProxy.cxx
  vtkSMDataTypeDomain.cxx
  vtkSMDisplayWindowProxy.cxx
  vtkSMDisplayerProxy.cxx
  vtkSMDomain.cxx
  vtkSMDomainIterator.cxx
  vtkSMDoubleRangeDomain.cxx
  vtkSMDoubleVectorProperty.cxx
  vtkSMEnumerationDomain.cxx
  vtkSMExtentDomain.cxx
  vtkSMFieldDataDomain.cxx
  vtkSMFixedTypeDomain.cxx
  vtkSMIdTypeVectorProperty.cxx
  vtkSMImplicitPlaneProxy.cxx
  vtkSMInformationHelper.cxx
  vtkSMInputArrayDomain.cxx
  vtkSMInputProperty.cxx
  vtkSMIntRangeDomain.cxx
  vtkSMIntVectorProperty.cxx
  vtkSMLookupTableProxy.cxx
  vtkSMNumberOfPartsDomain.cxx
  vtkSMObject.cxx
  vtkSMPart.cxx
  vtkSMProperty.cxx
  vtkSMPropertyAdaptor.cxx
  vtkSMPropertyIterator.cxx
  vtkSMPropertyStatusManager.cxx
  vtkSMProxy.cxx
  vtkSMProxyGroupDomain.cxx
  vtkSMProxyIterator.cxx
  vtkSMProxyManager.cxx
  vtkSMProxyProperty.cxx
  vtkSMScalarBarWidgetProxy.cxx
  vtkSMSimpleIntInformationHelper.cxx
  vtkSMSimpleDoubleInformationHelper.cxx
  vtkSMSimpleProxyInformationHelper.cxx
  vtkSMSimpleStringInformationHelper.cxx
  vtkSMSourceProxy.cxx
  vtkSMStringListDomain.cxx
  vtkSMStringListRangeDomain.cxx
  vtkSMStringVectorProperty.cxx
  vtkSMSubPropertyIterator.cxx
  vtkSMSummaryHelperProxy.cxx
  vtkSMTransformProxy.cxx
  vtkSMXDMFInformationHelper.cxx
  vtkSMXDMFPropertyDomain.cxx
  vtkSMVectorProperty.cxx
  vtkSMWriterProxy.cxx
  vtkSMDisplay.cxx
  vtkSMPartDisplay.cxx
  vtkSMCompositePartDisplay.cxx
  vtkSMLODPartDisplay.cxx
  vtkSMMultiDisplayPartDisplay.cxx
  vtkSMCubeAxesDisplay.cxx
  vtkSMPlotDisplay.cxx
  vtkSMPointLabelDisplay.cxx
  vtkSMXMLPVAnimationWriterProxy.cxx
  )

SET( ServerManagerNonWrapped_SRCS
  vtkPVBatchOptions.cxx
  vtkPVCaveRenderModule.cxx
  vtkPVCompositeRenderModule.cxx
  vtkPVDeskTopRenderModule.cxx
  vtkPVIceTRenderModule.cxx
  vtkPVMultiDisplayRenderModule.cxx
  vtkPVLODRenderModule.cxx
  vtkPVMPIRenderModule.cxx
  vtkPVSimpleRenderModule.cxx
  vtkSMXMLParser.cxx
  )

SET( ServerManager_SRCS
  ${ServerManagerCommon_SRCS}
  ${ServerManagerNonWrapped_SRCS}
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkSMInteractorObserverProxy.cxx
  vtkSM3DWidgetProxy.cxx
  vtkSMAnimationCueManipulatorProxy.cxx
  vtkSMDomain.cxx
  vtkSMInformationHelper.cxx
  vtkSMVectorProperty.cxx
  vtkSMDisplay.cxx
  ABSTRACT
  )


INCLUDE_DIRECTORIES(
  ${KWSYS_INCLUDE_DIR}
  ${VTK_BINARY_DIR}
  ${TCL_INCLUDE_PATH}
  ${ParaView_SOURCE_DIR}/Common/KWCommon
  ${ParaView_BINARY_DIR}/Common/KWCommon
  ${ParaView_SOURCE_DIR}/Servers/Filters
  ${ParaView_SOURCE_DIR}/Servers/Common
  ${ParaView_SOURCE_DIR}/Servers/ServerManager
  ${ParaView_BINARY_DIR}/Servers/ServerManager
)

SET(kits Common Filtering GenericFiltering IO Imaging Rendering Parallel Graphics Hybrid)
IF(VTK_USE_PATENTED)
  SET(kits ${kits} Patented)
ENDIF(VTK_USE_PATENTED)
FOREACH(kit ${kits})
  INCLUDE_DIRECTORIES(
    ${VTK_SOURCE_DIR}/${kit}
    ${VTK_BINARY_DIR}/${kit}
    )
ENDFOREACH(kit)

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
VTK_EXPORT_KIT("PVServerManager" "PVSERVERMANAGER" "${ServerManagerCommon_SRCS}")

SET(KIT_WRAP_DEPS "")
SET(VTK_KITS_DIR "${ParaView_BINARY_DIR}/Servers/ServerManager/Utilities")
PV_PRE_WRAP_VTK_CS(vtkPVServerManager PVServerManager PVSERVERMANAGER "${KIT_WRAP_DEPS}")

ADD_LIBRARY(vtkPVServerManager 
  ${ServerManager_SRCS} 
  ${CMInstantiator_SRCS} 
  ${vtkPVServerManagerCS_SRCS} 
  ${SMInstantiator_SRCS}
  ) 
TARGET_LINK_LIBRARIES (vtkPVServerManager vtkPVFiltersCS vtkClientServer)

# Create Tcl wrappers for SM classes.
IF (PARAVIEW_BUILD_GUI)
  SET(VTK_WRAP_TCL_EXE ${PARAVIEW_VTK_WRAP_TCL_EXE})
  SET(VTK_WRAP_TCL_INIT_EXE ${PARAVIEW_VTK_WRAP_TCL_INIT_EXE})
  SET(VTK_WRAP_TCL3_INIT_DIR "${VTKTclWrapping_SOURCE_DIR}")
  INCLUDE("${VTKTclWrapping_SOURCE_DIR}/vtkWrapTcl.cmake")
  VTK_WRAP_TCL3(
    vtkPVServerManagerTCL 
    ServerManagerTCL_SRCS 
    "${ServerManagerCommon_SRCS};${SMInstantiator_SRCS}"
    ""
    )
  SET(VTK_TCL_HOME "${ParaView_SOURCE_DIR}/GUI")
  IF(NOT WIN32)
    # Needed for vtktcl.c
    SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
    ADD_DEFINITIONS(-D_HPUX_SOURCE)
  ENDIF(NOT WIN32)
  ADD_LIBRARY(vtkPVServerManagerTCL ${ServerManagerTCL_SRCS}
    ${VTK_TCL_HOME}/vtktcl.c
    vtkPVProcessModuleBatchHelper.cxx)
  TARGET_LINK_LIBRARIES (vtkPVServerManagerTCL vtkPVServerManager vtkCommonTCL)
ENDIF (PARAVIEW_BUILD_GUI)


SET(CFG_INIT "/${CMAKE_CFG_INTDIR}")
IF(MAKE_SYSTEM OR UNIX)
  SET(CFG_INIT "")
ENDIF(MAKE_SYSTEM OR UNIX)
SET(CMD ${EXECUTABLE_OUTPUT_PATH}${CFG_INIT}/kwProcessXML${EXE_EXT})

ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkSMGeneratedModules.h
  DEPENDS 
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/filters.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/sources.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/readers.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/utilities.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/rendering.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/writers.xml
  kwProcessXML
  COMMAND ${CMD}
  ARGS ${CMAKE_CURRENT_BINARY_DIR}/vtkSMGeneratedModules.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources
  vtkSMDefaultModules Interface GetInterfaces
  filters readers sources utilities rendering writers)

SET_SOURCE_FILES_PROPERTIES(vtkSMApplication.cxx PROPERTIES OBJECT_DEPENDS 
  ${CMAKE_CURRENT_BINARY_DIR}/vtkSMGeneratedModules.h)

IF(BUILD_SHARED_LIBS)
  INSTALL_TARGETS(/lib
    vtkPVServerManagerTCL
    vtkPVServerManager)
  INSTALL_FILES(/lib/paraview-${PARAVIEW_VERSION}/ServerManager/Resources FILES
    Resources/filters.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/readers.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/rendering.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/sources.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/utilities.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/writers.xml
    )
ENDIF(BUILD_SHARED_LIBS)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)
