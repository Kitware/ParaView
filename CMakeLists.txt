CMAKE_MINIMUM_REQUIRED (VERSION 2.0)
PROJECT(ParaView)

#########################################################################
# Disallow in-source build
STRING(COMPARE EQUAL "${ParaView_SOURCE_DIR}" "${ParaView_BINARY_DIR}" INSOURCE)
IF(INSOURCE)
  MESSAGE(FATAL_ERROR "ParaView requires an out of source Build. Please create a separate binary directory and run CMake there.")
ENDIF(INSOURCE)

#########################################################################
# Common settings
#
# ParaView version number.  An even minor number corresponds to releases.
SET(PARAVIEW_VERSION_MAJOR 1)
SET(PARAVIEW_VERSION_MINOR 8)
SET(PARAVIEW_VERSION_PATCH 0)
SET(PARAVIEW_VERSION "${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}")
SET(PARAVIEW_VERSION_FULL "${PARAVIEW_VERSION}.${PARAVIEW_VERSION_PATCH}")
SET(CMAKE_MODULE_PATH ${ParaView_SOURCE_DIR}/CMake)
SET (LIBRARY_OUTPUT_PATH ${ParaView_BINARY_DIR}/bin CACHE INTERNAL
  "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${ParaView_BINARY_DIR}/bin CACHE INTERNAL
  "Single output directory for building all executables.")

CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Utilities/Release/ParaView.nsi.in
  ${ParaView_BINARY_DIR}/Utilities/Release/ParaView.nsi IMMEDIATE @ONLY)

# The client server wrapper macro needs this name for
# BUILD_SHARED_LIBS so it can work both inside and outside the tree.
SET(PARAVIEW_BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS}")

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

# Fix cxx flags
IF(CMAKE_COMPILER_IS_GNUCXX)
  # A GCC compiler.  Quiet warning about strstream deprecation.
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
ELSE(CMAKE_COMPILER_IS_GNUCXX)
  IF(CMAKE_SYSTEM MATCHES "OSF1-V.*")
     SET(CMAKE_CXX_FLAGS
         "${CMAKE_CXX_FLAGS} -timplicit_local -no_implicit_include")
  ENDIF(CMAKE_SYSTEM MATCHES "OSF1-V.*")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

IF(BUILD_DOCUMENTATION)
  SUBDIRS(Utilities/Doxygen)
ENDIF(BUILD_DOCUMENTATION)

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

IF(NOT PARAVIEW_TESTING_DART_SERVER)

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

SET(APPLICATION_BINARY_DIR "${ParaView_BINARY_DIR}")
SET(VTK_TCL_TK_MACROS_MODULE 
    "${ParaView_SOURCE_DIR}/VTK/CMake/vtkTclTkMacros.cmake")
SET(VTK_TCL_TK_INTERNALS_DIR 
    "${ParaView_SOURCE_DIR}/VTK/Utilities/TclTk/internals")
SET(VTK_TCL_TK_WRAPPING_DIR "${ParaView_SOURCE_DIR}/VTK/Wrapping/Tcl")
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
SET(VTK_HEADER_TESTING_PY "${ParaView_SOURCE_DIR}/VTK/Common/Testing/HeaderTesting.py")
SET(VTK_PRINT_SELF_CHECK_TCL "${ParaView_SOURCE_DIR}/VTK/Common/Testing/Tcl/PrintSelfCheck.tcl")
SET(VTK_FIND_STRING_TCL "${ParaView_SOURCE_DIR}/VTK/Common/Testing/Tcl/FindString.tcl")

SET(VTK_USE_PATENTED ON CACHE INTERNAL "" FORCE)
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
IF(NOT BUILD_SHARED_LIBS)
  SET(TCL_TK_STATIC ON CACHE INTERNAL "" FORCE)
ENDIF(NOT BUILD_SHARED_LIBS)

OPTION(VTK_USE_PATENTED "Build VTK with patented support" OFF)

