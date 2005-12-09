SET(KWWidgets_SHARED_LIBS ${BUILD_SHARED_LIBS})

CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkKWWidgetsConfigure.h.in 
  ${KWWidgets_BINARY_DIR}/vtkKWWidgetsConfigure.h)

IF(NOT KWWidgets_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${KWWidgets_INSTALL_INCLUDE_DIR} FILES 
    ${KWWidgets_BINARY_DIR}/vtkKWWidgetsConfigure.h)
  INSTALL_FILES(${KWWidgets_INSTALL_INCLUDE_DIR}/Templates FILES 
    vtkKWWidgetsPaths.h.in
    vtkKWWidgetSetSubclass.cxx.in
    vtkKWWidgetSetSubclass.h.in
    vtkKWWidgetWithLabelSubclass.cxx.in
    vtkKWWidgetWithLabelSubclass.h.in
    vtkKWWidgetWithScrollbarsSubclass.cxx.in
    vtkKWWidgetWithScrollbarsSubclass.h.in
    vtkKWWidgetWithSpinButtonsSubclass.cxx.in
    vtkKWWidgetWithSpinButtonsSubclass.h.in)
ENDIF(NOT KWWidgets_INSTALL_NO_DEVELOPMENT)
