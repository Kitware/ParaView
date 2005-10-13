PROJECT(ParaQ)
CMAKE_MINIMUM_REQUIRED(VERSION 2.2)

IF(NOT WIN32)
# prevent runtime problems on Unix/Mac when Qt & VTK use
# separate instances of these libraries
# this requires Qt to be configured to use the 
# system libs as well (which is the default)
  SET(VTK_USE_SYSTEM_PNG ON CACHE BOOL "Use system png")
  SET(VTK_USE_SYSTEM_JPEG ON CACHE BOOL "Use system jpeg")
  SET(VTK_USE_SYSTEM_FREETYPE ON CACHE BOOL "Use system freetype")
# ParaView/Utilities/hdf5/ doesn't build using system zlib
#  SET(VTK_USE_SYSTEM_ZLIB ON CACHE BOOL "Use system zlib")
ENDIF(NOT WIN32)

# build VTK/Qt support
SET(VTK_USE_GUISUPPORT ON CACHE BOOL "Build VTK with GUI Support" FORCE)
SET(VTK_USE_QVTK ON CACHE BOOL "Build VTK with Qt Support" FORCE)
SET(DESIRED_QT_VERSION 4 CACHE STRING "Use Qt4" FORCE)

# turn off building ParaView Tk based GUI
SET(PARAVIEW_BUILD_GUI OFF CACHE BOOL "Build end user applications (ParaView, pvclient, pvbatch)")

SUBDIRS(
  ParaView
  Client
)
#########################################################################
# Configure Testing
OPTION(BUILD_TESTING "Build ParaQ Testing" ON)
IF(BUILD_TESTING)
  MAKE_DIRECTORY(${ParaView_BINARY_DIR}/Testing/Temporary)
  ENABLE_TESTING()
  INCLUDE (CTest)
ENDIF(BUILD_TESTING)

