PROJECT(KWWidgets)

############################################################################
# to use this library in your code you will need to SUBDIR into this
# directory so that it gets built and then you will use the following variables
# in your CMakeLists files to get the proper include paths and libraries
SET (KWWidgets_INCLUDE_PATH 
  "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR}"
  CACHE INTERNAL "include paths for KWWidgets"
  )

SET (KWWidgets_LIBRARIES KWWidgetsTCL
  CACHE INTERNAL "libraries for KWWidgets")
########################################################################

INCLUDE_REGULAR_EXPRESSION("^(vtk|kw|icons).*$")

SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")

SET( KWWidgets_SRCS
  ${VTK_TCL_HOME}/vtktcl.c
  vtkKWApplication.cxx 
  vtkKWApplicationSettingsInterface.cxx
  vtkKWBWidgets.cxx
  vtkKWCanvas.cxx 
  vtkKWChangeColorButton.cxx 
  vtkKWCheckButton.cxx 
  vtkKWCheckButtonWithChangeColor.cxx
  vtkKWColorPresetSelector.cxx
  vtkKWColorTransferFunctionEditor.cxx
  vtkKWCornerAnnotation.cxx 
  vtkKWDialog.cxx 
  vtkKWEntry.cxx 
  vtkKWEvent.cxx
  vtkKWEventMap.cxx
  vtkKWExtent.cxx 
  vtkKWFrame.cxx
  vtkKWFrameLabeled.cxx 
  vtkKWGenericRenderWindowInteractor.cxx
  vtkKWHistogram.cxx
  vtkKWHistogramSet.cxx
  vtkKWHSVColorSelector.cxx
  vtkKWHeaderAnnotation.cxx 
  vtkKWIcon.cxx
  vtkKWLabel.cxx
  vtkKWListBox.cxx 
  vtkKWListSelectOrder.cxx
  vtkKWLoadSaveButton.cxx
  vtkKWLoadSaveDialog.cxx
  vtkKWMaterialPropertyWidget.cxx
  vtkKWMenu.cxx 
  vtkKWMenuButton.cxx 
  vtkKWMessageDialog.cxx 
  vtkKWMostRecentFilesUtilities.cxx
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
  vtkKWRange.cxx 
  vtkKWRegisteryUtilities.cxx
  vtkKWRenderWidget.cxx 
  vtkKWRenderWidgetCallbackCommand.cxx 
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
  vtkKWTextProperty.cxx
  vtkKWThumbWheel.cxx
  vtkKWTkUtilities.cxx
  vtkKWToolbar.cxx 
  vtkKWToolbarSet.cxx 
  vtkKWUserInterfaceManager.cxx
  vtkKWUserInterfaceNotebookManager.cxx
  vtkKWUserInterfacePanel.cxx
  vtkKWVolumeMaterialPropertyWidget.cxx
  vtkKWVolumePropertyWidget.cxx
  vtkKWWidget.cxx 
  vtkKWWidgetCollection.cxx 
  vtkKWWidgetLabeled.cxx 
  vtkKWWidgetSet.cxx 
  vtkKWWindow.cxx 
  vtkKWWindowCollection.cxx 
  )

# The "set of widgets" are automatically generated from some core widgets
# (i.e., CheckButton => vtkKWCheckButtonSet).
# If you modify this list, please update the README.txt file accordingly.

