PROJECT(KWWidgets)

############################################################################
# to use this library in your code you will need to SUBDIR into this
# directory so that it gets built and then you will use the following variables
# in your CMakeLists files to get the proper include paths and libraries
SET (KWWidgets_INCLUDE_PATH 
  "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR}"
  CACHE INTERNAL "include paths for KWWidgets"
  )

SET (KWWidgets_LIBRARIES KWWidgets
  CACHE INTERNAL "libraries for KWWidgets")
########################################################################

INCLUDE_REGULAR_EXPRESSION("^(vtk|kw|icons).*$")

SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
IF(NOT WIN32)
  ADD_DEFINITIONS(-D_HPUX_SOURCE)
ENDIF(NOT WIN32)

SET(KWWidgets_SRCS
  vtkKWApplication.cxx 
  vtkKWApplicationSettingsInterface.cxx
  vtkKWBWidgets.cxx
  vtkKWBalloonHelpManager.cxx
  vtkKWCanvas.cxx 
  vtkKWChangeColorButton.cxx 
  vtkKWCheckButton.cxx 
  vtkKWCheckButtonWithChangeColor.cxx
  vtkKWColorPresetSelector.cxx
  vtkKWColorTransferFunctionEditor.cxx
  vtkKWCornerAnnotationEditor.cxx 
  vtkKWDialog.cxx 
  vtkKWDragAndDropTargetSet.cxx
  vtkKWEntry.cxx 
  vtkKWEvent.cxx
  vtkKWEventMap.cxx
  vtkKWExtent.cxx 
  vtkKWFrame.cxx
  vtkKWFrameWithScrollbar.cxx
  vtkKWFrameLabeled.cxx 
  vtkKWGenericRenderWindowInteractor.cxx
  vtkKWHSVColorSelector.cxx
  vtkKWHeaderAnnotationEditor.cxx 
  vtkKWHistogram.cxx
  vtkKWHistogramSet.cxx
  vtkKWIcon.cxx
  vtkKWLabel.cxx
  vtkKWListBox.cxx 
  vtkKWListBoxToListBoxSelectionEditor.cxx
  vtkKWLoadSaveButton.cxx
  vtkKWLoadSaveDialog.cxx
  vtkKWMaterialPropertyWidget.cxx
  vtkKWMenu.cxx 
  vtkKWMenuButton.cxx 
  vtkKWMessageDialog.cxx 
  vtkKWMostRecentFilesManager.cxx
  vtkKWMultiColumnList.cxx
  vtkKWNotebook.cxx 
  vtkKWObject.cxx 
  vtkKWOptionMenu.cxx 
  vtkKWParameterValueFunctionEditor.cxx
  vtkKWParameterValueFunctionInterface.cxx
  vtkKWPiecewiseFunctionEditor.cxx
  vtkKWPopupButton.cxx 
  vtkKWPopupFrame.cxx 
  vtkKWPopupFrameCheckButton.cxx 
  vtkKWProgressGauge.cxx 
  vtkKWPushButton.cxx 
  vtkKWPushButtonWithMenu.cxx 
  vtkKWRadioButton.cxx 
  vtkKWRadioButtonSet.cxx 
  vtkKWRange.cxx 
  vtkKWRegistryHelper.cxx
  vtkKWRenderWidget.cxx 
  vtkKWRenderWidgetCallbackCommand.cxx 
  vtkKWResourceUtilities.cxx 
  vtkKWSaveImageDialog.cxx 
  vtkKWScalarBarAnnotation.cxx 
  vtkKWScalarComponentSelectionWidget.cxx
  vtkKWScale.cxx 
  vtkKWSegmentedProgressGauge.cxx
  vtkKWSelectionFrame.cxx
  vtkKWSelectionFrameLayoutManager.cxx
  vtkKWSimpleEntryDialog.cxx
  vtkKWSplashScreen.cxx
  vtkKWSplitFrame.cxx
  vtkKWTclInteractor.cxx
  vtkKWText.cxx 
  vtkKWTextPropertyEditor.cxx
  vtkKWThumbWheel.cxx
  vtkKWTkUtilities.cxx
  vtkKWToolbar.cxx 
  vtkKWToolbarSet.cxx 
  vtkKWTopLevel.cxx 
  vtkKWUserInterfaceManager.cxx
  vtkKWUserInterfaceNotebookManager.cxx
  vtkKWUserInterfacePanel.cxx
  vtkKWVolumeMaterialPropertyWidget.cxx
  vtkKWVolumePropertyWidget.cxx
  vtkKWWidget.cxx 
  vtkKWWidgetLabeled.cxx 
  vtkKWWidgetSet.cxx 
  vtkKWWindow.cxx 
  vtkKWWindowBase.cxx 
  )

