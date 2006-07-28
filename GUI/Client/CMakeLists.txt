PROJECT(PVClient)
INCLUDE_REGULAR_EXPRESSION("(vtk.*|pv.*|vtksys.*|[pP]a.*)")

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
  vtkPVCameraManipulator.cxx
  vtkPVProcessModuleGUIHelper.cxx
  vtkPVRenderViewProxyImplementation.cxx
  vtkPVJoystickFly.cxx
  vtkPVJoystickFlyIn.cxx
  vtkPVJoystickFlyOut.cxx
  vtkPVTrackballMoveActor.cxx
  vtkPVTrackballPan.cxx
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
  vtkInteractorStyleTrackballMultiActor.cxx
  vtkPVInteractorStyleCenterOfRotation.cxx
  vtkPVWidgetCollection.cxx
  vtkPVWorldPointPicker.cxx
  vtkPVXMLPackageParser.cxx
  vtkPVGUIClientOptions.cxx
  vtkXMLLookmarkElement.cxx 
  ${ExtraParaViewClientNonWrapped_SRCS}
  )

SET( ParaViewClient_SRCS
  vtkKWBoundsDisplay.cxx
  vtkKWLookmark.cxx
  vtkKWLookmarkFolder.cxx
  vtkKWView.cxx
  vtkPVNew3DWidget.cxx
  vtkPVNewImplicitPlane.cxx
  vtkPV3DWidget.cxx
  vtkPVAdvancedReaderModule.cxx
  vtkPVActiveTrackSelector.cxx
  vtkPVAnimationScene.cxx
  vtkPVAnimationCue.cxx
  vtkPVAnimationCueTree.cxx
  vtkPVAnimationManager.cxx
  vtkPVApplicationResources.cxx
  vtkPVApplicationSettingsInterface.cxx
  vtkPVArrayMenu.cxx
  vtkPVArraySelection.cxx
  vtkPVAttributeEditor.cxx
  vtkPVBooleanKeyFrame.cxx
  vtkPVBoundsDisplay.cxx
  vtkPVBoxWidget.cxx
  vtkPVCalculatorWidget.cxx
  vtkPVCameraAnimationCue.cxx
  vtkPVCameraControl.cxx
  vtkPVCameraIcon.cxx
  vtkPVCameraKeyFrame.cxx
  vtkPVCaveRenderModuleUI.cxx
  vtkPVColorMap.cxx
  vtkPVColorMapUI.cxx
  vtkPVColorSelectionWidget.cxx
  vtkPVComparativeVisDialog.cxx
  vtkPVComparativeVisManager.cxx
  vtkPVComparativeVisManagerGUI.cxx
  vtkPVComparativeVisProgressDialog.cxx
  vtkPVComparativeVisPropertyWidget.cxx
  vtkPVCompositeRenderModuleUI.cxx
  vtkPVConnectDialog.cxx
  vtkPVContainerWidget.cxx
  vtkPVContourEntry.cxx
  vtkPVCornerAnnotationEditor.cxx
  vtkPVCredits.cxx
  vtkPVCutEntry.cxx
  vtkPVIceTDesktopRenderModuleUI.cxx
  vtkPVDisplayGUI.cxx
  vtkPVDReaderModule.cxx
  vtkPVDWriter.cxx
  vtkPVDataAnalysis.cxx
  vtkPVDataSetReaderModule.cxx
  vtkPVBasicDSPFilterWidget.cxx
  vtkPVDummyWidget.cxx
  vtkPVEnSightReaderModule.cxx
  vtkPVErrorLogDisplay.cxx
  vtkPVEWriter.cxx
  vtkPVExponentialKeyFrame.cxx
  vtkPVExtentEntry.cxx
  vtkPVExtractDataSetsWidget.cxx
  vtkPVExtractPartsWidget.cxx
  vtkPVFieldMenu.cxx
  vtkPVFileEntry.cxx
  vtkPVGhostLevelDialog.cxx
  vtkPVGroupInputsWidget.cxx
  vtkPVHorizontalAnimationInterface.cxx
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
  vtkPVItemSelection.cxx
  vtkPVKeyFrame.cxx
  vtkPVLODRenderModuleUI.cxx
  vtkPVLabeledToggle.cxx
  vtkPVLineSourceWidget.cxx
  vtkPVLineWidget.cxx
  vtkPVListBoxToListBoxSelectionEditor.cxx
  vtkPVLookmark.cxx
  vtkPVLookmarkManager.cxx
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
  vtkPVPickBoxWidget.cxx
  vtkPVPickSphereWidget.cxx
  vtkPVPlotArraySelection.cxx
  vtkPVPlotDisplayLabelPropertiesDialog.cxx
  vtkPVProbe.cxx
  vtkPVPropertyKeyFrame.cxx
  vtkPVProxyKeyFrame.cxx
  vtkPVRampKeyFrame.cxx
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
  vtkPVSimpleAnimationCue.cxx
  vtkPVSinusoidKeyFrame.cxx
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
  vtkPVTimeLine.cxx
  vtkPVTimerLogDisplay.cxx
  vtkPVTraceFileDialog.cxx
  vtkPVTraceHelper.cxx
  vtkPVTracedWidget.cxx
  vtkPVTrackEditor.cxx
  vtkPVValueList.cxx
  vtkPVVCRControl.cxx
  vtkPVVectorEntry.cxx
  vtkPVVerticalAnimationInterface.cxx
  vtkPVVolumeAppearanceEditor.cxx
  vtkPVVolumePropertyWidget.cxx
  vtkPVWidget.cxx
  vtkPVWindow.cxx
  vtkPVWriter.cxx
  vtkPVXDMFParameters.cxx
  vtkXDMFReaderModule.cxx
  #  vtkPVRenderGroupDialog.cxx
  ${ExtraParaViewClient_SRCS}
  )

