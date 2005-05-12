PROJECT(PVClient)
INCLUDE_REGULAR_EXPRESSION("(vtk.*|pv.*|kwsys.*)")

IF (WIN32)
  ADD_DEFINITIONS(-DPARAVIEW_USE_WIN32_RW)
ELSE (WIN32)
  IF (APPLE)
    IF (VTK_USE_COCOA)
      ADD_DEFINITIONS(-DPARAVIEW_USE_COCOA_RW)
    ENDIF (VTK_USE_COCOA)
    IF (VTK_USE_CARBON)
      ADD_DEFINITIONS(-DPARAVIEW_USE_CARBON_RW)
    ENDIF (VTK_USE_CARBON)
    IF (VTK_USE_X)
      ADD_DEFINITIONS(-DPARAVIEW_USE_X_RW)
    ENDIF (VTK_USE_X)
  ELSE (APPLE)
    ADD_DEFINITIONS(-DPARAVIEW_USE_X_RW)
  ENDIF (APPLE)
ENDIF (WIN32)


SET( ParaViewShared_SRCS
  vtkPVApplication.cxx
  )

SET( ParaViewSharedNotWrapped_SRCS
  )

SET(ParaViewSharedNotTclWrapped_SRCS
  vtkPVProcessModuleGUIHelper.cxx
  vtkPVRenderViewProxyImplementation.cxx
  )

SET( ParaViewExtraWrapper_SRCS
  ${PVGUI_SOURCE_DIR}/Widgets/vtkKWApplication.h
  ${PVGUI_SOURCE_DIR}/Widgets/vtkKWObject.h
  )

SET(ParaViewServer_SRCS ${ParaViewServer_SRCS}
  )

SET( ParaViewClientNotWrapped_SRCS
  vtkPVAnimationBatchHelper.cxx
  vtkPVAxesActor.cxx
  vtkPVAxesWidget.cxx
  vtkInteractorStyleTrackballMultiActor.cxx
  vtkPVInteractorStyle.cxx
  vtkPVInteractorStyleCenterOfRotation.cxx
  vtkPVWidgetCollection.cxx
  vtkPVWorldPointPicker.cxx
  vtkPVXMLPackageParser.cxx
  vtkPVGUIClientOptions.cxx
  ${ExtraParaViewClientNonWrapped_SRCS}
  )

IF (PARAVIEW_USE_LOOKMARKS)
  SET( ParaViewClientNotWrapped_SRCS
    ${ParaViewClientNotWrapped_SRCS}
    vtkXMLLookmarkElement.cxx 
    )
ENDIF (PARAVIEW_USE_LOOKMARKS)

