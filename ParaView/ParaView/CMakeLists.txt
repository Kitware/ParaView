SET( ParaViewShared_SRCS
  vtkPVApplication.cxx
  vtkPVClientServerModule.cxx
  vtkPVExtentTranslator.cxx
  vtkPVGenericRenderWindowInteractor.cxx
  vtkPVGeometryFilter.cxx
  vtkPVLODActor.cxx
  vtkPVMPIProcessModule.cxx
  vtkPVPolyDataMapper.cxx
  vtkPVProcessModule.cxx
  vtkPVTreeComposite.cxx
  vtkStringList.cxx
  )

SET( ParaViewServer_SRCS
  vtkClientCompositeManager.cxx
  vtkCompleteArrays.cxx
  vtkExtractCTHPart.cxx
  vtkPVServerArraySelection.cxx
  vtkPVServerFileListing.cxx
  vtkPVServerObject.cxx
  vtkPVServerProbe.cxx
  vtkPVServerSelectTimeSet.cxx
  vtkPVSummaryHelper.cxx
  vtkSelectInputs.cxx
  vtkXMLCollectionReader.cxx
  vtkXMLPVAnimationWriter.cxx
  )

IF(VTK_USE_MPI)
  SET( ParaViewServer_SRCS
    ${ParaViewServer_SRCS}
    vtkMultiDisplayManager.cxx
    )
ENDIF(VTK_USE_MPI)
IF(PARAVIEW_BUILD_DEVELOPMENT)
  SET( ParaViewServer_SRCS
    ${ParaViewServer_SRCS}
    vtkCTHData.cxx
    vtkCTHDataToPolyDataFilter.cxx
    vtkCTHExtractAMRPart.cxx
    vtkCTHFractal.cxx
    vtkCTHOutlineFilter.cxx
    vtkCTHSource.cxx
    )
ENDIF(PARAVIEW_BUILD_DEVELOPMENT)

SET( ParaViewServerNonWrapped_SRCS
  )

SET( ParaViewExtraWrapper_SRCS
  ${PARAVIEW_SOURCE_DIR}/Widgets/vtkKWApplication.h
  ${PARAVIEW_SOURCE_DIR}/Widgets/vtkKWObject.h
  )

IF(XDMF_FOUND)
  SET(ParaViewServer_SRCS ${ParaViewServer_SRCS}
    vtkPVServerXDMFParameters.cxx
    )
ENDIF(XDMF_FOUND)