# Specify python build so that we can use vtkTkRenderWidget with no
# vtkRenderWindow wrapped.
ADD_DEFINITIONS(-DVTK_PYTHON_BUILD)
ADD_DEFINITIONS(-DVTK_LEAN_AND_MEAN)
OPTION(PARAVIEW_ENABLE_FPE
  "Build ParaView with Floating Point Exceptions turned on" OFF)
MARK_AS_ADVANCED(PARAVIEW_ENABLE_FPE)

OPTION(PARAVIEW_EXPERIMENTAL_USER
  "Build ParaView with all experimental options" OFF)
MARK_AS_ADVANCED(PARAVIEW_EXPERIMENTAL_USER)

CONFIGURE_FILE(${ParaView_SOURCE_DIR}/VTK/Utilities/TclTk/.NoDartCoverage
  ${ParaView_BINARY_DIR}/VTK/.NoDartCoverage)
SUBDIRS(VTK)


# Set the ICET MPI variables from the VTK ones.
# use a set cache internal so people don't try and use them
SET(ICET_MPIRUN_EXE "${VTK_MPIRUN_EXE}" CACHE INTERNAL 
  "This is set from VTK_MPIRUN_EXE.")
SET(ICET_MPI_PREFLAGS 
  "${VTK_MPI_NUMPROC_FLAG};${VTK_MPI_MAX_NUMPROCS};${VTK_MPI_PREFLAGS}" CACHE INTERNAL
  "This is set from a combination of VTK_MPI_NUMPROC_FLAG VTK_MPI_MAX_NUMPROCS VTK_MPI_PREFLAGS.")
SET(ICET_MPI_POSTFLAGS "${VTK_MPI_POSTFLAGS}"  CACHE INTERNAL
  "This is set from VTK_MPI_POSTFLAGS.")
SET(ICET_MPI_MAX_NUMPROCS "${VTK_MPI_MAX_NUMPROCS}"  CACHE INTERNAL
  "This is set from VTK_MPI_MAX_NUMPROCS.")



SET(VTK_INCLUDE_DIR
  ${ParaView_SOURCE_DIR}/VTK
  ${ParaView_BINARY_DIR}/VTK
  ${ParaView_SOURCE_DIR}/VTK/Utilities
  ${ParaView_BINARY_DIR}/VTK/Utilities
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

IF(VTK_USE_SYSTEM_ZLIB)
  SET(VTK_ZLIB_LIBRARIES ${ZLIB_LIBRARIES})
  SET(VTK_ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})
  SET(VTKZLIB_INCLUDE_DIR ${ZLIB_INCLUDE_DIR})
ELSE(VTK_USE_SYSTEM_ZLIB)
  SET(VTK_ZLIB_LIBRARIES vtkzlib)
  SET(VTK_ZLIB_INCLUDE_DIRS
    ${ParaView_SOURCE_DIR}/VTK/Utilities
    ${ParaView_BINARY_DIR}/VTK/Utilities
    ${ParaView_SOURCE_DIR}/VTK
    ${ParaView_BINARY_DIR}/VTK
    )
  SET(VTKZLIB_INCLUDE_DIR
    ${ParaView_SOURCE_DIR}/VTK/Utilities
    ${ParaView_BINARY_DIR}/VTK/Utilities
    ${ParaView_SOURCE_DIR}/VTK
    ${ParaView_BINARY_DIR}/VTK
    )
ENDIF(VTK_USE_SYSTEM_ZLIB)

#########################################################################
# Configure HDF5
OPTION(PARAVIEW_USE_SYSTEM_HDF5 "Use system installed HDF5" OFF)
IF(PARAVIEW_USE_SYSTEM_HDF5)

  INCLUDE(${ParaView_SOURCE_DIR}/CMake/FindHDF5.cmake)
  INCLUDE(${ParaView_SOURCE_DIR}/CMake/FindZLIB.cmake)
  SET(PARAVIEW_HDF5_LIBRARIES ${HDF5_LIBRARIES})

