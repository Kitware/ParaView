PROJECT(PVClient)

SET( ParaViewShared_SRCS
  vtkPVApplication.cxx
  )

SET( ParaViewSharedNotWrapped_SRCS
  vtkPVProgressHandler.cxx
  )

SET(ParaViewSharedNotTclWrapped_SRCS
  vtkPVProcessModule.cxx
  vtkPVGeometryFilter.cxx
  vtkPVPolyDataMapper.cxx
  vtkPVLODActor.cxx
  vtkPVTimerInformation.cxx
  vtkMPIMToNSocketConnection.cxx
  vtkPVExtentTranslator.cxx
  vtkPVClientServerModule.cxx
  vtkPVGenericRenderWindowInteractor.cxx
  vtkPVMPIProcessModule.cxx
  )

SET( ParaViewServer_SRCS
  vtkMultiDisplayManager.cxx
  vtkPVServerArraySelection.cxx
  vtkPVServerFileListing.cxx
  vtkPVServerObject.cxx
  vtkPVServerProbe.cxx
  vtkPVServerSelectTimeSet.cxx
  vtkPVSummaryHelper.cxx
  vtkM2NCollect.cxx
  vtkM2NDuplicate.cxx
  vtkPVServerXDMFParameters.cxx
  )

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
  ${PVGUI_SOURCE_DIR}/Widgets/vtkKWApplication.h
  ${PVGUI_SOURCE_DIR}/Widgets/vtkKWObject.h
  )

  SET(ParaViewServer_SRCS ${ParaViewServer_SRCS}
    )

SET( ParaViewClientNotWrapped_SRCS
  vtkPVAxesActor.cxx
  vtkPVAxesWidget.cxx
  vtkPVInteractorStyle.cxx
  vtkPVInteractorStyleCenterOfRotation.cxx
  vtkPVSourceCollection.cxx
  vtkPVBatchCompositeManager.cxx
  vtkPVCompositeRenderModule.cxx
  vtkPVMultiDisplayRenderModule.cxx
  vtkPVLODRenderModule.cxx
  vtkPVMPIRenderModule.cxx
  vtkPVRenderModule.cxx
  vtkPVWidgetCollection.cxx
  vtkPVWorldPointPicker.cxx
  vtkMPIMToNSocketConnectionPortInformation.cxx
  vtkPVXMLPackageParser.cxx
  )

SET( ParaViewClient_SRCS
  vtkKWBoundsDisplay.cxx
  vtkPV3DWidget.cxx
  vtkPVAdvancedReaderModule.cxx
  vtkPVAnimationInterface.cxx
  vtkPVAnimationInterfaceEntry.cxx
  vtkPVApplicationResources.cxx
  vtkPVApplicationSettingsInterface.cxx
  vtkPVArrayMenu.cxx
  vtkPVArraySelection.cxx
  vtkPVBoundsDisplay.cxx
  vtkPVBoxWidget.cxx
  vtkPVCalculatorWidget.cxx
  vtkPVCameraControl.cxx
  vtkPVCameraIcon.cxx
  vtkPVCameraManipulator.cxx
  vtkPVColorMap.cxx
  vtkPVCompositePartDisplay.cxx
  vtkPVCompositeRenderModuleUI.cxx
  vtkPVConnectDialog.cxx
  vtkPVContainerWidget.cxx
  vtkPVContourEntry.cxx
  vtkPVContourWidgetProperty.cxx
  vtkPVCutEntry.cxx
  vtkPVDReaderModule.cxx
  vtkPVDWriter.cxx
  vtkPVData.cxx
  vtkPVDataSetFileEntry.cxx
  vtkPVDataSetReaderModule.cxx
  vtkPVDisplay.cxx
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
  vtkPVInitialize.cxx
  vtkPVInputArrayRequirement.cxx
  vtkPVInputFixedTypeRequirement.cxx
  vtkPVInputGroupRequirement.cxx
  vtkPVInputMenu.cxx
  vtkPVInputProperty.cxx
  vtkPVInputRequirement.cxx
  vtkPVInteractorStyleControl.cxx
  vtkPVJoystickFly.cxx
  vtkPVJoystickFlyIn.cxx
  vtkPVJoystickFlyOut.cxx
  vtkPVLODPartDisplay.cxx
  vtkPVLODRenderModuleUI.cxx
  vtkPVLabel.cxx
  vtkPVLabeledToggle.cxx
  vtkPVLineSourceWidget.cxx
  vtkPVLineWidget.cxx
  vtkPVMPIRenderModuleUI.cxx
  vtkPVMinMax.cxx
  vtkPVMultiDisplayPartDisplay.cxx
  vtkPVMultiDisplayRenderModuleUI.cxx
  vtkPVNavigationWindow.cxx
  vtkPVObjectWidget.cxx
  vtkPVOrientScaleWidget.cxx
  vtkPVPLOT3DReaderModule.cxx
  vtkPVPart.cxx
  vtkPVPartDisplay.cxx
  vtkPVPickDisplay.cxx
  vtkPVPlotDisplay.cxx
  vtkPVPointSourceWidget.cxx
  vtkPVPointWidget.cxx
  vtkPVPick.cxx
  vtkPVProbe.cxx
  vtkPVRawReaderModule.cxx
  vtkPVReaderModule.cxx
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
  vtkPVSourceList.cxx
  vtkPVSourceWidget.cxx
  vtkPVSourcesNavigationWindow.cxx
  vtkPVSphereWidget.cxx
  vtkPVStringAndScalarListWidgetProperty.cxx
  vtkPVStringEntry.cxx
  vtkPVStringWidgetProperty.cxx
  vtkPVThumbWheel.cxx
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
  vtkPVWidgetProperty.cxx
  vtkPVWindow.cxx
  vtkPVWindowResources.cxx
  vtkPVWriter.cxx
  vtkPVXDMFParameters.cxx
  vtkXDMFReaderModule.cxx
  #  vtkPVRenderGroupDialog.cxx
  )

