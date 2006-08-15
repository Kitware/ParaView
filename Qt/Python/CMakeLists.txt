PROJECT(QtPython)

INCLUDE_DIRECTORIES(
  ${pqPython_SOURCE_DIR}
  ${QtWidgets_SOURCE_DIR}
  ${QtPython_BINARY_DIR}
  ${VTK_INCLUDE_DIR}
  ${PYTHON_INCLUDE_PATH}
)

# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")

QT4_WRAP_CPP(QtPython_MOC_BUILT_SOURCES
  pqPythonDialog.h
  pqPythonShell.h
  pqPythonStream.h
)

QT4_WRAP_UI(QtPython_UI_BUILT_SOURCES
  pqPythonDialog.ui
)

SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

ADD_LIBRARY(QtPython
  pqPythonDialog.cxx
  pqPythonDialog.h
  pqPythonShell.cxx
  pqPythonShell.h
  pqPythonStream.cxx
  pqPythonStream.h
  QtPythonExport.h
  ${QtPython_MOC_BUILT_SOURCES}
  ${QtPython_UI_BUILT_SOURCES}
)

TARGET_LINK_LIBRARIES(QtPython
  pqPython
  QtWidgets
  ${QT_LIBRARIES}
  ${PYTHON_LIBRARIES}
)

CONFIGURE_FILE(
  ${QtPython_SOURCE_DIR}/QtPythonConfig.h.in
  ${QtPython_BINARY_DIR}/QtPythonConfig.h
)

SOURCE_GROUP("Generated Files" FILES
  ${QtPython_MOC_BUILT_SOURCES}
  ${QtPython_UI_BUILT_SOURCES}
)

IF(NOT PV_INSTALL_NO_LIBRARIES)
  SET (targets_to_install QtPython)
    
  IF(PV_INSTALL_HAS_CMAKE_24)
    INSTALL(TARGETS ${targets_to_install}
      RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime
      LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime
      ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Development)
  ELSE(PV_INSTALL_HAS_CMAKE_24)
    INSTALL_TARGETS(${PV_INSTALL_LIB_DIR} ${targets_to_install})
  ENDIF(PV_INSTALL_HAS_CMAKE_24)

ENDIF(NOT PV_INSTALL_NO_LIBRARIES)


