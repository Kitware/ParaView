PROJECT(QtPython)

INCLUDE_DIRECTORIES(
  ${pqPython_SOURCE_DIR}
  ${QtWidgets_SOURCE_DIR}
  ${QtPython_BINARY_DIR}
  ${PYTHON_INCLUDE_PATH}
)

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  pqPythonShell.h
  pqPythonStream.h
)

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

