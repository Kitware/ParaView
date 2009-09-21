PROJECT(QtPython)

# Any build configured headers should be added to this 
# variable so that they also get installed.
SET (dynamicHeaders)

INCLUDE_DIRECTORIES(
  ${pqCore_SOURCE_DIR}
  ${pqCore_BINARY_DIR}
  ${QtWidgets_SOURCE_DIR}
  ${QtWidgets_BINARY_DIR}
  ${QtPython_BINARY_DIR}
  ${VTK_INCLUDE_DIR}
  ${PYTHON_INCLUDE_PATH}
  ${PARAVIEW_INCLUDE_DIRS}
  ${VTKPythonWrapping_SOURCE_DIR}/Executable
  ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt
)

# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")

QT4_WRAP_CPP(QtPython_MOC_BUILT_SOURCES
  pqPythonDialog.h
  pqPythonMacroSupervisor.h
  pqPythonManager.h
  pqPythonScriptEditor.h
  pqPythonShell.h
  pqPythonToolsWidget.h
)

QT4_WRAP_UI(QtPython_UI_BUILT_SOURCES
  pqPythonDialog.ui
  pqPythonToolsWidget.ui
)

SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

ADD_LIBRARY(QtPython
  pqPythonDialog.cxx
  pqPythonDialog.h
  pqPythonMacroSupervisor.cxx
  pqPythonMacroSupervisor.h
  pqPythonManager.cxx
  pqPythonManager.h
  pqPythonScriptEditor.cxx
  pqPythonScriptEditor.h
  pqPythonShell.cxx
  pqPythonShell.h
  pqPythonToolsWidget.cxx
  pqPythonToolsWidget.h
  QtPythonExport.h
  ${QtPython_MOC_BUILT_SOURCES}
  ${QtPython_UI_BUILT_SOURCES}
)

TARGET_LINK_LIBRARIES(QtPython
  pqCore
  pqWidgets
  ${QT_LIBRARIES}
  ${PYTHON_LIBRARIES}
  vtkPVPythonInterpretor
)

CONFIGURE_FILE(
  ${QtPython_SOURCE_DIR}/QtPythonConfig.h.in
  ${QtPython_BINARY_DIR}/QtPythonConfig.h)
SET(dynamicHeaders 
    "${dynamicHeaders};${QtPython_BINARY_DIR}/QtPythonConfig.h")

SOURCE_GROUP("Generated Files" FILES
  ${QtPython_MOC_BUILT_SOURCES}
  ${QtPython_UI_BUILT_SOURCES})

# Install
#--------------------------------------------------------------------------------------------------
# Libraries
IF(NOT PV_INSTALL_NO_LIBRARIES)
  SET (targets_to_install QtPython)
  INSTALL(TARGETS ${targets_to_install}
    RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
    LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
    ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development)
ENDIF(NOT PV_INSTALL_NO_LIBRARIES)
# Development
IF (NOT PV_INSTALL_NO_DEVELOPMENT)
  # Headers
  GLOB_INSTALL_DEVELOPMENT(
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${PV_INSTALL_INCLUDE_DIR} 
      "*.h;*.hxx;*.txx")
  INSTALL(
      FILES ${dynamicHeaders}
      DESTINATION ${PV_INSTALL_INCLUDE_DIR}
      COMPONENT Development)
  # QT resources
  GLOB_INSTALL_DEVELOPMENT( 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PV_INSTALL_INCLUDE_DIR}/Qt/Python/Resources
    "*.ui;*.ico;*.png;*.svg;*.xpm;*.rc;*.qrc")
ENDIF (NOT PV_INSTALL_NO_DEVELOPMENT)
