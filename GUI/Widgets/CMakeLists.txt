PROJECT(KWWidgets)

############################################################################
# to use this library in your code you will need to SUBDIR into this
# directory so that it gets built and then you will use the following variables
# in your CMakeLists files to get the proper include paths and libraries
SET (KWWidgets_INCLUDE_PATH 
  "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR}/Templates;${CMAKE_CURRENT_BINARY_DIR}"
  CACHE INTERNAL "include paths for KWWidgets"
  )

SET (KWWidgets_LIBRARIES KWWidgets
  CACHE INTERNAL "libraries for KWWidgets")
########################################################################

# We need VTK

IF (NOT VTK_SOURCE_DIR)
  FIND_PACKAGE(VTK REQUIRED) 
  INCLUDE(${VTK_USE_FILE})
  IF(NOT VTK_WRAP_TCL)
    MESSAGE(FATAL_ERROR "Please set VTK_WRAP_TCL to ON")
  ENDIF(NOT VTK_WRAP_TCL)
  INCLUDE(Dart)
  MARK_AS_ADVANCED(BUILD_TESTING DART_ROOT TCL_TCLSH)
  IF(BUILD_TESTING)
    ENABLE_TESTING()
  ENDIF(BUILD_TESTING)
  SET(BUILDNAME "${PROJECT_NAME}-${BUILDNAME}")
ENDIF (NOT VTK_SOURCE_DIR)

# Version

SET (KWWIDGETS_MAJOR_VERSION 1)
SET (KWWIDGETS_MINOR_VERSION 0)

# Output directories

IF(NOT LIBRARY_OUTPUT_PATH)
  SET(LIBRARY_OUTPUT_PATH ${KWWidgets_BINARY_DIR}/bin CACHE INTERNAL 
    "Single output directory for building all libraries.")
ENDIF(NOT LIBRARY_OUTPUT_PATH)

IF(NOT EXECUTABLE_OUTPUT_PATH)
  SET(EXECUTABLE_OUTPUT_PATH ${KWWidgets_BINARY_DIR}/bin CACHE INTERNAL 
    "Single output directory for building all executables.")
ENDIF(NOT EXECUTABLE_OUTPUT_PATH)

# Install directories

IF(NOT KW_INSTALL_BIN_DIR)
  SET(KW_INSTALL_BIN_DIR "/bin")
ENDIF(NOT KW_INSTALL_BIN_DIR)
IF(NOT KW_INSTALL_LIB_DIR)
  SET(KW_INSTALL_LIB_DIR "/lib/${PROJECT_NAME}")
ENDIF(NOT KW_INSTALL_LIB_DIR)
IF(NOT KW_INSTALL_SHARE_DIR)
  SET(KW_INSTALL_SHARE_DIR "/share/${PROJECT_NAME}")
ENDIF(NOT KW_INSTALL_SHARE_DIR)

# Some flags

INCLUDE_REGULAR_EXPRESSION("^(vtk|kw|icons).*$")
SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
IF(NOT WIN32)
  ADD_DEFINITIONS(-D_HPUX_SOURCE)
ENDIF(NOT WIN32)
SET (BUILD_SHARED_LIBS ${VTK_BUILD_SHARED_LIBS})

# Sources

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
  vtkKWCompositeWidget.cxx
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
  vtkKWScrollbar.cxx 
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
  vtkKWTkOptions.cxx
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
  vtkKWWidgetWithScrollbars.cxx 
  vtkKWWindow.cxx 
  vtkKWWindowBase.cxx 
  ${CMAKE_CURRENT_SOURCE_DIR}/Wrapping/Tcl/vtkKWWidgetsInitializeCommand.cxx
  )

# Abstract/pure virtual classes

SET_SOURCE_FILES_PROPERTIES(
  vtkKWMaterialPropertyWidget.cxx
  vtkKWParameterValueFunctionEditor.cxx
  vtkKWParameterValueFunctionInterface.cxx
  vtkKWUserInterfaceManager.cxx
  vtkKWWidgetSet.cxx
  vtkKWWidgetWithScrollbars.cxx 
  ABSTRACT
  )

# Helper classes

