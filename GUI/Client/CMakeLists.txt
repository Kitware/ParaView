PROJECT(PVClient)
INCLUDE_REGULAR_EXPRESSION("(vtk.*|pv.*)")
SET( ParaViewShared_SRCS
  vtkPVApplication.cxx
  )

SET( ParaViewSharedNotWrapped_SRCS
  )

SET(ParaViewSharedNotTclWrapped_SRCS
  vtkPickPointWidget.cxx
  vtkPVProcessModuleGUIHelper.cxx
  vtkPVLODActor.cxx
  vtkPVTimerInformation.cxx
  vtkPVExtentTranslator.cxx
  vtkPVGenericRenderWindowInteractor.cxx
  )

SET( ParaViewServer_SRCS
  vtkCaveRenderManager.cxx
  vtkMultiDisplayManager.cxx
  vtkPVServerArraySelection.cxx
  vtkPVServerFileListing.cxx
  vtkPVServerObject.cxx
  vtkPVServerProbe.cxx
  vtkPVServerSelectTimeSet.cxx
  vtkPVSummaryHelper.cxx
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
  vtkPVCaveRenderModule.cxx
  vtkPVCompositeRenderModule.cxx
  vtkPVMultiDisplayRenderModule.cxx
  vtkPVLODRenderModule.cxx
  vtkPVMPIRenderModule.cxx
  vtkPVRenderModule.cxx
  vtkPVWidgetCollection.cxx
  vtkPVWorldPointPicker.cxx
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
  vtkPVCaveRenderModuleUI.cxx
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
  vtkPVSourceCollection.cxx
  vtkPVSourceList.cxx
  vtkPVSourceWidget.cxx
  vtkPVSourcesNavigationWindow.cxx
  vtkPVSphereWidget.cxx
  vtkPVStringAndScalarListWidgetProperty.cxx
  vtkPVStringEntry.cxx
  vtkPVStringWidgetProperty.cxx
	vtkPVTempTessellatorEntry.cxx
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
  vtkPVVolumeAppearanceEditor.cxx
  vtkPVWidget.cxx
  vtkPVWidgetProperty.cxx
  vtkPVWindow.cxx
  vtkPVWindowResources.cxx
  vtkPVWriter.cxx
  vtkPVXDMFParameters.cxx
  vtkXDMFReaderModule.cxx
  vtkPVPluginsDialog.cxx
  #  vtkPVRenderGroupDialog.cxx
  )

