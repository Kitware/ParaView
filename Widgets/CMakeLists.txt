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
)

ABSTRACT_FILES(
vtkKWComposite 
vtkKWView 
)

WRAP_EXCLUDE_FILES(kwinit)

VTK_WRAP_TCL(vtkKWWidgetsTCL KWWidgetsTCL_SRCS KWWidgets_SRCS)
ADD_LIBRARY(vtkKWWidgetsTCL KWWidgetsTCL_SRCS KWWidgets_SRCS)
TARGET_LINK_LIBRARIES (vtkKWWidgetsTCL vtkCommonTCL vtkFilteringTCL   
                       vtkGraphicsTCL vtkImagingTCL vtkRenderingTCL vtkIOTCL
		       vtkHybridTcl ${TCL_LIBRARY}  ${TK_LIBRARY})

LINK_LIBRARIES (
vtkCommon 
vtkFiltering
vtkImaging
vtkGraphics
vtkIO
vtkRendering
vtkHybrid
vtkKWWidgets
${TCL_LIBRARY}
${TK_LIBRARY}
)

