PROJECT(ParaQ)
CMAKE_MINIMUM_REQUIRED(VERSION 2.2)

# Setup output directories.
SET (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL
  "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL
  "Single output directory for building all executables.")

IF(NOT WIN32)
# prevent runtime problems on Unix/Mac when Qt & VTK use
# separate instances of these libraries
  SET(VTK_USE_SYSTEM_FREETYPE ON CACHE BOOL "Use system freetype")
ENDIF(NOT WIN32)

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
#########################################################################
# Configure Testing
OPTION(BUILD_TESTING "Build ParaQ Testing" ON)
IF(BUILD_TESTING)
  MAKE_DIRECTORY(${ParaView_BINARY_DIR}/Testing/Temporary)
  ENABLE_TESTING()
  INCLUDE (CTest)
ENDIF(BUILD_TESTING)