INCLUDE_DIRECTORIES(
  ${KWSYS_INCLUDE_DIR}
  ${PVSERVERCOMMON_INCLUDE_DIR}
  ${PVFILTERS_INCLUDE_DIR}
  ${PVGUI_BINARY_DIR}/Client
  ${PVGUI_SOURCE_DIR}/Client
  ${PVSERVERMANAGER_INCLUDE_DIR}
  ${PVWIDGETS_INCLUDE_DIR}
  ${VTKCLIENTSERVER_INCLUDE_DIR}
  ${VTK_INCLUDE_DIR}
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
# Adds extra ParaView link directories
#
MACRO(PARAVIEW_EXTRA_LINK_DIRECTORIES DIRS)
  SET(current_dirs ${PARAVIEW_EXTRA_LINK_DIRECTORIES})
  FOREACH(dir ${DIRS})
    IF(${current_dirs} MATCHES ${dir})
    ELSE(${current_dirs} MATCHES ${dir})
      SET(current_dirs ${current_dirs} ${dir})
    ENDIF(${current_dirs} MATCHES ${dir})
  ENDFOREACH(dir)
  SET(PARAVIEW_EXTRA_LINK_DIRECTORIES "${current_dirs}" CACHE INTERNAL "")
ENDMACRO(PARAVIEW_EXTRA_LINK_DIRECTORIES)

#
# Imported project adds source files
# This ones need to be fixed, so that first are not wrapped. This will be fixed
# once the server changes are merged.
#
MACRO(PARAVIEW_INCLUDE_SOURCES SRCS)
  SET(ParaViewServerNonWrapped_SRCS ${ParaViewServerNonWrapped_SRCS} ${SRCS})
ENDMACRO(PARAVIEW_INCLUDE_SOURCES SRCS)
MACRO(PARAVIEW_INCLUDE_CLIENT_SOURCES SRCS)
  SET(ParaViewClient_SRCS ${ParaViewClient_SRCS} ${SRCS})
ENDMACRO(PARAVIEW_INCLUDE_CLIENT_SOURCES)
MACRO(PARAVIEW_INCLUDE_EXECUTABLE_SOURCES SRCS)
  SET(ParaViewBinary_SRCS ${ParaViewBinary_SRCS} ${SRCS})
ENDMACRO(PARAVIEW_INCLUDE_EXECUTABLE_SOURCES SRCS)
MACRO(PARAVIEW_INCLUDE_WRAPPED_SOURCES SRCS)
  SET(ParaViewServer_SRCS ${ParaViewServer_SRCS} ${SRCS})
ENDMACRO(PARAVIEW_INCLUDE_WRAPPED_SOURCES SRCS)


FOREACH(external EXODUS SAF)
  OPTION(PARAVIEW_USE_${external} "Build using ${external} library. Requires access to ${external} libraries" OFF)
  MARK_AS_ADVANCED(PARAVIEW_USE_${external})
  IF(PARAVIEW_USE_${external})
    FIND_PATH(${external}_SOURCE_DIR 
      ${external}ParaViewImport.cmake
      ../Trilab/ExodusReader
      ../../Trilab/ExodusReader
      ../../../Trilab/ExodusReader)

    IF(EXISTS ${${external}_SOURCE_DIR}/${external}ParaViewImport.cmake)
      INCLUDE(${${external}_SOURCE_DIR}/${external}ParaViewImport.cmake)
    ELSE(EXISTS ${${external}_SOURCE_DIR}/${external}ParaViewImport.cmake)
      MESSAGE(SEND_ERROR "Cannot find ${external} sources directory")
    ENDIF(EXISTS ${${external}_SOURCE_DIR}/${external}ParaViewImport.cmake)
  ENDIF(PARAVIEW_USE_${external})
ENDFOREACH(external)

IF(PARAVIEW_USE_SAF)
  SET( ParaViewClient_SRCS
    ${ParaViewClient_SRCS}
    ${SAF_SOURCE_DIR}/vtkPVSAFReaderModule.cxx )

  SET( ParaViewClient_SRCS
      ${ParaViewClient_SRCS}
      ${SAF_SOURCE_DIR}/../Filtering/vtkPVBasicDSPFilterWidget.cxx )
  INCLUDE_DIRECTORIES(${SAF_SOURCE_DIR}/../Filtering/)

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
TARGET_LINK_LIBRARIES(vtkKWParaView vtkKWWidgetsTCL vtkPVServerManagerTCL ${PARAVIEW_ADDITIONAL_LIBRARIES})

IF(PARAVIEW_PRO_BUILD)
  INCLUDE_DIRECTORIES(${PARAVIEW_PRO_INCLUDE_DIRS})
  TARGET_LINK_LIBRARIES(vtkKWParaView ParaViewProClientIO)
ENDIF(PARAVIEW_PRO_BUILD)



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
CONFIGURE_FILE(${PVGUI_SOURCE_DIR}/Client/pvTestDriverConfig.h.in
  ${PVGUI_BINARY_DIR}/Client/pvTestDriverConfig.h
  ESCAPE_QUOTES)

ADD_EXECUTABLE(pvTestDriver pvTestDriver.cxx)
ADD_DEPENDENCIES(pvTestDriver paraview)
TARGET_LINK_LIBRARIES(pvTestDriver kwsys)

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
  vtkPVDefaultModules Interface GetInterfaces
  Filters Manipulators Readers Sources Writers)

SET_SOURCE_FILES_PROPERTIES(vtkPVInitialize.cxx PROPERTIES OBJECT_DEPENDS 
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPVGeneratedModules.h)

INCLUDE (${PVGUI_BINARY_DIR}/Client/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${PVGUI_SOURCE_DIR}/Client/LocalUserOptions.cmake OPTIONAL)

INSTALL_TARGETS(/bin paraview)

IF(BUILD_SHARED_LIBS)
  INSTALL_TARGETS(/lib vtkKWParaViewCS
    vtkKWParaView)
ENDIF(BUILD_SHARED_LIBS)
