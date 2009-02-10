# Requires ParaView_SOURCE_DIR and ParaView_BINARY_DIR to be set.
#########################################################################


# GLOB_INSTALL_DEVELOPMENT: 
#     Scrape directory for glob pattern 
#     install the found files to Development
#     component.
#                   
# from:    directory to scrape.
# to:      destination
# exts:    list of glob patterns
# 
# NOTE: To search in directory other than the current
#       put the search dir in "exts".
MACRO(GLOB_INSTALL_DEVELOPMENT from to exts)
  #message(${from}\n${to})
  SET(filesToInstall)
  FOREACH(ext ${exts})
    #message(${ext})
    SET(files)
    FILE(GLOB files RELATIVE ${from} ${ext})
    IF(files)
      SET(filesToInstall "${filesToInstall};${files}")
    ENDIF(files)
  ENDFOREACH(ext)
  IF(filesToInstall)
    #message("${filesToInstall}\n\n")
    INSTALL(
        FILES ${filesToInstall}
        DESTINATION ${to}
        COMPONENT Develeopment)
  ENDIF(filesToInstall)
ENDMACRO(GLOB_INSTALL_DEVELOPMENT)

# Common settings
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
SET(VTK_USE_ANSI_STDLIB ${PARAVIEW_USE_ANSI_STDLIB})
SET(VTK_HEADER_TESTING_PY "${ParaView_SOURCE_DIR}/VTK/Common/Testing/HeaderTesting.py")
SET(VTK_PRINT_SELF_CHECK_TCL "${ParaView_SOURCE_DIR}/VTK/Common/Testing/Tcl/PrintSelfCheck.tcl")
SET(VTK_FIND_STRING_TCL "${ParaView_SOURCE_DIR}/VTK/Common/Testing/Tcl/FindString.tcl")

SET(VTK_USE_RENDERING ON CACHE INTERNAL "" FORCE)
SET(VTK_WRAP_TCL OFF CACHE INTERNAL "" FORCE)
SET(VTK_WRAP_JAVA OFF CACHE INTERNAL "" FORCE)
SET(VTK_USE_MATROX_IMAGING OFF CACHE INTERNAL "" FORCE)

SET(VTK_DIR "${ParaView_BINARY_DIR}/VTK" CACHE INTERNAL "" FORCE)
SET(VTK_SOURCE_DIR "${ParaView_SOURCE_DIR}/VTK" CACHE INTERNAL "" FORCE)
SET(VTK_CMAKE_DIR "${VTK_SOURCE_DIR}/CMake" CACHE INTERNAL "" FORCE)
SET(VTK_FOUND 1)
FIND_PATH(VTK_DATA_ROOT VTKData.readme ${ParaView_SOURCE_DIR}/../VTKData $ENV{VTK_DATA_ROOT})
MARK_AS_ADVANCED(VTK_DATA_ROOT)
MARK_AS_ADVANCED(BUILD_EXAMPLES)
MARK_AS_ADVANCED(BUILD_EXAMPLES)
MARK_AS_ADVANCED(VTK_USE_GEOVIS)
MARK_AS_ADVANCED(VTK_USE_VIEWS)
MARK_AS_ADVANCED(VTK_USE_GL2PS)

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

# ParaView needs static Tcl/Tk if not using shared libraries.
IF(NOT BUILD_SHARED_LIBS)
  SET(VTK_TCL_TK_STATIC ON CACHE INTERNAL "" FORCE)
ENDIF(NOT BUILD_SHARED_LIBS)

# Setup install directories.
IF(NOT PV_INSTALL_BIN_DIR)
  SET(PV_INSTALL_BIN_DIR bin)
ENDIF(NOT PV_INSTALL_BIN_DIR)
IF(NOT PV_INSTALL_INCLUDE_DIR)
  SET(PV_INSTALL_INCLUDE_DIR include/paraview-${PARAVIEW_VERSION})
ENDIF(NOT PV_INSTALL_INCLUDE_DIR)
IF(NOT PV_INSTALL_LIB_DIR)
  SET(PV_INSTALL_LIB_DIR lib/paraview-${PARAVIEW_VERSION})
ENDIF(NOT PV_INSTALL_LIB_DIR)
IF(NOT PV_INSTALL_DATA_DIR)
  SET(PV_INSTALL_DATA_DIR share/paraview-${PARAVIEW_VERSION})