ELSE(PARAVIEW_USE_SYSTEM_HDF5)

  SET(PARAVIEW_HDF5_LIBRARIES vtkhdf5)
  SET(HDF5_ZLIB_HEADER "vtk_zlib.h")
  SET(HDF5_INCLUDE_DIR 
    ${ParaView_SOURCE_DIR}/Utilities/hdf5
    ${ParaView_BINARY_DIR}/Utilities/hdf5)

  SET(HDF5_CONFIG ${ParaView_BINARY_DIR}/Utilities/hdf5/HDF5Config.cmake)
  SUBDIRS(Utilities/hdf5)

ENDIF(PARAVIEW_USE_SYSTEM_HDF5)

#########################################################################
# Configure Xdmf
SET(XDMF_NOT_INSTALL_INCLUDE_FILES ON)
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
MARK_AS_ADVANCED(CLEAR VTK_USE_MPI)
IF(VTK_USE_MPI)
  OPTION(PARAVIEW_USE_ICE_T "Use IceT multi display manager" ON)
  MARK_AS_ADVANCED(PARAVIEW_USE_ICE_T)
  IF (BUILD_TESTING)
    OPTION(ICET_BUILD_TESTING "Build and run the ICE-T tests." OFF)
    MARK_AS_ADVANCED(ICET_BUILD_TESTING)
  ENDIF (BUILD_TESTING)
  IF(PARAVIEW_USE_ICE_T)
    SET(ICE_T_INCLUDE_DIR 
      ${ParaView_SOURCE_DIR}/Utilities/IceT/src/include
      ${ParaView_BINARY_DIR}/Utilities/IceT/src/include
      )
    SUBDIRS(Utilities/IceT)
  ENDIF(PARAVIEW_USE_ICE_T)
ENDIF(VTK_USE_MPI)

#########################################################################
# Configure KWsys
SET(KWSYS_NAMESPACE kwsys)
SET(KWSYS_USE_Process 1)
SET(KWSYS_USE_SystemTools 1)
SET(KWSYS_USE_RegularExpression 1)
SET(KWSYS_USE_CommandLineArguments 1)
SET(KWSYS_HEADER_ROOT ${ParaView_BINARY_DIR}/Utilities/KWSys)
IF(NOT VTK_USE_ANSI_STDLIB)
  SET(KWSYS_FORCE_OLD_STREAMS 1)
ENDIF(NOT VTK_USE_ANSI_STDLIB)
SET(KWSYS_INCLUDE_DIR ${ParaView_BINARY_DIR}/Utilities/KWSys)
IF(BUILD_SHARED_LIBS)
  SET(KWSYS_LIBRARY_INSTALL_DIR "/lib")
ENDIF(BUILD_SHARED_LIBS)
SUBDIRS(Utilities/KWSys)

SET(PARAVIEW_KWSYS_INCLUDE_DIRS ${KWSYS_INCLUDE_DIR})

#########################################################################
# Configure Common
SET(KWCOMMON_INCLUDE_DIR
  ${ParaView_SOURCE_DIR}/Common/KWCommon
  ${ParaView_BINARY_DIR}/Common/KWCommon)
SUBDIRS(Common)

#########################################################################
# Configure VTKClientServer wrapping
SET(KW_INSTALL_LIB_DIR "/lib")
SET(VTKCLIENTSERVER_INCLUDE_DIR
  ${ParaView_SOURCE_DIR}/Utilities/VTKClientServer
  ${ParaView_BINARY_DIR}/Utilities/VTKClientServer
  )
SUBDIRS(Utilities/VTKClientServer)

#########################################################################
# Configure Tcl wrapping
SUBDIRS(Utilities/VTKTclWrapping)