SET( ParaViewClient_SRCS
  vtkKWBoundsDisplay.cxx
  vtkPV3DWidget.cxx
  vtkPVAdvancedReaderModule.cxx
  vtkPVAnimationInterface.cxx
  vtkPVAnimationInterfaceEntry.cxx
  vtkPVApplicationResources.cxx
  vtkPVApplicationSettingsInterface.cxx
  vtkPVArrayInformation.cxx
  vtkPVArrayMenu.cxx
  vtkPVArraySelection.cxx
  vtkPVAxesActor.cxx
  vtkPVAxesWidget.cxx
  vtkPVBatchCompositeManager.cxx
  vtkPVBoundsDisplay.cxx
  vtkPVBoxWidget.cxx
  vtkPVCalculatorWidget.cxx
  vtkPVCameraControl.cxx
  vtkPVCameraIcon.cxx
  vtkPVCameraManipulator.cxx
  vtkPVClassNameInformation.cxx
  vtkPVColorMap.cxx
  vtkPVCompositeBuffer.cxx
  vtkPVCompositePartDisplay.cxx
  vtkPVCompositeRenderModule.cxx
  vtkPVCompositeRenderModuleUI.cxx
  vtkPVCompositeUtilities.cxx
  vtkPVConnectDialog.cxx
  vtkPVContainerWidget.cxx
  vtkPVContourEntry.cxx
  vtkPVContourWidgetProperty.cxx
  vtkPVCutEntry.cxx
  vtkPVDReaderModule.cxx
  vtkPVDWriter.cxx
  vtkPVData.cxx
  vtkPVDataInformation.cxx
  vtkPVDataSetAttributesInformation.cxx
  vtkPVDataSetFileEntry.cxx
  vtkPVDataSetReaderModule.cxx
  vtkPVDummyWidget.cxx
  vtkPVEnSightReaderModule.cxx
  vtkPVErrorLogDisplay.cxx
  vtkPVExtentEntry.cxx
  vtkPVExtentWidgetProperty.cxx
  vtkPVExtractPartsWidget.cxx
  vtkPVFieldMenu.cxx
  vtkPVFileEntry.cxx
  vtkPVFileEntryProperty.cxx
  vtkPVGhostLevelDialog.cxx
  vtkPVGroupInputsWidget.cxx
  vtkPVImplicitPlaneWidget.cxx
  vtkPVIndexWidgetProperty.cxx
  vtkPVInformation.cxx
  vtkPVInitialize.cxx
  vtkPVInputArrayRequirement.cxx
  vtkPVInputFixedTypeRequirement.cxx
  vtkPVInputGroupRequirement.cxx
  vtkPVInputMenu.cxx
  vtkPVInputProperty.cxx
  vtkPVInputRequirement.cxx
  vtkPVInteractorStyle.cxx
  vtkPVInteractorStyleCenterOfRotation.cxx
  vtkPVInteractorStyleControl.cxx
  vtkPVJoystickFly.cxx
  vtkPVJoystickFlyIn.cxx
  vtkPVJoystickFlyOut.cxx
  vtkPVLODPartDisplay.cxx
  vtkPVLODPartDisplayInformation.cxx
  vtkPVLODRenderModule.cxx
  vtkPVLODRenderModuleUI.cxx
  vtkPVLabel.cxx
  vtkPVLabeledToggle.cxx
  vtkPVLineSourceWidget.cxx
  vtkPVLineWidget.cxx
  vtkPVMPIRenderModule.cxx
  vtkPVMPIRenderModuleUI.cxx
  vtkPVMinMax.cxx
  vtkPVMultiDisplayPartDisplay.cxx
  vtkPVMultiDisplayRenderModule.cxx
  vtkPVMultiDisplayRenderModuleUI.cxx
  vtkPVNavigationWindow.cxx
  vtkPVNumberOfOutputsInformation.cxx
  vtkPVObjectWidget.cxx
  vtkPVOrientScaleWidget.cxx
  vtkPVPLOT3DReaderModule.cxx
  vtkPVPart.cxx
  vtkPVPartDisplay.cxx
  vtkPVPointSourceWidget.cxx
  vtkPVPointWidget.cxx
  vtkPVProbe.cxx
  vtkPVPushButton.cxx
  vtkPVRawReaderModule.cxx
  vtkPVReaderModule.cxx
  vtkPVRenderModule.cxx
  vtkPVRenderModuleUI.cxx
  vtkPVRenderView.cxx
  vtkPVSaveBatchScriptDialog.cxx
  vtkPVScalarListWidgetProperty.cxx
  vtkPVScalarRangeLabel.cxx
  vtkPVScale.cxx
  vtkPVScaleFactorEntry.cxx
  vtkPVSelectCTHArrays.cxx
  vtkPVSelectCustomReader.cxx
  vtkPVSelectTimeSet.cxx
  vtkPVSelectWidget.cxx
  vtkPVSelectionList.cxx
  vtkPVServerFileDialog.cxx
  vtkPVSource.cxx
  vtkPVSourceCollection.cxx
  vtkPVSourceList.cxx
  vtkPVSourceWidget.cxx
  vtkPVSourcesNavigationWindow.cxx
  vtkPVSphereWidget.cxx
  vtkPVStringAndScalarListWidgetProperty.cxx
  vtkPVStringEntry.cxx
  vtkPVStringWidgetProperty.cxx
  vtkPVTimerInformation.cxx
  vtkPVTimerLogDisplay.cxx
  vtkPVTraceFileDialog.cxx
  vtkPVTrackballMoveActor.cxx
  vtkPVTrackballPan.cxx
  vtkPVTrackballRoll.cxx
  vtkPVTrackballRotate.cxx
  vtkPVTrackballZoom.cxx
  vtkPVValueList.cxx
  vtkPVVectorEntry.cxx
  vtkPVWidget.cxx
  vtkPVWidgetCollection.cxx
  vtkPVWidgetProperty.cxx
  vtkPVWindow.cxx
  vtkPVWindowResources.cxx
  vtkPVWorldPointPicker.cxx
  vtkPVWriter.cxx
  vtkPVXDMFParameters.cxx
  vtkPVXMLElement.cxx
  vtkPVXMLPackageParser.cxx
  vtkPVXMLParser.cxx
  vtkXDMFReaderModule.cxx
  #  vtkPVRenderGroupDialog.cxx
  )

