INCLUDE_REGULAR_EXPRESSION("^(vtk|kw|icons).*$")
SOURCE_FILES( KWWidgets_SRCS
kwinit
vtkContainer
vtkCornerAnnotation 
vtkKWActorComposite 
vtkKWApplication 
vtkKWChangeColorButton 
vtkKWCheckButton 
vtkKWComposite 
vtkKWCompositeCollection 
vtkKWCornerAnnotation 
vtkKWDialog 
vtkKWEntry 
vtkKWEvent
vtkKWExtent 
vtkKWGenericComposite 
vtkKWHashTable
vtkKWHashTableIterator
vtkKWIcon
vtkKWImageLabel
vtkKWLabel 
vtkKWLabeledEntry 
vtkKWLabeledFrame 
vtkKWListBox 
vtkKWLoadSaveDialog
vtkKWMenu 
vtkKWMenuButton 
vtkKWMessageDialog 
vtkKWNotebook 
vtkKWObject 
vtkKWOKCancelDialog 
vtkKWOptionMenu 
vtkKWPointerArray
vtkKWProgressGauge 
vtkKWProcessStatistics
vtkKWPushButton 
vtkKWRadioButton 
vtkKWRegisteryUtilities
vtkKWSaveImageDialog 
vtkKWScale 
vtkKWSerializer 
vtkKWSplitFrame
vtkKWTclInteractor
vtkKWText 
vtkKWToolbar 
vtkKWView 
vtkKWViewCollection 
vtkKWVolumeComposite 
vtkKWWidget 
vtkKWWidgetCollection 
vtkKWWindow 
vtkKWWindowCollection 
vtkKWXtEmbeddedWidget 
${VTK_SOURCE_DIR}/Wrapping/Tcl/vtktcl 
)

ABSTRACT_FILES(
vtkKWComposite 
vtkContainer
vtkKWView 
vtkKWFileDialog
)


IF (WIN32)
  SOURCE_FILES( KWWidgets_SRCS 
    vtkKWWin32RegisteryUtilities
  )
  WRAP_EXCLUDE_FILES(
    vtkKWWin32RegisteryUtilities
  )
ELSE (WIN32)
  ADD_DEFINITIONS(-D_HPUX_SOURCE)
  SOURCE_FILES( KWWidgets_SRCS 
    vtkKWUNIXRegisteryUtilities
  )
  WRAP_EXCLUDE_FILES(
    vtkKWUNIXRegisteryUtilities
  )
ENDIF (WIN32)
  
WRAP_EXCLUDE_FILES(
  kwinit 
  vtktcl 
  vtkKWEvent
  vtkKWHashTable 
  vtkContainer
  vtkKWRegisteryUtilities
)

VTK_WRAP_TCL(vtkKWWidgetsTCL KWWidgetsTCL_SRCS KWWidgets_SRCS)
ADD_LIBRARY(vtkKWWidgetsTCL KWWidgetsTCL_SRCS KWWidgets_SRCS)

LINK_LIBRARIES( vtkpng vtkzlib vtkjpeg )

IF (VTK_USE_PARALLEL)
  LINK_LIBRARIES(vtkParallelTCL vtkParallel)
  IF (WIN32)
   LINK_LIBRARIES( wsock32 )
  ENDIF (WIN32)
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

OPTION(USE_NOTEBOOK_ICONS "Display Notebook with icons." OFF)
MARK_AS_ADVANCED(USE_NOTEBOOK_ICONS)

CONFIGURE_FILE(${KWWIDGETS_SOURCE_DIR}/vtkKWWidgetsConfigure.h.in ${KWWIDGETS_BINARY_DIR}/vtkKWWidgetsConfigure.h)
INCLUDE_DIRECTORIES(${KWWIDGETS_BINARY_DIR})


