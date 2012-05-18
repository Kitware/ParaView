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
  SET(filesToInstall)
  FOREACH(ext ${exts})
    SET(files)
    FILE(GLOB files RELATIVE ${from} ${ext})
    IF(files)
      SET(filesToInstall "${filesToInstall};${files}")
    ENDIF(files)
  ENDFOREACH(ext)
  IF(filesToInstall)
    INSTALL(
        FILES ${filesToInstall}
        DESTINATION ${to}
        COMPONENT Development)
  ENDIF(filesToInstall)
ENDMACRO(GLOB_INSTALL_DEVELOPMENT)

# GLOB_RECURSIVE_INSTALL_DEVELOPMENT:
#     Scrape directory for glob pattern
#     install the found files to Development
#     component.  Will recurse subdirectories
#
# from:    directory to scrape.
# to:      destination
# exts:    list of glob patterns
MACRO(GLOB_RECURSIVE_INSTALL_DEVELOPMENT from to exts)  
  SET(filesToInstall)
  FOREACH(ext ${exts})
    SET(files)
    FILE(GLOB_RECURSE files RELATIVE ${from} ${ext})
    IF(files)
      SET(filesToInstall "${filesToInstall};${files}")
    ENDIF(files)
  ENDFOREACH(ext)
  IF(filesToInstall)
    INSTALL(
        FILES ${filesToInstall}
        DESTINATION ${to}
        COMPONENT Development)
  ENDIF(filesToInstall)
ENDMACRO(GLOB_RECURSIVE_INSTALL_DEVELOPMENT)

# Common settings
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${ParaView_SOURCE_DIR}/VTK/CMake")

# Choose static or shared libraries.
option(BUILD_SHARED_LIBS "Build VTK with shared libraries." ON)

set(PV_INSTALL_EXPORT_NAME ParaViewTargets)
set(VTK_INSTALL_EXPORT_NAME ${PV_INSTALL_EXPORT_NAME})
set(HDF5_EXPORTED_TARGETS ${PV_INSTALL_EXPORT_NAME})

# Configure VTK library versions to be paraview-specific.
SET(VTK_NO_LIBRARY_VERSION 1)
SET(VTK_LIBRARY_PROPERTIES VERSION "pv${PARAVIEW_VERSION}")

# Setup output directories.
SET (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL
  "Single output directory for building all executables.")

# Use the new version of the variable names for output of build process.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if(UNIX)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
else()
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

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
SET(VTK_HEADER_TESTING_PY "${ParaView_SOURCE_DIR}/VTK/Testing/Core/HeaderTesting.py")
SET(VTK_PRINT_SELF_CHECK_TCL "${ParaView_SOURCE_DIR}/VTK/Common/Testing/Tcl/PrintSelfCheck.tcl")
SET(VTK_FIND_STRING_TCL "${ParaView_SOURCE_DIR}/VTK/Common/Testing/Tcl/FindString.tcl")

SET(VTK_WRAP_TCL OFF CACHE INTERNAL "" FORCE)
SET(VTK_USE_MATROX_IMAGING OFF CACHE INTERNAL "" FORCE)

SET(VTK_DIR "${ParaView_BINARY_DIR}/VTK" CACHE INTERNAL "" FORCE)
SET(VTK_SOURCE_DIR "${ParaView_SOURCE_DIR}/VTK" CACHE INTERNAL "" FORCE)
SET(VTK_CMAKE_DIR "${VTK_SOURCE_DIR}/CMake" CACHE INTERNAL "" FORCE)
SET(VTK_FOUND 1)
FIND_PATH(VTK_DATA_ROOT VTKData.readme ${ParaView_SOURCE_DIR}/../VTKData $ENV{VTK_DATA_ROOT})
FIND_PATH(VTK_LARGE_DATA_ROOT VTKLargeData.readme ${ParaView_SOURCE_DIR}/../VTKLargeData $ENV{VTK_LARGE_DATA_ROOT})
MARK_AS_ADVANCED(VTK_DATA_ROOT)
MARK_AS_ADVANCED(VTK_LARGE_DATA_ROOT)
MARK_AS_ADVANCED(BUILD_EXAMPLES)
MARK_AS_ADVANCED(BUILD_EXAMPLES)
MARK_AS_ADVANCED(VTK_USE_GL2PS)
MARK_AS_ADVANCED(VTK_WRAP_JAVA)

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
IF(NOT PV_INSTALL_PLUGIN_DIR)
  IF(WIN32)
    SET(PV_INSTALL_PLUGIN_DIR ${PV_INSTALL_BIN_DIR})
  ELSE(WIN32)
    SET(PV_INSTALL_PLUGIN_DIR ${PV_INSTALL_LIB_DIR})
  ENDIF(WIN32)