ENDIF(NOT PV_INSTALL_DATA_DIR)

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
    SET(PV_FORWARD_DIR_INSTALL "../${PV_EXE_INSTALL}")
    SET(PV_FORWARD_PATH_BUILD "\"${PV_FORWARD_DIR_BUILD}\"")
    SET(PV_FORWARD_PATH_INSTALL "\"${PV_FORWARD_DIR_INSTALL}\"")
    FOREACH(ldir ${PARAVIEW_EXTRA_LINK_DIRECTORIES})
      SET(PV_FORWARD_PATH_BUILD "${PV_FORWARD_PATH_BUILD}, \"${ldir}\"")
      SET(PV_FORWARD_PATH_INSTALL "${PV_FORWARD_PATH_INSTALL}, \"${ldir}\"")
    ENDFOREACH(ldir)
  ENDIF(NOT WIN32)
ENDIF(BUILD_SHARED_LIBS AND CMAKE_SKIP_RPATH)

# Install no development files by default, but allow the user to get
# them installed by setting PV_INSTALL_DEVELOPMENT to true. 
OPTION(PARAVIEW_INSTALL_DEVELOPMENT "Install ParaView plugin development files." OFF)
  MARK_AS_ADVANCED(PARAVIEW_INSTALL_DEVELOPMENT)
  IF(NOT PARAVIEW_INSTALL_DEVELOPMENT)
    SET (PV_INSTALL_NO_DEVELOPMENT 1)
  ELSE (NOT PARAVIEW_INSTALL_DEVELOPMENT)
    SET (PV_INSTALL_NO_DEVELOPMENT 0)
  ENDIF(NOT PARAVIEW_INSTALL_DEVELOPMENT)

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

#PV requires minimum 2.4
SET(VTK_INSTALL_HAS_CMAKE_24 1)
# Send VTK executables to the ParaView LIBRARY directory (not a mistake).
# Send VTK include files to the ParaView include directory
# Send VTK libraries to the ParaView library directory.
SET(VTK_INSTALL_BIN_DIR "/${PV_INSTALL_BIN_DIR}")
SET(VTK_INSTALL_INCLUDE_DIR "/${PV_INSTALL_INCLUDE_DIR}")
SET(VTK_INSTALL_LIB_DIR "/${PV_INSTALL_LIB_DIR}")
SET(VTK_INSTALL_PACKAGE_DIR "/${PV_INSTALL_LIB_DIR}")
# VTK and KWCommon should install only the components paraview does.
SET(VTK_INSTALL_NO_DOCUMENTATION 1)
SET(VTK_INSTALL_NO_DEVELOPMENT ${PV_INSTALL_NO_DEVELOPMENT})
# This will disable installing of vtkpython executable and the vtk python
# module. This is essential since we don't want to conflict with kosher VTK
# installations.
SET (VTK_INSTALL_NO_PYTHON 1)
SET (VTK_INSTALL_NO_VTKPYTHON 1)
# Tell VTK to install python extension modules using CMake so they get installed
# with the other python extension modules ParaView creates.
SET (VTK_INSTALL_PYTHON_USING_CMAKE 1)

