IF(NOT KWWidgets_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${KWWidgets_INSTALL_INCLUDE_DIR}/Templates FILES 
    vtkKWWidgetSetSubclass.cxx.in
    vtkKWWidgetSetSubclass.h.in
    vtkKWWidgetWithLabelSubclass.cxx.in
    vtkKWWidgetWithLabelSubclass.h.in
    vtkKWWidgetWithScrollbarsSubclass.cxx.in
    vtkKWWidgetWithScrollbarsSubclass.h.in
    vtkKWWidgetWithSpinButtonsSubclass.cxx.in
    vtkKWWidgetWithSpinButtonsSubclass.h.in)
ENDIF(NOT KWWidgets_INSTALL_NO_DEVELOPMENT)
