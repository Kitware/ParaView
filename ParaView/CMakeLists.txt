SOURCE_FILES( ParaView_SRCS
vtkInteractorStyleExtent 
vtkCameraInteractor 
vtkClipPlane 
vtkCutMaterial 
vtkCutPlane 
vtkDummyRenderWindowInteractor 
vtkImageOutlineFilter 
vtkInteractor 
vtkInteractorStyleCamera 
vtkInteractorStyleGridExtent 
vtkInteractorStyleImageExtent 
vtkInteractorStylePlane 
vtkInteractorStylePlaneSource 
vtkInteractorStyleSphere 
vtkKWCenterOfRotation 
vtkKWFlyInteractor 
vtkKWInteractor 
vtkKWRotateCameraInteractor 
vtkKWSelectPointInteractor
vtkKWTranslateCameraInteractor 
vtkPOPReader 
vtkPVActorComposite 
vtkPVApplication 
vtkPVArrayCalculator 
vtkPVContour 
vtkPVData 
vtkPVDataSetReaderInterface 
vtkPVEnSightReaderInterface 
vtkPVExtentTranslator 
vtkPVGeometryFilter
vtkPVGlyph3D 
vtkPVInputMenu 
vtkPVMethodInterface 
vtkPVOutputWindow
vtkPVProbe
vtkPVRenderView 
vtkPVSelectionList 
vtkPVSource 
vtkPVSourceCollection 
vtkPVSourceInterface 
vtkPVSourceList 
vtkPVThreshold 
vtkPVTreeComposite 
vtkPVWindow 
vtkPVWorldPointPicker 
vtkSimpleFieldDataToAttributeDataFilter 
vtkSingleContourFilter 
vtkStringList 
)

ABSTRACT_FILES(
vtkInteractorStyleExtent 
)

VTK_WRAP_TCL(vtkKWParaViewTCL ParaViewTCL_SRCS ParaView_SRCS)
ADD_LIBRARY(vtkKWParaViewTCL ParaViewTCL_SRCS ParaView_SRCS)
TARGET_LINK_LIBRARIES (vtkKWParaViewTCL vtkKWWidgetsTCL vtkCommonTCL 
                       vtkFilteringTCL   
                       vtkGraphicsTCL 
                       vtkImagingTCL 
                       vtkRenderingTCL
                       vtkParallelTCL
                       vtkPatentedTCL 
		       vtkHybridTCL
                       vtkIOTCL
                       ${TCL_LIBRARY} 
                       ${TK_LIBRARY})

LINK_LIBRARIES (
vtkCommon 
vtkFiltering
vtkImaging
vtkGraphics
vtkIO
vtkRendering
vtkParallel
vtkPatented
vtkHybrid
${TCL_LIBRARY}
${TK_LIBRARY}
)

IF (TK_LIBRARY)
    LINK_LIBRARIES (${TK_LIBRARY})
ENDIF (TK_LIBRARY)

IF (WIN32)
  LINK_LIBRARIES( wsock32 )
ENDIF (WIN32)

ADD_EXECUTABLE(ParaView WIN32 ParaView) 
LINK_LIBRARIES ( ${OPENGL_LIBRARY} )				    
LINK_LIBRARIES( png zlib )
INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_PATH})

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

TARGET_LINK_LIBRARIES (ParaView vtkKWParaViewTCL vtkKWWidgetsTCL vtkCommonTCL 
                       vtkFilteringTCL   
                       vtkGraphicsTCL 
                       vtkImagingTCL 
                       vtkRenderingTCL 
                       vtkParallelTCL
                       vtkPatentedTCL 
                       vtkHybridTCL
                       vtkIOTCL)

IF (HAVE_PNG)
  TARGET_LINK_LIBRARIES(ParaView ${PNG_LIBRARY})
  IF (ZLIB_LIBRARY)
    TARGET_LINK_LIBRARIES(ParaView ${ZLIB_LIBRARY})
  ENDIF (ZLIB_LIBRARY)
ENDIF (HAVE_PNG)
