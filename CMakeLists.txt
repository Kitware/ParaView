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
SET(CXX_TEST_PATH ${EXECUTABLE_OUTPUT_PATH})

INCLUDE_DIRECTORIES(${ParaView_BINARY_DIR})
CONFIGURE_FILE(${ParaView_SOURCE_DIR}/vtkPVConfig.h.in
  ${ParaView_BINARY_DIR}/vtkPVConfig.h
  ESCAPE_QUOTES)

INCLUDE(${ParaView_SOURCE_DIR}/VTK/CMake/vtkSelectStreamsLibrary.cmake)
VTK_SELECT_STREAMS_LIBRARY(PARAVIEW_USE_ANSI_STDLIB
                           ${ParaView_SOURCE_DIR}/VTK)

#########################################################################
# Configure Testing
OPTION(BUILD_TESTING "Build ParaView Complete Testing" ON)
OPTION(PARAVIEW_TESTING_DART_SERVER "Use ParaView tree for Dart server. Should be OFF" OFF)
MARK_AS_ADVANCED(PARAVIEW_TESTING_DART_SERVER)
IF(BUILD_TESTING)
  MAKE_DIRECTORY(${ParaView_BINARY_DIR}/Testing/Temporary)
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
# 
# something_ISSET is set for vtkDependentOption to not change it to bool. For
# more information, check VTK/CMake/vtkDependentOption.cmake
SET(VTK_USE_HYBRID_ISSET ON)
SET(VTK_USE_PARALLEL_ISSET ON)
SET(VTK_USE_VOLUMEPRO_ISSET OFF)
SET(VTK_USE_GL2PS_ISSET OFF)
SET(VTK_USE_ANSI_STDLIB ${PARAVIEW_USE_ANSI_STDLIB})

SET(VTK_USE_RENDERING ON CACHE INTERNAL "" FORCE)
SET(VTK_WRAP_TCL OFF CACHE INTERNAL "" FORCE)
SET(VTK_WRAP_PYTHON OFF CACHE INTERNAL "" FORCE)
SET(VTK_WRAP_JAVA OFF CACHE INTERNAL "" FORCE)
SET(VTK_USE_VIDEO_FOR_WINDOWS OFF CACHE INTERNAL "" FORCE)
SET(VTK_USE_MATROX_IMAGING OFF CACHE INTERNAL "" FORCE)

SET(VTK_DIR "${ParaView_BINARY_DIR}/VTK" CACHE INTERNAL "" FORCE)
SET(VTK_FOUND 1)
FIND_PATH(VTK_DATA_ROOT VTKData.readme ${ParaView_SOURCE_DIR}/../VTKData $ENV{VTK_DATA_ROOT})
MARK_AS_ADVANCED(VTK_DATA_ROOT)
MARK_AS_ADVANCED(BUILD_EXAMPLES)

OPTION(VTK_USE_PATENTED "Build VTK with patented support" OFF)

# Specify python build so that we can use vtkTkRenderWidget with no
# vtkRenderWindow wrapped.
ADD_DEFINITIONS(-DVTK_PYTHON_BUILD)

SUBDIRS(VTK)

# This is a hack for now. Once the transition is over, this macro goes aways
ADD_DEFINITIONS(-DPARAVIEW_NEW_SOURCE_ORGANIZATION)
SET(VTK_INCLUDE_DIR
  ${ParaView_SOURCE_DIR}/VTK
  ${ParaView_BINARY_DIR}/VTK
  )
SET(kits Common Filtering IO Imaging Rendering Parallel Graphics Hybrid)
IF(VTK_USE_PATENTED)
  SET(kits ${kits} Patented)
ENDIF(VTK_USE_PATENTED)
FOREACH(kit ${kits})
  SET(VTK_INCLUDE_DIR ${VTK_INCLUDE_DIR}
    ${ParaView_SOURCE_DIR}/VTK/${kit}
    ${ParaView_BINARY_DIR}/VTK/${kit}
    )
ENDFOREACH(kit)
SET(VTKZLIB_INCLUDE_DIR
  ${ParaView_SOURCE_DIR}/VTK/Utilities/zlib
  ${ParaView_BINARY_DIR}/VTK/Utilities/zlib)
SET(VTKEXPAT_INCLUDE_DIR
  ${ParaView_SOURCE_DIR}/VTK/Utilities/expat
  ${ParaView_BINARY_DIR}/VTK/Utilities/expat)

#########################################################################
# Configure HDF5
SET(PARAVIEW_HDF5_LIBRARIES vtkhdf5)
SET(VTK_ZLIB_LIBRARIES vtkzlib)
SET(VTK_ZLIB_INCLUDE_DIRS
  ${ParaView_SOURCE_DIR}/VTK/Utilities/zlib
  ${ParaView_BINARY_DIR}/VTK/Utilities/zlib)
SET(HDF5_INCLUDE_DIR 
  ${ParaView_SOURCE_DIR}/Utilities/hdf5
  ${ParaView_BINARY_DIR}/Utilities/hdf5)

SUBDIRS(Utilities/hdf5)

#########################################################################
# Configure Xdmf
SET(XDMF_REGENERATE_YACCLEX OFF CACHE INTERNAL "" FORCE)
SET(XDMF_REGENERATE_WRAPPERS OFF CACHE INTERNAL "" FORCE)
SET(XDMF_WRAP_PYTHON OFF CACHE INTERNAL "" FORCE)
SET(XDMF_WRAP_TCL OFF CACHE INTERNAL "" FORCE)
SET(XDMF_KITS_DIR "${ParaView_BINARY_DIR}/Utilities/Xdmf/vtk/Utilities")
SET(XDMF_INCLUDE_DIRS
  "${ParaView_SOURCE_DIR}/Utilities/Xdmf/vtk"
  "${ParaView_BINARY_DIR}/Utilities/Xdmf/vtk")
SET(PARAVIEW_LINK_XDMF ON)
SUBDIRS(Utilities/Xdmf)

#########################################################################
# Configure IceT
OPTION(PARAVIEW_USE_ICE_T "Use IceT multi display manager" OFF)
IF(PARAVIEW_USE_ICE_T)
  SET(ICE_T_INCLUDE_DIR 
    ${ParaView_SOURCE_DIR}/Utilities/IceT/src/include
    ${ParaView_BINARY_DIR}/Utilities/IceT/src/include
    )
  SUBDIRS(Utilities/IceT)
ENDIF(PARAVIEW_USE_ICE_T)

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
SET(KWSYS_INCLUDE_DIR ${ParaView_BINARY_DIR}/Utilities/KWSys)
SUBDIRS(Utilities/KWSys)

#########################################################################
# Configure Common
SET(KWCOMMON_INCLUDE_DIR
  ${ParaView_SOURCE_DIR}/Common/KWCommon
  ${ParaView_BINARY_DIR}/Common/KWCommon)
SUBDIRS(Common)

#########################################################################
# Configure VTKClientServer wrapping
SET(VTKCLIENTSERVER_INCLUDE_DIR
  ${ParaView_SOURCE_DIR}/Utilities/VTKClientServer
  ${ParaView_BINARY_DIR}/Utilities/VTKClientServer
  )
SUBDIRS(Utilities/VTKClientServer)

#########################################################################
# Configure Tcl wrapping
SUBDIRS(Utilities/VTKTclWrapping)

#########################################################################
# Configure Server
SET(PVFILTERS_INCLUDE_DIR
  ${ParaView_SOURCE_DIR}/Servers/Filters
  ${ParaView_BINARY_DIR}/Servers/Filters)
SUBDIRS(Servers)

#########################################################################
# Configure GUI
SUBDIRS(GUI)

