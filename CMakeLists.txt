PROJECT(ParaQ)
CMAKE_MINIMUM_REQUIRED(VERSION 2.2)

#########################################################################
# Disallow in-source build
STRING(COMPARE EQUAL "${ParaQ_SOURCE_DIR}" 
  "${ParaQ_BINARY_DIR}" INSOURCE)
IF(INSOURCE)
  MESSAGE(FATAL_ERROR "ParaQ requires an out of source Build. Please create a separate binary directory and run CMake there.")
ENDIF(INSOURCE)

#########################################################################
SET(PARAQ_VERSION_MAJOR 3)
SET(PARAQ_VERSION_MINOR 0)
SET(PARAQ_VERSION_PATCH 0)
SET(PARAQ_VERSION "${PARAQ_VERSION_MAJOR}.${PARAQ_VERSION_MINOR}")
SET(PARAQ_VERSION_FULL "${PARAQ_VERSION}.${PARAQ_VERSION_PATCH}")

# See VTK/CMakeLists.txt for an explanation of this default selection.
IF(PARAQ_VERSION_MINOR MATCHES "[02468]$")
  # This is a release version.  Default to not use rpath.
  SET(VTK_USE_RPATH_DEFAULT OFF)
ELSE(PARAQ_VERSION_MINOR MATCHES "[02468]$")
  # This is a development version.  Default to use rpath.
  SET(VTK_USE_RPATH_DEFAULT ON)
ENDIF(PARAQ_VERSION_MINOR MATCHES "[02468]$")

#########################################################################
# build VTK/Qt support
OPTION(PARAQ_BUILD_GUI "Build ParaQ Client. This requires Qt." ON)
MARK_AS_ADVANCED(PARAQ_BUILD_GUI)
IF (PARAQ_BUILD_GUI)
  SET(VTK_USE_GUISUPPORT ON CACHE BOOL "Build VTK with GUI Support" FORCE)
  SET(VTK_USE_QVTK ON CACHE BOOL "Build VTK with Qt Support" FORCE)
  SET(DESIRED_QT_VERSION 4 CACHE STRING "Use Qt4" FORCE)
ENDIF (PARAQ_BUILD_GUI)


# Work around CMake 2.2.x FindPythonLibs bug.
# Do not find Python subdirectory as python debug library.
SET(PYTHON_DEBUG_LIBRARY "" CACHE FILEPATH "Purposely empty: work around CMake FindPythonLibs bug")


#########################################################################
# Configure Testing
OPTION(BUILD_TESTING "Build ParaQ Testing" ON)
IF(BUILD_TESTING)
  SET(PARAQ_TEST_DIR    ${ParaQ_BINARY_DIR}/Testing/Temporary)
  MAKE_DIRECTORY(${PARAQ_TEST_DIR})
  ENABLE_TESTING()
  INCLUDE (CTest)
ENDIF(BUILD_TESTING)

#########################################################################
# Configure data directory
FIND_PATH(PARAQ_DATA_ROOT ParaQData.readme ${ParaQ_SOURCE_DIR}/../ParaQData $ENV{PARAQ_DATA_ROOT})

#########################################################################
# Configure Python
OPTION(PARAQ_EMBED_PYTHON "Embedded Python Interpreter" ON)

IF(PARAQ_EMBED_PYTHON)
  SUBDIRS(Utilities/pqPython)
  ADD_DEFINITIONS(-DPARAQ_EMBED_PYTHON)

  SET(BUILD_SHARED_LIBS ON CACHE BOOL "Build ParaQ using shared libraries" FORCE)
  SET(PARAVIEW_WRAP_PYTHON ON CACHE BOOL "Wrap ParaView server manager into Python" FORCE)
ENDIF(PARAQ_EMBED_PYTHON)

#########################################################################
IF(BUILD_SHARED_LIBS)
  ADD_DEFINITIONS(-DPARAQ_BUILD_SHARED_LIBS)
ENDIF(BUILD_SHARED_LIBS)


#########################################################################
# Set the ServerManager test data dir,
SET(PVServerManagerTestData ${PARAQ_DATA_ROOT})

#########################################################################
# Include the file that most of the environment setup (except GUI).
SET(ParaView_SOURCE_DIR ${ParaQ_SOURCE_DIR})
SET(ParaView_BINARY_DIR ${ParaQ_BINARY_DIR})
INCLUDE(${ParaQ_SOURCE_DIR}/CMake/ParaViewCommon.cmake)

#########################################################################
IF(BUILD_DOCUMENTATION)
  SUBDIRS(Utilities/Doxygen)
ENDIF(BUILD_DOCUMENTATION)

#########################################################################
CONFIGURE_FILE(${ParaView_SOURCE_DIR}/vtkPQConfig.h.in
  ${ParaView_BINARY_DIR}/vtkPQConfig.h
  ESCAPE_QUOTES)

#########################################################################
IF (PARAQ_BUILD_GUI)
  SUBDIRS(GUI)
ENDIF (PARAQ_BUILD_GUI)

#########################################################################
OPTION(PARAQ_BUILD_APPLICATIONS "Build Applications suite." ON)
IF (PARAQ_BUILD_APPLICATIONS)
  SUBDIRS(Applications)
ENDIF (PARAQ_BUILD_APPLICATIONS)

# Find hints file for the client/server wrapping
FIND_FILE(VTK_WRAP_HINTS hints ${ParaView_SOURCE_DIR}/VTK/Wrapping)
MARK_AS_ADVANCED(VTK_WRAP_HINTS)