SET( ParaViewClient_SRCS
  vtkKWBoundsDisplay.cxx
  vtkKWView.cxx
  vtkPV3DWidget.cxx
  vtkPVAdvancedReaderModule.cxx
  vtkPVTimeLine.cxx
  vtkPVActiveTrackSelector.cxx
  vtkPVAnimationScene.cxx
  vtkPVAnimationCue.cxx
  vtkPVAnimationCueTree.cxx
  vtkPVKeyFrame.cxx
  vtkPVRampKeyFrame.cxx
  vtkPVBooleanKeyFrame.cxx
  vtkPVExponentialKeyFrame.cxx
  vtkPVSinusoidKeyFrame.cxx
  vtkPVAnimationManager.cxx
  vtkPVComparativeVisManager.cxx
  vtkPVHorizontalAnimationInterface.cxx
  vtkPVVerticalAnimationInterface.cxx
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
  vtkPVCaveRenderModuleUI.cxx
  vtkPVColorMap.cxx
  vtkPVColorSelectionWidget.cxx
  vtkPVCompositeRenderModuleUI.cxx
  vtkPVConnectDialog.cxx
  vtkPVContainerWidget.cxx
  vtkPVContourEntry.cxx
  vtkPVCornerAnnotationEditor.cxx
  vtkPVCutEntry.cxx
  vtkPVIceTDesktopRenderModuleUI.cxx
  vtkPVDisplayGUI.cxx
  vtkPVDReaderModule.cxx
  vtkPVDWriter.cxx
  vtkPVDataSetReaderModule.cxx
  vtkPVDummyWidget.cxx
  vtkPVEnSightReaderModule.cxx
  vtkPVErrorLogDisplay.cxx
  vtkPVExtentEntry.cxx
  vtkPVExtractPartsWidget.cxx
  vtkPVFieldMenu.cxx
  vtkPVFileEntry.cxx
  vtkPVGhostLevelDialog.cxx
  vtkPVGroupInputsWidget.cxx
  vtkPVIceTRenderModuleUI.cxx
  vtkPVImplicitPlaneWidget.cxx
  vtkPVInformationGUI.cxx
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
  vtkPVLODRenderModuleUI.cxx
  vtkPVLabeledToggle.cxx
  vtkPVLineSourceWidget.cxx
  vtkPVLineWidget.cxx
  vtkPVMPIRenderModuleUI.cxx
  vtkPVMinMax.cxx
  vtkPVMultiDisplayRenderModuleUI.cxx
  vtkPVNavigationWindow.cxx
  vtkPVObjectWidget.cxx
  vtkPVOrientScaleWidget.cxx
  vtkPVPLOT3DReaderModule.cxx
  vtkPVPointSourceWidget.cxx
  vtkPVPointWidget.cxx
  vtkPVPick.cxx
  vtkPVProbe.cxx
  vtkPVRawReaderModule.cxx
  vtkPVReaderModule.cxx
  vtkPVRenderModuleUI.cxx
  vtkPVRenderView.cxx
  vtkPVSaveBatchScriptDialog.cxx
  vtkPVScalarRangeLabel.cxx
  vtkPVScale.cxx
  vtkPVScaleFactorEntry.cxx
  vtkPVSelectArrays.cxx
  vtkPVSelectCustomReader.cxx
  vtkPVSelectTimeSet.cxx
  vtkPVSelectWidget.cxx
  vtkPVSelectionList.cxx
  vtkPVServerFileDialog.cxx
  vtkPVSource.cxx
  vtkPVSourceCollection.cxx
  vtkPVSourceList.cxx
  vtkPVSourceNotebook.cxx
  vtkPVSourcesNavigationWindow.cxx
  vtkPVSphereWidget.cxx
  vtkPVStringEntry.cxx
  vtkPVTempTessellatorEntry.cxx
  vtkPVTextPropertyEditor.cxx
  vtkPVThumbWheel.cxx
  vtkPVTimerLogDisplay.cxx
  vtkPVTraceFileDialog.cxx
  vtkPVTraceHelper.cxx
  vtkPVTracedWidget.cxx
  vtkPVTrackballMoveActor.cxx
  vtkPVTrackballPan.cxx
  vtkPVTrackballRoll.cxx
  vtkPVTrackballRotate.cxx
  vtkPVTrackballZoom.cxx
  vtkPVValueList.cxx
  vtkPVVCRControl.cxx
  vtkPVVectorEntry.cxx
  vtkPVVolumeAppearanceEditor.cxx
  vtkPVVolumePropertyWidget.cxx
  vtkPVWidget.cxx
  vtkPVWindow.cxx
  vtkPVWriter.cxx
  vtkPVXDMFParameters.cxx
  vtkXDMFReaderModule.cxx
  vtkPVPluginsDialog.cxx
  #  vtkPVRenderGroupDialog.cxx
  ${ExtraParaViewClient_SRCS}
  )

IF (PARAVIEW_USE_LOOKMARKS)
  SET( ParaViewClient_SRCS
    ${ParaViewClient_SRCS}
    vtkKWLookmark.cxx
    vtkKWLookmarkFolder.cxx
    vtkPVLookmark.cxx
    vtkPVLookmarkManager.cxx
    )
ENDIF (PARAVIEW_USE_LOOKMARKS)


