SOURCE_FILES( KWWidgets_SRCS
kwinit
vtkKWLoadSaveDialog
vtkKWHashTable
vtkKWPointerArray
vtkKWComposite 
vtkKWView 
vtkCornerAnnotation 
vtkKWActorComposite 
vtkKWApplication 
vtkKWChangeColorButton 
vtkKWCheckButton 
vtkKWCompositeCollection 
vtkKWCornerAnnotation 
vtkKWDialog 
vtkKWEntry 
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
vtkKWSplitFrame
vtkKWTclInteractor
vtkKWText 
vtkKWToolbar 
vtkKWWidget 
vtkKWWidgetCollection 
vtkKWViewCollection 
vtkKWVolumeComposite 
vtkKWWindow 
vtkKWTclCommand
vtkKWWindowCollection 
vtkKWXtEmbeddedWidget 
${VTK_SOURCE_DIR}/Wrapping/Tcl/vtktcl 
)

ABSTRACT_FILES(
vtkKWComposite 
vtkKWView 
)


IF (WIN32)
  SOURCE_FILES( KWWidgets_SRCS vtkKWRegisteryUtilities )
  WRAP_EXCLUDE_FILES(
    vtkKWRegisteryUtilities
    vtkKWTclCommand
  )
ENDIF (WIN32)
  
WRAP_EXCLUDE_FILES(kwinit vtktcl vtkKWHashTable vtkKWTclCommand)

VTK_WRAP_TCL(vtkKWWidgetsTCL KWWidgetsTCL_SRCS KWWidgets_SRCS)
ADD_LIBRARY(vtkKWWidgetsTCL KWWidgetsTCL_SRCS KWWidgets_SRCS)

IF (VTK_USE_PARALLEL)
  LINK_LIBRARIES(vtkParallelTCL vtkParallel)
ENDIF (VTK_USE_PARALLEL)
IF (VTK_USE_PATENTED)
  LINK_LIBRARIES(vtkPatentedTCL vtkPatented)
ENDIF (VTK_USE_PATENTED)
IF (VTK_USE_HYBRID)
  LINK_LIBRARIES(vtkHybridTCL vtkHybrid)
ENDIF (VTK_USE_HYBRID)

IF (WIN32)
  IF (TK_XLIB_PATH)
    INCLUDE_DIRECTORIES(${TK_XLIB_PATH})
  ENDIF (TK_XLIB_PATH) 
ENDIF (WIN32)

LINK_LIBRARIES (
vtkRenderingTCL 
vtkImagingTCL 
vtkGraphicsTCL 
vtkIOTCL
vtkFilteringTCL   
vtkCommonTCL 
vtkRendering
vtkImaging
vtkGraphics
vtkIO
vtkFiltering
vtkCommon 
${TCL_LIBRARY}
${TK_LIBRARY}
)

IF(CMAKE_HAS_X)
   LINK_LIBRARIES(-lXt)
   LINK_LIBRARIES(${CMAKE_X_LIBS})
   ADD_DEFINITIONS(${CMAKE_X_CFLAGS})
ENDIF(CMAKE_HAS_X)

INSTALL_TARGETS(/lib vtkKWWidgetsTCL)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/Widgets/vtkTclVersion.h.in ${CMAKE_BINARY_DIR}/vtkTclVersion.h)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
