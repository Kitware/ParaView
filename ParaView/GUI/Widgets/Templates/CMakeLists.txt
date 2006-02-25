CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkKWWidgetsConfigure.h.in 
  ${KWWidgets_BINARY_DIR}/vtkKWWidgetsConfigure.h)

IF(NOT KWWidgets_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${KWWidgets_INSTALL_INCLUDE_DIR} FILES 
    ${KWWidgets_BINARY_DIR}/vtkKWWidgetsConfigure.h)
  INSTALL_FILES(${KWWidgets_INSTALL_INCLUDE_DIR}/Templates FILES 
    KWWidgetsSetupPaths.bat.in
    KWWidgetsSetupPaths.cmake.in
    KWWidgetsSetupPaths.csh.in
    KWWidgetsSetupPaths.sh.in
    KWWidgetsSetupPathsLauncher.c.in
    vtkKWWidgetSetSubclass.cxx.in
    vtkKWWidgetSetSubclass.h.in
    vtkKWWidgetsPaths.h.in
    vtkKWWidgetWithLabelSubclass.cxx.in
    vtkKWWidgetWithLabelSubclass.h.in
    vtkKWWidgetWithScrollbarsSubclass.cxx.in
    vtkKWWidgetWithScrollbarsSubclass.h.in
    vtkKWWidgetWithSpinButtonsSubclass.cxx.in
    vtkKWWidgetWithSpinButtonsSubclass.h.in)
ENDIF(NOT KWWidgets_INSTALL_NO_DEVELOPMENT)
