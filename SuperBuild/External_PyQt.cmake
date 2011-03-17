# The PyQt external project

ExternalProject_Add(PyQt
  URL ${PYQT_URL}/${PYQT_GZ}
  URL_MD5 ${PYQT_MD5}
  DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/PyQt
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND ${PYTHON_EXECUTABLE} configure.py -q ${QT_QMAKE_EXECUTABLE} --confirm-license
  DEPENDS
    ${PyQt_dependencies}
  )