ENDIF(NOT PV_INSTALL_PLUGIN_DIR)
IF(NOT PV_INSTALL_DATA_DIR)
  SET(PV_INSTALL_DATA_DIR share/paraview-${PARAVIEW_VERSION})
ENDIF(NOT PV_INSTALL_DATA_DIR)
IF(NOT PV_INSTALL_CMAKE_DIR)
  SET(PV_INSTALL_CMAKE_DIR ${PV_INSTALL_LIB_DIR}/CMake)
ENDIF(NOT PV_INSTALL_CMAKE_DIR)
IF(NOT PV_INSTALL_DOC_DIR)
  SET(PV_INSTALL_DOC_DIR share/doc/paraview-${PARAVIEW_VERSION})
ENDIF(NOT PV_INSTALL_DOC_DIR)

#########################################################################
# Install no development files by default, but allow the user to get
# them installed by setting PV_INSTALL_DEVELOPMENT to true.
OPTION(PARAVIEW_INSTALL_DEVELOPMENT "Install ParaView plugin development files." OFF)
MARK_AS_ADVANCED(PARAVIEW_INSTALL_DEVELOPMENT)
IF(NOT PARAVIEW_INSTALL_DEVELOPMENT)
  SET (PV_INSTALL_NO_DEVELOPMENT 1)
  SET(QT_TESTING_INSTALL_DEVELOPMENT 0)
ELSE (NOT PARAVIEW_INSTALL_DEVELOPMENT)
  SET (PV_INSTALL_NO_DEVELOPMENT 0)
  SET (QT_TESTING_INSTALL_DEVELOPMENT 1)
ENDIF(NOT PARAVIEW_INSTALL_DEVELOPMENT)

OPTION(PARAVIEW_INSTALL_THIRD_PARTY_LIBRARIES "Enable installation of third party libraries such as Qt and FFMPEG." OFF)
MARK_AS_ADVANCED(PARAVIEW_INSTALL_THIRD_PARTY_LIBRARIES)

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
set(VTK_INSTALL_BIN_DIR "${PV_INSTALL_BIN_DIR}")
set(VTK_INSTALL_INCLUDE_DIR "${PV_INSTALL_INCLUDE_DIR}")
set(VTK_INSTALL_LIB_DIR "${PV_INSTALL_LIB_DIR}")
set(VTK_INSTALL_PACKAGE_DIR "${PV_INSTALL_LIB_DIR}")
set(VTK_MODULES_DIR ${CMAKE_BINARY_DIR}/${VTK_INSTALL_PACKAGE_DIR}/Modules)
# VTK and KWCommon should install only the components paraview does.
SET(VTK_INSTALL_NO_DOCUMENTATION 1)
SET(VTK_INSTALL_NO_DEVELOPMENT ${PV_INSTALL_NO_DEVELOPMENT})
# This will disable installing of vtkpython executable and the vtk python
# module. This is essential since we don't want to conflict with kosher VTK
# installations.
SET (VTK_INSTALL_NO_PYTHON ON)
SET (VTK_INSTALL_NO_VTKPYTHON ON)
# --- this code seems to be half-baked, so disabling that for now. Let's just do
# traditional python module installation for this release ---
## Tell VTK to install python extension modules using CMake so they get installed
## with the other python extension modules ParaView creates.
##IF(WIN32)
##  SET (VTK_INSTALL_PYTHON_USING_CMAKE ON)
##ELSE()
##  SET (VTK_INSTALL_PYTHON_USING_CMAKE OFF)
##ENDIF()
SET (VTK_INSTALL_PYTHON_USING_CMAKE ON)

