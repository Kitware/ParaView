PROJECT(Widgets)
INCLUDE_REGULAR_EXPRESSION("^(vtk|kw|icons).*$")
SOURCE_FILES( KWWidgets_SRCS
vtkCornerAnnotation.cxx
vtkKWActorComposite.cxx 
vtkKWApplication.cxx 
vtkKWBWidgets.cxx
vtkKWChangeColorButton.cxx 
vtkKWCheckButton.cxx 
vtkKWComposite.cxx 
vtkKWCompositeCollection.cxx 
vtkKWCornerAnnotation.cxx 
vtkKWDialog.cxx 
vtkKWEntry.cxx 
vtkKWEvent.cxx
vtkKWExtent.cxx 
vtkKWFrame.cxx
vtkKWGenericComposite.cxx 
vtkKWImageLabel.cxx
vtkKWLabel.cxx
vtkKWLabeledEntry.cxx 
vtkKWLabeledFrame.cxx 
vtkKWListBox.cxx 
vtkKWLoadSaveDialog.cxx
vtkKWMenu.cxx 
vtkKWMenuButton.cxx 
vtkKWMessageDialog.cxx 
vtkKWNotebook.cxx 
vtkKWOKCancelDialog.cxx 
vtkKWObject.cxx 
vtkKWOptionMenu.cxx 
vtkKWProgressGauge.cxx 
vtkKWPushButton.cxx 
vtkKWRadioButton.cxx 
vtkKWSaveImageDialog.cxx 
vtkKWScale.cxx 
vtkKWSplitFrame.cxx
vtkKWTclInteractor.cxx
vtkKWText.cxx 
vtkKWToolbar.cxx 
vtkKWView.cxx 
vtkKWViewCollection.cxx 
vtkKWWidget.cxx 
vtkKWWidgetCollection.cxx 
vtkKWWindow.cxx 
vtkKWWindowCollection.cxx 
vtkKWXtEmbeddedWidget.cxx 
${VTK_SOURCE_DIR}/Wrapping/Tcl/vtktcl.c
)

ABSTRACT_FILES(
vtkKWComposite 
vtkKWView 
)

WRAP_EXCLUDE_FILES(
  vtktcl 
  vtkKWEvent
)

OPTION(USE_INSTALLED_TCLTK_PACKAGES "Use the intalled Tcl/Tk packages instead of kwinit." OFF)
MARK_AS_ADVANCED(USE_INSTALLED_TCLTK_PACKAGES)

IF (NOT USE_INSTALLED_TCLTK_PACKAGES)
  SOURCE_FILES( KWWidgets_SRCS
    kwinit.c
  )
  WRAP_EXCLUDE_FILES(
    kwinit 
  )
ENDIF (NOT USE_INSTALLED_TCLTK_PACKAGES)

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
vtkKWCommonTCL
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

SET(VTK_WRAP_HINTS ${Widgets_SOURCE_DIR}/hints)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)

IF(NOT WIN32)
  ADD_DEFINITIONS(-D_HPUX_SOURCE)
ENDIF(NOT WIN32)

OPTION(USE_NOTEBOOK_ICONS "Display Notebook with icons." OFF)
MARK_AS_ADVANCED(USE_NOTEBOOK_ICONS)

CONFIGURE_FILE(${KWWIDGETS_SOURCE_DIR}/vtkKWWidgetsConfigure.h.in ${KWWIDGETS_BINARY_DIR}/vtkKWWidgetsConfigure.h)
INCLUDE_DIRECTORIES(${KWWIDGETS_BINARY_DIR})