#########################################################################
# Configure ParaViewPlus
IF(EXISTS "${ParaView_SOURCE_DIR}/ParaViewPlus")
  OPTION(PARAVIEW_PLUS_BUILD "Build ParaViewPlus part. Requires Kitware License and ParaViewPlus source" OFF)
  MARK_AS_ADVANCED(PARAVIEW_PLUS_BUILD)
  IF(PARAVIEW_PLUS_BUILD)
    SUBDIRS(ParaViewPlus)
    SET(PARAVIEW_PLUS_INCLUDE_DIRS
      ${ParaView_SOURCE_DIR}/ParaViewPlus/IO
      )
  ENDIF(PARAVIEW_PLUS_BUILD)
ENDIF(EXISTS "${ParaView_SOURCE_DIR}/ParaViewPlus")

#########################################################################
# Import external projects, such as EXODUS and SAF

SET(PARAVIEW_ADDITIONAL_LIBRARIES)

#
# Imported project adds libraries
#
MACRO(PARAVIEW_LINK_LIBRARIES LIBS)
  SET(PARAVIEW_ADDITIONAL_LIBRARIES ${PARAVIEW_ADDITIONAL_LIBRARIES} ${LIBS})
ENDMACRO(PARAVIEW_LINK_LIBRARIES LIBS)

#
# Adds extra ParaView link directories
#
MACRO(PARAVIEW_EXTRA_LINK_DIRECTORIES DIRS)
  SET(current_dirs ${PARAVIEW_EXTRA_LINK_DIRECTORIES})
  FOREACH(dir ${DIRS})
    IF(${current_dirs} MATCHES ${dir})
    ELSE(${current_dirs} MATCHES ${dir})
      SET(current_dirs ${current_dirs} ${dir})
    ENDIF(${current_dirs} MATCHES ${dir})
  ENDFOREACH(dir)
  SET(PARAVIEW_EXTRA_LINK_DIRECTORIES "${current_dirs}" CACHE INTERNAL "")
ENDMACRO(PARAVIEW_EXTRA_LINK_DIRECTORIES)

#
# Imported project adds source files
# This ones need to be fixed, so that first are not wrapped. This will be fixed
# once the server changes are merged.
#
SET(ExtraParaViewServerNonWrapped_SRCS)
SET(ExtraParaViewClient_SRCS)
SET(ExtraParaViewBinary_SRCS)
SET(ExtraParaViewServer_SRCS)

MACRO(PARAVIEW_INCLUDE_SOURCES SRCS)
  SET(ExtraParaViewServerNonWrapped_SRCS
    ${ExtraParaViewServerNonWrapped_SRCS} ${SRCS})
ENDMACRO(PARAVIEW_INCLUDE_SOURCES SRCS)
MACRO(PARAVIEW_INCLUDE_CLIENT_SOURCES SRCS)
  SET(ExtraParaViewClient_SRCS ${ExtraParaViewClient_SRCS} ${SRCS})
ENDMACRO(PARAVIEW_INCLUDE_CLIENT_SOURCES)
MACRO(PARAVIEW_INCLUDE_NONWRAPPED_CLIENT_SOURCES SRCS)
  SET(ExtraParaViewClientNonWrapped_SRCS ${ExtraParaViewClientNonWrapped_SRCS}
    ${SRCS})
ENDMACRO(PARAVIEW_INCLUDE_NONWRAPPED_CLIENT_SOURCES)
MACRO(PARAVIEW_INCLUDE_EXECUTABLE_SOURCES SRCS)
  SET(ExtraParaViewBinary_SRCS ${ExtraParaViewBinary_SRCS} ${SRCS})
ENDMACRO(PARAVIEW_INCLUDE_EXECUTABLE_SOURCES SRCS)
MACRO(PARAVIEW_INCLUDE_WRAPPED_SOURCES SRCS)
  SET(ExtraParaViewServer_SRCS ${ExtraParaViewServer_SRCS} ${SRCS})
ENDMACRO(PARAVIEW_INCLUDE_WRAPPED_SOURCES SRCS)

