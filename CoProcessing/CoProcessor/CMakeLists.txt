project(CoProcessor)
cmake_minimum_required(VERSION 2.6)
include_directories(${CoProcessor_SOURCE_DIR})

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    ${PARAVIEW_KWSYS_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIR}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

if(PARAVIEW_USE_MPI)
  set(COPROCESSOR_USE_MPI 1)
endif(PARAVIEW_USE_MPI)

CONFIGURE_FILE("${CoProcessing_SOURCE_DIR}/CoProcessor/CPSystemInformation.h.in"
               "${CoProcessing_BINARY_DIR}/CoProcessor/CPSystemInformation.h"
               @ONLY IMMEDIATE)
include_directories(
  ${CoProcessing_BINARY_DIR}/CoProcessor
)

set(CoProcessor_SRCS
  vtkCPDataDescription.cxx
  vtkCPInputDataDescription.cxx
  vtkCPPipeline.cxx
  vtkCPProcessor.cxx
)
IF (PARAVIEW_ENABLE_PYTHON)
  set(CoProcessor_SRCS
    ${CoProcessor_SRCS}
    vtkCPPythonHelper.cxx
    vtkCPPythonScriptPipeline.cxx
  )
ENDIF (PARAVIEW_ENABLE_PYTHON)

set_source_files_properties(
  vtkCPPipeline
  ABSTRACT)
IF (PARAVIEW_ENABLE_PYTHON)
  set_source_files_properties(
    vtkCPProcessModulePythonHelper
    vtkCPPythonHelper
    WRAP_EXCLUDE)
ENDIF (PARAVIEW_ENABLE_PYTHON)
add_library(vtkCoProcessor ${CoProcessor_SRCS})
set(CoProcessor_LIBS
  vtkCommonCore
  vtkFiltersCore
  vtkPVVTKExtensions
  vtkPVServerManager
  )
IF (PARAVIEW_ENABLE_PYTHON)
  set(CoProcessor_LIBS
    ${CoProcessor_LIBS}
    vtkPVPython
  )
ENDIF (PARAVIEW_ENABLE_PYTHON)
target_link_libraries(vtkCoProcessor ${CoProcessor_LIBS})

IF (PARAVIEW_ENABLE_PYTHON)
  IF(NOT APPLE)
    FIND_LIBRARY(PYTHON_UTIL_LIBRARY
      NAMES util
      DOC "Utility library needed for pvpython"
      )
    MARK_AS_ADVANCED(PYTHON_UTIL_LIBRARY)
    IF(PYTHON_UTIL_LIBRARY)
      SET(PYTHON_UTIL_LIBRARY_LIB ${PYTHON_UTIL_LIBRARY})
    ENDIF(PYTHON_UTIL_LIBRARY)
  ENDIF(NOT APPLE)
  # Create Python bindings for the co-processing library
  set(BUILD_SHARED_LIBS ON CACHE BOOL "Needed for python scripting.")
  INCLUDE(FindPythonLibs)
  SET(VTK_WRAP_INCLUDE_DIRS ${VTK_INCLUDE_DIR})
  INCLUDE(${VTK_CMAKE_DIR}/vtkWrapPython.cmake)

  VTK_WRAP_PYTHON3(vtkCoProcessorPython CPPython_SRCS "${CoProcessor_SRCS}")

  # Create a shared library containing the python wrappers.  Executables
  # can link to this but it is not directly loaded dynamically as a
  # module.
  ADD_LIBRARY(vtkCoProcessorPythonD ${CPPython_SRCS})
  TARGET_LINK_LIBRARIES(vtkCoProcessorPythonD vtkCoProcessor vtkIOCorePythonD)

  # On some UNIX platforms the python library is static and therefore
  # should not be linked into the shared library.  Instead the symbols
  # are exported from the python executable so that they can be used by
  # shared libraries that are linked or loaded.  On Windows and OSX we
  # want to link to the python libray to resolve its symbols
  # immediately.
  IF(WIN32 OR APPLE)
    TARGET_LINK_LIBRARIES (vtkCoProcessorPythonD ${VTK_PYTHON_LIBRARIES} ${PYTHON_UTIL_LIBRARY_LIB})
  ENDIF(WIN32 OR APPLE)

  # Create a python module that can be loaded dynamically.  It links to
  # the shared library containing the wrappers for this kit.
  PYTHON_ADD_MODULE(vtkCoProcessorPython vtkCoProcessorPythonInit.cxx)
  TARGET_LINK_LIBRARIES(vtkCoProcessorPython vtkCoProcessorPythonD)

  # Python extension modules on Windows must have the extension ".pyd"
  # instead of ".dll" as of Python 2.5.  Older python versions do support
  # this suffix.
  IF(WIN32 AND NOT CYGWIN)
    SET_TARGET_PROPERTIES(vtkCoProcessorPython PROPERTIES SUFFIX ".pyd")
  ENDIF(WIN32 AND NOT CYGWIN)

  IF(WIN32 OR APPLE)
    TARGET_LINK_LIBRARIES (vtkCoProcessorPython ${VTK_PYTHON_LIBRARIES} ${PYTHON_UTIL_LIBRARY_LIB})
  ENDIF(WIN32 OR APPLE)

  SET(PYTHON_MODULE_PREFIX "")
  PYTHON_WRITE_MODULES_HEADER(${CMAKE_CURRENT_BINARY_DIR}/cppythonmodules.h)
  GET_PROPERTY(PY_STATIC_MODULES_LIST GLOBAL PROPERTY PY_STATIC_MODULES_LIST)
  IF(PY_STATIC_MODULES_LIST)
    TARGET_LINK_LIBRARIES(vtkCoProcessor ${PY_STATIC_MODULES_LIST})
  ENDIF(PY_STATIC_MODULES_LIST)

ENDIF (PARAVIEW_ENABLE_PYTHON)

# Install
#----------------------------------------------------------------------------------------------------
# Libraries
set(CoProcessor_INSTALL
  vtkCoProcessor
  )

IF (PARAVIEW_ENABLE_PYTHON)
  set(CoProcessor_INSTALL
    ${CoProcessor_INSTALL}
    vtkCoProcessorPython 
    vtkCoProcessorPythonD
    )
ENDIF (PARAVIEW_ENABLE_PYTHON)

IF(NOT PV_INSTALL_NO_LIBRARIES)
  INSTALL(TARGETS ${CoProcessor_INSTALL}
      RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
      ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development)
ENDIF(NOT PV_INSTALL_NO_LIBRARIES)
#
# Development
IF (NOT PV_INSTALL_NO_DEVELOPMENT)
  GLOB_INSTALL_DEVELOPMENT(
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${PV_INSTALL_INCLUDE_DIR}
      "*.h;*.hxx;*.txx")
ENDIF (NOT PV_INSTALL_NO_DEVELOPMENT)

if(BUILD_TESTING)
  add_subdirectory(Testing)
endif(BUILD_TESTING)

