SOURCE_FILES( ParaView_SRCS
vtkCameraInteractor 
vtkDummyRenderer
vtkDummyRenderWindow
vtkInteractor 
vtkKWBoundsDisplay
vtkKWCenterOfRotation 
vtkKWFlyInteractor 
vtkKWInteractor 
vtkKWRotateCameraInteractor 
vtkKWSelectPointInteractor
vtkKWTranslateCameraInteractor 
vtkPVActorComposite 
vtkPVAnimationInterface
vtkPVApplication 
vtkPVArrayCalculator 
vtkPVArrayMenu 
vtkPVArraySelection
vtkPVBoundsDisplay
vtkPVClipPlane
vtkPVCutPlane
vtkPVContour 
vtkPVContourEntry 
vtkPVData 
vtkPVDataSetReaderInterface 
vtkPVEnSightReaderInterface 
vtkPVExtentTranslator 
vtkPVFileEntry
vtkPVGlyph3D 
vtkPVInputMenu 
vtkPVLabeledToggle
vtkPVLODActor
vtkPVMethodInterface 
vtkPVObjectWidget
vtkPVPlaneWidget
vtkPVMinMax
vtkPVProbe
vtkPVRenderView 
vtkPVScalarRangeLabel 
vtkPVScale 
vtkPVSelectionList 
vtkPVSource 
vtkPVSourceInterface 
vtkPVSourceInterfaceParser
vtkPVStringEntry
vtkPVThreshold 
vtkPVTreeComposite 
vtkPVVectorEntry
vtkPVWidget
vtkPVWindow 
vtkPVWorldPointPicker 
vtkStringList 
)

WRAP_EXCLUDE_FILES(
vtkPVSourceInterfaceParser
)

VTK_WRAP_TCL(vtkKWParaViewTCL ParaViewTCL_SRCS ParaView_SRCS)
ADD_LIBRARY(vtkKWParaViewTCL ParaViewTCL_SRCS ParaView_SRCS)

CONFIGURE_FILE(${PARAVIEW_SOURCE_DIR}/ParaView/vtkPVSourceInterfaceDirectories.h.in
               ${PARAVIEW_BINARY_DIR}/ParaView/vtkPVSourceInterfaceDirectories.h
               ESCAPE_QUOTES)

CONFIGURE_FILE(${PARAVIEW_SOURCE_DIR}/ParaView/vtkPVDemoPaths.h.in
               ${PARAVIEW_BINARY_DIR}/ParaView/vtkPVDemoPaths.h
               ESCAPE_QUOTES)

INCLUDE_DIRECTORIES(${PARAVIEW_BINARY_DIR}/ParaView)

INCLUDE_DIRECTORIES(${EXPAT_SOURCE_DIR}/xmlparse)

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

LINK_LIBRARIES (vtkKWWidgetsTCL 
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

LINK_LIBRARIES (Expat)

TARGET_LINK_LIBRARIES (ParaView
                       vtkKWParaViewTCL)

INCLUDE (${PARAVIEW_BINARY_DIR}/ParaView/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${PARAVIEW_SOURCE_DIR}/ParaView/LocalUserOptions.cmake OPTIONAL)


INSTALL_TARGETS(/lib vtkKWParaViewTCL)
INSTALL_TARGETS(/bin ParaView)