INCLUDE_DIRECTORIES(
  ${KWSYS_INCLUDE_DIR}
  ${PVSERVERCOMMON_INCLUDE_DIR}
  ${PVFILTERS_INCLUDE_DIR}
  ${PVGUI_BINARY_DIR}/Client
  ${PVGUI_SOURCE_DIR}/Client
  ${PVSERVERMANAGER_INCLUDE_DIR}
  ${KWCommon_INCLUDE_PATH}
  ${KWWidgets_INCLUDE_PATH}
  ${VTKCLIENTSERVER_INCLUDE_DIR}
  ${VTK_INCLUDE_DIR}
  ${XDMF_INCLUDE_DIRS}
  )
IF(VTK_USE_MPI)
  INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
ENDIF(VTK_USE_MPI)

IF(BUILD_TESTING)
  SET( ParaViewClient_SRCS ${ParaViewClient_SRCS}
    vtkKWTesting.cxx
    )
ENDIF(BUILD_TESTING)



SET(ParaViewBinary_SRCS ParaView.cxx)

SET(PVClient_SRCS
  pvclient.cxx)

IF (PARAVIEW_USE_ICE_T)
  SET( ParaViewClient_SRCS ${ParaViewClient_SRCS})
ENDIF (PARAVIEW_USE_ICE_T)

SET_SOURCE_FILES_PROPERTIES(
  vtkPVApplicationResources
  WRAP_EXCLUDE
  )

SET_SOURCE_FILES_PROPERTIES(
  vtkKWView.cxx
  vtkPV3DWidget
  vtkPVJoystickFly
  vtkPVObjectWidget
  vtkPVValueList
  vtkPVWidget
  vtkPVKeyFrame
  ABSTRACT
  )



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

SET(KIT_WRAP_DEPS PVFilters PVCommon)
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
TARGET_LINK_LIBRARIES(vtkKWParaView 
  ${KWCommon_LIBRARIES} 
  ${KWWidgets_LIBRARIES} 
  vtkPVServerManagerTCL 
  vtkPVServerCommonTCL 
  vtkMPEG2Encode)

CONFIGURE_FILE(
  ${PVGUI_SOURCE_DIR}/Client/vtkPVSourceInterfaceDirectories.h.in
  ${PVGUI_BINARY_DIR}/Client/vtkPVSourceInterfaceDirectories.h
  ESCAPE_QUOTES)

CONFIGURE_FILE(${PVGUI_SOURCE_DIR}/Client/vtkPVHelpPaths.h.in
  ${PVGUI_BINARY_DIR}/Client/vtkPVHelpPaths.h
  ESCAPE_QUOTES)

IF(WIN32 AND VTK_TK_RESOURCE_FILE)
  GET_FILENAME_COMPONENT(TK_RESOURCE_PATH ${VTK_TK_RESOURCE_FILE} PATH)
  INCLUDE_DIRECTORIES(${TK_RESOURCE_PATH})
  SET(ParaViewBinary_SRCS ${ParaViewBinary_SRCS} Resources/ParaView.rc)
  SET(PVClient_SRCS ${PVClient_SRCS} Resources/ParaView.rc)
ENDIF(WIN32 AND VTK_TK_RESOURCE_FILE)

SET(PV_EXE_BUILD_LIST paraview pvclient pvTestDriver)
SET(PV_EXE_INSTALL_LIST paraview pvclient)

ADD_EXECUTABLE(paraview${PV_EXE_SUFFIX} WIN32 ${ParaViewBinary_SRCS}) 
ADD_EXECUTABLE(pvclient${PV_EXE_SUFFIX} WIN32 ${PVClient_SRCS})

INSTALL_TARGETS(${PV_EXE_INSTALL} paraview${PV_EXE_SUFFIX} pvclient${PV_EXE_SUFFIX})

SET(PV_TEST_CLEAN_COMMAND "" CACHE STRING
  "Command to run after a failed test to cleanup processes.  Example: \"killall -9 rsh paraview\"")
MARK_AS_ADVANCED(PV_TEST_CLEAN_COMMAND)