FOREACH (WidgetType 
    CheckButton 
    Entry
    Label
    LabelLabeled
    PushButton
    RadioButton
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

IF (WIN32)
  SET( KWWidgets_SRCS ${KWWidgets_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/Utilities/vtkKWSetApplicationIconTclCommand.c
    vtkKWWin32RegisteryUtilities.cxx
    )
  SET_SOURCE_FILES_PROPERTIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/Utilities/vtkKWSetApplicationIconTclCommand.c
    WRAP_EXCLUDE
    )
ELSE (WIN32)
  SET( KWWidgets_SRCS ${KWWidgets_SRCS}
    vtkKWUNIXRegisteryUtilities.cxx
    )
ENDIF (WIN32)

SET_SOURCE_FILES_PROPERTIES(
  vtkKWMaterialPropertyWidget.cxx
  vtkKWParameterValueFunctionEditor.cxx
  vtkKWParameterValueFunctionInterface.cxx
  vtkKWUserInterfaceManager.cxx
  vtkKWWidgetSet.cxx
  ABSTRACT
  )

SET_SOURCE_FILES_PROPERTIES(
  ${VTK_TCL_HOME}/vtktcl.c
  vtkKWGenericRenderWindowInteractor.cxx
  vtkKWRenderWidgetCallbackCommand.cxx 
  vtkKWTkUtilities.cxx
  vtkKWWidgetCollection.cxx
  vtkKWWindowCollection.cxx
  vtkKWEvent.cxx
  WRAP_EXCLUDE
  )

# If needed, copy the Tcl/Tk support files needed for vtkKWApplication
# at run-time to initialize Tcl/Tk

IF (VTK_TCL_TK_COPY_SUPPORT_LIBRARY)
  IF (VTK_TCL_SUPPORT_LIBRARY_PATH AND VTK_TK_SUPPORT_LIBRARY_PATH)
    INCLUDE(${VTK_TCL_TK_MACROS})

    IF (EXECUTABLE_OUTPUT_PATH)
      VTK_COPY_TCL_TK_SUPPORT_FILES_TO_BUILD_DIR (
        ${VTK_TCL_SUPPORT_LIBRARY_PATH}
        ${VTK_TK_SUPPORT_LIBRARY_PATH}
        ${EXECUTABLE_OUTPUT_PATH} "TclTk/lib")
    ENDIF (EXECUTABLE_OUTPUT_PATH)

    VTK_GET_TCL_TK_VERSION ("TCL_TK_MAJOR_VERSION" "TCL_TK_MINOR_VERSION")
    IF (TCL_TK_MAJOR_VERSION AND TCL_TK_MINOR_VERSION)
      SET (TCL_TK_VERSION "${TCL_TK_MAJOR_VERSION}.${TCL_TK_MINOR_VERSION}")
    ENDIF (TCL_TK_MAJOR_VERSION AND TCL_TK_MINOR_VERSION)

    VTK_GET_TCL_SUPPORT_FILES(${VTK_TCL_SUPPORT_LIBRARY_PATH} "TCL_SUPPORT_FILES")
    IF(KW_INSTALL_LIB_DIR)
      FOREACH (file ${TCL_SUPPORT_FILES})
        STRING (REGEX REPLACE "${VTK_TCL_SUPPORT_LIBRARY_PATH}/" "" base ${file})
        GET_FILENAME_COMPONENT(dest ${base} PATH)
        INSTALL_FILES(${KW_INSTALL_LIB_DIR}/TclTk/lib/tcl${TCL_TK_VERSION}/${dest} FILES ${file})
      ENDFOREACH (file)
    ENDIF(KW_INSTALL_LIB_DIR)

    VTK_GET_TK_SUPPORT_FILES(${VTK_TK_SUPPORT_LIBRARY_PATH} "TK_SUPPORT_FILES")
    IF(KW_INSTALL_LIB_DIR)
      FOREACH (file ${TK_SUPPORT_FILES})
        STRING (REGEX REPLACE "${VTK_TK_SUPPORT_LIBRARY_PATH}/" "" base ${file})
        GET_FILENAME_COMPONENT(dest ${base} PATH)
        INSTALL_FILES(${KW_INSTALL_LIB_DIR}/TclTk/lib/tk${TCL_TK_VERSION}/${dest} FILES ${file})
      ENDFOREACH (file)
    ENDIF(KW_INSTALL_LIB_DIR)

  ENDIF (VTK_TCL_SUPPORT_LIBRARY_PATH AND VTK_TK_SUPPORT_LIBRARY_PATH)
ENDIF (VTK_TCL_TK_COPY_SUPPORT_LIBRARY)

IF(BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(BUILD_TESTING)

SUBDIRS(Utilities)

IF(VTK_WRAP_TCL)
  VTK_WRAP_TCL2(KWWidgetsTCL KWWidgetsTCL_SRCS ${KWWidgets_SRCS})
ELSE(VTK_WRAP_TCL)
  SET(VTK_WRAP_TCL3_INIT_DIR "${VTKTclWrapping_SOURCE_DIR}")
  INCLUDE("${VTKTclWrapping_SOURCE_DIR}/vtkWrapTcl.cmake")
  VTK_WRAP_TCL3(KWWidgetsTCL KWWidgetsTCL_SRCS "${KWWidgets_SRCS}" "")
ENDIF(VTK_WRAP_TCL)

ADD_LIBRARY(KWWidgetsTCL ${KWWidgetsTCL_SRCS} ${KWWidgets_SRCS})

TARGET_LINK_LIBRARIES (KWWidgetsTCL
  ${KWCommon_LIBRARIES}
  )

IF(KWSYS_NAMESPACE)
  TARGET_LINK_LIBRARIES (KWWidgetsTCL ${KWSYS_NAMESPACE})
ENDIF(KWSYS_NAMESPACE)

IF(VTK_WRAP_TCL)
  TARGET_LINK_LIBRARIES (KWWidgetsTCL
    vtkHybridTCL
    vtkCommonTCL
    )
ELSE(VTK_WRAP_TCL)
  TARGET_LINK_LIBRARIES (KWWidgetsTCL
    vtkHybrid
    vtkCommonTCL
    )
ENDIF(VTK_WRAP_TCL)

IF (VTK_USE_PARALLEL AND VTK_WRAP_TCL)
  TARGET_LINK_LIBRARIES(KWWidgetsTCL vtkParallelTCL)
ENDIF (VTK_USE_PARALLEL AND VTK_WRAP_TCL)

IF (VTK_USE_PATENTED AND VTK_WRAP_TCL)
  TARGET_LINK_LIBRARIES(KWWidgetsTCL vtkPatentedTCL)
ENDIF (VTK_USE_PATENTED AND VTK_WRAP_TCL)

SET(VTK_WRAP_HINTS ${KWWidgets_SOURCE_DIR}/hints)

IF(NOT WIN32)
  ADD_DEFINITIONS(-D_HPUX_SOURCE)
ENDIF(NOT WIN32)

OPTION(USE_NOTEBOOK_ICONS "Display Notebook with icons." OFF)
MARK_AS_ADVANCED(USE_NOTEBOOK_ICONS)

CONFIGURE_FILE(
  ${KWWidgets_SOURCE_DIR}/Templates/vtkKWWidgetsConfigure.h.in 
  ${KWWidgets_BINARY_DIR}/vtkKWWidgetsConfigure.h)

INCLUDE_DIRECTORIES(
  ${KWWidgets_BINARY_DIR}
  ${KWWidgets_SOURCE_DIR}
  ${VTK_INCLUDE_DIR}
  ${KWSYS_INCLUDE_DIR}
  )

IF(BUILD_SHARED_LIBS)
  IF(NOT KW_INSTALL_LIB_DIR)
    SET(KW_INSTALL_LIB_DIR /lib)
  ENDIF(NOT KW_INSTALL_LIB_DIR)
  INSTALL_TARGETS(${KW_INSTALL_LIB_DIR} KWWidgetsTCL)
ENDIF(BUILD_SHARED_LIBS)

