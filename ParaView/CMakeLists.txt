SOURCE_FILES( ParaView_SRCS
vtkInteractorStyleExtent 
vtkCameraInteractor 
vtkClipPlane 
vtkCutMaterial 
vtkCutPlane 
vtkDummyRenderWindowInteractor 
vtkFastGeometryFilter 
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
vtkPVGlyph3D 
vtkPVInputMenu 
vtkPVMethodInterface 
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

ADD_LIBRARY(vtkKWParaView ParaView_SRCS)

VTK_WRAP_TCL(vtkKWParaViewTCL ParaViewTCL_SRCS ParaView_SRCS)
ADD_LIBRARY(vtkKWParaViewTCL ParaViewTCL_SRCS)
TARGET_LINK_LIBRARIES (vtkKWParaViewTCL vtkKWWidgetsTCL vtkCommonTCL 
                       vtkFilteringTCL   
                       vtkGraphicsTCL 
                       vtkImagingTCL 
                       vtkRenderingTCL
                       vtkParallelTCL
                       vtkPatentedTCL 
		       vtkHybridTCL
                       vtkIOTCL)

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
vtkKWWidgets
vtkKWParaView 
)

IF (TK_LIBRARY)
    LINK_LIBRARIES (${TK_LIBRARY})
ENDIF (TK_LIBRARY)


ADD_EXECUTABLE(ParaView WIN32 ParaView ${VTK_SOURCE_DIR}/Wrapping/vtktcl) 
LINK_LIBRARIES ( ${OPENGL_LIBRARY} )				    
TARGET_LINK_LIBRARIES (ParaView vtkKWParaViewTCL vtkKWWidgetsTCL vtkCommonTCL 
                       vtkFilteringTCL   
                       vtkGraphicsTCL 
                       vtkImagingTCL 
                       vtkRenderingTCL 
                       vtkParallelTCL
                       vtkPatentedTCL 
                       vtkHybridTCL
                       vtkIOTCL)
