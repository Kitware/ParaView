SOURCE_FILES( ParaView_SRCS
#vtkPVDataSetReaderInterface 
#vtkPVEnSightReaderInterface 
#vtkPVMethodInterface 
#vtkPVSourceInterface 
#vtkPVSourceInterfaceParser
vtkDummyRenderWindow
vtkDummyRenderer
vtkKWBoundsDisplay
vtkKWExtractGeometryByScalar
vtkPV3DWidget
vtkPVAdvancedReaderModule
vtkPVAnimationInterface
vtkPVApplication 
vtkPVArrayCalculator 
vtkPVArrayMenu 
vtkPVArraySelection
vtkPVBoundsDisplay
vtkPVComponentSelection
vtkPVConnectivityFilter
vtkPVContainerWidget
vtkPVContour 
vtkPVContourEntry 
vtkPVData 
vtkPVDataSetFileEntry
vtkPVDataSetReaderModule
vtkPVEnSightReaderModule
vtkPVErrorLogDisplay
vtkPVExtentEntry
vtkPVExtentTranslator 
vtkPVExtractGeometryByScalar
vtkPVFileEntry
vtkPVGenericRenderWindowInteractor
vtkPVGlyph3D 
vtkPVInputMenu 
vtkPVInteractorStyleCenterOfRotation
vtkPVInteractorStyleFly
vtkPVInteractorStyleRotateCamera
vtkPVInteractorStyleTranslateCamera
vtkPVLODActor
vtkPVLabel
vtkPVLabeledToggle
vtkPVLineWidget
vtkPVMinMax
vtkPVNavigationWindow
vtkPVObjectWidget
vtkPVPlaneWidget
vtkPVProbe
vtkPVReaderModule
vtkPVRenderView 
vtkPVScalarRangeLabel 
vtkPVScale 
vtkPVSelectWidget 
vtkPVSelectionList
vtkPVSource 
vtkPVSourceCollection
vtkPVSphereWidget
vtkPVStringEntry
vtkPVThreshold 
vtkPVTimerLogDisplay
vtkPVTreeComposite 
vtkPVVectorEntry
vtkPVWidget
vtkPVWidgetCollection
vtkPVWindow 
vtkPVWizard
vtkPVWorldPointPicker 
vtkPVXMLElement
vtkPVXMLPackageParser
vtkPVXMLParser
vtkStringList 
)

ABSTRACT_FILES(
vtkPVObjectWidget
vtkPVWidget
vtkPV3DWidget
)
WRAP_EXCLUDE_FILES(
vtkPVSourceInterfaceParser
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

ADD_EXECUTABLE(ParaView WIN32 ParaView) 

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

