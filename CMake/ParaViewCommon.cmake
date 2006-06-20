# Requires ParaView_SOURCE_DIR and ParaView_BINARY_DIR to be set.

#########################################################################
# Common settings
#
# ParaView version number.  An even minor number corresponds to releases.
SET(PARAVIEW_VERSION_MAJOR 2)
SET(PARAVIEW_VERSION_MINOR 5)
SET(PARAVIEW_VERSION_PATCH 0)
SET(PARAVIEW_VERSION "${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}")
SET(PARAVIEW_VERSION_FULL "${PARAVIEW_VERSION}.${PARAVIEW_VERSION_PATCH}")
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${ParaView_SOURCE_DIR}/VTK/CMake")

# Configure VTK library versions to be paraview-specific.
SET(VTK_NO_LIBRARY_VERSION 1)
SET(VTK_LIBRARY_PROPERTIES VERSION "pv${PARAVIEW_VERSION}")

# Setup output directories.
SET (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL
  "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL
  "Single output directory for building all executables.")

#########################################################################
# Configure VTK
# force some values to be set by default as they are required by ParaView
# 
# something_ISSET is set for vtkDependentOption to not change it to bool. For
# more information, check VTK/CMake/vtkDependentOption.cmake
SET(VTK_USE_HYBRID_ISSET ON)
SET(VTK_USE_PARALLEL_ISSET ON)
SET(VTK_USE_VOLUMERENDERING_ISSET ON)
SET(VTK_USE_GL2PS_ISSET OFF)
SET(VTK_USE_ANSI_STDLIB ${PARAVIEW_USE_ANSI_STDLIB})
SET(VTK_HEADER_TESTING_PY "${ParaView_SOURCE_DIR}/VTK/Common/Testing/HeaderTesting.py")
SET(VTK_PRINT_SELF_CHECK_TCL "${ParaView_SOURCE_DIR}/VTK/Common/Testing/Tcl/PrintSelfCheck.tcl")
SET(VTK_FIND_STRING_TCL "${ParaView_SOURCE_DIR}/VTK/Common/Testing/Tcl/FindString.tcl")

SET(VTK_USE_RENDERING ON CACHE INTERNAL "" FORCE)
SET(VTK_WRAP_TCL OFF CACHE INTERNAL "" FORCE)
SET(VTK_WRAP_PYTHON OFF CACHE INTERNAL "" FORCE)
SET(VTK_WRAP_JAVA OFF CACHE INTERNAL "" FORCE)
SET(VTK_USE_MATROX_IMAGING OFF CACHE INTERNAL "" FORCE)

SET(VTK_DIR "${ParaView_BINARY_DIR}/VTK" CACHE INTERNAL "" FORCE)
SET(VTK_SOURCE_DIR "${ParaView_SOURCE_DIR}/VTK" CACHE INTERNAL "" FORCE)
SET(VTK_CMAKE_DIR "${VTK_SOURCE_DIR}/CMake" CACHE INTERNAL "" FORCE)
SET(VTK_FOUND 1)
FIND_PATH(VTK_DATA_ROOT VTKData.readme ${ParaView_SOURCE_DIR}/../VTKData $ENV{VTK_DATA_ROOT})
MARK_AS_ADVANCED(VTK_DATA_ROOT)
MARK_AS_ADVANCED(BUILD_EXAMPLES)

# Include the UseX option.
INCLUDE(${VTK_CMAKE_DIR}/vtkDependentOption.cmake)
SET(PARAVIEW_NEED_X 1)
IF(APPLE)
  IF(NOT VTK_USE_X)
    SET(PARAVIEW_NEED_X 0)
  ENDIF(NOT VTK_USE_X)
ENDIF(APPLE)
IF(PARAVIEW_NEED_X)
  INCLUDE(${VTK_CMAKE_DIR}/vtkUseX.cmake)
ENDIF(PARAVIEW_NEED_X)
SET(VTK_DONT_INCLUDE_USE_X 1)