CONFIGURE_FILE(${PVGUI_SOURCE_DIR}/Client/pvTestDriverConfig.h.in
  ${PVGUI_BINARY_DIR}/Client/pvTestDriverConfig.h @ONLY IMMEDIATE
  ESCAPE_QUOTES)

ADD_EXECUTABLE(pvTestDriver${PV_EXE_SUFFIX} pvTestDriver.cxx)
ADD_DEPENDENCIES(pvTestDriver${PV_EXE_SUFFIX} paraview${PV_EXE_SUFFIX})
TARGET_LINK_LIBRARIES(pvTestDriver${PV_EXE_SUFFIX} kwsys)

IF(VTK_USE_CARBON)
  FIND_PROGRAM(VTK_APPLE_RESOURCE Rez /Developer/Tools)
  IF(VTK_APPLE_RESOURCE)
    ADD_CUSTOM_COMMAND(
      TARGET paraview${PV_EXE_SUFFIX}
      POST_BUILD
      COMMAND ${VTK_APPLE_RESOURCE}
      ARGS Carbon.r -o ${EXECUTABLE_OUTPUT_PATH}/paraview${PV_EXE_SUFFIX}
      )
  ENDIF(VTK_APPLE_RESOURCE)
ENDIF(VTK_USE_CARBON)

TARGET_LINK_LIBRARIES (pvclient${PV_EXE_SUFFIX} 
  vtkKWParaViewCS
  vtkClientServer 
  ${PARAVIEW_ADDITIONAL_LIBRARIES})
TARGET_LINK_LIBRARIES (paraview${PV_EXE_SUFFIX} 
  vtkKWParaViewCS
  vtkClientServer 
  ${PARAVIEW_ADDITIONAL_LIBRARIES})

TARGET_LINK_LIBRARIES(vtkKWParaView vtkPVFilters)
TARGET_LINK_LIBRARIES(vtkKWParaView vtkXdmfCS)

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
SET(CMD ${EXECUTABLE_OUTPUT_PATH}${CFG_INIT}/kwProcessXML${EXE_EXT})

ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkPVGeneratedModules.h
  DEPENDS 
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Filters.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Manipulators.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Sources.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Readers.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Writers.xml
  kwProcessXML
  COMMAND ${CMD}
  ARGS ${CMAKE_CURRENT_BINARY_DIR}/vtkPVGeneratedModules.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources
  vtkPVDefaultModules Interface GetInterfaces
  Filters Manipulators Readers Sources Writers)

SET_SOURCE_FILES_PROPERTIES(vtkPVInitialize.cxx PROPERTIES OBJECT_DEPENDS 
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPVGeneratedModules.h)

INCLUDE (${PVGUI_BINARY_DIR}/Client/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${PVGUI_SOURCE_DIR}/Client/LocalUserOptions.cmake OPTIONAL)

IF(BUILD_SHARED_LIBS)
  INSTALL_TARGETS(${PV_INSTALL_LIB_DIR} vtkKWParaViewCS
    vtkKWParaView)
ENDIF(BUILD_SHARED_LIBS)

# Add shared link forwarding executables if necessary.
IF(PV_NEED_SHARED_FORWARD)
  FOREACH(pvexe ${PV_EXE_BUILD_LIST})
    SET(PV_FORWARD_EXE ${pvexe}${PV_EXE_SUFFIX})
    CONFIGURE_FILE(
      ${ParaView_SOURCE_DIR}/Servers/Executables/pv-forward.c.in
      ${CMAKE_CURRENT_BINARY_DIR}/${pvexe}-forward.c
      @ONLY IMMEDIATE)
    ADD_EXECUTABLE(${pvexe} ${CMAKE_CURRENT_BINARY_DIR}/${pvexe}-forward.c)
    ADD_DEPENDENCIES(${pvexe} ${pvexe}${PV_EXE_SUFFIX})
  ENDFOREACH(pvexe)
  FOREACH(pvexe ${PV_EXE_INSTALL_LIST})
    INSTALL_TARGETS(${PV_INSTALL_BIN_DIR} ${pvexe})
  ENDFOREACH(pvexe)
ENDIF(PV_NEED_SHARED_FORWARD)