# Abstract/pure virtual classes

SET_SOURCE_FILES_PROPERTIES(
  vtkKWMaterialPropertyWidget.cxx
  vtkKWParameterValueFunctionEditor.cxx
  vtkKWParameterValueFunctionInterface.cxx
  vtkKWUserInterfaceManager.cxx
  vtkKWWidgetSet.cxx
  ABSTRACT
  )

# Helper classes

SET_SOURCE_FILES_PROPERTIES(
  vtkKWEvent.cxx
  vtkKWGenericRenderWindowInteractor.cxx
  vtkKWRenderWidgetCallbackCommand.cxx 
  WRAP_EXCLUDE
  )

# Add the registry code, and the set_icon command on win32

IF (WIN32)
  SET(KWWidgets_SRCS ${KWWidgets_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/Utilities/vtkKWSetApplicationIconTclCommand.c
    vtkKWWin32RegistryHelper.cxx)
  SET_SOURCE_FILES_PROPERTIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/Utilities/vtkKWSetApplicationIconTclCommand.c
    WRAP_EXCLUDE)
ELSE (WIN32)
  SET(KWWidgets_SRCS ${KWWidgets_SRCS} 
    vtkKWUNIXRegistryHelper.cxx)
ENDIF (WIN32)

# The "set of widgets" are automatically generated from some core widgets
# (i.e., CheckButton => vtkKWCheckButtonSet).
# If you modify this list, please update the README.txt file accordingly.

FOREACH (WidgetType 
    CheckButton 
    Entry
    Label
    LabelLabeled
    PushButton
    Scale
    )
  SET (WidgetType ${WidgetType})
  CONFIGURE_FILE (
    ${KWWidgets_SOURCE_DIR}/Templates/vtkKWWidgetSetSubclass.h.in 
    ${KWWidgets_BINARY_DIR}/vtkKW${WidgetType}Set.h IMMEDIATE)
  CONFIGURE_FILE (
    ${KWWidgets_SOURCE_DIR}/Templates/vtkKWWidgetSetSubclass.cxx.in 
    ${KWWidgets_BINARY_DIR}/vtkKW${WidgetType}Set.cxx IMMEDIATE)
  SET (KWWidgets_SRCS 
    ${KWWidgets_SRCS} ${KWWidgets_BINARY_DIR}/vtkKW${WidgetType}Set.cxx)
ENDFOREACH (WidgetType)

# The "labeled widgets" are automatically generated from some core widgets
# (i.e., CheckButton => vtkKWCheckButtonLabeled)
# If you modify this list, please update the README.txt file accordingly.

FOREACH (WidgetType 
    CheckButton 
    CheckButtonSet
    Entry
    Label
    LoadSaveButton
    OptionMenu
    PopupButton
    PushButton
    PushButtonSet
    Text
    RadioButtonSet
    ScaleSet
    )
  SET (WidgetType ${WidgetType})
  CONFIGURE_FILE (
    ${KWWidgets_SOURCE_DIR}/Templates/vtkKWWidgetLabeledSubclass.h.in 
    ${KWWidgets_BINARY_DIR}/vtkKW${WidgetType}Labeled.h IMMEDIATE)
  CONFIGURE_FILE (
    ${KWWidgets_SOURCE_DIR}/Templates/vtkKWWidgetLabeledSubclass.cxx.in 
    ${KWWidgets_BINARY_DIR}/vtkKW${WidgetType}Labeled.cxx IMMEDIATE)
  SET (KWWidgets_SRCS 
    ${KWWidgets_SRCS} ${KWWidgets_BINARY_DIR}/vtkKW${WidgetType}Labeled.cxx)
ENDFOREACH (WidgetType)

