PROJECT(ParaView)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

#########################################################################
# Disallow in-source build
STRING(COMPARE EQUAL "${ParaView_SOURCE_DIR}" 
  "${ParaView_BINARY_DIR}" INSOURCE)
IF(INSOURCE)
  MESSAGE(FATAL_ERROR "ParaView requires an out of source Build. Please create a separate binary directory and run CMake there.")
ENDIF(INSOURCE)

#########################################################################
SET(PARAVIEW_VERSION_MAJOR 2)
SET(PARAVIEW_VERSION_MINOR 9)
SET(PARAVIEW_VERSION_PATCH 0)
SET(PARAVIEW_VERSION "${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}")
SET(PARAVIEW_VERSION_FULL "${PARAVIEW_VERSION}.${PARAVIEW_VERSION_PATCH}")

# See VTK/CMakeLists.txt for an explanation of this default selection.
IF(PARAVIEW_VERSION_MINOR MATCHES "[02468]$")
  # This is a release version.  Default to not use rpath.
  SET(VTK_USE_RPATH_DEFAULT OFF)
ELSE(PARAVIEW_VERSION_MINOR MATCHES "[02468]$")
  # This is a development version.  Default to use rpath.
  SET(VTK_USE_RPATH_DEFAULT ON)
ENDIF(PARAVIEW_VERSION_MINOR MATCHES "[02468]$")

#########################################################################
# build VTK/Qt support
OPTION(PARAVIEW_BUILD_QT_GUI "Build ParaView Qt Client. This requires Qt." ON)
MARK_AS_ADVANCED(PARAVIEW_BUILD_QT_GUI)
IF (PARAVIEW_BUILD_QT_GUI)
  SET(VTK_USE_GUISUPPORT ON CACHE BOOL "Build VTK with GUI Support" FORCE)
  SET(VTK_USE_QVTK ON CACHE BOOL "Build VTK with Qt Support" FORCE)
  SET(DESIRED_QT_VERSION 4 CACHE STRING "Use Qt4" FORCE)
ENDIF (PARAVIEW_BUILD_QT_GUI)


# Work around CMake 2.2.x FindPythonLibs bug.
# Do not find Python subdirectory as python debug library.
SET(PYTHON_DEBUG_LIBRARY "" CACHE FILEPATH "Purposely empty: work around CMake FindPythonLibs bug")


#########################################################################
# Configure Testing
OPTION(BUILD_TESTING "Build ParaView Testing" ON)
IF(BUILD_TESTING)
  SET(PARAVIEW_TEST_DIR    ${ParaView_BINARY_DIR}/Testing/Temporary)
  MAKE_DIRECTORY(${PARAVIEW_TEST_DIR})
  ENABLE_TESTING()
  INCLUDE (CTest)
ENDIF(BUILD_TESTING)

#########################################################################
# Configure data directory
IF(NOT PARAVIEW_DATA_ROOT AND PARAQ_DATA_ROOT)
  SET(PARAVIEW_DATA_ROOT "${PARAQ_DATA_ROOT}")
ENDIF(NOT PARAVIEW_DATA_ROOT AND PARAQ_DATA_ROOT)
FIND_PATH(PARAVIEW_DATA_ROOT ParaViewData.readme
  ${ParaView_SOURCE_DIR}/ParaViewData
  ${ParaView_SOURCE_DIR}/../ParaViewData
  ${ParaView_SOURCE_DIR}/ParaQData
  ${ParaView_SOURCE_DIR}/../ParaQData
  $ENV{PARAVIEW_DATA_ROOT}
  )

#########################################################################
# Configure Python
OPTION(PARAVIEW_EMBED_PYTHON "Embedded Python Interpreter" ON)

IF(PARAVIEW_EMBED_PYTHON)

  SUBDIRS(Utilities/pqPython)
  SET(BUILD_SHARED_LIBS ON CACHE BOOL "Build ParaView using shared libraries" FORCE)
  SET(PARAVIEW_WRAP_PYTHON ON CACHE BOOL "Wrap ParaView server manager into Python" FORCE)

ELSE(PARAVIEW_EMBED_PYTHON)

  SET(PARAVIEW_WRAP_PYTHON OFF CACHE BOOL "Wrap ParaView server manager into Python" FORCE)

ENDIF(PARAVIEW_EMBED_PYTHON)

#########################################################################
IF(BUILD_SHARED_LIBS)
  ADD_DEFINITIONS(-DPARAVIEW_BUILD_SHARED_LIBS)
ENDIF(BUILD_SHARED_LIBS)


#########################################################################
# Set the ServerManager test data dir,
SET(PVServerManagerTestData ${PARAVIEW_DATA_ROOT})

#########################################################################
# Include the file that most of the environment setup (except GUI).
SET(ParaView_SOURCE_DIR ${ParaView_SOURCE_DIR})
SET(ParaView_BINARY_DIR ${ParaView_BINARY_DIR})
INCLUDE(${ParaView_SOURCE_DIR}/CMake/ParaViewCommon.cmake)

#########################################################################
IF(BUILD_DOCUMENTATION)
  SUBDIRS(Utilities/Doxygen)
ENDIF(BUILD_DOCUMENTATION)

#########################################################################
CONFIGURE_FILE(${ParaView_SOURCE_DIR}/vtkPQConfig.h.in
  ${ParaView_BINARY_DIR}/vtkPQConfig.h
  ESCAPE_QUOTES)

#########################################################################
IF (PARAVIEW_BUILD_QT_GUI)
  SUBDIRS(GUI)
ENDIF (PARAVIEW_BUILD_QT_GUI)

#########################################################################
IF (PARAVIEW_BUILD_QT_GUI)
SUBDIRS(Applications)
ENDIF (PARAVIEW_BUILD_QT_GUI)

# Find hints file for the client/server wrapping
FIND_FILE(VTK_WRAP_HINTS hints ${ParaView_SOURCE_DIR}/VTK/Wrapping)
MARK_AS_ADVANCED(VTK_WRAP_HINTS)
