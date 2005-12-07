IF(WIN32)
  IF(NOT KWWidgets_INSTALL_NO_DEVELOPMENT)
    INSTALL_FILES(${KWWidgets_INSTALL_DATA_DIR}/Resources FILES 
      ${KWWidgets_RESOURCE_DIR}/KWWidgets.rc.in)
    INSTALL_FILES(${KWWidgets_INSTALL_INCLUDE_DIR}/Resources FILES 
      ${KWWidgets_RESOURCE_DIR}/KWWidgets.rc.h
      ${KWWidgets_RESOURCE_DIR}/KWWidgetsIcon16.ico
      ${KWWidgets_RESOURCE_DIR}/KWWidgetsIcon32.ico)
  ENDIF(NOT KWWidgets_INSTALL_NO_DEVELOPMENT)
ENDIF(WIN32)
