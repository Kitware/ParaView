PROJECT(ParaQ)
CMAKE_MINIMUM_REQUIRED(VERSION 2.2)

# Setup output directories.
SET (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL
  "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL
  "Single output directory for building all executables.")

# build VTK/Qt support
SET(VTK_USE_GUISUPPORT ON CACHE BOOL "Build VTK with GUI Support" FORCE)
SET(VTK_USE_QVTK ON CACHE BOOL "Build VTK with Qt Support" FORCE)
SET(DESIRED_QT_VERSION 4 CACHE STRING "Use Qt4" FORCE)

# turn off building ParaView Tk based GUI
SET(PARAVIEW_BUILD_GUI OFF CACHE BOOL "Build end user applications (ParaView, pvclient, pvbatch)")

SUBDIRS(
  ParaView
  Qt
)

# Configure Testing
OPTION(BUILD_TESTING "Build ParaQ Testing" ON)
IF(BUILD_TESTING)
  SET(PARAQ_TEST_DIR
    ${ParaView_BINARY_DIR}/Testing/Temporary
  )
  MAKE_DIRECTORY(${PARAQ_TEST_DIR})
  ENABLE_TESTING()
  INCLUDE (CTest)
ENDIF(BUILD_TESTING)

# Configure data directory
FIND_PATH(PARAQ_DATA_ROOT ParaQData.readme ${ParaQ_SOURCE_DIR}/../ParaQData $ENV{PARAQ_DATA_ROOT})

# Configure Python
OPTION(PARAQ_EMBED_PYTHON "Embedded Python Interpreter" ON)

IF(PARAQ_EMBED_PYTHON)
  SUBDIRS(
    Python
  )
  ADD_DEFINITIONS(-DPARAQ_EMBED_PYTHON)
ENDIF(PARAQ_EMBED_PYTHON)
