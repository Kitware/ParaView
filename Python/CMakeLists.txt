CMAKE_MINIMUM_REQUIRED(VERSION 2.2)

PROJECT(pqPython)

FIND_PACKAGE(PythonLibs)

INCLUDE_DIRECTORIES(
 ${ParaQ_SOURCE_DIR}/ParaView/VTK/Common
 ${ParaQ_BINARY_DIR}/ParaView/VTK
 ${PYTHON_INCLUDE_PATH}
)

ADD_LIBRARY(pqPython
  pqPythonExport.h
  pqPythonInterpreter.cxx
  pqPythonInterpreter.h
)

TARGET_LINK_LIBRARIES(pqPython
  ${PYTHON_LIBRARIES}
  ${PYTHON_UTIL_LIBRARY}
)

