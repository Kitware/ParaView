PROJECT(KWWidgets)
INCLUDE_REGULAR_EXPRESSION("^(vtk|kw|icons).*$")

SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")

SET( KWWidgets_SRCS
  ${VTK_TCL_HOME}/vtktcl.c
  vtkKWActorComposite.cxx 
  vtkKWApplication.cxx 
  vtkKWApplicationSettingsInterface.cxx
  vtkKWBWidgets.cxx
  vtkKWCanvas.cxx 
  vtkKWChangeColorButton.cxx 
  vtkKWCheckButton.cxx 
  vtkKWCheckButtonSet.cxx 
  vtkKWCheckButtonWithChangeColor.cxx
  vtkKWComposite.cxx 
  vtkKWCompositeCollection.cxx 
  vtkKWCornerAnnotation.cxx 
  vtkKWDialog.cxx 
  vtkKWEntry.cxx 
  vtkKWEntrySet.cxx 
  vtkKWExtent.cxx 
  vtkKWFrame.cxx
  vtkKWGenericComposite.cxx 
  vtkKWGenericRenderWindowInteractor.cxx
  vtkKWHeaderAnnotation.cxx 
  vtkKWLabel.cxx
  vtkKWLabeledCheckButton.cxx
  vtkKWLabeledCheckButtonSet.cxx 
  vtkKWLabeledEntry.cxx 
  vtkKWLabeledFrame.cxx 
  vtkKWLabeledLabel.cxx 
  vtkKWLabeledLabelSet.cxx
  vtkKWLabeledLoadSaveButton.cxx
  vtkKWLabeledOptionMenu.cxx 
  vtkKWLabeledPopupButton.cxx 
  vtkKWLabeledPushButton.cxx
  vtkKWLabeledPushButtonSet.cxx 
  vtkKWLabeledRadioButtonSet.cxx 
  vtkKWLabeledScaleSet.cxx 
  vtkKWLabeledText.cxx 
  vtkKWLabeledWidget.cxx 
  vtkKWListBox.cxx 
  vtkKWListSelectOrder.cxx
  vtkKWLoadSaveButton.cxx
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
  vtkKWRenderWidgetCallbackCommand.cxx 
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
  vtkKWUserInterfaceManager.cxx
  vtkKWView.cxx
  ABSTRACT
  )

SET_SOURCE_FILES_PROPERTIES(
  ${VTK_TCL_HOME}/vtktcl.c
  vtkKWCompositeCollection.cxx
  vtkKWGenericRenderWindowInteractor.cxx
  vtkKWRenderWidgetCallbackCommand.cxx 
  vtkKWTkUtilities.cxx
  vtkKWViewCollection.cxx
  vtkKWWidgetCollection.cxx
  vtkKWWindowCollection.cxx
  WRAP_EXCLUDE
  )

IF (DO_NOT_BUILD_XML_RW)
  SET_SOURCE_FILES_PROPERTIES(
    vtkKWScalarBarAnnotation.cxx 
    vtkKWApplication.cxx 
    vtkKWCornerAnnotation.cxx 
    vtkKWHeaderAnnotation.cxx 
    COMPILE_FLAGS -DDO_NOT_BUILD_XML_RW
    )
ENDIF (DO_NOT_BUILD_XML_RW)

# If needed, copy the Tcl/Tk support files needed for vtkKWApplication
# at run-time to initialize Tcl/Tk

IF (TCL_TK_COPY_SUPPORT_LIBRARY)
  IF (TCL_SUPPORT_LIBRARY_PATH AND TK_SUPPORT_LIBRARY_PATH)
    INCLUDE(${VTK_TCL_TK_MACROS})

    IF (EXECUTABLE_OUTPUT_PATH)
      VTK_COPY_TCL_TK_SUPPORT_FILES_TO_BUILD_DIR (
        ${TCL_SUPPORT_LIBRARY_PATH}
        ${TK_SUPPORT_LIBRARY_PATH}
        ${EXECUTABLE_OUTPUT_PATH} "TclTk/lib")
    ENDIF (EXECUTABLE_OUTPUT_PATH)

    VTK_GET_TCL_TK_VERSION ("TCL_TK_MAJOR_VERSION" "TCL_TK_MINOR_VERSION")
    IF (TCL_TK_MAJOR_VERSION AND TCL_TK_MINOR_VERSION)
      SET (TCL_TK_VERSION "${TCL_TK_MAJOR_VERSION}.${TCL_TK_MINOR_VERSION}")
    ENDIF (TCL_TK_MAJOR_VERSION AND TCL_TK_MINOR_VERSION)

    VTK_GET_TCL_SUPPORT_FILES(${TCL_SUPPORT_LIBRARY_PATH} "TCL_SUPPORT_FILES")
    IF(KW_INSTALL_LIB_DIR)
      FOREACH (file ${TCL_SUPPORT_FILES})
        STRING (REGEX REPLACE "${TCL_SUPPORT_LIBRARY_PATH}/" "" base ${file})
        GET_FILENAME_COMPONENT(dest ${base} PATH)
        INSTALL_FILES(${KW_INSTALL_LIB_DIR}/TclTk/lib/tcl${TCL_TK_VERSION}/${dest} FILES ${file})
      ENDFOREACH (file)
    ENDIF(KW_INSTALL_LIB_DIR)

    VTK_GET_TK_SUPPORT_FILES(${TK_SUPPORT_LIBRARY_PATH} "TK_SUPPORT_FILES")
    IF(KW_INSTALL_LIB_DIR)
      FOREACH (file ${TK_SUPPORT_FILES})
        STRING (REGEX REPLACE "${TK_SUPPORT_LIBRARY_PATH}/" "" base ${file})
        GET_FILENAME_COMPONENT(dest ${base} PATH)
        INSTALL_FILES(${KW_INSTALL_LIB_DIR}/TclTk/lib/tk${TCL_TK_VERSION}/${dest} FILES ${file})
      ENDFOREACH (file)
    ENDIF(KW_INSTALL_LIB_DIR)

  ENDIF (TCL_SUPPORT_LIBRARY_PATH AND TK_SUPPORT_LIBRARY_PATH)