INCLUDE_DIRECTORIES(
  ${PVFILTERS_INCLUDE_DIR}
  ${PVGUI_BINARY_DIR}/Client
  ${PVGUI_SOURCE_DIR}/Client
  ${PVWIDGETS_INCLUDE_DIR}
  ${VTKCLIENTSERVER_INCLUDE_DIR}
  ${VTK_INCLUDE_DIR}
  ${KWSYS_INCLUDE_DIR}
  ${XDMF_INCLUDE_DIRS}
  )
IF(VTK_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF(VTK_USE_MPI)



SET(ParaViewBinary_SRCS
  ParaView.cxx)

IF (PARAVIEW_USE_ICE_T)
  SET( ParaViewClientNotWrapped_SRCS
    ${ParaViewClientNotWrapped_SRCS}
    vtkPVDeskTopRenderModule.cxx
    vtkPVIceTDisplayRenderModule.cxx
    vtkPVIceTRenderModule.cxx
    )
  SET( ParaViewClient_SRCS
    ${ParaViewClient_SRCS}
    vtkPVDeskTopRenderModuleUI.cxx
    vtkPVIceTDisplayRenderModuleUI.cxx
    vtkPVIceTRenderModuleUI.cxx
    )

ENDIF (PARAVIEW_USE_ICE_T)

SET_SOURCE_FILES_PROPERTIES(
  vtkPVApplicationResources
  vtkPVWindowResources
  WRAP_EXCLUDE
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkPV3DWidget
  vtkPVDisplay
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
  ${ParaViewClient_SRCS} ${ParaViewClientNotWrapped_SRCS} ${ParaViewSharedNotTclWrapped_SRCS})

# Setup vtkInstantiator registration for this library's classes.
INCLUDE(${ParaView_SOURCE_DIR}/VTK/CMake/vtkMakeInstantiator.cmake)
VTK_MAKE_INSTANTIATOR3(vtkParaViewInstantiator PVInstantiator_SRCS
  "${ParaView_SRCS}"
  VTK_EXPORT
  "${PVGUI_BINARY_DIR}/Client" "")

# load the ClientServer command
INCLUDE(${ParaView_SOURCE_DIR}/Utilities/VTKClientServer/vtkClientServer.cmake)
CS_INITIALIZE_WRAP()

# Wrap PVServer
# Create ClientServer wrappers for ParaView classes.
SET(VTK_BINARY_DIR "${PVClient_BINARY_DIR}")
INCLUDE(${VTK_SOURCE_DIR}/CMake/vtkExportKit.cmake)
VTK_EXPORT_KIT("KWParaView" "PVSERVER" "${ParaViewServer_SRCS};${ParaViewShared_SRCS};${ParaView_SOURCE_DIR}/GUI/Widgets/vtkKWApplication.cxx;${ParaView_SOURCE_DIR}/GUI/Widgets/vtkKWObject.cxx;${ParaViewSharedNotTclWrapped_SRCS}")

SET(KIT_WRAP_DEPS PVFilters)
SET(VTK_KITS_DIR "${ParaView_BINARY_DIR}/GUI/Client/Utilities")
PV_WRAP_VTK_CS(KWParaView PVSERVER "${KIT_WRAP_DEPS}")

# Wrap Tcl code
SET(VTK_WRAP_TCL3_INIT_DIR "${VTKTclWrapping_SOURCE_DIR}")
INCLUDE("${VTKTclWrapping_SOURCE_DIR}/vtkWrapTcl.cmake")
VTK_WRAP_TCL3(vtkKWParaView ParaViewTCL_SRCS "${ParaViewShared_SRCS};${ParaViewClient_SRCS}" "")
ADD_LIBRARY(vtkKWParaView ${ParaViewTCL_SRCS} ${PVInstantiator_SRCS}
  ${ParaViewServer_SRCS} 
  ${ParaViewServerNonWrapped_SRCS}
  ${ParaViewSharedNotWrapped_SRCS}
  ${ParaView_SRCS})
TARGET_LINK_LIBRARIES(vtkKWParaView vtkKWWidgetsTCL vtkPVServerManagerTCL)




IF(PARAVIEW_BUILD_DEVELOPMENT)
  FIND_LIBRARY(PARAVIEW_BLT_LIBRARY BLT
    PATHS /usr/lib /usr/local/lib
    DOC "Optional BLT library. This might be used for a graph widget in the future."
    )
  MARK_AS_ADVANCED(PARAVIEW_BLT_LIBRARY)
ENDIF(PARAVIEW_BUILD_DEVELOPMENT)

CONFIGURE_FILE(
  ${PVGUI_SOURCE_DIR}/Client/vtkPVSourceInterfaceDirectories.h.in
  ${PVGUI_BINARY_DIR}/Client/vtkPVSourceInterfaceDirectories.h
  ESCAPE_QUOTES)

CONFIGURE_FILE(${PVGUI_SOURCE_DIR}/Client/vtkPVDemoPaths.h.in
  ${PVGUI_BINARY_DIR}/Client/vtkPVDemoPaths.h
  ESCAPE_QUOTES)

CONFIGURE_FILE(${PVGUI_SOURCE_DIR}/Client/vtkPVHelpPaths.h.in
  ${PVGUI_BINARY_DIR}/Client/vtkPVHelpPaths.h
  ESCAPE_QUOTES)

IF(WIN32 AND TK_RESOURCE_FILE)
  GET_FILENAME_COMPONENT(TK_RESOURCE_PATH ${TK_RESOURCE_FILE} PATH)
  INCLUDE_DIRECTORIES(${TK_RESOURCE_PATH})
  SET(ParaViewBinary_SRCS ${ParaViewBinary_SRCS} Resources/ParaView.rc)
ENDIF(WIN32 AND TK_RESOURCE_FILE)

ADD_EXECUTABLE(paraview WIN32 ${ParaViewBinary_SRCS}) 
CONFIGURE_FILE(${PVGUI_SOURCE_DIR}/Client/TestClientServer.h.in
  ${PVGUI_BINARY_DIR}/Client/TestClientServer.h
  ESCAPE_QUOTES)

ADD_EXECUTABLE(TestClientServer TestClientServer.cxx)
TARGET_LINK_LIBRARIES(TestClientServer kwsys)

IF(VTK_USE_CARBON)
  FIND_PROGRAM(VTK_APPLE_RESOURCE Rez /Developer/Tools)
  IF(VTK_APPLE_RESOURCE)
    ADD_CUSTOM_COMMAND(
      TARGET paraview
      POST_BUILD
      COMMAND ${VTK_APPLE_RESOURCE}
      ARGS Carbon.r -o ${EXECUTABLE_OUTPUT_PATH}/paraview
      )
  ENDIF(VTK_APPLE_RESOURCE)
ENDIF(VTK_USE_CARBON)

TARGET_LINK_LIBRARIES (paraview 
  vtkKWParaViewCS
  vtkClientServer 
  ${PARAVIEW_ADDITIONAL_LIBRARIES})
IF(PARAVIEW_BUILD_DEVELOPMENT)
  IF (PARAVIEW_BLT_LIBRARY)
    TARGET_LINK_LIBRARIES(vtkKWParaView ${PARAVIEW_BLT_LIBRARY})
  ENDIF (PARAVIEW_BLT_LIBRARY)
ENDIF(PARAVIEW_BUILD_DEVELOPMENT)

TARGET_LINK_LIBRARIES(vtkKWParaView vtkPVFilters)
TARGET_LINK_LIBRARIES(vtkKWParaView vtkXdmfCS)

IF(PARAVIEW_BUILD_DEVELOPMENT)
  TARGET_LINK_LIBRARIES(vtkKWParaView vtkPVDevelopment)
ENDIF(PARAVIEW_BUILD_DEVELOPMENT)


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

SET_SOURCE_FILES_PROPERTIES(vtkPVInitialize.cxx PROPERTIES OBJECT_DEPENDS 
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPVGeneratedModules.h)

INCLUDE (${PVGUI_BINARY_DIR}/Client/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${PVGUI_SOURCE_DIR}/Client/LocalUserOptions.cmake OPTIONAL)