SET(ParaViewBinary_SRCS
  ParaView.cxx)

IF (PARAVIEW_USE_ICE_T)
  SET( ParaViewClient_SRCS
    ${ParaViewClient_SRCS}
    #vtkIceTComposite.cxx
    vtkIceTFactory.cxx
    vtkPVDeskTopRenderModule.cxx
    vtkPVDeskTopRenderModuleUI.cxx
    vtkPVIceTDisplayRenderModule.cxx
    vtkPVIceTDisplayRenderModuleUI.cxx
    vtkPVIceTRenderModule.cxx
    vtkPVIceTRenderModuleUI.cxx
    )
  SET( ParaViewServer_SRCS
    ${ParaViewServer_SRCS}
    vtkDesktopDeliveryServer.cxx
    vtkIceTRenderManager.cxx
    vtkIceTClientCompositeManager.cxx
    vtkIceTRenderer.cxx
    )
  SET( ParaViewShared_SRCS
    ${ParaViewShared_SRCS}
    vtkDesktopDeliveryClient.cxx
    )
  SET_SOURCE_FILES_PROPERTIES(
    vtkIceTFactory.cxx
    ABSTRACT
    )
ENDIF (PARAVIEW_USE_ICE_T)





SET_SOURCE_FILES_PROPERTIES(
  vtkPVApplicationResources
  vtkPVWindowResources
  WRAP_EXCLUDE
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkPV3DWidget
  vtkPVInformation
  vtkPVJoystickFly
  vtkPVObjectWidget
  vtkPVSourceWidget
  vtkPVWidget
  ABSTRACT
  )

SET(PARAVIEW_ADDITIONAL_LIBRARIES)

#
# Imported project adds libraries
#
MACRO(PARAVIEW_LINK_LIBRARIES LIBS)
  SET(PARAVIEW_ADDITIONAL_LIBRARIES ${PARAVIEW_ADDITIONAL_LIBRARIES} ${LIBS})
ENDMACRO(PARAVIEW_LINK_LIBRARIES LIBS)

#
# Imported project adds source files
# This ones need to be fixed, so that first are not wrapped. This will be fixed
# once the server changes are merged.
#
MACRO(PARAVIEW_INCLUDE_SOURCES SRCS)
  SET(ParaViewServerNonWrapped_SRCS ${ParaViewServerNonWrapped_SRCS} ${SRCS})
ENDMACRO(PARAVIEW_INCLUDE_SOURCES SRCS)
MACRO(PARAVIEW_INCLUDE_EXECUTABLE_SOURCES SRCS)
  SET(ParaViewBinary_SRCS ${ParaViewBinary_SRCS} ${SRCS})
ENDMACRO(PARAVIEW_INCLUDE_EXECUTABLE_SOURCES SRCS)
MACRO(PARAVIEW_INCLUDE_WRAPPED_SOURCES SRCS)
  SET(ParaViewServer_SRCS ${ParaViewServer_SRCS} ${SRCS})
ENDMACRO(PARAVIEW_INCLUDE_WRAPPED_SOURCES SRCS)