# Choose static or shared libraries.  This provides BUILD_SHARED_LIBS
# and VTK_USE_RPATH.
INCLUDE(${VTK_CMAKE_DIR}/vtkSelectSharedLibraries.cmake)

# Disable the install targets if using the rpath feature.
IF(NOT WIN32)
  IF(VTK_USE_RPATH)
    SET(PV_INSTALL_NO_DEVELOPMENT 1)
    SET(PV_INSTALL_NO_RUNTIME 1)
  ENDIF(VTK_USE_RPATH)
ENDIF(NOT WIN32)

# ParaView needs static Tcl/Tk if not using shared libraries.
IF(NOT BUILD_SHARED_LIBS)
  SET(VTK_TCL_TK_STATIC ON CACHE INTERNAL "" FORCE)
ENDIF(NOT BUILD_SHARED_LIBS)

# Setup install directories.
IF(NOT PV_INSTALL_BIN_DIR)
  SET(PV_INSTALL_BIN_DIR ${PV_INSTALL_ROOT}/bin)
ENDIF(NOT PV_INSTALL_BIN_DIR)
IF(NOT PV_INSTALL_INCLUDE_DIR)
  SET(PV_INSTALL_INCLUDE_DIR ${PV_INSTALL_ROOT}/include/paraview-${PARAVIEW_VERSION})
ENDIF(NOT PV_INSTALL_INCLUDE_DIR)
IF(NOT PV_INSTALL_LIB_DIR)
  SET(PV_INSTALL_LIB_DIR ${PV_INSTALL_ROOT}/lib/paraview-${PARAVIEW_VERSION})
ENDIF(NOT PV_INSTALL_LIB_DIR)
IF(NOT PV_INSTALL_DATA_DIR)
  SET(PV_INSTALL_DATA_DIR ${PV_INSTALL_ROOT}/share/paraview-${PARAVIEW_VERSION})
ENDIF(NOT PV_INSTALL_DATA_DIR)

# Install no development files by default, but allow the user to get
# them installed by setting PV_INSTALL_DEVELOPMENT to true.  Disable
# the option altogether if PV_INSTALL_NO_DEVELOPMENT is already set to
# true.
IF(NOT PV_INSTALL_NO_DEVELOPMENT)
  # Leave this option out until we write code to install paraview headers
  # other than VTK.
  #OPTION(PV_INSTALL_DEVELOPMENT "Install ParaView plugin development files."
  #  OFF)
  #MARK_AS_ADVANCED(PV_INSTALL_DEVELOPMENT)
  IF(NOT PV_INSTALL_DEVELOPMENT)
    SET(PV_INSTALL_NO_DEVELOPMENT 1)
  ENDIF(NOT PV_INSTALL_DEVELOPMENT)
ENDIF(NOT PV_INSTALL_NO_DEVELOPMENT)

SET(PV_INSTALL_NO_LIBRARIES)
IF(BUILD_SHARED_LIBS)
  IF(PV_INSTALL_NO_RUNTIME AND PV_INSTALL_NO_DEVELOPMENT)
    SET(PV_INSTALL_NO_LIBRARIES 1)
  ENDIF(PV_INSTALL_NO_RUNTIME AND PV_INSTALL_NO_DEVELOPMENT)
ELSE(BUILD_SHARED_LIBS)
  IF(PV_INSTALL_NO_DEVELOPMENT)
    SET(PV_INSTALL_NO_LIBRARIES 1)
  ENDIF(PV_INSTALL_NO_DEVELOPMENT)
ENDIF(BUILD_SHARED_LIBS)

# Send VTK executables to the ParaView LIBRARY directory (not a mistake).
# Send VTK include files to the ParaView include directory
# Send VTK libraries to the ParaView library directory.