FOREACH(external EXODUS SAF)
  OPTION(PARAVIEW_USE_${external} "Build using ${external} library. Requires access to ${external} libraries" OFF)
  MARK_AS_ADVANCED(PARAVIEW_USE_${external})
  IF(PARAVIEW_USE_${external})
    FIND_PATH(${external}_SOURCE_DIR 
      ${external}ParaViewImport.cmake
      ../vtkSNL/IO
      ../../vtkSNL/IO
      ../../../vtkSNL/IO)

    IF(EXISTS ${${external}_SOURCE_DIR}/${external}ParaViewImport.cmake)
      INCLUDE(${${external}_SOURCE_DIR}/${external}ParaViewImport.cmake)
    ELSE(EXISTS ${${external}_SOURCE_DIR}/${external}ParaViewImport.cmake)
      MESSAGE(SEND_ERROR "Cannot find ${external} sources directory: ${${external}_SOURCE_DIR}")
    ENDIF(EXISTS ${${external}_SOURCE_DIR}/${external}ParaViewImport.cmake)
  ENDIF(PARAVIEW_USE_${external})
ENDFOREACH(external)

#########################################################################
# Configure Server
SET(PVFILTERS_INCLUDE_DIR
  ${ParaView_SOURCE_DIR}/Servers/Filters
  ${ParaView_BINARY_DIR}/Servers/Filters)
SET(PVSERVERMANAGER_INCLUDE_DIR
  ${ParaView_SOURCE_DIR}/Servers/ServerManager
  ${ParaView_BINARY_DIR}/Servers/ServerManager)
SET(PVSERVERCOMMON_INCLUDE_DIR
  ${ParaView_SOURCE_DIR}/Servers/Common
  ${ParaView_BINARY_DIR}/Servers/Common)

SUBDIRS(Servers)

#########################################################################
# Configure GUI
SUBDIRS(GUI)


SUBDIRS(Servers/Executables)

#-----------------------------------------------------------------------------
# Help outside projects build ParaView plugin packages.

EXPORT_LIBRARY_DEPENDENCIES(${ParaView_BINARY_DIR}/ParaViewLibraryDepends.cmake)

# For ParaView from build tree:
SET(PARAVIEW_VTK_DIR ${VTK_DIR})
SET(VTK_WRAP_ClientServer_EXE_CONFIG ${VTK_WRAP_ClientServer_EXE})
SET(VTKCS_SOURCE_DIR ${ParaView_SOURCE_DIR}/Utilities/VTKClientServer)
SET(PARAVIEW_CMAKE_CLIENT_SERVER_EXTENSION
  ${VTKCS_SOURCE_DIR}/CMake/vtkWrapClientServer.cmake)
SET(PARAVIEW_USE_FILE ${ParaView_SOURCE_DIR}/ParaViewUse.cmake)
SET(PARAVIEW_INCLUDE_DIRS
  ${ParaView_SOURCE_DIR}/Utilities/VTKClientServer
  ${ParaView_SOURCE_DIR}/Utilities/hdf5
  ${ParaView_SOURCE_DIR}/Common/KWCommon
  ${ParaView_SOURCE_DIR}/Servers/Filters
  ${ParaView_SOURCE_DIR}/Servers/ServerManager
  ${ParaView_SOURCE_DIR}/Servers/Common
  ${ParaView_SOURCE_DIR}/GUI/Widgets
  ${ParaView_SOURCE_DIR}/GUI/Client
#
  ${ParaView_BINARY_DIR}
  ${ParaView_BINARY_DIR}/Utilities/VTKClientServer
  ${ParaView_BINARY_DIR}/Utilities/hdf5
  ${ParaView_BINARY_DIR}/Common/KWCommon
  ${ParaView_BINARY_DIR}/Servers/Filters
  ${ParaView_BINARY_DIR}/Servers/ServerManager
  ${ParaView_BINARY_DIR}/Servers/Common
  ${ParaView_BINARY_DIR}/GUI/Widgets
  ${ParaView_BINARY_DIR}/GUI/Client
  )
