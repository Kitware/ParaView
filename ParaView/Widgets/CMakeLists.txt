PROJECT(Widgets)
INCLUDE_REGULAR_EXPRESSION("^(vtk|kw|icons).*$")

SET( KWWidgets_SRCS
${VTK_TCL_HOME}/vtktcl.c
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
vtkKWExtent.cxx 
vtkKWFrame.cxx
vtkKWGenericComposite.cxx 
vtkKWHeaderAnnotation.cxx 
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
vtkKWLabeledScaleSet.cxx 
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
vtkKWPopupFrame.cxx 
vtkKWPopupFrameCheckButton.cxx 
vtkKWProgressGauge.cxx 
vtkKWPushButton.cxx 
vtkKWPushButtonSet.cxx 
vtkKWRadioButton.cxx 
vtkKWRadioButtonSet.cxx
vtkKWRange.cxx 
vtkKWRenderWidget.cxx 
vtkKWRenderWidgetCommand.cxx 
vtkKWSaveImageDialog.cxx 
vtkKWScalarBarAnnotation.cxx 
vtkKWScalarComponentSelectionWidget.cxx
vtkKWScale.cxx 
vtkKWScaleSet.cxx 
vtkKWSegmentedProgressGauge.cxx
vtkKWSelectionFrame.cxx
vtkKWSimpleEntryDialog.cxx
vtkKWSplashScreen.cxx
vtkKWSplitFrame.cxx
vtkKWTclInteractor.cxx
vtkKWText.cxx 
vtkKWTextProperty.cxx
vtkKWThumbWheel.cxx
vtkKWTkUtilities.cxx
vtkKWToolbar.cxx 
vtkKWToolbarSet.cxx 
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
)

SET_SOURCE_FILES_PROPERTIES(
  vtkKWComposite.cxx
  vtkKWRenderWidget.cxx
  vtkKWUserInterfaceManager.cxx
  vtkKWView.cxx
  ABSTRACT
  )

SET_SOURCE_FILES_PROPERTIES(
  ${VTK_TCL_HOME}/vtktcl.c
  vtkKWRenderWidgetCommand.cxx 
  vtkKWTkUtilities.cxx
  WRAP_EXCLUDE
  )

IF (NOT DO_NOT_BUILD_XML_RW)
  INCLUDE_DIRECTORIES(
    ${KWCommon_SOURCE_DIR}/XML
    ${Widgets_SOURCE_DIR}/XML
  )
  SET (KWWidgets_SRCS ${KWWidgets_SRCS}
    ${Widgets_SOURCE_DIR}/XML/vtkXMLCornerAnnotationReader.cxx
    ${Widgets_SOURCE_DIR}/XML/vtkXMLCornerAnnotationWriter.cxx
    ${Widgets_SOURCE_DIR}/XML/vtkXMLKWRenderWidgetReader.cxx
    ${Widgets_SOURCE_DIR}/XML/vtkXMLKWRenderWidgetWriter.cxx
    ${Widgets_SOURCE_DIR}/XML/vtkXMLKWUserInterfaceManagerReader.cxx
    ${Widgets_SOURCE_DIR}/XML/vtkXMLKWUserInterfaceManagerWriter.cxx
    ${Widgets_SOURCE_DIR}/XML/vtkXMLKWUserInterfaceNotebookManagerReader.cxx
    ${Widgets_SOURCE_DIR}/XML/vtkXMLKWUserInterfaceNotebookManagerWriter.cxx
    ${Widgets_SOURCE_DIR}/XML/vtkXMLKWWindowReader.cxx
    ${Widgets_SOURCE_DIR}/XML/vtkXMLKWWindowWriter.cxx
  )
ELSE (NOT DO_NOT_BUILD_XML_RW)
  SET_SOURCE_FILES_PROPERTIES(
	vtkKWScalarBarAnnotation.cxx 
    vtkKWApplication.cxx 
    vtkKWCornerAnnotation.cxx 
    vtkKWHeaderAnnotation.cxx 
    COMPILE_FLAGS -DDO_NOT_BUILD_XML_RW
  )
ENDIF (NOT DO_NOT_BUILD_XML_RW)

OPTION(USE_INSTALLED_TCLTK_PACKAGES "Use the intalled/system Tcl/Tk packages instead of kwinit or embedded packages." OFF)
MARK_AS_ADVANCED(USE_INSTALLED_TCLTK_PACKAGES)

IF (NOT USE_INSTALLED_TCLTK_PACKAGES)
  SET (COMPILE_KWINIT 1)
  IF (WIN32)
    IF (TK_LIBRARY MATCHES tk84)
      SET (COMPILE_KWINIT 0)
    ENDIF (TK_LIBRARY MATCHES tk84)
  ENDIF (WIN32)
  SUBDIRS(TclTk)
  IF (COMPILE_KWINIT)
    SET( KWWidgets_SRCS ${KWWidgets_SRCS}
      kwinit.c
    )
    SET_SOURCE_FILES_PROPERTIES(
      kwinit 
      WRAP_EXCLUDE
    )
  ENDIF (COMPILE_KWINIT)
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

VTK_WRAP_TCL2(vtkKWWidgetsTCL KWWidgetsTCL_SRCS ${KWWidgets_SRCS})
ADD_LIBRARY(vtkKWWidgetsTCL ${KWWidgetsTCL_SRCS} ${KWWidgets_SRCS})

TARGET_LINK_LIBRARIES (vtkKWWidgetsTCL
vtkKWCommonTCL
vtkHybridTCL
vtkHybrid
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

IF(BUILD_SHARED_LIBS)
  IF(NOT KW_INSTALL_LIB_DIR)
    SET(KW_INSTALL_LIB_DIR /lib)
  ENDIF(NOT KW_INSTALL_LIB_DIR)
  INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} vtkKWWidgetsTCL)
ENDIF(BUILD_SHARED_LIBS)