INCLUDE_DIRECTORIES(
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
  ${ExtraParaViewGUIIncludes}
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
  vtkPVKeyFrame
  vtkPVObjectWidget
  vtkPVPropertyKeyFrame
  vtkPVProxyKeyFrame.cxx
  vtkPVValueList
  vtkPVWidget
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
INCLUDE(${VTK_CMAKE_DIR}/vtkExportKit.cmake)
VTK_EXPORT_KIT("KWParaView" "PVSERVER" "${ParaViewServer_SRCS};${ParaViewShared_SRCS};${ParaView_SOURCE_DIR}/GUI/Widgets/vtkKWApplication.cxx;${ParaView_SOURCE_DIR}/GUI/Widgets/vtkKWObject.cxx;${ParaViewSharedNotTclWrapped_SRCS}")

SET(KIT_WRAP_DEPS PVFilters PVServerCommon)
SET(VTK_KITS_DIR "${ParaView_BINARY_DIR}/GUI/Client/Utilities")
PV_WRAP_VTK_CS(KWParaView PVSERVER "${KIT_WRAP_DEPS}")

# Wrap Tcl code
INCLUDE("${VTK_CMAKE_DIR}/vtkWrapTcl.cmake")
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
  vtkPVServerCommonTCL)

CONFIGURE_FILE(
  ${PVGUI_SOURCE_DIR}/Client/vtkPVSourceInterfaceDirectories.h.in
  ${PVGUI_BINARY_DIR}/Client/vtkPVSourceInterfaceDirectories.h
  ESCAPE_QUOTES)

CONFIGURE_FILE(${PVGUI_SOURCE_DIR}/Client/vtkPVHelpPaths.h.in
  ${PVGUI_BINARY_DIR}/Client/vtkPVHelpPaths.h
  ESCAPE_QUOTES)

IF (WIN32)
  IF (NOT BORLAND)
    IF(NOT CYGWIN)
      INCLUDE_DIRECTORIES(${VTK_TK_RESOURCES_DIR})
      SET(ParaViewBinary_SRCS ${ParaViewBinary_SRCS} Resources/ParaView.rc)
      SET(PVClient_SRCS ${PVClient_SRCS} Resources/ParaView.rc)
    ENDIF(NOT CYGWIN)
  ENDIF (NOT BORLAND)
ENDIF (WIN32)

