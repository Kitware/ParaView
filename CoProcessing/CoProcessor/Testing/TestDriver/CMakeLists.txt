PROJECT(TestDriver)
cmake_minimum_required(VERSION 2.6)

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    ${PARAVIEW_KWSYS_INCLUDE_DIRS}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

include_directories(
  ${TestDriver_SOURCE_DIR} 
)
 
SET(TestDriver_SRCS
  vtkCPBaseFieldBuilder.cxx
  vtkCPBaseGridBuilder.cxx
  vtkCPCellFieldBuilder.cxx
  vtkCPConstantScalarFieldFunction.cxx
  vtkCPFieldBuilder.cxx
  vtkCPFileGridBuilder.cxx
  vtkCPGridBuilder.cxx
  vtkCPLinearScalarFieldFunction.cxx
  vtkCPMultiBlockGridBuilder.cxx
  vtkCPNodalFieldBuilder.cxx
  vtkCPScalarFieldFunction.cxx
  vtkCPTensorFieldFunction.cxx
  vtkCPTestDriver.cxx
  vtkCPUniformGridBuilder.cxx
  vtkCPUnstructuredGridBuilder.cxx
  vtkCPVector3FieldFunction.cxx
)

SET_SOURCE_FILES_PROPERTIES(
  vtkCPBaseFieldBuilder
  vtkCPBaseGridBuilder
  vtkCPFieldBuilder
  vtkCPFileGridBuilder
  vtkCPGridBuilder
  vtkCPScalarFieldFunction
  vtkCPTensorFieldFunction
  vtkCPUnstructuredGridBuilder
  vtkCPVector3FieldFunction
  ABSTRACT
)

ADD_LIBRARY(vtkCPTestDriver
  ${TestDriver_SRCS} 
)
TARGET_LINK_LIBRARIES(vtkCPTestDriver vtkCoProcessor vtkFiltering)

IF (VTK_WRAP_PYTHON)
  # Create Python bindings for the co-processing library
  VTK_WRAP_PYTHON3(vtkCPTestDriverPython TestDriverPython_SRCS "${TestDriver_SRCS}")

  # Create a shared library containing the python wrappers.  Executables
  # can link to this but it is not directly loaded dynamically as a
  # module.
  ADD_LIBRARY(vtkCPTestDriverPythonD ${TestDriverPython_SRCS})
  TARGET_LINK_LIBRARIES(vtkCPTestDriverPythonD vtkCPTestDriver vtkIOPythonD)

  # On some UNIX platforms the python library is static and therefore
  # should not be linked into the shared library.  Instead the symbols
  # are exported from the python executable so that they can be used by
  # shared libraries that are linked or loaded.  On Windows and OSX we
  # want to link to the python libray to resolve its symbols
  # immediately.
  IF(WIN32 OR APPLE)
    TARGET_LINK_LIBRARIES (vtkCPTestDriverPythonD ${PYTHON_LIBRARIES})
  ENDIF(WIN32 OR APPLE)

  # Create a python module that can be loaded dynamically.  It links to
  # the shared library containing the wrappers for this kit.
  PYTHON_ADD_MODULE(vtkCPTestDriverPython vtkCPTestDriverPythonInit.cxx)
  TARGET_LINK_LIBRARIES(vtkCPTestDriverPython vtkCPTestDriverPythonD)

  # Python extension modules on Windows must have the extension ".pyd"
  # instead of ".dll" as of Python 2.5.  Older python versions do support
  # this suffix.
  IF(WIN32 AND NOT CYGWIN)
    SET_TARGET_PROPERTIES(vtkCPTestDriverPython PROPERTIES SUFFIX ".pyd")
  ENDIF(WIN32 AND NOT CYGWIN)

  IF(WIN32 OR APPLE)
    TARGET_LINK_LIBRARIES (vtkCPTestDriverPython ${PYTHON_LIBRARIES})
  ENDIF(WIN32 OR APPLE)
ENDIF (VTK_WRAP_PYTHON)

option(BUILD_TESTING "Build TestDriver Testing" ON)
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif(BUILD_TESTING)