ENDIF (TCL_TK_COPY_SUPPORT_LIBRARY)

IF (WIN32)
  SET( KWWidgets_SRCS ${KWWidgets_SRCS}
    kwappicon.c
    )
  SET_SOURCE_FILES_PROPERTIES(
    kwappicon 
    WRAP_EXCLUDE
    )
ENDIF (WIN32)

IF(BUILD_TESTING)
  SET( KWWidgets_SRCS ${KWWidgets_SRCS}
    vtkKWTesting.cxx
    )
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)

IF(VTK_WRAP_TCL)
  VTK_WRAP_TCL2(vtkKWWidgetsTCL KWWidgetsTCL_SRCS ${KWWidgets_SRCS})
ELSE(VTK_WRAP_TCL)
  SET(VTK_WRAP_TCL3_INIT_DIR "${VTKTclWrapping_SOURCE_DIR}")
  INCLUDE("${VTKTclWrapping_SOURCE_DIR}/vtkWrapTcl.cmake")
  VTK_WRAP_TCL3(vtkKWWidgetsTCL KWWidgetsTCL_SRCS "${KWWidgets_SRCS}" "")
ENDIF(VTK_WRAP_TCL)

ADD_LIBRARY(vtkKWWidgetsTCL ${KWWidgetsTCL_SRCS} ${KWWidgets_SRCS})

IF(VTK_WRAP_TCL)
  TARGET_LINK_LIBRARIES (vtkKWWidgetsTCL
    vtkKWCommonTCL
    vtkHybridTCL
    vtkCommonTCL
    )
ELSE(VTK_WRAP_TCL)
  TARGET_LINK_LIBRARIES (vtkKWWidgetsTCL
    vtkKWCommon
    vtkHybrid
    vtkCommonTCL
    )
ENDIF(VTK_WRAP_TCL)

IF (VTK_USE_PARALLEL AND VTK_WRAP_TCL)
  TARGET_LINK_LIBRARIES(vtkKWWidgetsTCL vtkParallelTCL)
ENDIF (VTK_USE_PARALLEL AND VTK_WRAP_TCL)

IF (VTK_USE_PATENTED AND VTK_WRAP_TCL)
  TARGET_LINK_LIBRARIES(vtkKWWidgetsTCL vtkPatentedTCL)
ENDIF (VTK_USE_PATENTED AND VTK_WRAP_TCL)

SET(VTK_WRAP_HINTS ${KWWidgets_SOURCE_DIR}/hints)

IF(NOT WIN32)
  ADD_DEFINITIONS(-D_HPUX_SOURCE)
ENDIF(NOT WIN32)

OPTION(USE_NOTEBOOK_ICONS "Display Notebook with icons." OFF)
MARK_AS_ADVANCED(USE_NOTEBOOK_ICONS)

CONFIGURE_FILE(${KWWidgets_SOURCE_DIR}/vtkKWWidgetsConfigure.h.in ${KWWidgets_BINARY_DIR}/vtkKWWidgetsConfigure.h)
IF(VTK_WRAP_TCL)
  INCLUDE_DIRECTORIES(${KWWidgets_BINARY_DIR})
ELSE(VTK_WRAP_TCL)
  INCLUDE_DIRECTORIES(
    ${KWWidgets_BINARY_DIR}
    ${KWWidgets_SOURCE_DIR}
    ${VTK_INCLUDE_DIR}
    )
ENDIF(VTK_WRAP_TCL)

IF(BUILD_SHARED_LIBS)
  IF(NOT KW_INSTALL_LIB_DIR)
    SET(KW_INSTALL_LIB_DIR /lib)
  ENDIF(NOT KW_INSTALL_LIB_DIR)
  INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} vtkKWWidgetsTCL)
ENDIF(BUILD_SHARED_LIBS)

IF (WIN32)
  ADD_DEFINITIONS(-DPARAVIEW_USE_WIN32_RW)
ELSE (WIN32)
  IF (APPLE)
    IF (VTK_USE_COCOA)
      ADD_DEFINITIONS(-DPARAVIEW_USE_COCOA_RW)
    ENDIF (VTK_USE_COCOA)
    IF (VTK_USE_CARBON)
      ADD_DEFINITIONS(-DPARAVIEW_USE_CARBON_RW)
    ENDIF (VTK_USE_CARBON)
    IF (VTK_USE_X)
      ADD_DEFINITIONS(-DPARAVIEW_USE_X_RW)
    ENDIF (VTK_USE_X)
  ELSE (APPLE)
    ADD_DEFINITIONS(-DPARAVIEW_USE_X_RW)
  ENDIF (APPLE)
ENDIF (WIN32)