SET(PV_EXE_BUILD_LIST paraview pvclient pvTestDriver)
SET(PV_EXE_INSTALL_LIST paraview pvclient)

ADD_EXECUTABLE(paraview${PV_EXE_SUFFIX} WIN32 ${ParaViewBinary_SRCS}) 
ADD_EXECUTABLE(pvclient${PV_EXE_SUFFIX} WIN32 ${PVClient_SRCS})

IF(NOT PV_INSTALL_NO_RUNTIME)
  INSTALL_TARGETS(${PV_EXE_INSTALL} paraview${PV_EXE_SUFFIX} pvclient${PV_EXE_SUFFIX})
ENDIF(NOT PV_INSTALL_NO_RUNTIME)

SET(PV_TEST_CLEAN_COMMAND "" CACHE STRING
  "Command to run after a failed test to cleanup processes.  Example: \"killall -9 rsh paraview\"")
MARK_AS_ADVANCED(PV_TEST_CLEAN_COMMAND)

CONFIGURE_FILE(${PVGUI_SOURCE_DIR}/Client/pvTestDriverConfig.h.in
  ${PVGUI_BINARY_DIR}/Client/pvTestDriverConfig.h @ONLY IMMEDIATE
  ESCAPE_QUOTES)

ADD_EXECUTABLE(pvTestDriver${PV_EXE_SUFFIX} pvTestDriver.cxx)
ADD_DEPENDENCIES(pvTestDriver${PV_EXE_SUFFIX} paraview${PV_EXE_SUFFIX})
TARGET_LINK_LIBRARIES(pvTestDriver${PV_EXE_SUFFIX} vtksys)

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

SET(resourceFiles
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Filters.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Manipulators.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Sources.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Readers.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Writers.xml
  ${PARAVIEW_EXTRA_GUI_RESOURCES}
  )
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkPVGeneratedModules.h
  DEPENDS ${resourceFiles}
  kwProcessXML
  COMMAND ${CMD}
  ARGS ${CMAKE_CURRENT_BINARY_DIR}/vtkPVGeneratedModules.h
  vtkPVDefaultModules Interface GetInterfaces
  ${resourceFiles})

SET(PARAVIEW_INCLUDE_MODULES_TO_PVINITIALIZE "")
FOREACH(rf ${resourceFiles})
  STRING(REGEX REPLACE "^.*/(.*).xml$" "\\1" moduleName "${rf}")
  SET(oneModule "  init_string = vtkPVDefaultModules${moduleName}GetInterfaces();\n")
  SET(oneModule "${oneModule}  win->ReadSourceInterfacesFromString(init_string);\n")
  SET(oneModule "${oneModule}  delete[] init_string;\n")
  SET(PARAVIEW_INCLUDE_MODULES_TO_PVINITIALIZE
    "${PARAVIEW_INCLUDE_MODULES_TO_PVINITIALIZE}\n${oneModule}")
ENDFOREACH(rf)
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkParaViewIncludeModulesToPVInitialize.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkParaViewIncludeModulesToPVInitialize.h"
  @ONLY IMMEDIATE)
SET_SOURCE_FILES_PROPERTIES(vtkPVInitialize.cxx PROPERTIES OBJECT_DEPENDS 
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPVGeneratedModules.h)

INCLUDE (${PVGUI_BINARY_DIR}/Client/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${PVGUI_SOURCE_DIR}/Client/LocalUserOptions.cmake OPTIONAL)

IF(NOT PV_INSTALL_NO_LIBRARIES)
  INSTALL_TARGETS(${PV_INSTALL_LIB_DIR} vtkKWParaViewCS
    vtkKWParaView)
ENDIF(NOT PV_INSTALL_NO_LIBRARIES)

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
  IF(NOT PV_INSTALL_NO_RUNTIME)
    FOREACH(pvexe ${PV_EXE_INSTALL_LIST})
      INSTALL_TARGETS(${PV_INSTALL_BIN_DIR} ${pvexe})
    ENDFOREACH(pvexe)
  ENDIF(NOT PV_INSTALL_NO_RUNTIME)
ENDIF(PV_NEED_SHARED_FORWARD)
