SOURCE_FILES( ParaView_SRCS
vtkAllToNRedistributePolyData.cxx
vtkBalancedRedistributePolyData.cxx
vtkDummyRenderWindow.cxx
vtkDummyRenderer.cxx
vtkRedistributePolyData.cxx
vtkWeightedRedistributePolyData.cxx
vtkKWBoundsDisplay.cxx
vtkKWExtractGeometryByScalar.cxx
vtkPassThroughFilter.cxx
vtkPV3DWidget.cxx
vtkPVAdvancedReaderModule.cxx
vtkPVAnimationInterface.cxx
vtkPVApplication.cxx 
vtkPVArrayCalculator.cxx 
vtkPVArrayMenu.cxx 
vtkPVArraySelection.cxx
vtkPVBoundsDisplay.cxx
vtkPVColorMap.cxx
vtkPVComponentSelection.cxx
vtkPVConnectivityFilter.cxx
vtkPVContainerWidget.cxx
vtkPVContour.cxx 
vtkPVContourEntry.cxx 
vtkPVData.cxx 
vtkPVDataSetFileEntry.cxx
vtkPVDataSetReaderModule.cxx
vtkPVEnSightReaderModule.cxx
vtkPVErrorLogDisplay.cxx
vtkPVExtentEntry.cxx
vtkPVExtentTranslator.cxx 
vtkPVExtractGeometryByScalar.cxx
vtkPVFileEntry.cxx
vtkPVGenericRenderWindowInteractor.cxx
vtkPVGlyph3D.cxx 
vtkPVInputMenu.cxx 
vtkPVInteractorStyleCenterOfRotation.cxx
vtkPVInteractorStyleFly.cxx
vtkPVInteractorStyleRotateCamera.cxx
vtkPVInteractorStyleTranslateCamera.cxx
vtkPVLODActor.cxx
vtkPVLabel.cxx
vtkPVLabeledToggle.cxx
vtkPVLineWidget.cxx
vtkPVMinMax.cxx
vtkPVNavigationWindow.cxx
vtkPVObjectWidget.cxx
vtkPVPlaneWidget.cxx
vtkPVPointWidget.cxx
vtkPVProbe.cxx
vtkPVReaderModule.cxx
vtkPVRenderView.cxx 
vtkPVScalarRangeLabel.cxx 
vtkPVScale.cxx 
vtkPVSelectWidget.cxx 
vtkPVSelectionList.cxx
vtkPVSelectTimeSet.cxx
vtkPVSource.cxx 
vtkPVSourceCollection.cxx
vtkPVSphereWidget.cxx
vtkPVStringEntry.cxx
vtkPVThreshold.cxx 
vtkPVTimerLogDisplay.cxx
vtkPVTreeComposite.cxx 
vtkPVVectorEntry.cxx
vtkPVWidget.cxx
vtkPVWidgetCollection.cxx
vtkPVWindow.cxx 
#vtkPVWizard.cxx
vtkPVWorldPointPicker.cxx 
vtkPVXMLElement.cxx
vtkPVXMLPackageParser.cxx
vtkPVXMLParser.cxx
vtkStringList.cxx 
)

IF (VTK_USE_MPI)
  SOURCE_FILES( ParaView_SRCS vtkPVEnSightVerifier.cxx )
ENDIF (VTK_USE_MPI)

ABSTRACT_FILES(
vtkPVObjectWidget
vtkPVWidget
vtkPV3DWidget
)

# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR(vtkParaViewInstantiator PVInstantiator_SRCS
                      ParaView_SRCS
                      EXPORT_MACRO VTK_EXPORT
                      HEADER_LOCATION ${PARAVIEW_BINARY_DIR}/ParaView)

VTK_WRAP_TCL(vtkKWParaViewTCL ParaViewTCL_SRCS PVInstantiator_SRCS ParaView_SRCS)
ADD_LIBRARY(vtkKWParaViewTCL ParaViewTCL_SRCS PVInstantiator_SRCS ParaView_SRCS)

CONFIGURE_FILE(${PARAVIEW_SOURCE_DIR}/ParaView/vtkPVSourceInterfaceDirectories.h.in
               ${PARAVIEW_BINARY_DIR}/ParaView/vtkPVSourceInterfaceDirectories.h
               ESCAPE_QUOTES)

CONFIGURE_FILE(${PARAVIEW_SOURCE_DIR}/ParaView/vtkPVDemoPaths.h.in
               ${PARAVIEW_BINARY_DIR}/ParaView/vtkPVDemoPaths.h
               ESCAPE_QUOTES)

INCLUDE_DIRECTORIES(${PARAVIEW_BINARY_DIR}/ParaView)

#  INCLUDE_DIRECTORIES(${VTKEXPAT_SOURCE_DIR})

IF (TK_LIBRARY)
  LINK_LIBRARIES (${TK_LIBRARY})
ENDIF (TK_LIBRARY)

IF (WIN32)
  LINK_LIBRARIES( wsock32 )
ENDIF (WIN32)

ADD_EXECUTABLE(ParaView WIN32 ParaView.cxx) 

LINK_LIBRARIES( vtkpng vtkzlib vtkjpeg vtktiff )

IF (VTK_MANGLE_MESA)
  IF (MESA_INCLUDE_PATH)
    INCLUDE_DIRECTORIES(${MESA_INCLUDE_PATH})
  ENDIF(MESA_INCLUDE_PATH)     
  LINK_LIBRARIES (${MESA_LIBRARY})
ENDIF(VTK_MANGLE_MESA)

IF(CMAKE_HAS_X)
   LINK_LIBRARIES(-lXt)
   LINK_LIBRARIES(${CMAKE_X_LIBS})
   ADD_DEFINITIONS(${CMAKE_X_CFLAGS})
ENDIF(CMAKE_HAS_X)

IF (WIN32)
  IF (TK_XLIB_PATH)
    INCLUDE_DIRECTORIES(${TK_XLIB_PATH})
  ENDIF (TK_XLIB_PATH) 
ENDIF (WIN32)

LINK_LIBRARIES (vtkKWCommonTCL
	        vtkKWWidgetsTCL 
                vtkCommonTCL 
                vtkFilteringTCL   
                vtkGraphicsTCL 
                vtkImagingTCL 
                vtkRenderingTCL
                vtkParallelTCL
		vtkHybridTCL
                vtkIOTCL
                vtkCommon 
                vtkFiltering
                vtkImaging
                vtkGraphics
                vtkIO
                vtkRendering
                vtkParallel
                vtkHybrid
                ${TCL_LIBRARY} 
                ${TK_LIBRARY})


IF (VTK_USE_PATENTED)
 LINK_LIBRARIES(vtkPatentedTCL vtkPatented)
ENDIF (VTK_USE_PATENTED)

IF (VTK_USE_VIDEO_FOR_WINDOWS)
  LINK_LIBRARIES( vfw32 )
ENDIF (VTK_USE_VIDEO_FOR_WINDOWS)

LINK_LIBRARIES (vtkexpat)

TARGET_LINK_LIBRARIES (ParaView
                       vtkKWParaViewTCL)

INCLUDE (${PARAVIEW_BINARY_DIR}/ParaView/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${PARAVIEW_SOURCE_DIR}/ParaView/LocalUserOptions.cmake OPTIONAL)


INSTALL_TARGETS(/lib vtkKWParaViewTCL)
INSTALL_TARGETS(/bin ParaView)