SET_SOURCE_FILES_PROPERTIES(
  vtkKWEvent.cxx
  vtkKWGenericRenderWindowInteractor.cxx
  vtkKWRenderWidgetCallbackCommand.cxx 
  ${CMAKE_CURRENT_SOURCE_DIR}/Wrapping/Tcl/vtkKWWidgetsInitializeCommand.cxx 
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
# (i.e., vtkKWCheckButton => vtkKWCheckButtonSet).
# If you modify this list, please update the README.txt file accordingly.

FOREACH (WidgetType 
    vtkKWCheckButton 
    vtkKWEntry
    vtkKWLabel
    vtkKWLabelLabeled
    vtkKWPushButton
    vtkKWScale
    )
  SET (WidgetType ${WidgetType})
  CONFIGURE_FILE (
    ${KWWidgets_SOURCE_DIR}/Templates/vtkKWWidgetSetSubclass.h.in 
    ${KWWidgets_BINARY_DIR}/Templates/${WidgetType}Set.h IMMEDIATE)
  CONFIGURE_FILE (
    ${KWWidgets_SOURCE_DIR}/Templates/vtkKWWidgetSetSubclass.cxx.in 
    ${KWWidgets_BINARY_DIR}/Templates/${WidgetType}Set.cxx IMMEDIATE)
  SET (KWWidgets_SRCS 
    ${KWWidgets_SRCS} ${KWWidgets_BINARY_DIR}/Templates/${WidgetType}Set.cxx)
ENDFOREACH (WidgetType)

# The "labeled widgets" are automatically generated from some core widgets
# (i.e., vtkKWCheckButton => vtkKWCheckButtonLabeled)
# If you modify this list, please update the README.txt file accordingly.

FOREACH (WidgetType 
    vtkKWCheckButton 
    vtkKWCheckButtonSet
    vtkKWEntry
    vtkKWLabel
    vtkKWLoadSaveButton
    vtkKWOptionMenu
    vtkKWPopupButton
    vtkKWPushButton
    vtkKWPushButtonSet
    vtkKWText
    vtkKWTextWithScrollbars
    vtkKWRadioButtonSet
    vtkKWScaleSet
    )
  SET (WidgetType ${WidgetType})
  CONFIGURE_FILE (
    ${KWWidgets_SOURCE_DIR}/Templates/vtkKWWidgetLabeledSubclass.h.in 
    ${KWWidgets_BINARY_DIR}/Templates/${WidgetType}Labeled.h IMMEDIATE)
  CONFIGURE_FILE (
    ${KWWidgets_SOURCE_DIR}/Templates/vtkKWWidgetLabeledSubclass.cxx.in 
    ${KWWidgets_BINARY_DIR}/Templates/${WidgetType}Labeled.cxx IMMEDIATE)
  SET (KWWidgets_SRCS 
    ${KWWidgets_SRCS} ${KWWidgets_BINARY_DIR}/Templates/${WidgetType}Labeled.cxx)
ENDFOREACH (WidgetType)

# The "widgets with scrollbars" are automatically generated from some core
# widgets
# (i.e., vtkKWText => vtkKWTextWithScrollbars)
# If you modify this list, please update the README.txt file accordingly.

FOREACH (WidgetType 
    vtkKWCanvas
    vtkKWListBox
    vtkKWMultiColumnList
    vtkKWText 
    )
  SET (WidgetType ${WidgetType})
  CONFIGURE_FILE (
    ${KWWidgets_SOURCE_DIR}/Templates/vtkKWWidgetWithScrollbarsSubclass.h.in 
    ${KWWidgets_BINARY_DIR}/Templates/${WidgetType}WithScrollbars.h IMMEDIATE)
  CONFIGURE_FILE (
    ${KWWidgets_SOURCE_DIR}/Templates/vtkKWWidgetWithScrollbarsSubclass.cxx.in 
    ${KWWidgets_BINARY_DIR}/Templates/${WidgetType}WithScrollbars.cxx IMMEDIATE)
  SET (KWWidgets_SRCS 
    ${KWWidgets_SRCS} ${KWWidgets_BINARY_DIR}/Templates/${WidgetType}WithScrollbars.cxx)
ENDFOREACH (WidgetType)

# Include dirs

INCLUDE_DIRECTORIES(
  ${KWWidgets_INCLUDE_PATH}
  ${VTK_INCLUDE_DIR}
  )

# Wrap the classes in Tcl

SET(VTK_WRAP_HINTS ${KWWidgets_SOURCE_DIR}/hints)
SET(VTK_WRAP_TCL3_INIT_DIR "${VTK_SOURCE_DIR}/Wrapping")
INCLUDE("${VTK_SOURCE_DIR}/CMake/vtkWrapTcl.cmake")
VTK_WRAP_TCL3(KWWidgets KWWidgetsTCL_SRCS "${KWWidgets_SRCS}" "vtkKWWidgetsInitializeCommand" "${KWWIDGETS_MAJOR_VERSION}.${KWWIDGETS_MINOR_VERSION}")

SUBDIRS(Wrapping/Tcl)

# Build the library

ADD_LIBRARY(KWWidgets ${KWWidgetsTCL_SRCS} ${KWWidgets_SRCS})

IF(VTK_WRAP_TCL)
  TARGET_LINK_LIBRARIES (KWWidgets
    vtkHybridTCL
    vtkCommonTCL
    )
ELSE(VTK_WRAP_TCL)
  TARGET_LINK_LIBRARIES (KWWidgets
    vtkHybrid
    vtkCommonTCL
    )
ENDIF(VTK_WRAP_TCL)
TARGET_LINK_LIBRARIES (KWWidgets vtkpng)

# We need KWSys

TARGET_LINK_LIBRARIES (KWWidgets vtksys)

# If needed, copy the Tcl/Tk support files needed at run-time 
# to initialize Tcl/Tk

IF (VTK_TCL_TK_COPY_SUPPORT_LIBRARY)
  IF (VTK_TCL_SUPPORT_LIBRARY_PATH AND VTK_TK_SUPPORT_LIBRARY_PATH)
    INCLUDE(${VTK_TCL_TK_MACROS})
    VTK_COPY_TCL_TK_SUPPORT_FILES_TO_DIR(
      ${VTK_TCL_SUPPORT_LIBRARY_PATH}
      ${VTK_TK_SUPPORT_LIBRARY_PATH}
      "${PROJECT_BINARY_DIR}/lib")
    VTK_COPY_TCL_TK_SUPPORT_FILES_TO_DIR(
      ${VTK_TCL_SUPPORT_LIBRARY_PATH}
      ${VTK_TK_SUPPORT_LIBRARY_PATH}
      "${KW_INSTALL_LIB_DIR}/TclTk/lib"
      INSTALL)
  ENDIF (VTK_TCL_SUPPORT_LIBRARY_PATH AND VTK_TK_SUPPORT_LIBRARY_PATH)
ENDIF (VTK_TCL_TK_COPY_SUPPORT_LIBRARY)

# Testing ?

IF (BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF (BUILD_TESTING)

# Examples ?

IF (DEFINED BUILD_EXAMPLES)
ELSE (DEFINED BUILD_EXAMPLES)
  OPTION(BUILD_EXAMPLES "Build KWWidgets examples." ON)
  MARK_AS_ADVANCED(BUILD_EXAMPLES)
ENDIF (DEFINED BUILD_EXAMPLES)
IF(BUILD_TESTING)
  SET(BUILD_EXAMPLES ON)
ENDIF(BUILD_TESTING)
IF (BUILD_EXAMPLES)
  SUBDIRS(Examples)
ENDIF (BUILD_EXAMPLES)

# Build the utilities

SUBDIRS(Utilities)

# Install the library

IF(BUILD_SHARED_LIBS)
  INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} KWWidgets)