SET(VTK_INSTALL_BIN_DIR ${PV_INSTALL_LIB_DIR})
SET(VTK_INSTALL_INCLUDE_DIR ${PV_INSTALL_INCLUDE_DIR})
SET(VTK_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
SET(VTK_INSTALL_PACKAGE_DIR ${PV_INSTALL_LIB_DIR})

# VTK and KWCommon should install only the components paraview does.

SET(VTK_INSTALL_NO_DOCUMENTATION 1)
SET(VTK_INSTALL_NO_DEVELOPMENT ${PV_INSTALL_NO_DEVELOPMENT})

SET(KWCommon_INSTALL_BIN_DIR ${PV_INSTALL_BIN_DIR})
SET(KWCommon_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
SET(KWCommon_INSTALL_DATA_DIR ${PV_INSTALL_DATA_DIR})
SET(KWCommon_INSTALL_INCLUDE_DIR ${PV_INSTALL_INCLUDE_DIR})
SET(KWCommon_INSTALL_PACKAGE_DIR ${PV_INSTALL_LIB_DIR})
SET(KWCommon_INSTALL_NO_DEVELOPMENT ${PV_INSTALL_NO_DEVELOPMENT})
SET(KWCommon_INSTALL_NO_RUNTIME ${PV_INSTALL_NO_RUNTIME})
SET(KWCommon_INSTALL_NO_DOCUMENTATION 1)

SET(KWCommonPro_INSTALL_BIN_DIR ${PV_INSTALL_BIN_DIR})
SET(KWCommonPro_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
SET(KWCommonPro_INSTALL_DATA_DIR ${PV_INSTALL_DATA_DIR})
SET(KWCommonPro_INSTALL_INCLUDE_DIR ${PV_INSTALL_INCLUDE_DIR})
SET(KWCommonPro_INSTALL_PACKAGE_DIR ${PV_INSTALL_LIB_DIR})
SET(KWCommonPro_INSTALL_NO_DEVELOPMENT ${PV_INSTALL_NO_DEVELOPMENT})
SET(KWCommonPro_INSTALL_NO_RUNTIME ${PV_INSTALL_NO_RUNTIME})
SET(KWCommonPro_INSTALL_NO_DOCUMENTATION 1)


#########################################################################
# The client server wrapper macro needs this name for
# BUILD_SHARED_LIBS so it can work both inside and outside the tree.
SET(PARAVIEW_BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS}")

SET(CXX_TEST_PATH ${EXECUTABLE_OUTPUT_PATH})

INCLUDE_DIRECTORIES(
  # For vtkPVConfig.h.
  ${ParaView_BINARY_DIR}

  # For possible use of internal/stdio_core.h SGI workaround.
  ${ParaView_BINARY_DIR}/VTK/Utilities
  )

INCLUDE(${ParaView_SOURCE_DIR}/VTK/CMake/vtkSelectStreamsLibrary.cmake)
VTK_SELECT_STREAMS_LIBRARY(PARAVIEW_USE_ANSI_STDLIB
                           ${ParaView_SOURCE_DIR}/VTK)

# Fix cxx flags
IF(CMAKE_COMPILER_IS_GNUCXX)
  # A GCC compiler.  Quiet warning about strstream deprecation.
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
  IF(APPLE)
    # The fink package on OSX sets the environment variable LD_PREBIND
    # which breaks paraview linking.  Add this option to tell the
    # linker to ignore the environment variable.
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -noprebind")
  ENDIF(APPLE)
ELSE(CMAKE_COMPILER_IS_GNUCXX)
  IF(CMAKE_SYSTEM MATCHES "OSF1-V.*")
     SET(CMAKE_CXX_FLAGS
         "${CMAKE_CXX_FLAGS} -timplicit_local -no_implicit_include")
  ENDIF(CMAKE_SYSTEM MATCHES "OSF1-V.*")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

# Disable deprecation of the entire C library.
IF(CMAKE_COMPILER_2005)
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
ENDIF(CMAKE_COMPILER_2005)

#########################################################################
# Configure Testing
OPTION(BUILD_TESTING "Build ParaView Complete Testing" ON)
IF(BUILD_TESTING)
  MAKE_DIRECTORY(${CMAKE_BINARY_DIR}/Testing/Temporary)
  INCLUDE (Dart)
  ENABLE_TESTING()
ENDIF(BUILD_TESTING)

#########################################################################
# Specify python build so that we can use vtkTkRenderWidget with no
# vtkRenderWindow wrapped.
ADD_DEFINITIONS(-DVTK_PYTHON_BUILD)

OPTION(PARAVIEW_ENABLE_FPE
  "Build ParaView with Floating Point Exceptions turned on" OFF)
MARK_AS_ADVANCED(PARAVIEW_ENABLE_FPE)

OPTION(PARAVIEW_EXPERIMENTAL_USER
  "Build ParaView with all experimental options" OFF)
MARK_AS_ADVANCED(PARAVIEW_EXPERIMENTAL_USER)

OPTION(PARAVIEW_ALWAYS_SECURE_CONNECTION
  "Build ParaView with enforced secure connection (--connect-id)" OFF)
MARK_AS_ADVANCED(PARAVIEW_ALWAYS_SECURE_CONNECTION)

CONFIGURE_FILE(${ParaView_SOURCE_DIR}/VTK/Utilities/TclTk/.NoDartCoverage
  ${ParaView_BINARY_DIR}/VTK/.NoDartCoverage)
SUBDIRS(VTK)

#########################################################################
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
SET(kits Common Filtering GenericFiltering IO Imaging Rendering Parallel Graphics Hybrid VolumeRendering Widgets)
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
MARK_AS_ADVANCED(PARAVIEW_USE_SYSTEM_HDF5)
IF(PARAVIEW_USE_SYSTEM_HDF5)

  INCLUDE(${ParaView_SOURCE_DIR}/CMake/FindHDF5.cmake)
  INCLUDE(${ParaView_SOURCE_DIR}/CMake/FindZLIB.cmake)
  SET(PARAVIEW_HDF5_LIBRARIES ${HDF5_LIBRARIES})

ELSE(PARAVIEW_USE_SYSTEM_HDF5)

  SET(VTKHDF5_INSTALL_NO_DEVELOPMENT ${PV_INSTALL_NO_DEVELOPMENT})
  SET(VTKHDF5_INSTALL_NO_RUNTIME ${PV_INSTALL_NO_RUNTIME})
  SET(VTKHDF5_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
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


SET(XDMF_INSTALL_NO_DEVELOPMENT ${PV_INSTALL_NO_DEVELOPMENT})
SET(XDMF_INSTALL_NO_RUNTIME ${PV_INSTALL_NO_RUNTIME})
SET(XDMF_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
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
# Configure mpeg2 encoding
SET(VTKMPEG2_INSTALL_NO_DEVELOPMENT ${PV_INSTALL_NO_DEVELOPMENT})
SET(VTKMPEG2_INSTALL_NO_RUNTIME ${PV_INSTALL_NO_RUNTIME})
SET(VTKMPEG2_INSTALL_BIN_DIR ${PV_INSTALL_BIN_DIR})
SET(VTKMPEG2_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
SET(VTKMPEG2_INCLUDE_DIRS
  "${ParaView_SOURCE_DIR}/Utilities/vtkmpeg2"
  "${ParaView_BINARY_DIR}/Utilities/vtkmpeg2")

#########################################################################
# Configure ffmpeg encoding
MARK_AS_ADVANCED(CLEAR
  FFMPEG_INCLUDE_DIR
  FFMPEG_avformat_LIBRARY
  FFMPEG_avcodec_LIBRARY
  FFMPEG_avutil_LIBRARY
  )

#########################################################################
# Configure IceT
SET(ICET_INSTALL_BIN_DIR ${PV_INSTALL_BIN_DIR})
SET(ICET_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
SET(ICET_INSTALL_INCLUDE_DIR ${PV_INSTALL_INCLUDE_DIR})
SET(ICET_INSTALL_NO_DEVELOPMENT ${PV_INSTALL_NO_DEVELOPMENT})
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

  # Needed for mpich 2
  ADD_DEFINITIONS("-DMPICH_IGNORE_CXX_SEEK")
ENDIF(VTK_USE_MPI)

#########################################################################
# Configure Common
SUBDIRS(Common)

#########################################################################
# Configure VTKClientServer wrapping
SET(VTKCLIENTSERVER_INCLUDE_DIR
  ${ParaView_SOURCE_DIR}/Utilities/VTKClientServer
  ${ParaView_BINARY_DIR}/Utilities/VTKClientServer
  )
SUBDIRS(Utilities/VTKClientServer)

#########################################################################
# Configure Python wrapping
OPTION(PARAVIEW_WRAP_PYTHON "Wrap ParaView server manager into Python" OFF)
MARK_AS_ADVANCED(PARAVIEW_WRAP_PYTHON)
IF(PARAVIEW_WRAP_PYTHON)
  FIND_PACKAGE(PythonLibs REQUIRED)
  SUBDIRS(Utilities/VTKPythonWrapping)
  IF(PV_INSTALL_NO_LIBRARIES)
    SET(VTKPythonWrapping_INSTALL_LIBRARIES 0)
  ELSE(PV_INSTALL_NO_LIBRARIES)
    SET(VTKPythonWrapping_INSTALL_LIBRARIES 1)
  ENDIF(PV_INSTALL_NO_LIBRARIES)
  SET(VTKPythonWrapping_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
  SET(VTKPythonWrapping_INSTALL_BIN_DIR ${PV_INSTALL_BIN_DIR})
ENDIF(PARAVIEW_WRAP_PYTHON)

#########################################################################
# Configure Tcl Wraping.
# We can't remove this from ParaViewCommon.cmake since it must be
# included after VTK has been included but before ServerManager.
IF (PARAVIEW_BUILD_GUI)
  SUBDIRS(Utilities/VTKTclWrapping)
  SET(VTKTclWrapping_INSTALL_NO_DEVELOPMENT ${PV_INSTALL_NO_DEVELOPMENT})
  SET(VTKTclWrapping_INSTALL_NO_RUNTIME ${PV_INSTALL_NO_RUNTIME})
  SET(VTKTclWrapping_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
  SET(VTKTclWrapping_INSTALL_BIN_DIR ${PV_INSTALL_BIN_DIR})
ENDIF (PARAVIEW_BUILD_GUI)

#########################################################################
# Import external projects, such as SAF.

SET(PARAVIEW_EXTRA_SERVERMANAGER_RESOURCES)

#
# Imported project adds server manager resources
#
MACRO(PARAVIEW_INCLUDE_SERVERMANAGER_RESOURCES RESOURCES)
  SET(PARAVIEW_EXTRA_SERVERMANAGER_RESOURCES ${PARAVIEW_EXTRA_SERVERMANAGER_RESOURCES} ${RESOURCES})
ENDMACRO(PARAVIEW_INCLUDE_SERVERMANAGER_RESOURCES RESOURCES)

SET(PARAVIEW_EXTRA_GUI_RESOURCES)

#
# Imported project adds GUI resources
#
MACRO(PARAVIEW_INCLUDE_GUI_RESOURCES RESOURCES)
  SET(PARAVIEW_EXTRA_GUI_RESOURCES ${PARAVIEW_EXTRA_GUI_RESOURCES} ${RESOURCES})
ENDMACRO(PARAVIEW_INCLUDE_GUI_RESOURCES RESOURCES)

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
    STRING(COMPARE EQUAL "${current_dirs}" "${dir}"
      paraview_extra_link_directories_already_there)
    IF(NOT paraview_extra_link_directories_already_there)
      SET(current_dirs ${current_dirs} "${dir}")
    ENDIF(NOT paraview_extra_link_directories_already_there)
  ENDFOREACH(dir)
  SET(PARAVIEW_EXTRA_LINK_DIRECTORIES "${current_dirs}" CACHE INTERNAL "")
ENDMACRO(PARAVIEW_EXTRA_LINK_DIRECTORIES)
LINK_DIRECTORIES(${PARAVIEW_EXTRA_LINK_DIRECTORIES})

#
# Imported project adds source files
# This ones need to be fixed, so that first are not wrapped. This will be fixed
# once the server changes are merged.
#
SET(ExtraParaViewServerNonWrapped_SRCS)
SET(ExtraParaViewServerManagerNonWrapped_SRCS)
SET(ExtraParaViewClient_SRCS)
SET(ExtraParaViewBinary_SRCS)
SET(ExtraParaViewServer_SRCS)
SET(ExtraParaViewServerFiltersIncludes)
SET(ExtraParaViewGUIIncludes)

MACRO(PARAVIEW_INCLUDE_SERVERMANAGER_SOURCES SRCS)
  SET(ExtraParaViewServerManagerNonWrapped_SRCS
    ${ExtraParaViewServerManagerNonWrapped_SRCS} ${SRCS})
ENDMACRO(PARAVIEW_INCLUDE_SERVERMANAGER_SOURCES)
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
MACRO(PARAVIEW_FILTERS_INCLUDE_DIRECTORIES SRCS)
  SET(ExtraParaViewServerFiltersIncludes ${ExtraParaViewServerFiltersIncludes} ${SRCS})
ENDMACRO(PARAVIEW_FILTERS_INCLUDE_DIRECTORIES SRCS)
MACRO(PARAVIEW_GUI_INCLUDE_DIRECTORIES SRCS)
  SET(ExtraParaViewGUIIncludes ${ExtraParaViewGUIIncludes} ${SRCS})
ENDMACRO(PARAVIEW_GUI_INCLUDE_DIRECTORIES SRCS)

SET(PARAVIEW_EXTRA_EXTERNAL_MODULES
  "" CACHE STRING "Extra modules that ParaView will try to import. The modules have to provide ParaViewImport.cmake file.")
MARK_AS_ADVANCED(PARAVIEW_EXTRA_EXTERNAL_MODULES)
FOREACH(external SAF SSLIB_SAF ${PARAVIEW_EXTRA_EXTERNAL_MODULES})
  OPTION(PARAVIEW_USE_${external} "Build using ${external} library. Requires access to ${external} libraries" OFF)
  MARK_AS_ADVANCED(PARAVIEW_USE_${external})
  IF(PARAVIEW_USE_${external})
    FIND_PATH(${external}_SOURCE_DIR 
      ${external}ParaViewImport.cmake
      ${CMAKE_CURRENT_SOURCE_DIR}/../${external}
      ${CMAKE_CURRENT_SOURCE_DIR}/vtkSNL/IO
      ${CMAKE_CURRENT_SOURCE_DIR}/../vtkSNL/IO
      ${CMAKE_CURRENT_SOURCE_DIR}/../../vtkSNL/IO
      ${CMAKE_CURRENT_SOURCE_DIR}/../../../vtkSNL/IO
      )

    IF(EXISTS ${${external}_SOURCE_DIR}/${external}ParaViewImport.cmake)
      INCLUDE(${${external}_SOURCE_DIR}/${external}ParaViewImport.cmake)
    ELSE(EXISTS ${${external}_SOURCE_DIR}/${external}ParaViewImport.cmake)
      MESSAGE(SEND_ERROR "Cannot find ${external} sources directory: ${${external}_SOURCE_DIR}")
    ENDIF(EXISTS ${${external}_SOURCE_DIR}/${external}ParaViewImport.cmake)
    SET(PARAVIEW_CONFIG_EXTRA_DEFINES
      "${PARAVIEW_CONFIG_EXTRA_DEFINES}\n#define PARAVIEW_USE_${external} 1")
  ELSE(PARAVIEW_USE_${external})
    SET(PARAVIEW_CONFIG_EXTRA_DEFINES
      "${PARAVIEW_CONFIG_EXTRA_DEFINES}\n/* #undef PARAVIEW_USE_${external} */")
  ENDIF(PARAVIEW_USE_${external})
ENDFOREACH(external)
IF(PARAVIEW_USE_AMRCTH OR AMRCTH_SOURCE_DIR)
  MESSAGE("Use of external AMRCTH code was removed since the code is now present in the ParaView. Please remove the cache variables: PARAVIEW_USE_AMRCTH and AMRCTH_SOURCE_DIR")
ENDIF(PARAVIEW_USE_AMRCTH OR AMRCTH_SOURCE_DIR)

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
# Configure Python executable
IF(PARAVIEW_WRAP_PYTHON)
  SUBDIRS(Utilities/VTKPythonWrapping/Executable)
ENDIF(PARAVIEW_WRAP_PYTHON)

#########################################################################
# Configure Servers executables
SUBDIRS(Servers/Executables)


#########################################################################
CONFIGURE_FILE(${ParaView_SOURCE_DIR}/CMake/CTestCustom.ctest.in
  ${ParaView_BINARY_DIR}/CTestCustom.ctest @ONLY)

#########################################################################
# Enable shared link forwarding support if needed.
SET(PV_EXE_SUFFIX)
SET(PV_EXE_INSTALL ${PV_INSTALL_BIN_DIR})
IF(BUILD_SHARED_LIBS AND CMAKE_SKIP_RPATH)
  IF(NOT WIN32)
    SET(PV_NEED_SHARED_FORWARD 1)
    SET(PV_EXE_SUFFIX -real)
    SET(PV_EXE_INSTALL ${PV_INSTALL_LIB_DIR})
    SET(PV_FORWARD_DIR_BUILD "${EXECUTABLE_OUTPUT_PATH}")
    SET(PV_FORWARD_DIR_INSTALL "..${PV_EXE_INSTALL}")
    SET(PV_FORWARD_PATH_BUILD "\"${PV_FORWARD_DIR_BUILD}\"")
    SET(PV_FORWARD_PATH_INSTALL "\"${PV_FORWARD_DIR_INSTALL}\"")
    FOREACH(ldir ${PARAVIEW_EXTRA_LINK_DIRECTORIES})
      SET(PV_FORWARD_PATH_BUILD "${PV_FORWARD_PATH_BUILD}, \"${ldir}\"")
      SET(PV_FORWARD_PATH_INSTALL "${PV_FORWARD_PATH_INSTALL}, \"${ldir}\"")
    ENDFOREACH(ldir)
  ENDIF(NOT WIN32)
ENDIF(BUILD_SHARED_LIBS AND CMAKE_SKIP_RPATH)

#########################################################################
SET(PARAVIEW_INCLUDE_DIRS
  ${ParaView_SOURCE_DIR}/Utilities/VTKClientServer
  ${ParaView_SOURCE_DIR}/Utilities/hdf5
  ${KWCommon_INCLUDE_PATH}
  ${ParaView_SOURCE_DIR}/Servers/Filters
  ${ParaView_SOURCE_DIR}/Servers/ServerManager
  ${ParaView_SOURCE_DIR}/Servers/Common
#
  ${ParaView_BINARY_DIR}
  ${ParaView_BINARY_DIR}/Utilities/VTKClientServer
  ${ParaView_BINARY_DIR}/Utilities/hdf5
  ${ParaView_BINARY_DIR}/Servers/Filters
  ${ParaView_BINARY_DIR}/Servers/ServerManager
  ${ParaView_BINARY_DIR}/Servers/Common
  ${XDMF_INCLUDE_DIRS}
  )

CONFIGURE_FILE(${ParaView_SOURCE_DIR}/vtkPVConfig.h.in
  ${ParaView_BINARY_DIR}/vtkPVConfig.h
  ESCAPE_QUOTES IMMEDIATE)