SET(PARAVIEW_GUI_INCLUDE_DIRS
  ${TCL_INCLUDE_PATH}
  ${TK_INCLUDE_PATH}
  ${KWCOMMON_INCLUDE_DIR}
  ${VTKZLIB_INCLUDE_DIR})
SET(PARAVIEW_LIBRARY_DIRS ${LIBRARY_OUTPUT_PATH} ${PARAVIEW_EXTRA_LINK_DIRECTORIES})
SET(PARAVIEW_LIBRARY_DEPENDS_FILE ${ParaView_BINARY_DIR}/ParaViewLibraryDepends.cmake)
SET(PARAVIEW_GUI_CLIENT_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/paraview)
SET(PARAVIEW_SERVER_MANAGER_RESOURCES_DIRECTORY ${ParaView_SOURCE_DIR}/Servers/ServerManager/Resources)
CONFIGURE_FILE(${ParaView_SOURCE_DIR}/ParaViewConfig.cmake.in
               ${ParaView_BINARY_DIR}/ParaViewConfig.cmake @ONLY IMMEDIATE)

# For installed ParaView:
SET(PARAVIEW_VTK_DIR ${CMAKE_INSTALL_PREFIX}/lib/vtk)
SET(VTK_WRAP_ClientServer_EXE_CONFIG ${CMAKE_INSTALL_PREFIX}/lib/paraview-${PARAVIEW_VERSION}/vtkWrapClientServer)
SET(PARAVIEW_CMAKE_CLIENT_SERVER_EXTENSION ${CMAKE_INSTALL_PREFIX}/lib/paraview-${PARAVIEW_VERSION}/CMake/vtkWrapClientServer.cmake)
SET(PARAVIEW_USE_FILE ${CMAKE_INSTALL_PREFIX}/lib/paraview-${PARAVIEW_VERSION}/ParaViewUse.cmake)
SET(PARAVIEW_INCLUDE_DIRS ${CMAKE_INSTALL_PREFIX}/include/paraview-${PARAVIEW_VERSION})
SET(PARAVIEW_GUI_INCLUDE_DIRS ${PARAVIEW_INCLUDE_DIRS})
SET(PARAVIEW_LIBRARY_DIRS ${CMAKE_INSTALL_PREFIX}/lib/paraview-${PARAVIEW_VERSION})
SET(PARAVIEW_LIBRARY_DEPENDS_FILE ${CMAKE_INSTALL_PREFIX}/lib/paraview-${PARAVIEW_VERSION}/ParaViewLibraryDepends.cmake)
SET(PARAVIEW_GUI_CLIENT_EXECUTABLE ${CMAKE_INSTALL_PREFIX}/bin/paraview)
SET(PARAVIEW_SERVER_MANAGER_RESOURCES_DIRECTORY ${CMAKE_INSTALL_PREFIX}/lib/paraview-${PARAVIEW_VERSION}/ServerManager/Resources)
CONFIGURE_FILE(${ParaView_SOURCE_DIR}/ParaViewConfig.cmake.in
               ${ParaView_BINARY_DIR}/Utilities/ParaViewConfig.cmake @ONLY IMMEDIATE)

INSTALL_FILES(/lib/paraview-${PARAVIEW_VERSION}/CMake FILES
  ${VTKCS_SOURCE_DIR}/CMake/vtkWrapClientServer.cmake)
INSTALL_FILES(/lib/paraview-${PARAVIEW_VERSION} FILES
  ${ParaView_SOURCE_DIR}/ParaViewUse.cmake
  ${ParaView_BINARY_DIR}/ParaViewLibraryDepends.cmake
  ${ParaView_BINARY_DIR}/Utilities/ParaViewConfig.cmake
  )

CONFIGURE_FILE(${ParaView_SOURCE_DIR}/CMake/CTestCustom.ctest.in
  ${ParaView_BINARY_DIR}/CTestCustom.ctest @ONLY)

ENDIF(NOT PARAVIEW_TESTING_DART_SERVER)

