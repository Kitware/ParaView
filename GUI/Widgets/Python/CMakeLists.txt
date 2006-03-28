PROJECT(QtPython)

INCLUDE_DIRECTORIES(
  ${pqPython_SOURCE_DIR}
  ${QtWidgets_SOURCE_DIR}
  ${QtPython_BINARY_DIR}
  ${PYTHON_INCLUDE_PATH}
)

# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  pqPythonShell.h
  pqPythonStream.h
)

SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

ADD_LIBRARY(QtPython
  pqPythonShell.cxx
  pqPythonShell.h
  pqPythonStream.cxx
  pqPythonStream.h
  QtPythonExport.h
  ${MOC_BUILT_SOURCES}
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
  ${MOC_BUILT_SOURCES}
)