OPTION(PARAVIEW_USE_EXODUS "Build using Exodus library. Requires access to Exodus libraries" OFF)
MARK_AS_ADVANCED(PARAVIEW_USE_EXODUS)
IF(PARAVIEW_USE_EXODUS)
  FIND_PATH(EXODUS_SOURCE_DIR 
    ExodusParaViewImport.cmake
    ../Trilab/ExodusReader
    ../../Trilab/ExodusReader
    ../../../Trilab/ExodusReader)

  IF(EXISTS ${EXODUS_SOURCE_DIR}/ExodusParaViewImport.cmake)
    INCLUDE(${EXODUS_SOURCE_DIR}/ExodusParaViewImport.cmake)
  ELSE(EXISTS ${EXODUS_SOURCE_DIR}/ExodusParaViewImport.cmake)
    MESSAGE(SEND_ERROR "Cannot find Exodus sources directory")
  ENDIF(EXISTS ${EXODUS_SOURCE_DIR}/ExodusParaViewImport.cmake)
ENDIF(PARAVIEW_USE_EXODUS)

OPTION(PARAVIEW_USE_SAF "Build using SAF library. Requires access to SAF libraries and HDF5 version 1.6 libraries" OFF)
MARK_AS_ADVANCED(PARAVIEW_USE_SAF)

IF(PARAVIEW_USE_SAF)
  FIND_PATH(SAF_SOURCE_DIR 
    SAFParaViewImport.cmake
    ../Trilab/ExodusReader
    ../../Trilab/ExodusReader
    ../../../Trilab/ExodusReader)

  IF(EXISTS ${SAF_SOURCE_DIR}/SAFParaViewImport.cmake)
    INCLUDE(${SAF_SOURCE_DIR}/SAFParaViewImport.cmake)
  ELSE(EXISTS ${SAF_SOURCE_DIR}/SAFParaViewImport.cmake)
    MESSAGE(SEND_ERROR "Cannot find SAF sources directory")
  ENDIF(EXISTS ${SAF_SOURCE_DIR}/SAFParaViewImport.cmake)

  SET( ParaViewClient_SRCS
    ${ParaViewClient_SRCS}
    ${SAF_SOURCE_DIR}/vtkPVSAFReaderModule.cxx )

ENDIF(PARAVIEW_USE_SAF)

SET(ParaView_SRCS ${ParaView_SRCS} ${ParaViewShared_SRCS}
  ${ParaViewServer_SRCS} ${ParaViewServerNonWrapped_SRCS}
  ${ParaViewClient_SRCS})

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR2(vtkParaViewInstantiator PVInstantiator_SRCS
  ${ParaView_SRCS}
  EXPORT_MACRO VTK_EXPORT
  HEADER_LOCATION ${PARAVIEW_BINARY_DIR}/ParaView)

# Create Tcl wrappers for ParaView classes.
VTK_WRAP_TCL2(vtkKWParaViewTCL ParaViewTCL_SRCS ${PVInstantiator_SRCS}
  ${ParaViewShared_SRCS} ${ParaViewClient_SRCS})
ADD_LIBRARY(vtkKWParaViewTCL ${ParaViewTCL_SRCS} ${PVInstantiator_SRCS}
  ${ParaView_SRCS})

# Create ClientServer wrappers for ParaView classes.
VTK_WRAP_ClientServer(vtkParaViewServerCS ParaViewServerCS_SRCS
  ${ParaViewShared_SRCS} ${ParaViewServer_SRCS}
  ${ParaViewExtraWrapper_SRCS})
ADD_LIBRARY(vtkParaViewServerCS ${ParaViewServerCS_SRCS}
  ${ParaViewServer_SRCS})
TARGET_LINK_LIBRARIES(vtkParaViewServerCS vtkPVFiltersCS vtkKWWidgetsTCL vtkKWParaViewTCL)

IF(PARAVIEW_BUILD_DEVELOPMENT)
  FIND_LIBRARY(PARAVIEW_BLT_LIBRARY BLT
    PATHS /usr/lib /usr/local/lib
    DOC "Optional BLT library. This might be used for a graph widget in the future."
    )
  MARK_AS_ADVANCED(PARAVIEW_BLT_LIBRARY)
