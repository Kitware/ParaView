SOURCE_FILES( KWWidgets_SRCS
kwinit
vtkKWComposite 
vtkKWView 
vtkCornerAnnotation 
vtkKWActorComposite 
vtkKWApplication 
vtkKWCallbackSpecification 
vtkKWChangeColorButton 
vtkKWCheckButton 
vtkKWCompositeCollection 
vtkKWCornerAnnotation 
vtkKWDialog 
vtkKWEntry 
vtkKWEventNotifier 
vtkKWExtent 
vtkKWGenericComposite 
vtkKWLabel 
vtkKWLabeledEntry 
vtkKWLabeledFrame 
vtkKWListBox 
vtkKWMenu 
vtkKWMenuButton 
vtkKWMessageDialog 
vtkKWNotebook 
vtkKWObject 
vtkKWOKCancelDialog 
vtkKWOptionMenu 
vtkKWProgressGauge 
vtkKWPushButton 
vtkKWRadioButton 
vtkKWSaveImageDialog 
vtkKWScale 
vtkKWSerializer 
vtkKWText 
vtkKWToolbar 
vtkKWWidget 
vtkKWWidgetCollection 
vtkKWViewCollection 
vtkKWVolumeComposite 
vtkKWWindow 
vtkKWWindowCollection 
vtkKWXtEmbeddedWidget 
${VTK_SOURCE_DIR}/Wrapping/vtktcl
)

ABSTRACT_FILES(
vtkKWComposite 
vtkKWView 
)

WRAP_EXCLUDE_FILES(kwinit vtktcl)

VTK_WRAP_TCL(vtkKWWidgetsTCL KWWidgetsTCL_SRCS KWWidgets_SRCS)
ADD_LIBRARY(vtkKWWidgetsTCL KWWidgetsTCL_SRCS KWWidgets_SRCS)
TARGET_LINK_LIBRARIES (vtkKWWidgetsTCL vtkCommonTCL vtkFilteringTCL   
                       vtkGraphicsTCL vtkImagingTCL vtkRenderingTCL vtkIOTCL
		       vtkHybridTCL vtkParallelTCL vtkPatentedTCL
                       ${TCL_LIBRARY} ${TK_LIBRARY})

LINK_LIBRARIES (
vtkCommon 
vtkFiltering
vtkImaging
vtkGraphics
vtkIO
vtkRendering
vtkHybrid
vtkParallel
vtkPatented
${TCL_LIBRARY}
${TK_LIBRARY}
)