# KWCommon config
#TODO move this stuff into /ParaView3/Common/CMakeLists.txt 
SET(PV_INSTALL_HAS_CMAKE_24 1)
SET(PV_INSTALL_BIN_DIR_CM24 ${PV_INSTALL_BIN_DIR})
SET(PV_INSTALL_LIB_DIR_CM24 ${PV_INSTALL_LIB_DIR})
SET(PV_INSTALL_INCLUDE_DIR_CM24 ${PV_INSTALL_INCLUDE_DIR})
SET(KWCommon_INSTALL_BIN_DIR "/${PV_INSTALL_BIN_DIR}") #NOTE the  "/" is required by INSTALL_TARGETS
SET(KWCommon_INSTALL_LIB_DIR "/${PV_INSTALL_LIB_DIR}")
SET(KWCommon_INSTALL_DATA_DIR "/${PV_INSTALL_DATA_DIR}")
SET(KWCommon_INSTALL_INCLUDE_DIR "/${PV_INSTALL_INCLUDE_DIR}")
SET(KWCommon_INSTALL_PACKAGE_DIR "/${PV_INSTALL_LIB_DIR}")
SET(KWCommon_INSTALL_NO_LIBRARIES ${PV_INSTALL_NO_LIBRARIES})
SET(KWCommon_INSTALL_NO_DEVELOPMENT ${PV_INSTALL_NO_DEVELOPMENT})
SET(KWCommon_INSTALL_NO_RUNTIME ${PV_INSTALL_NO_RUNTIME})
SET(KWCommon_INSTALL_NO_DOCUMENTATION 1)
# ??
SET(KWCommonPro_INSTALL_BIN_DIR ${PV_INSTALL_BIN_DIR})
SET(KWCommonPro_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
SET(KWCommonPro_INSTALL_DATA_DIR ${PV_INSTALL_DATA_DIR})
SET(KWCommonPro_INSTALL_INCLUDE_DIR ${PV_INSTALL_INCLUDE_DIR})
SET(KWCommonPro_INSTALL_PACKAGE_DIR ${PV_INSTALL_LIB_DIR})
SET(KWCommonPro_INSTALL_NO_DEVELOPMENT ${PV_INSTALL_NO_DEVELOPMENT})
SET(KWCommonPro_INSTALL_NO_RUNTIME ${PV_INSTALL_NO_RUNTIME})
SET(KWCommonPro_INSTALL_NO_DOCUMENTATION 1)
SET(KWCommonPro_INSTALL_NO_DEVELOPMENT ${PV_INSTALL_NO_DEVELOPMENT})

# XDMF config
SET(XDMF_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
SET(XDMF_INSTALL_BIN_DIR ${PV_INSTALL_BIN_DIR})
SET(XDMF_INSTALL_INCLUDE_DIR "${PV_INSTALL_INCLUDE_DIR}/Xdmf")
SET(XDMF_INSTALL_INCLUDE_VTK_DIR "${PV_INSTALL_INCLUDE_DIR}/Xdmf")



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
    EXECUTE_PROCESS(COMMAND sw_vers -productVersion
      OUTPUT_VARIABLE OSX_VERSION)
    IF(OSX_VERSION MATCHES "10.[01234].")
      # The fink package on OSX sets the environment variable LD_PREBIND
      # which breaks paraview linking.  Add this option to tell the
      # linker to ignore the environment variable.
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -noprebind")
    ENDIF(OSX_VERSION MATCHES "10.[01234].")
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

# On AIX using the IBM xlC compiler check whether the compiler knows about
# -binitfini:poe_remote_main, which is required to get MPI executables working
# on Purple
IF(CMAKE_SYSTEM MATCHES AIX  AND CMAKE_CXX_COMPILER MATCHES xlC)
  INCLUDE(CheckCCompilerFlag)
  CHECK_C_COMPILER_FLAG("-binitfini:poe_remote_main" _XLC_COMPILER_HAS_INITFINI)
  IF(_XLC_COMPILER_HAS_INITFINI)
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -binitfini:poe_remote_main")
  ENDIF(_XLC_COMPILER_HAS_INITFINI)
ENDIF(CMAKE_SYSTEM MATCHES AIX  AND CMAKE_CXX_COMPILER MATCHES xlC)

# Cray Xt3/Catamount does support only a subset of networking/posix/etc. functions
# nevertheless many of them are part of the libc, but when linking the linker warns
#: warning: warning: socket is not implemented and will always fail
# --fatal-warnings makes the test fail in this case
IF(CMAKE_SYSTEM MATCHES Catamount)

   MESSAGE(STATUS "\n\n*************** -Wl,--fatal-warnings NOT enabled for tests, see ParaView3/CMake/ParaViewCommon/ for details !\n\n")

# The following line is commented out, because I didn't have time to test it.
# When uncommenting some more tests may fail, which is the intended purpose and the right thing.
#  SET(CMAKE_REQUIRED_FLAGS -Wl,--fatal-warnings)
ENDIF(CMAKE_SYSTEM MATCHES Catamount)



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

# Change VTK default, since VTK is set up to enable TK when python wrapping is
# enabled.
OPTION(VTK_USE_TK "Build VTK with Tk support" OFF)

CONFIGURE_FILE(${ParaView_SOURCE_DIR}/VTK/Utilities/TclTk/.NoDartCoverage
  ${ParaView_BINARY_DIR}/VTK/.NoDartCoverage)
ADD_SUBDIRECTORY(VTK)

#########################################################################
# Set the ICET MPI variables from the VTK ones.
# use a set cache internal so people don't try and use them
SET(ICET_MPIRUN_EXE "${VTK_MPIRUN_EXE}" CACHE INTERNAL 
  "This is set from VTK_MPIRUN_EXE.")
SET(ICET_MPI_PREFLAGS 
  "${VTK_MPI_PRENUMPROC_FLAGS};${VTK_MPI_NUMPROC_FLAG};${VTK_MPI_MAX_NUMPROCS};${VTK_MPI_PREFLAGS}" CACHE INTERNAL
  "This is set from a combination of VTK_MPI_PREFLAGS VTK_MPI_NUMPROC_FLAG VTK_MPI_MAX_NUMPROCS VTK_MPI_PREFLAGS.")
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

IF(VTK_USE_INFOVIS)
  SET(VTK_INCLUDE_DIR ${VTK_INCLUDE_DIR}
    ${ParaView_SOURCE_DIR}/VTK/Infovis
    ${ParaView_BINARY_DIR}/VTK/Infovis
    )
ENDIF(VTK_USE_INFOVIS)

IF(VTK_USE_VIEWS)
  SET(VTK_INCLUDE_DIR ${VTK_INCLUDE_DIR}
    ${ParaView_SOURCE_DIR}/VTK/Views
    ${ParaView_BINARY_DIR}/VTK/Views
  )
ENDIF(VTK_USE_VIEWS)

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
# Configure Python wrapping
IF(PARAVIEW_ENABLE_PYTHON)
  FIND_PACKAGE(PythonLibs REQUIRED)
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
  ADD_SUBDIRECTORY(Utilities/VTKPythonWrapping)
  IF(PV_INSTALL_NO_LIBRARIES)
    SET(VTKPythonWrapping_INSTALL_LIBRARIES 0)
  ELSE(PV_INSTALL_NO_LIBRARIES)
    SET(VTKPythonWrapping_INSTALL_LIBRARIES 1)
  ENDIF(PV_INSTALL_NO_LIBRARIES)
  SET(VTKPythonWrapping_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
  SET(VTKPythonWrapping_INSTALL_BIN_DIR ${PV_INSTALL_BIN_DIR})
  MARK_AS_ADVANCED(PYTHON_INCLUDE_PATH PYTHON_LIBRARY)
ENDIF(PARAVIEW_ENABLE_PYTHON)

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
  IF(VTK_USE_SYSTEM_ZLIB)
    SET(HDF5_ZLIB_HEADER "zlib.h")
  ELSE(VTK_USE_SYSTEM_ZLIB)
    SET(HDF5_ZLIB_HEADER "vtk_zlib.h")
  ENDIF(VTK_USE_SYSTEM_ZLIB)
  SET(HDF5_INCLUDE_DIR 
    ${ParaView_SOURCE_DIR}/Utilities/hdf5
    ${ParaView_BINARY_DIR}/Utilities/hdf5)

  SET(HDF5_CONFIG ${ParaView_BINARY_DIR}/Utilities/hdf5/HDF5Config.cmake)
  ADD_SUBDIRECTORY(Utilities/hdf5)

ENDIF(PARAVIEW_USE_SYSTEM_HDF5)

#########################################################################
# Configure SILO
OPTION(PARAVIEW_USE_SILO "Use SILO library" OFF)
MARK_AS_ADVANCED(PARAVIEW_USE_SILO)
IF(PARAVIEW_USE_SILO)
  INCLUDE(${ParaView_SOURCE_DIR}/CMake/FindSILO.cmake)
ENDIF(PARAVIEW_USE_SILO)

#########################################################################
# Configure Xdmf

SET(XDMF_INSTALL_NO_DEVELOPMENT ${PV_INSTALL_NO_DEVELOPMENT})
SET(XDMF_INSTALL_NO_RUNTIME ${PV_INSTALL_NO_RUNTIME})
SET(XDMF_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
SET(XDMF_REGENERATE_YACCLEX OFF CACHE INTERNAL "" FORCE)
SET(XDMF_REGENERATE_WRAPPERS OFF CACHE INTERNAL "" FORCE)
SET(XDMF_WRAP_PYTHON OFF CACHE INTERNAL "" FORCE)
SET(XDMF_WRAP_TCL OFF CACHE INTERNAL "" FORCE)
SET(XDMF_KITS_DIR "${ParaView_BINARY_DIR}/Utilities/Xdmf2/vtk/Utilities")
SET(XDMF_INCLUDE_DIRS
  "${ParaView_SOURCE_DIR}/Utilities/Xdmf2/vtk"
  "${ParaView_BINARY_DIR}/Utilities/Xdmf2/vtk")
SET(PARAVIEW_LINK_XDMF ON)
ADD_SUBDIRECTORY(Utilities/Xdmf2)


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
    IF (PARAVIEW_TEST_COMPOSITING)
      # Force the testing of IceT if we are testing compositing.
      SET(ICET_BUILD_TESTING ON
        CACHE BOOL "Build and run the ICE-T tests." FORCE)
    ENDIF (PARAVIEW_TEST_COMPOSITING)
  ENDIF (BUILD_TESTING)
  IF(PARAVIEW_USE_ICE_T)
    SET(ICE_T_INCLUDE_DIR 
      ${ParaView_SOURCE_DIR}/Utilities/IceT/src/include
      ${ParaView_BINARY_DIR}/Utilities/IceT/src/include
      )
    ADD_SUBDIRECTORY(Utilities/IceT)
  ENDIF(PARAVIEW_USE_ICE_T)

  # Needed for mpich 2
  ADD_DEFINITIONS("-DMPICH_IGNORE_CXX_SEEK")
ENDIF(VTK_USE_MPI)

#########################################################################
# Configure Common
ADD_SUBDIRECTORY(Common)

#########################################################################
# Configure VTKClientServer wrapping
SET(VTKCLIENTSERVER_INCLUDE_DIR
  ${ParaView_SOURCE_DIR}/Utilities/VTKClientServer
  ${ParaView_BINARY_DIR}/Utilities/VTKClientServer
  )
ADD_SUBDIRECTORY(Utilities/VTKClientServer)

#########################################################################
# Configure Tcl Wraping.
# We can't remove this from ParaViewCommon.cmake since it must be
# included after VTK has been included but before ServerManager.
IF (PARAVIEW_BUILD_GUI)
  ADD_SUBDIRECTORY(Utilities/VTKTclWrapping)
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
    IF(NOT "${current_dirs}" STREQUAL "${dir}")
      SET(current_dirs ${current_dirs} "${dir}")
    ENDIF(NOT "${current_dirs}" STREQUAL "${dir}")
  ENDFOREACH(dir)
  SET(PARAVIEW_EXTRA_LINK_DIRECTORIES "${current_dirs}" CACHE INTERNAL "")
ENDMACRO(PARAVIEW_EXTRA_LINK_DIRECTORIES)
LINK_DIRECTORIES(${PARAVIEW_EXTRA_LINK_DIRECTORIES})

#
# Imported project adds source files
# This ones need to be fixed, so that first are not wrapped. This will be fixed
# once the server changes are merged.
#
SET(ExtraParaViewCSWrapped_SRCS)
SET(ExtraParaViewServerNonWrapped_SRCS)
SET(ExtraParaViewServerManagerNonWrapped_SRCS)
SET(ExtraParaViewClient_SRCS)
SET(ExtraParaViewBinary_SRCS)
SET(ExtraParaViewServer_SRCS)
SET(ExtraParaViewServerFiltersIncludes)
SET(ExtraParaViewGUIIncludes)

MACRO(PARAVIEW_INCLUDE_CS_WRAPPED_SOURCES SRCS)
  SET(ExtraParaViewCSWrapped_SRCS
    ${ExtraParaViewCSWrapped_SRCS} ${SRCS})
ENDMACRO(PARAVIEW_INCLUDE_CS_WRAPPED_SOURCES)

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
FOREACH(external ${PARAVIEW_EXTRA_EXTERNAL_MODULES})
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

ADD_SUBDIRECTORY(Servers)

#########################################################################
# Configure Python executable
IF(PARAVIEW_ENABLE_PYTHON)
  ADD_SUBDIRECTORY(Utilities/VTKPythonWrapping/Executable)
ENDIF(PARAVIEW_ENABLE_PYTHON)

#########################################################################
# Configure Servers executables
ADD_SUBDIRECTORY(Servers/Executables)


#########################################################################
CONFIGURE_FILE(${ParaView_SOURCE_DIR}/CMake/CTestCustom.ctest.in
  ${ParaView_BINARY_DIR}/CTestCustom.ctest @ONLY)

#########################################################################
SET(PARAVIEW_INCLUDE_DIRS
  ${ParaView_SOURCE_DIR}/Utilities/VTKClientServer
  ${KWCommon_INCLUDE_PATH}
  ${ParaView_SOURCE_DIR}/Servers/Filters
  ${ParaView_SOURCE_DIR}/Servers/ServerManager
  ${ParaView_SOURCE_DIR}/Servers/Common
  ${ParaView_SOURCE_DIR}/Utilities/VTKPythonWrapping/Executable
#
  ${ParaView_BINARY_DIR}
  ${ParaView_BINARY_DIR}/Utilities/VTKClientServer
  ${ParaView_BINARY_DIR}/Servers/Filters
  ${ParaView_BINARY_DIR}/Servers/ServerManager
  ${ParaView_BINARY_DIR}/Servers/Common
  ${XDMF_INCLUDE_DIRS}
  ${HDF5_INCLUDE_DIR}
  )

CONFIGURE_FILE(${ParaView_SOURCE_DIR}/vtkPVConfig.h.in
  ${ParaView_BINARY_DIR}/vtkPVConfig.h
  ESCAPE_QUOTES IMMEDIATE)

