SOURCE_FILES( Widgets_SRCS
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

ADD_LIBRARY(vtkKWWidgets Widgets_SRCS)

WRAP_EXCLUDE_FILES(kwinit)

VTK_WRAP_TCL(vtkKWWidgetsTCL WidgetsTCL_SRCS Widgets_SRCS)
ADD_LIBRARY(vtkKWWidgetsTCL WidgetsTCL_SRCS)
TARGET_LINK_LIBRARIES (vtkKWWidgetsTCL vtkCommonTCL vtkFilteringTCL   
                       vtkGraphicsTCL vtkImagingTCL vtkRenderingTCL vtkIOTCL)

LINK_LIBRARIES (
vtkCommon 
vtkFiltering
vtkImaging
vtkGraphics
vtkIO
vtkRendering
vtkKWWidgets 
)

