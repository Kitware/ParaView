SET( ParaView_SRCS
  vtkAllToNRedistributePolyData.cxx
  vtkBalancedRedistributePolyData.cxx
  vtkClientCompositeManager.cxx
  vtkCompositeProp.cxx
  vtkExtractCTHPart.cxx
  vtkGroup.cxx
  vtkKWBoundsDisplay.cxx
  vtkKWExtractGeometryByScalar.cxx
  vtkMergeArrays.cxx
  vtkMultiOut.cxx
  vtkMultiOut2.cxx
  vtkMultiOut3.cxx
  vtkPV3DWidget.cxx
  vtkPVAdvancedReaderModule.cxx
  vtkPVAnimationInterface.cxx
  vtkPVApplication.cxx 
  vtkPVApplicationResources.cxx 
  vtkPVApplicationSettingsInterface.cxx
  vtkPVArrayInformation.cxx
  vtkPVArrayMenu.cxx 
  vtkPVArraySelection.cxx
  vtkPVBoundsDisplay.cxx
  vtkPVCalculatorWidget.cxx
  vtkPVCameraIcon.cxx
  vtkPVCameraManipulator.cxx
  vtkPVClientServerModule.cxx
  vtkPVClipDataSet.cxx
  vtkPVColorMap.cxx
  vtkPVComponentSelection.cxx
  vtkPVCompositeBuffer.cxx
  vtkPVCompositeUtilities.cxx
  vtkPVConnectivityFilter.cxx
  vtkPVContainerWidget.cxx
  vtkPVContourEntry.cxx 
  vtkPVContourFilter.cxx
  vtkPVData.cxx 
  vtkPVDataInformation.cxx
  vtkPVDataSetAttributesInformation.cxx
  vtkPVDataSetFileEntry.cxx
  vtkPVDataSetReaderModule.cxx
  vtkPVDummyWidget.cxx
  vtkPVDuplicatePolyData.cxx
  vtkPVEnSightArraySelection.cxx
  vtkPVEnSightReaderModule.cxx
  vtkPVErrorLogDisplay.cxx
  vtkPVExtentEntry.cxx
  vtkPVExtentTranslator.cxx 
  vtkPVExtractGeometryByScalar.cxx
  vtkPVExtractPartsWidget.cxx
  vtkPVFieldMenu.cxx
  vtkPVFileEntry.cxx
  vtkPVGenericRenderWindowInteractor.cxx
  vtkPVGhostLevelDialog.cxx
  vtkPVGlyphFilter.cxx
  vtkPVGroupInputsWidget.cxx
  vtkPVImageContinuousDilate3D.cxx
  vtkPVImageContinuousErode3D.cxx
  vtkPVImageGradient.cxx
  vtkPVImageGradientMagnitude.cxx
  vtkPVImageMedian3D.cxx
  vtkPVImplicitPlaneWidget.cxx
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
  vtkPVInteractorStyleFly.cxx
  vtkPVJoystickFly.cxx
  vtkPVJoystickFlyIn.cxx
  vtkPVJoystickFlyOut.cxx
  vtkPVLODActor.cxx
  vtkPVLabel.cxx
  vtkPVLabeledToggle.cxx
  vtkPVLineSourceWidget.cxx
  vtkPVLineWidget.cxx
  vtkPVMPIProcessModule.cxx
  vtkPVMinMax.cxx
  vtkPVNavigationWindow.cxx
  vtkPVObjectWidget.cxx
  vtkPVPart.cxx
  vtkPVPLOT3DReaderModule.cxx
  vtkPVPivotManipulator.cxx
  vtkPVPlaneWidget.cxx
  vtkPVPointSourceWidget.cxx
  vtkPVPointWidget.cxx
  vtkPVProbe.cxx
  vtkPVProcessModule.cxx
  vtkPVPushButton.cxx
  vtkPVReaderModule.cxx
  vtkPVRenderGroupDialog.cxx
  vtkPVRenderView.cxx 
  vtkPVRibbonFilter.cxx
  vtkPVScalarRangeLabel.cxx 
  vtkPVScale.cxx 
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
  vtkPVStringEntry.cxx
  vtkPVThresholdFilter.cxx
  vtkPVTiledDisplayManager.cxx
  vtkPVTimerLogDisplay.cxx
  vtkPVTraceFileDialog.cxx
  vtkPVTrackballMoveActor.cxx
  vtkPVTrackballPan.cxx
  vtkPVTrackballRoll.cxx
  vtkPVTrackballRotate.cxx
  vtkPVTrackballZoom.cxx
  vtkPVTreeComposite.cxx 
  vtkPVUpdateSuppressor.cxx
  vtkPVVectorEntry.cxx
  vtkPVWarpScalar.cxx
  vtkPVWarpVector.cxx
  vtkPVWidget.cxx
  vtkPVWidgetCollection.cxx
  vtkPVWindow.cxx 
  vtkPVWindowResources.cxx 
  vtkPVWorldPointPicker.cxx 
  vtkPVWriter.cxx
  vtkPVXMLElement.cxx
  vtkPVXMLPackageParser.cxx
  vtkPVXMLParser.cxx
  vtkPickDataSetFilter.cxx
  vtkRedistributePolyData.cxx
  vtkSelectInputs.cxx
  vtkStringList.cxx 
  vtkTiledDisplaySchedule.cxx
  vtkWeightedRedistributePolyData.cxx
  vtkXDMFReaderModule.cxx
  )

