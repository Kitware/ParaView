CMAKE_MINIMUM_REQUIRED (VERSION 1.8)
PROJECT(ParaView)

#########################################################################
# Common settings
#
# ParaView version number.  An even minor number corresponds to releases.
SET(PARAVIEW_VERSION_MAJOR 1)
SET(PARAVIEW_VERSION_MINOR 3)
SET(PARAVIEW_VERSION_PATCH 0)
SET(PARAVIEW_VERSION "${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}")
SET(PARAVIEW_VERSION_FULL "${PARAVIEW_VERSION}.${PARAVIEW_VERSION_PATCH}")
SET(CMAKE_MODULE_PATH ${ParaView_SOURCE_DIR}/CMake)
SET (LIBRARY_OUTPUT_PATH ${ParaView_BINARY_DIR}/bin CACHE INTERNAL
  "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${ParaView_BINARY_DIR}/bin CACHE INTERNAL
  "Single output directory for building all executables.")

# No XML IO
SET(DO_NOT_BUILD_XML_RW 1)

#########################################################################
# Configure Testing
OPTION(BUILD_TESTING "Build ParaView Complete Testing" OFF)
OPTION(PARAVIEW_TESTING_DART_SERVER "Use ParaView tree for Dart server. Should be OFF" OFF)
MARK_AS_ADVANCED(BUILD_TESTING PARAVIEW_TESTING_DART_SERVER)
IF(BUILD_TESTING)
  ENABLE_TESTING()
  IF(PARAVIEW_TESTING_DART_SERVER)
    INCLUDE (${CMAKE_ROOT}/Modules/FindDart.cmake)
  ELSE(PARAVIEW_TESTING_DART_SERVER)
    SET(DART_ROOT "" CACHE INTERNAL "" FORCE)
  ENDIF(PARAVIEW_TESTING_DART_SERVER)
  INCLUDE (${CMAKE_ROOT}/Modules/Dart.cmake)
ENDIF(BUILD_TESTING)

#########################################################################
# Configure TclTk
IF(UNIX)
  # On UNIX, Tcl needs the math library.
  IF(UNIX)
    LINK_LIBRARIES(-lm)
  ENDIF(UNIX)

  # If using X with Tk, we need to link to X when we link
  # to Tk in order to support static linking.
  IF(CMAKE_HAS_X)
    LINK_LIBRARIES(-lXt ${CMAKE_X_LIBS})
  ENDIF(CMAKE_HAS_X)
ENDIF(UNIX)

SUBDIRS(Utilities/TclTk)

#########################################################################
# Configure VTK
# force some values to be set by default as they are required by ParaView
SET(VTK_USE_HYBRID_FORCE ON)
SET(VTK_WRAP_TCL OFF CACHE INTERNAL "" FORCE)
SET(VTK_USE_PARALLEL_FORCE ON)
SET(VTK_USE_RENDERING ON CACHE INTERNAL "" FORCE)

SET(VTK_USE_VOLUMEPRO_FORCE OFF)
SET(VTK_WRAP_PYTHON OFF CACHE INTERNAL "" FORCE)
SET(VTK_WRAP_JAVA OFF CACHE INTERNAL "" FORCE)
SET(VTK_USE_GL2PS_FORCE OFF)

SET(VTK_DIR "${ParaView_BINARY_DIR}/VTK")

SUBDIRS(VTK)
IF(EXISTS "${VTK_DIR}/VTKLibraryDepends.cmake")
ELSE(EXISTS "${VTK_DIR}/VTKLibraryDepends.cmake")
  SET(VTK_NO_LIBRARY_DEPENDS 1)
ENDIF(EXISTS "${VTK_DIR}/VTKLibraryDepends.cmake")
IF(EXISTS "${VTK_DIR}/VTKConfig.cmake")
  FIND_PACKAGE(VTK QUIET)
  IF(VTK_FOUND)
    INCLUDE(${VTK_USE_FILE})
  ENDIF(VTK_FOUND)
ENDIF(EXISTS "${VTK_DIR}/VTKConfig.cmake")


#########################################################################
# Configure HDF5
SET(PARAVIEW_HDF5_LIBRARIES vtkhdf5)
SUBDIRS(Utilities/hdf5)

#########################################################################
# Configure Xdmf
SUBDIRS(Utilities/Xdmf)

#########################################################################
# Configure KWsys
SET(KWSYS_NAMESPACE kwsys)
SET(KWSYS_USE_Process 1)
SET(KWSYS_USE_SystemTools 1)
SET(KWSYS_USE_RegularExpression 1)
SET(KWSYS_HEADER_ROOT ${ParaView_BINARY_DIR}/Utilities/KWSys)
IF(NOT VTK_USE_ANSI_STDLIB)
  SET(KWSYS_FORCE_OLD_STREAMS 1)
ENDIF(NOT VTK_USE_ANSI_STDLIB)
SUBDIRS(Utilities/KWSys)

#########################################################################
# Configure Xdmf
SUBDIRS(Common)

#########################################################################
# Configure VTKClientServer wrapping
SUBDIRS(Utilities/VTKClientServer)

#########################################################################
# Configure Tcl wrapping
SUBDIRS(Utilities/VTKTclWrapping)

#########################################################################
# Configure Server
SUBDIRS(Servers)

#########################################################################
# Configure GUI
SUBDIRS(GUI)