# Include dirs

INCLUDE_DIRECTORIES(
  ${KWWidgets_BINARY_DIR}
  ${KWWidgets_SOURCE_DIR}
  ${VTK_INCLUDE_DIR}
  ${KWSYS_INCLUDE_DIR}
  )

# Wrap the classes in Tcl

SET(VTK_WRAP_HINTS ${KWWidgets_SOURCE_DIR}/hints)

IF(VTK_WRAP_TCL)
  VTK_WRAP_TCL2(KWWidgets KWWidgetsTCL_SRCS ${KWWidgets_SRCS})
ELSE(VTK_WRAP_TCL)
  SET(VTK_WRAP_TCL3_INIT_DIR "${VTKTclWrapping_SOURCE_DIR}")
  INCLUDE("${VTKTclWrapping_SOURCE_DIR}/vtkWrapTcl.cmake")
  VTK_WRAP_TCL3(KWWidgets KWWidgetsTCL_SRCS "${KWWidgets_SRCS}" "")
ENDIF(VTK_WRAP_TCL)

# Build the library

ADD_LIBRARY(KWWidgets ${KWWidgetsTCL_SRCS} ${KWWidgets_SRCS})

IF(VTK_WRAP_TCL)
  TARGET_LINK_LIBRARIES (KWWidgets
    vtkHybridTCL
    vtkCommonTCL
    )
  IF (VTK_USE_PARALLEL)
    TARGET_LINK_LIBRARIES(KWWidgets vtkParallelTCL)
  ENDIF (VTK_USE_PARALLEL)
  IF (VTK_USE_PATENTED)
    TARGET_LINK_LIBRARIES(KWWidgets vtkPatentedTCL)
  ENDIF (VTK_USE_PATENTED)
ELSE(VTK_WRAP_TCL)
  TARGET_LINK_LIBRARIES (KWWidgets
    vtkHybrid
    vtkCommonTCL
    )
ENDIF(VTK_WRAP_TCL)

# We need kwsys

IF(KWSYS_NAMESPACE)
  TARGET_LINK_LIBRARIES (KWWidgets ${KWSYS_NAMESPACE})
ENDIF(KWSYS_NAMESPACE)

# If needed, copy the Tcl/Tk support files needed at run-time 
# to initialize Tcl/Tk

IF (VTK_TCL_TK_COPY_SUPPORT_LIBRARY)
  IF (VTK_TCL_SUPPORT_LIBRARY_PATH AND VTK_TK_SUPPORT_LIBRARY_PATH)
    INCLUDE(${VTK_TCL_TK_MACROS})
    IF (EXECUTABLE_OUTPUT_PATH)
      VTK_COPY_TCL_TK_SUPPORT_FILES_TO_BUILD_DIR (
        ${VTK_TCL_SUPPORT_LIBRARY_PATH}
        ${VTK_TK_SUPPORT_LIBRARY_PATH}
        ${EXECUTABLE_OUTPUT_PATH} "TclTk/lib")
    ENDIF (EXECUTABLE_OUTPUT_PATH)
  ENDIF (VTK_TCL_SUPPORT_LIBRARY_PATH AND VTK_TK_SUPPORT_LIBRARY_PATH)
ENDIF (VTK_TCL_TK_COPY_SUPPORT_LIBRARY)

# Testing ? Examples ?

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)

IF (BUILD_EXAMPLES)
  SUBDIRS(Examples)
ENDIF (BUILD_EXAMPLES)

# Build the utilities

SUBDIRS(Utilities)

# Install the library

IF(BUILD_SHARED_LIBS)
  IF(NOT KW_INSTALL_LIB_DIR)
    SET(KW_INSTALL_LIB_DIR /lib)
  ENDIF(NOT KW_INSTALL_LIB_DIR)
  INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} KWWidgets)
ENDIF(BUILD_SHARED_LIBS)

# Configure some build settings

SET (KWWIDGETS_SHARED_LIBS ${BUILD_SHARED_LIBS})
CONFIGURE_FILE(
  ${KWWidgets_SOURCE_DIR}/Templates/vtkKWWidgetsConfigure.h.in 
  ${KWWidgets_BINARY_DIR}/vtkKWWidgetsConfigure.h)