ENDIF(PARAVIEW_BUILD_DEVELOPMENT)

TARGET_LINK_LIBRARIES(vtkKWParaViewTCL vtkKWWidgetsTCL vtkClientServer ${PARAVIEW_ADDITIONAL_LIBRARIES})
IF(PARAVIEW_BUILD_DEVELOPMENT)
  IF (PARAVIEW_BLT_LIBRARY)
    TARGET_LINK_LIBRARIES(vtkKWParaViewTCL ${PARAVIEW_BLT_LIBRARY})
  ENDIF (PARAVIEW_BLT_LIBRARY)
ENDIF(PARAVIEW_BUILD_DEVELOPMENT)

TARGET_LINK_LIBRARIES (vtkKWParaViewTCL vtkPVFiltersTCL)
IF(XDMF_FOUND)
  TARGET_LINK_LIBRARIES(vtkKWParaViewTCL vtkXdmfTCL vtkXdmfCS)
ENDIF(XDMF_FOUND)
IF(PARAVIEW_BUILD_DEVELOPMENT)
  TARGET_LINK_LIBRARIES(vtkKWParaViewTCL vtkPVDevelopmentTCL vtkPVDevelopmentCS)
ENDIF(PARAVIEW_BUILD_DEVELOPMENT)
IF(PARAVIEW_USE_ICE_T)
  TARGET_LINK_LIBRARIES(vtkKWParaViewTCL icet icet_mpi icet_strategies)
  INCLUDE_DIRECTORIES(${ICE_T_INCLUDE_DIR})
ENDIF(PARAVIEW_USE_ICE_T)

CONFIGURE_FILE(
  ${PARAVIEW_SOURCE_DIR}/ParaView/vtkPVSourceInterfaceDirectories.h.in
  ${PARAVIEW_BINARY_DIR}/ParaView/vtkPVSourceInterfaceDirectories.h
  ESCAPE_QUOTES)

CONFIGURE_FILE(${PARAVIEW_SOURCE_DIR}/ParaView/vtkPVDemoPaths.h.in
  ${PARAVIEW_BINARY_DIR}/ParaView/vtkPVDemoPaths.h
  ESCAPE_QUOTES)

CONFIGURE_FILE(${PARAVIEW_SOURCE_DIR}/ParaView/vtkPVHelpPaths.h.in
  ${PARAVIEW_BINARY_DIR}/ParaView/vtkPVHelpPaths.h
  ESCAPE_QUOTES)

CONFIGURE_FILE(${PARAVIEW_SOURCE_DIR}/ParaView/vtkPVConfig.h.in
  ${PARAVIEW_BINARY_DIR}/ParaView/vtkPVConfig.h
  ESCAPE_QUOTES)

INCLUDE_DIRECTORIES(${PARAVIEW_BINARY_DIR}/ParaView
  ${PARAVIEW_SOURCE_DIR}/Filters)

IF(PARAVIEW_BUILD_DEVELOPMENT)
  INCLUDE_DIRECTORIES(${PARAVIEW_SOURCE_DIR}/Development)
ENDIF(PARAVIEW_BUILD_DEVELOPMENT)

IF(WIN32 AND TK_RESOURCE_FILE)
  GET_FILENAME_COMPONENT(TK_RESOURCE_PATH ${TK_RESOURCE_FILE} PATH)
  INCLUDE_DIRECTORIES(${TK_RESOURCE_PATH})
  SET(ParaViewBinary_SRCS ${ParaViewBinary_SRCS} Resources/ParaView.rc)
ENDIF(WIN32 AND TK_RESOURCE_FILE)

ADD_EXECUTABLE(paraview WIN32 ${ParaViewBinary_SRCS}) 
CONFIGURE_FILE(${PARAVIEW_SOURCE_DIR}/ParaView/TestClientServer.h.in
  ${PARAVIEW_BINARY_DIR}/ParaView/TestClientServer.h
  ESCAPE_QUOTES)