ENDIF(BUILD_SHARED_LIBS)

# HTML Help can help
# http://msdn.microsoft.com/
# http://msdn.microsoft.com/library/default.asp?url=/library/en-us/htmlhelp/html/vsconwhtshw.asp

IF (WIN32)
  IF(HTML_HELP_INCLUDE_PATH AND HTML_HELP_LIBRARY)
  ELSE(HTML_HELP_INCLUDE_PATH AND HTML_HELP_LIBRARY)
    INCLUDE(FindHTMLHelp)
  ENDIF(HTML_HELP_INCLUDE_PATH AND HTML_HELP_LIBRARY)
  IF(HTML_HELP_INCLUDE_PATH AND HTML_HELP_LIBRARY)
    INCLUDE_DIRECTORIES(${HTML_HELP_INCLUDE_PATH})
    LINK_LIBRARIES( ${HTML_HELP_LIBRARY} )
    SET (KWWIDGETS_HAS_HTML_HELP 1)
  ELSE(HTML_HELP_INCLUDE_PATH AND HTML_HELP_LIBRARY)
    SET (KWWIDGETS_HAS_HTML_HELP 0)
  ENDIF(HTML_HELP_INCLUDE_PATH AND HTML_HELP_LIBRARY)
ENDIF (WIN32)

# Configure some build settings

SET (KWWIDGETS_SHARED_LIBS ${BUILD_SHARED_LIBS})
CONFIGURE_FILE(
  ${KWWidgets_SOURCE_DIR}/Templates/vtkKWWidgetsConfigure.h.in 
  ${KWWidgets_BINARY_DIR}/vtkKWWidgetsConfigure.h)

# The Resources/KWWidgets.rc.in can be configured for examples/samples
# or use apps. Here are some defaults values. Typically, you should then use:
# CONFIGURE_FILE(${KWWidgets_SOURCE_DIR}/Resources/KWWidgets.rc.in foo.rc)
# ADD_EXECUTABLE(... foo.rc)

IF(WIN32)
  IF(VTK_TK_RESOURCE_FILE)
    GET_FILENAME_COMPONENT(TK_RESOURCE_PATH ${VTK_TK_RESOURCE_FILE} PATH)
    INCLUDE_DIRECTORIES(${TK_RESOURCE_PATH})
  ENDIF(VTK_TK_RESOURCE_FILE)
  SET (KWWIDGETS_RES_MAJOR_VERSION "${KWWIDGETS_MAJOR_VERSION}")
  SET (KWWIDGETS_RES_MINOR_VERSION "${KWWIDGETS_MINOR_VERSION}")
  SET (KWWIDGETS_RES_APP_NAME "Unknown")
  SET (KWWIDGETS_RES_FILE_NAME "Unknown")
  SET (KWWIDGETS_RES_COMPANY_NAME "Kitware Inc.")
ENDIF(WIN32)