SET (VTK_INSTALL_NO_QT_PLUGIN ON)
SET (VTK_INSTALL_NO_LIBRARIES ${PV_INSTALL_NO_LIBRARIES})

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

# XDMF config
SET(XDMF_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
SET(XDMF_INSTALL_BIN_DIR ${PV_INSTALL_BIN_DIR})
SET(XDMF_INSTALL_INCLUDE_DIR "${PV_INSTALL_INCLUDE_DIR}/Xdmf")
SET(XDMF_INSTALL_INCLUDE_VTK_DIR "${PV_INSTALL_INCLUDE_DIR}/Xdmf")
SET(XDMF_INSTALL_EXPORT_NAME ${PV_INSTALL_EXPORT_NAME})
SET(XDMF_WRAP_PYTHON_INSTALL_DIR ${PV_INSTALL_LIB_DIR}/site-packages/Xdmf)

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

# always require standards-compliant C++ environment
set(PARAVIEW_USE_ANSI_STDLIB TRUE)

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

# -----------------------------------------------------------------------------
# Disable deprecation warnings for standard C and STL functions in VS2005 and
# later (no, we don't need IF(CMAKE_COMPILER_2005) ... )
# -----------------------------------------------------------------------------
IF(MSVC_VERSION EQUAL 1400 OR MSVC_VERSION GREATER 1400 OR MSVC10)
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS)
  ADD_DEFINITIONS(-D_SCL_SECURE_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
ENDIF(MSVC_VERSION EQUAL 1400 OR MSVC_VERSION GREATER 1400 OR MSVC10)

# On AIX using the IBM xlC compiler check whether the compiler knows about
# -binitfini:poe_remote_main, which is required to get MPI executables working
# on Purple
IF(CMAKE_SYSTEM MATCHES AIX  AND CMAKE_CXX_COMPILER MATCHES xlC)
  INCLUDE(CheckCCompilerFlag)
  CHECK_C_COMPILER_FLAG("-binitfini:poe_remote_main" _XLC_COMPILER_HAS_INITFINI)
  IF(_XLC_COMPILER_HAS_INITFINI AND PARAVIEW_USE_MPI)
    SET(MPI_EXTRA_LIBRARY "${MPI_EXTRA_LIBRARY} -binitfini:poe_remote_main" CACHE STRING "" FORCE)
  ENDIF(_XLC_COMPILER_HAS_INITFINI AND PARAVIEW_USE_MPI)
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

# Is this a 32 bit or 64bit build. Display this in about dialog.
if("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
  set(PARAVIEW_BUILD_ARCHITECTURE "64")
else()
  set(PARAVIEW_BUILD_ARCHITECTURE "32")
endif()

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

OPTION(PARAVIEW_ALWAYS_SECURE_CONNECTION
  "Build ParaView with enforced secure connection (--connect-id)" OFF)
MARK_AS_ADVANCED(PARAVIEW_ALWAYS_SECURE_CONNECTION)

# Change VTK default, since VTK is set up to enable TK when python wrapping is
# enabled.
OPTION(VTK_USE_TK "Build VTK with Tk support" OFF)

# Set this to get VTKs FOR LOOP "fix" to apply too all of Paraviews Source.
SET(VTK_USE_FOR_SCOPE_WORKAROUND TRUE)

OPTION(PARAVIEW_DISABLE_VTK_TESTING "Disable VTK Testing" OFF)
MARK_AS_ADVANCED(PARAVIEW_DISABLE_VTK_TESTING)
IF (PARAVIEW_DISABLE_VTK_TESTING)
  SET (__pv_build_testing ${BUILD_TESTING})
  SET (__pv_build_examples ${BUILD_EXAMPLES})
  SET (BUILD_TESTING OFF)
  SET (BUILD_EXAMPLES OFF)
ENDIF (PARAVIEW_DISABLE_VTK_TESTING)
ADD_SUBDIRECTORY(VTK)
IF (PARAVIEW_DISABLE_VTK_TESTING)
  SET (BUILD_TESTING ${__pv_build_testing})
  SET (BUILD_EXAMPLES ${__pv_build_examples})
ENDIF (PARAVIEW_DISABLE_VTK_TESTING)

#########################################################################
# Set the ICET MPI variables from the VTK ones.
# use a set cache internal so people don't try and use them
SET(ICET_MPIRUN_EXE "${VTK_MPIRUN_EXE}" CACHE INTERNAL
  "This is set from VTK_MPIRUN_EXE.")
SET(ICET_MPI_NUMPROC_FLAG
  "${VTK_MPI_PRENUMPROC_FLAGS};${VTK_MPI_NUMPROC_FLAG}"
  CACHE INTERNAL
  "This is set from a combination of VTK_MPI_PRENUMPROC_FLAGS and VTK_MPI_NUMPROC_FLAG"
)
SET(ICET_MPI_PREFLAGS "${VTK_MPI_PREFLAGS}" CACHE INTERNAL
  "This is set from VTK_MPI_PREFLAGS.")
SET(ICET_MPI_POSTFLAGS "${VTK_MPI_POSTFLAGS}"  CACHE INTERNAL
  "This is set from VTK_MPI_POSTFLAGS.")
SET(ICET_MPI_MAX_NUMPROCS "${VTK_MPI_MAX_NUMPROCS}"  CACHE INTERNAL
  "This is set from VTK_MPI_MAX_NUMPROCS.")


# include VTKConfig.cmake so that VTK_<blah> variables are defined
# in ParaView scope.
SET(VTK_DIR ${PARAVIEW_VTK_DIR})
FIND_PACKAGE(VTK)
IF(VTK_FOUND)
  include(${ParaView_BINARY_DIR}/VTK/VTKConfig.cmake)
ENDIF()

include_directories(${VTK_INCLUDE_DIRS})
set(VTK_INCLUDE_DIR ${VTK_INCLUDE_DIRS})

set(VTK_INCLUDE_DIR ${VTK_INCLUDE_DIR}
  ${ParaView_SOURCE_DIR}/VTK
  ${ParaView_BINARY_DIR}/VTK
  ${ParaView_SOURCE_DIR}/VTK/Utilities
  ${ParaView_BINARY_DIR}/VTK/Utilities
  ${ParaView_SOURCE_DIR}/VTK/Wrapping
  ${ParaView_BINARY_DIR}/VTK/Wrapping
  )

IF(PARAVIEW_ENABLE_PYTHON)
  SET(VTK_INCLUDE_DIR ${VTK_INCLUDE_DIR}
    ${ParaView_SOURCE_DIR}/VTK/Wrapping/Python
    ${ParaView_BINARY_DIR}/VTK/Wrapping/Python
    ${ParaView_SOURCE_DIR}/VTK/Wrapping/PythonCore
    )
ENDIF(PARAVIEW_ENABLE_PYTHON)




SET(VTK_ZLIB_LIBRARIES ${vtkzlib_LIBRARIES})
SET(VTK_ZLIB_INCLUDE_DIRS ${vtkzlib_INCLUDE_DIRS})
SET(VTKZLIB_INCLUDE_DIR ${vtkzlib_INCLUDE_DIRS})

#########################################################################
# Configure HDF5
set(HDF5_INCLUDE_DIR ${vtkhdf5_INCLUDE_DIRS})
set(PARAVIEW_HDF5_LIBRARIES ${vtkhdf5_LIBRARIES})

# Override QtTesting install variables
IF (PARAVIEW_BUILD_QT_GUI)
  SET(QtTesting_INSTALL_BIN_DIR ${PV_INSTALL_BIN_DIR})
  SET(QtTesting_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
  SET(QtTesting_INSTALL_CMAKE_DIR ${PV_INSTALL_CMAKE_DIR})
  SET(QT_TESTING_INSTALL_EXPORT_NAME ${PV_INSTALL_EXPORT_NAME})
  IF (NOT PV_INSTALL_NO_LIBRARIES)
    SET_PROPERTY(GLOBAL APPEND PROPERTY VTK_TARGETS QtTesting)
  ENDIF (NOT PV_INSTALL_NO_LIBRARIES)
ENDIF()

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
# Configure mpi4py
IF(PARAVIEW_ENABLE_PYTHON AND PARAVIEW_USE_MPI)
  ADD_SUBDIRECTORY(Utilities/mpi4py)
ENDIF(PARAVIEW_ENABLE_PYTHON AND PARAVIEW_USE_MPI)



#########################################################################
# Configure Xdmf

SET(XDMF_INSTALL_NO_DEVELOPMENT ${PV_INSTALL_NO_DEVELOPMENT})
SET(XDMF_INSTALL_NO_RUNTIME ${PV_INSTALL_NO_RUNTIME})
SET(XDMF_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
SET(XDMF_REGENERATE_YACCLEX OFF CACHE INTERNAL "" FORCE)
SET(XDMF_REGENERATE_WRAPPERS OFF CACHE INTERNAL "" FORCE)
# Xdmf needs packaging fixes for this to be enabled.
SET(XDMF_WRAP_PYTHON OFF CACHE INTERNAL "" FORCE)
MARK_AS_ADVANCED(FORCE XDMF_WRAP_PYTHON XDMF_WRAP_CSHARP)
MARK_AS_ADVANCED(XDMF_REGENERATE_WRAPPERS XDMF_REGENERATE_YACCLEX)

SET(XDMF_WRAP_TCL OFF CACHE INTERNAL "" FORCE)
SET(XDMF_KITS_DIR "${ParaView_BINARY_DIR}/Utilities/Xdmf2/vtk/Utilities")
SET(XDMF_INCLUDE_DIRS
  "${ParaView_SOURCE_DIR}/Utilities/Xdmf2/vtk"
  "${ParaView_BINARY_DIR}/Utilities/Xdmf2/vtk")
SET(PARAVIEW_LINK_XDMF ON)

IF (NOT PV_INSTALL_NO_LIBRARIES)
  SET_PROPERTY(GLOBAL APPEND PROPERTY VTK_TARGETS Xdmf)
ENDIF (NOT PV_INSTALL_NO_LIBRARIES)
IF(XDMF_WRAP_PYTHON)
  SET_PROPERTY(GLOBAL APPEND PROPERTY VTK_TARGETS _Xdmf)
Endif()

ADD_SUBDIRECTORY(Utilities/Xdmf2)

#########################################################################
# Configure protobuf
SET (PROTOBUF_INSTALL_BIN_DIR ${PV_INSTALL_BIN_DIR})
SET (PROTOBUF_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
SET (PROTOBUF_INSTALL_EXPORT_NAME ${PV_INSTALL_EXPORT_NAME})
IF (NOT PV_INSTALL_NO_LIBRARIES)
  SET_PROPERTY(GLOBAL APPEND PROPERTY VTK_TARGETS protobuf)
ENDIF (NOT PV_INSTALL_NO_LIBRARIES)
ADD_SUBDIRECTORY(Utilities/protobuf)

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
# Configure chroma-subsampling of the Ogg/Theora writer
IF(Module_vtkoggtheora)
  SET(PARAVIEW_OGGTHEORA_USE_SUBSAMPLING FALSE CACHE STRING
    "Use 4:2:0 chroma-subsampling when writing Ogg/Theora movies")
  MARK_AS_ADVANCED(PARAVIEW_OGGTHEORA_USE_SUBSAMPLING)
  IF(VTK_USE_SYSTEM_OGGTHEORA AND OGGTHEORA_NO_444_SUBSAMPLING)
    SET(PARAVIEW_OGGTHEORA_USE_SUBSAMPLING TRUE CACHE STRING
      "Must use 4:2:0 subsampling with this version of system-Ogg/Theora." FORCE)
  ENDIF(VTK_USE_SYSTEM_OGGTHEORA AND OGGTHEORA_NO_444_SUBSAMPLING)
ENDIF()

#########################################################################
# Configure IceT
SET(ICET_INSTALL_BIN_DIR ${PV_INSTALL_BIN_DIR})
SET(ICET_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
SET(ICET_INSTALL_INCLUDE_DIR ${PV_INSTALL_INCLUDE_DIR})
SET(ICET_INSTALL_NO_DEVELOPMENT ${PV_INSTALL_NO_DEVELOPMENT})
SET(ICET_INSTALL_EXPORT_NAME ${PV_INSTALL_EXPORT_NAME})
MARK_AS_ADVANCED(CLEAR PARAVIEW_USE_MPI)

IF(PARAVIEW_USE_MPI)
  OPTION(PARAVIEW_USE_ICE_T "Use IceT multi display manager" ON)
  MARK_AS_ADVANCED(PARAVIEW_USE_ICE_T)
  IF (BUILD_TESTING)
    OPTION(ICET_BUILD_TESTING "Build and run the IceT tests." ON)
    MARK_AS_ADVANCED(ICET_BUILD_TESTING)
    IF (PARAVIEW_TEST_COMPOSITING)
      # Force the testing of IceT if we are testing compositing.
      SET(ICET_BUILD_TESTING ON CACHE BOOL "Build and run the IceT tests." FORCE)
    ENDIF (PARAVIEW_TEST_COMPOSITING)
  ELSE(BUILD_TESTING)
    IF(ICET_BUILD_TESTING)
      SET(ICET_BUILD_TESTING OFF CACHE BOOL "Build and run the IceT tests." FORCE)
    ENDIF(ICET_BUILD_TESTING)
  ENDIF (BUILD_TESTING)
  IF(PARAVIEW_USE_ICE_T)

    # Export IceT's Targets
    IF (NOT PV_INSTALL_NO_LIBRARIES)
      SET_PROPERTY(GLOBAL APPEND PROPERTY VTK_TARGETS IceTCore IceTMPI IceTGL)
    ENDIF (NOT PV_INSTALL_NO_LIBRARIES)

    SET(ICET_INCLUDE_DIR
      ${ParaView_SOURCE_DIR}/Utilities/IceT/src/include
      ${ParaView_BINARY_DIR}/Utilities/IceT/src/include
      )
    ADD_SUBDIRECTORY(Utilities/IceT)

  ENDIF(PARAVIEW_USE_ICE_T)

  # Needed for mpich 2
  ADD_DEFINITIONS("-DMPICH_IGNORE_CXX_SEEK")
ENDIF()

 #########################################################################
# Configure VisItBridge
OPTION(PARAVIEW_USE_VISITBRIDGE "Use VisIt Bridge" OFF)
SET(VISITBRIDGE_USE_SILO OFF)
SET(VISITBRIDGE_USE_CGNS OFF)
SET(VISITBRIDGE_USE_MILI OFF)
IF(PARAVIEW_USE_VISITBRIDGE)
  ADD_SUBDIRECTORY(Utilities/VisItBridge)
  #these are need for the client server wrappings of the databases
  #and the avt algorithms
  SET(VISITAVTALGORITHMS_KITS_DIR
    "${ParaView_BINARY_DIR}/Utilities/VisItBridge/AvtAlgorithms/Utilities")
  SET(VISITAVTALGORITHMS_INCLUDE_DIRS
    "${ParaView_SOURCE_DIR}/Utilities/VisItBridge/AvtAlgorithms"
    "${ParaView_BINARY_DIR}/Utilities/VisItBridge/AvtAlgorithms"
    )

  SET(VISITDATABASES_KITS_DIR
    "${ParaView_BINARY_DIR}/Utilities/VisItBridge/databases/Utilities")
  SET(VISITDATABASES_INCLUDE_DIRS
    ${VISITAVTALGORITHMS_INCLUDE_DIRS}
    "${ParaView_SOURCE_DIR}/Utilities/VisItBridge/databases"
    "${ParaView_BINARY_DIR}/Utilities/VisItBridge/databases"
    )
  SET(VISITBRIDGE_READERS_XML_FILE "${ParaView_SOURCE_DIR}/Utilities/VisItBridge/databases/visit_readers.xml")
  SET(VISITBRIDGE_READERS_GUI_XML_FILE "${ParaView_SOURCE_DIR}/Utilities/VisItBridge/databases/visit_readers_gui.xml")

  #needed for plugins to be able to use the VisIt Plugin macros
  SET(VISITBRIDGE_CMAKE_DIR
     "${ParaView_SOURCE_DIR}/Utilities/VisItBridge/CMake")
  SET(VISITBRIDGE_USE_FILE "${ParaView_BINARY_DIR}/Utilities/VisItBridge/VisItBridgeUse.cmake")
ENDIF(PARAVIEW_USE_VISITBRIDGE)


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
IF(PARAVIEW_USE_VISITBRIDGE)
  PARAVIEW_INCLUDE_SERVERMANAGER_RESOURCES(${VISITBRIDGE_READERS_XML_FILE})
  PARAVIEW_INCLUDE_GUI_RESOURCES(${VISITBRIDGE_READERS_GUI_XML_FILE})
ENDIF(PARAVIEW_USE_VISITBRIDGE)

ADD_SUBDIRECTORY(ParaViewCore)

#########################################################################
# Configure Python executable
IF(PARAVIEW_ENABLE_PYTHON)
  ADD_SUBDIRECTORY(Utilities/VTKPythonWrapping/Executable)
ENDIF(PARAVIEW_ENABLE_PYTHON)

#########################################################################
# Configure Paraview Minimum Settings
OPTION(PARAVIEW_MINIMAL_BUILD "Compile paraview for minimum image" OFF)
MARK_AS_ADVANCED(PARAVIEW_MINIMAL_BUILD)
IF(PARAVIEW_MINIMAL_BUILD)
  SET(PARAVIEW_MINIMAL_BUILD_CLASS_FILE "NOTFOUND" CACHE FILEPATH
    "A text file listing the vtk classes to include in the minimal initializer list.")
ENDIF(PARAVIEW_MINIMAL_BUILD)

#########################################################################
CONFIGURE_FILE(${ParaView_CMAKE_DIR}/CTestCustom.ctest.in
  ${ParaView_BINARY_DIR}/CTestCustom.ctest @ONLY)

#########################################################################
SET(PARAVIEW_INCLUDE_DIRS
  ${HDF5_INCLUDE_DIR}
  ${KWCommon_INCLUDE_PATH}
  ${PVClientServerCore_BINARY_DIR}
  ${PVClientServerCore_SOURCE_DIR}
  ${PVCommon_BINARY_DIR}
  ${PVCommon_SOURCE_DIR}
  ${PVServerImplementation_BINARY_DIR}
  ${PVServerImplementation_SOURCE_DIR}
  ${PVServerManager_BINARY_DIR}
  ${PVServerManager_SOURCE_DIR}
  ${PVVTKExtensions_BINARY_DIR}
  ${PVVTKExtensions_SOURCE_DIR}
  ${ParaView_BINARY_DIR}
  ${ParaView_BINARY_DIR}/Utilities/VTKClientServer
  ${ParaView_BINARY_DIR}/VTK/Wrapping
  ${ParaView_SOURCE_DIR}/Utilities/VTKClientServer
  ${ParaView_SOURCE_DIR}/Utilities/VTKPythonWrapping/Executable
  ${ParaView_SOURCE_DIR}/VTK/Wrapping
  ${XDMF_INCLUDE_DIRS}
  )

IF(PARAVIEW_USE_VISITBRIDGE)
  SET(PARAVIEW_INCLUDE_DIRS ${PARAVIEW_INCLUDE_DIRS}
  ${VISITAVTALGORITHMS_INCLUDE_DIRS})
ENDIF(PARAVIEW_USE_VISITBRIDGE)

# ParaView needs to know if we are on X
set(PARAVIEW_USE_X ${VTK_USE_X})

CONFIGURE_FILE(${ParaView_SOURCE_DIR}/vtkPVConfig.h.in
  ${ParaView_BINARY_DIR}/vtkPVConfig.h
  ESCAPE_QUOTES IMMEDIATE)

IF (NOT PV_INSTALL_NO_DEVELOPMENT)
  INSTALL(
      FILES  ${ParaView_BINARY_DIR}/vtkPVConfig.h
      DESTINATION ${PV_INSTALL_INCLUDE_DIR}
      COMPONENT Development)
ENDIF (NOT PV_INSTALL_NO_DEVELOPMENT)
