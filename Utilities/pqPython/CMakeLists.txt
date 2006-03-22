PROJECT(pqPython)

FIND_PACKAGE(PythonLibs)

INCLUDE_DIRECTORIES(
 ${ParaQ_SOURCE_DIR}/VTK/Common
 ${ParaQ_BINARY_DIR}/VTK
 ${PYTHON_INCLUDE_PATH}
)

ADD_LIBRARY(pqPython
  pqPythonExport.h
  pqPythonInterpreter.cxx
  pqPythonInterpreter.h
)

IF (WIN32)
  TARGET_LINK_LIBRARIES(pqPython
    ${PYTHON_LIBRARIES}
  )
ELSE (WIN32)
  TARGET_LINK_LIBRARIES(pqPython
    ${PYTHON_LIBRARIES}
    ${PYTHON_UTIL_LIBRARY}
  )
ENDIF (WIN32)