IF (VTK_USE_PATENTED)
  SET( ParaView_SRCS
    ${ParaView_SRCS}
    vtkPVKitwareContourFilter)
ENDIF (VTK_USE_PATENTED)

IF(NOT PARAVIEW_NO_HDF5)
  SET(ParaView_SRCS ${ParaView_SRCS} vtkHDF5RawImageReader.cxx)
ENDIF(NOT PARAVIEW_NO_HDF5)

SET_SOURCE_FILES_PROPERTIES(
  vtkPVApplicationResources
  vtkPVWindowResources
  WRAP_EXCLUDE
  )

IF (VTK_USE_MPI)
  SET( ParaView_SRCS ${ParaView_SRCS} vtkPVEnSightVerifier.cxx )
ENDIF (VTK_USE_MPI)

SET_SOURCE_FILES_PROPERTIES(
  vtkPVObjectWidget
  vtkPVWidget
  vtkPV3DWidget
  vtkPVJoystickFly
  vtkPVSourceWidget
  ABSTRACT
  )

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR2(vtkParaViewInstantiator PVInstantiator_SRCS
  ${ParaView_SRCS}
  EXPORT_MACRO VTK_EXPORT
  HEADER_LOCATION ${PARAVIEW_BINARY_DIR}/ParaView)

VTK_WRAP_TCL(vtkKWParaViewTCL ParaViewTCL_SRCS ${PVInstantiator_SRCS} 
  ${ParaView_SRCS})
ADD_LIBRARY(vtkKWParaViewTCL ${ParaViewTCL_SRCS} ${PVInstantiator_SRCS} 
  ${ParaView_SRCS})

TARGET_LINK_LIBRARIES (vtkKWParaViewTCL
  vtkKWWidgetsTCL 
  vtkParallel)
IF(XDMF_FOUND)
  TARGET_LINK_LIBRARIES(vtkKWParaViewTCL vtkXdmfTCL)
ENDIF(XDMF_FOUND)


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

INCLUDE_DIRECTORIES(${PARAVIEW_BINARY_DIR}/ParaView)

SET(ParaViewBinary_SRCS
  ParaView.cxx)

IF(WIN32)
  SET(ParaViewBinary_SRCS ${ParaViewBinary_SRCS} ParaView.rc)
ENDIF(WIN32)

ADD_EXECUTABLE(ParaView WIN32 ${ParaViewBinary_SRCS}) 

TARGET_LINK_LIBRARIES (ParaView
  vtkKWParaViewTCL)

IF(NOT PARAVIEW_NO_HDF5)
  TARGET_LINK_LIBRARIES(vtkKWParaViewTCL ${PARAVIEW_HDF5_LIBRARIES})
ENDIF(NOT PARAVIEW_NO_HDF5)

ADD_EXECUTABLE(ProcessXML ProcessXML.cxx)
TARGET_LINK_LIBRARIES(ProcessXML vtkCommon)

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

SET_SOURCE_FILES_PROPERTIES(vtkPVInitialize.cxx PROPERTIES OBJECT_DEPENDS 
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPVGeneratedModules.h)

INCLUDE (${PARAVIEW_BINARY_DIR}/ParaView/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${PARAVIEW_SOURCE_DIR}/ParaView/LocalUserOptions.cmake OPTIONAL)


INSTALL_TARGETS(/lib vtkKWParaViewTCL)
INSTALL_TARGETS(/bin ParaView)