ADD_EXECUTABLE(TestClientServer TestClientServer.cxx)
TARGET_LINK_LIBRARIES(TestClientServer kwsys)
IF(VTK_USE_CARBON)
  FIND_PROGRAM(VTK_APPLE_RESOURCE Rez /Developer/Tools)
  IF(VTK_APPLE_RESOURCE)
    ADD_CUSTOM_COMMAND(
      SOURCE paraview
      COMMAND ${VTK_APPLE_RESOURCE}
      ARGS Carbon.r -o ${EXECUTABLE_OUTPUT_PATH}/paraview
      TARGET paraview
      )
  ENDIF(VTK_APPLE_RESOURCE)
ENDIF(VTK_USE_CARBON)

TARGET_LINK_LIBRARIES (paraview vtkKWParaViewTCL vtkParaViewServerCS)

IF(UNIX)
  IF(NOT APPLE)
    ADD_EXECUTABLE(ParaView OldParaView.cxx)
  ENDIF(NOT APPLE)
ENDIF(UNIX)


ADD_EXECUTABLE(ProcessXML ProcessXML.cxx)
TARGET_LINK_LIBRARIES(ProcessXML vtkCommon kwsys)

SET(EXE_EXT "")
IF(WIN32)
  SET(EXE_EXT ".exe")
ENDIF(WIN32)

SET(MAKE_SYSTEM)
IF(CMAKE_BUILD_TOOL MATCHES "make")
  SET(MAKE_SYSTEM 1)
ENDIF(CMAKE_BUILD_TOOL MATCHES "make")

SET(CFG_INIT "/${CMAKE_CFG_INTDIR}")
IF(MAKE_SYSTEM OR UNIX)
  SET(CFG_INIT "")
ENDIF(MAKE_SYSTEM OR UNIX)
SET(CMD ${EXECUTABLE_OUTPUT_PATH}${CFG_INIT}/ProcessXML${EXE_EXT})

SET(cmver "${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
IF("${cmver}" GREATER 1.6)
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkPVGeneratedModules.h
    DEPENDS 
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Filters.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Manipulators.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Sources.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Readers.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Writers.xml
    ProcessXML
    COMMAND ${CMD}
    ARGS ${CMAKE_CURRENT_BINARY_DIR}/vtkPVGeneratedModules.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources
    Filters Manipulators Readers Sources Writers)
ELSE("${cmver}" GREATER 1.6)
  ADD_CUSTOM_COMMAND(
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/ProcessXML.cxx
    TARGET vtkKWParaViewTCL
    DEPENDS 
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Filters.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Manipulators.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Sources.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Readers.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Writers.xml
    ${CMD}
    OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/vtkPVGeneratedModules.h
    COMMAND ${CMD}
    ARGS ${CMAKE_CURRENT_BINARY_DIR}/vtkPVGeneratedModules.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources
    Filters Manipulators Readers Sources Writers)
  ADD_DEPENDENCIES(vtkKWParaViewTCL ProcessXML)
ENDIF("${cmver}" GREATER 1.6)

SET_SOURCE_FILES_PROPERTIES(vtkPVInitialize.cxx PROPERTIES OBJECT_DEPENDS 
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPVGeneratedModules.h)

INCLUDE (${PARAVIEW_BINARY_DIR}/ParaView/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${PARAVIEW_SOURCE_DIR}/ParaView/LocalUserOptions.cmake OPTIONAL)

IF(BUILD_SHARED_LIBS)
  INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} vtkKWParaViewTCL vtkParaViewServerCS)
ENDIF(BUILD_SHARED_LIBS)

INSTALL_TARGETS(/bin paraview)

#OUTPUT_REQUIRED_FILES(${PARAVIEW_SOURCE_DIR}/ParaView/ParaView.cxx 
#                      ${PARAVIEW_BINARY_DIR}/ReqList.txt)
