PROJECT(Widgets)
INCLUDE_REGULAR_EXPRESSION("^(vtk|kw|icons).*$")
SET( KWWidgets_SRCS
vtkCornerAnnotation.cxx
vtkKWActorComposite.cxx 
vtkKWApplication.cxx 
vtkKWApplicationSettingsInterface.cxx
vtkKWBWidgets.cxx
vtkKWChangeColorButton.cxx 
vtkKWCheckButton.cxx 
vtkKWCheckButtonSet.cxx 
vtkKWCheckButtonWithChangeColor.cxx
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
vtkKWLabeledCheckButton.cxx
vtkKWLabeledCheckButtonSet.cxx 
vtkKWLabeledEntry.cxx 
vtkKWLabeledFrame.cxx 
vtkKWLabeledLabel.cxx 
vtkKWLabeledLabelSet.cxx
vtkKWLabeledOptionMenu.cxx 
vtkKWLabeledPopupButton.cxx 
vtkKWLabeledPushButton.cxx
vtkKWLabeledPushButtonSet.cxx 
vtkKWLabeledRadioButtonSet.cxx 
vtkKWLabeledText.cxx 
vtkKWLabeledWidget.cxx 
vtkKWListBox.cxx 
vtkKWLoadSaveDialog.cxx
vtkKWMenu.cxx 
vtkKWMenuButton.cxx 
vtkKWMessageDialog.cxx 
vtkKWNotebook.cxx 
vtkKWOKCancelDialog.cxx 
vtkKWObject.cxx 
vtkKWOptionMenu.cxx 
vtkKWPopupButton.cxx 
vtkKWProgressGauge.cxx 
vtkKWPushButton.cxx 
vtkKWPushButtonSet.cxx 
vtkKWRadioButton.cxx 
vtkKWRadioButtonSet.cxx
vtkKWRenderWidget.cxx 
vtkKWSaveImageDialog.cxx 
vtkKWScale.cxx 
vtkKWSegmentedProgressGauge.cxx
vtkKWSelectionFrame.cxx
vtkKWSplashScreen.cxx
vtkKWSplitFrame.cxx
vtkKWTclInteractor.cxx
vtkKWText.cxx 
vtkKWTextProperty.cxx
vtkKWThumbWheel.cxx
vtkKWTkUtilities.cxx
vtkKWToolbar.cxx 
vtkKWUserInterfaceManager.cxx
vtkKWUserInterfaceNotebookManager.cxx
vtkKWUserInterfacePanel.cxx
vtkKWView.cxx 
vtkKWViewCollection.cxx 
vtkKWWidget.cxx 
vtkKWWidgetCollection.cxx 
vtkKWWindow.cxx 
vtkKWWindowCollection.cxx 
vtkKWXtEmbeddedWidget.cxx 
${VTK_TCL_HOME}/vtktcl.c
)

SET_SOURCE_FILES_PROPERTIES(
  vtkKWComposite.cxx
  vtkKWRenderWidget.cxx
  vtkKWView.cxx
  vtkKWUserInterfaceManager.cxx
  ABSTRACT
  )

SET_SOURCE_FILES_PROPERTIES(
  ${VTK_TCL_HOME}/vtktcl.c
  vtkKWEvent.cxx
  vtkKWTkUtilities.cxx
  WRAP_EXCLUDE
  )

OPTION(USE_INSTALLED_TCLTK_PACKAGES "Use the intalled Tcl/Tk packages instead of kwinit." OFF)
MARK_AS_ADVANCED(USE_INSTALLED_TCLTK_PACKAGES)

IF (NOT USE_INSTALLED_TCLTK_PACKAGES)
  SET( KWWidgets_SRCS ${KWWidgets_SRCS}
    kwinit.c
  )
  SET_SOURCE_FILES_PROPERTIES(
    kwinit 
    WRAP_EXCLUDE
    )
ENDIF (NOT USE_INSTALLED_TCLTK_PACKAGES)

IF (WIN32)
  SET( KWWidgets_SRCS ${KWWidgets_SRCS}
    kwappicon.c
  )
  SET_SOURCE_FILES_PROPERTIES(
    kwappicon 
    WRAP_EXCLUDE
  )
ENDIF (WIN32)

VTK_WRAP_TCL(vtkKWWidgetsTCL KWWidgetsTCL_SRCS ${KWWidgets_SRCS})
ADD_LIBRARY(vtkKWWidgetsTCL ${KWWidgetsTCL_SRCS} ${KWWidgets_SRCS})

TARGET_LINK_LIBRARIES (vtkKWWidgetsTCL
vtkKWCommonTCL
vtkHybridTCL
vtkHybrid
${TCL_LIBRARY}
${TK_LIBRARY}
)

IF (VTK_USE_PARALLEL)
  TARGET_LINK_LIBRARIES(vtkKWWidgetsTCL vtkParallelTCL)
ENDIF (VTK_USE_PARALLEL)

IF (VTK_USE_PATENTED)
  TARGET_LINK_LIBRARIES(vtkKWWidgetsTCL vtkPatentedTCL)
ENDIF (VTK_USE_PATENTED)

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


