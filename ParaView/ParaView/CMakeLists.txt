SET( ParaView_SRCS
vtkAllToNRedistributePolyData.cxx
vtkBalancedRedistributePolyData.cxx
vtkClientCompositeManager.cxx
vtkCompositeProp.cxx
vtkDuplicatePolyData.cxx
vtkExtractCTHPart.cxx
vtkGroup.cxx
vtkRedistributePolyData.cxx
vtkWeightedRedistributePolyData.cxx
vtkKWBoundsDisplay.cxx
vtkKWExtractGeometryByScalar.cxx
vtkMultiOut.cxx
vtkMultiOut2.cxx
vtkMultiOut3.cxx
vtkPickDataSetFilter.cxx
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
vtkPVColorMap.cxx
vtkPVComponentSelection.cxx
vtkPVConnectivityFilter.cxx
vtkPVContainerWidget.cxx
vtkPVContour.cxx 
vtkPVContourEntry.cxx 
vtkPVData.cxx 
vtkPVDataInformation.cxx
vtkPVDataSetAttributesInformation.cxx
vtkPVDataSetFileEntry.cxx
vtkPVDataSetReaderModule.cxx
vtkPVDummyWidget.cxx
vtkPVEnSightArraySelection.cxx
vtkPVEnSightReaderModule.cxx
vtkPVErrorLogDisplay.cxx
vtkPVExtentEntry.cxx
vtkPVExtentTranslator.cxx 
vtkPVExtractGeometryByScalar.cxx
vtkPVExtractPartsWidget.cxx
vtkPVFileEntry.cxx
vtkPVGenericRenderWindowInteractor.cxx
vtkPVGhostLevelDialog.cxx
vtkPVGlyph3D.cxx 
vtkPVGroupInputsWidget.cxx
vtkPVImplicitPlaneWidget.cxx
vtkPVInitialize.cxx
vtkPVInputMenu.cxx 
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
vtkPVLineWidget.cxx
vtkPVLineSourceWidget.cxx
vtkPVMinMax.cxx
vtkPVMPIProcessModule.cxx
vtkPVNavigationWindow.cxx
vtkPVObjectWidget.cxx
vtkPVPart.cxx
vtkPVPassThrough.cxx
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
vtkPVScalarRangeLabel.cxx 
vtkPVScale.cxx 
vtkPVSelectCustomReader.cxx
vtkPVSelectWidget.cxx 
vtkPVSelectionList.cxx
vtkPVSelectTimeSet.cxx
vtkPVSource.cxx 
vtkPVSourceCollection.cxx
vtkPVSourceList.cxx
vtkPVSourcesNavigationWindow.cxx
vtkPVSourceWidget.cxx
vtkPVSphereWidget.cxx
vtkPVStringEntry.cxx
vtkPVThreshold.cxx 
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
vtkPVWidget.cxx
vtkPVWidgetCollection.cxx
vtkPVWindow.cxx 
vtkPVWindowResources.cxx 
#vtkPVWizard.cxx
vtkPVWorldPointPicker.cxx 
vtkPVWriter.cxx
vtkPVXMLElement.cxx
vtkPVXMLPackageParser.cxx
vtkPVXMLParser.cxx
vtkSelectInputs.cxx
vtkStringList.cxx 
vtkTiledDisplayManager.cxx

vtkPVWarpScalar
vtkPVClipDataSet
vtkPVContourFilter
vtkPVGlyphFilter
vtkPVThresholdFilter
vtkPVRibbonFilter
vtkPVWarpVector
vtkPVImageGradient
vtkPVImageGradientMagnitude
vtkPVImageMedian3D
vtkPVImageContinuousDilate3D
vtkPVImageContinuousErode3D
)

IF (VTK_USE_PATENTED)
 SET( ParaView_SRCS
      ${ParaView_SRCS}
      vtkPVKitwareContourFilter)
ENDIF (VTK_USE_PATENTED)

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
VTK_MAKE_INSTANTIATOR(vtkParaViewInstantiator PVInstantiator_SRCS
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

INCLUDE (${PARAVIEW_BINARY_DIR}/ParaView/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${PARAVIEW_SOURCE_DIR}/ParaView/LocalUserOptions.cmake OPTIONAL)


INSTALL_TARGETS(/lib vtkKWParaViewTCL)
INSTALL_TARGETS(/bin ParaView)

