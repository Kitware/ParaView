CMAKE_MINIMUM_REQUIRED(VERSION 1.4)
PROJECT(PARAVIEW)
INCLUDE_REGULAR_EXPRESSION("^(vtk).*$")

# load the ClientServer command
IF (COMMAND VTK_WRAP_ClientServer)
ELSE (COMMAND VTK_WRAP_ClientServer)
    TRY_COMPILE(COMPILE_OK
         ${PARAVIEW_BINARY_DIR}/ClientServer/CMake
         ${PARAVIEW_SOURCE_DIR}/ClientServer/CMake
         ClientServer_LOADED_COMMANDS OUTPUT_VARIABLE TRYOUT)
    IF (COMPILE_OK)
         LOAD_COMMAND(VTK_WRAP_ClientServer
            ${PARAVIEW_BINARY_DIR}/ClientServer/CMake
            ${PARAVIEW_BINARY_DIR}/ClientServer/CMake/Debug)
    ELSE (COMPILE_OK)
         MESSAGE("failed to compile ParaView ClientServer extensions to CMake:\n${TRYOUT}")
    ENDIF (COMPILE_OK)
ENDIF (COMMAND VTK_WRAP_ClientServer)
UTILITY_SOURCE(VTK_WRAP_ClientServer_EXE vtkWrapClientServer ClientServer/Wrapping vtkWrapClientServer.c)
MARK_AS_ADVANCED(VTK_WRAP_ClientServer_EXE)

SUBDIRS(ClientServer/Wrapping)
#-----------------------------------------------------------------------------
# ParaView version number.  An even minor number corresponds to releases.
SET(PARAVIEW_VERSION_MAJOR 1)
SET(PARAVIEW_VERSION_MINOR 1)
SET(PARAVIEW_VERSION_PATCH 0)
SET(PARAVIEW_VERSION "${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}")
SET(PARAVIEW_VERSION_FULL "${PARAVIEW_VERSION}.${PARAVIEW_VERSION_PATCH}")

SET (LIBRARY_OUTPUT_PATH ${PARAVIEW_BINARY_DIR}/bin/ CACHE PATH "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${PARAVIEW_BINARY_DIR}/bin/ CACHE PATH "Single output directory for building all executables.")

SET (CXX_TEST_PATH ${EXECUTABLE_OUTPUT_PATH})

SET (PARAVIEW_DATA_ROOT ${PARAVIEW_SOURCE_DIR}/Data CACHE PATH "Where the Data, Baseline and Utility directories are located.")

SET (PARAVIEW_CAN_BUILD 1)

IF (WIN32)

 FIND_PATH( HTML_HELP_PATH htmlhelp.h
           "c:/program files/html help workshop"
           "c:/program files/html help workshop/include"
           "[HKEY_CURRENT_USER\\Software\\Microsoft\\HTML Help Workshop;InstallDir]/include" )

 IF (HTML_HELP_PATH)
   INCLUDE_DIRECTORIES(${HTML_HELP_PATH})
 ELSE (HTML_HELP_PATH)
   SET (PARAVIEW_CAN_BUILD 0)
   MESSAGE("ParaView requires htmlhelp to build. Please install html help."
           "You can find html help on the msdn web site here:\n"
           "http://msdn.microsoft.com/\n"
           "http://msdn.microsoft.com/library/default.asp?url=/library/en-us/htmlhelp/html/vsconwhtshw.asp")
 ENDIF (HTML_HELP_PATH)


 FIND_LIBRARY( HTML_HELP_LIBRARY htmlhelp
              "c:/program files/html help workshop"
              "c:/program files/html help workshop/LIB"
              "[HKEY_CURRENT_USER\\Software\\Microsoft\\HTML Help Workshop;InstallDir]/lib")

 IF (HTML_HELP_LIBRARY)
   LINK_LIBRARIES( ${HTML_HELP_LIBRARY} )
 ELSE (HTML_HELP_LIBRARY)
   SET (PARAVIEW_CAN_BUILD 0)
   MESSAGE("ParaView requires htmlhelp to build. Please set HTML_HELP_PATH")
 ENDIF (HTML_HELP_LIBRARY)

ENDIF (WIN32)

IF(NOT PARAVIEW_NO_HDF5)
  OPTION(PARAVIEW_USE_SYSTEM_HDF5 "Use sytstem HDF5" OFF)
ENDIF(NOT PARAVIEW_NO_HDF5)

IF(ParaViewComplete_BINARY_DIR)
  SET(VTK_NO_LIBRARY_DEPENDS 1)
  INCLUDE (${ParaViewComplete_BINARY_DIR}/VTK/VTKConfig.cmake)
  INCLUDE (${ParaViewComplete_BINARY_DIR}/VTK/UseVTK.cmake)
  IF(PARAVIEW_LINK_XDMF)
    SET(XDMF_NO_LIBRARY_DEPENDS 1)
    SET(XDMF_FOUND 1)
    INCLUDE (${ParaViewComplete_BINARY_DIR}/Xdmf/XDMFConfig.cmake)
    INCLUDE (${ParaViewComplete_SOURCE_DIR}/Xdmf/UseXDMF.cmake)
    IF(NOT PARAVIEW_NO_HDF5)
      SET(PARAVIEW_USE_SYSTEM_HDF5 "${PARAVIEW_USE_SYSTEM_HDF5}" CACHE INTERNAL "Imported from XDMF")
      SET(HDF5_LIBRARY "${HDF5_LIBRARY}" CACHE INTERNAL "Imported from XDMF")
      SET(HDF5_INCLUDE_DIR "${HDF5_INCLUDE_DIR}" CACHE INTERNAL "Imported from XDMF")
      SET(PARAVIEW_USE_SYSTEM_HDF5 1)
      SET(HDF5_LIBRARY ${XDMF_HDF5_LIBRARIES})
      SET(HDF5_INCLUDE_DIR)
    ENDIF(NOT PARAVIEW_NO_HDF5)
  ENDIF(PARAVIEW_LINK_XDMF)
ELSE(ParaViewComplete_BINARY_DIR)
  OPTION (PARAVIEW_LINK_XDMF "Link XDMF library to paraview" OFF)
  IF(PARAVIEW_LINK_XDMF)
    FIND_PACKAGE(XDMF)
    IF(XDMF_FOUND)
      INCLUDE(${XDMF_USE_FILE})
      IF(NOT XDMF_VTK_DIR)
        MESSAGE(FATAL_ERROR
          "Xdmf was not build with VTK support. "
          "Either rebuild Xdmf with VTK support or turn off "
          "linking XDMF library to ParaView")
      ENDIF(NOT XDMF_VTK_DIR)
      IF(EXISTS ${XDMF_VTK_DIR}/VTKConfig.cmake)
      ELSE(EXISTS ${XDMF_VTK_DIR}/VTKConfig.cmake)
        MESSAGE(FATAL_ERROR
          "The selected Xdmf has broken VTK support.")
      ENDIF(EXISTS ${XDMF_VTK_DIR}/VTKConfig.cmake)
    ENDIF(XDMF_FOUND)
  ENDIF(PARAVIEW_LINK_XDMF)

  # include module to find vtk
  IF(XDMF_VTK_DIR)
    SET(VTK_DIR "${XDMF_VTK_DIR}" CACHE INTERNAL "Imported from XDMF")
    SET(VTK_FOUND 1)
    INCLUDE(${XDMF_VTK_DIR}/VTKConfig.cmake)
    IF(NOT PARAVIEW_NO_HDF5)
      SET(PARAVIEW_USE_SYSTEM_HDF5 "${PARAVIEW_USE_SYSTEM_HDF5}" CACHE INTERNAL "Imported from XDMF")
      SET(HDF5_LIBRARY "${HDF5_LIBRARY}" CACHE INTERNAL "Imported from XDMF")
      SET(HDF5_INCLUDE_DIR "${HDF5_INCLUDE_DIR}" CACHE INTERNAL "Imported from XDMF")
      SET(PARAVIEW_USE_SYSTEM_HDF5 1)
      SET(HDF5_LIBRARY ${XDMF_HDF5_LIBRARIES})
      SET(HDF5_INCLUDE_DIR)
    ENDIF(NOT PARAVIEW_NO_HDF5)
  ELSE(XDMF_VTK_DIR)
    FIND_PACKAGE(VTK)
    SET(VTK_DIR "${VTK_DIR}" CACHE PATH "The directory containing VTKConfig.cmake." FORCE)
    SET(PARAVIEW_USE_SYSTEM_HDF5 "${PARAVIEW_USE_SYSTEM_HDF5}" CACHE BOOL "Use system HDF5" FORCE)
    IF("${HDF5_LIBRARY}" MATCHES "^ *$")
    ELSE("${HDF5_LIBRARY}" MATCHES "^ *$")
      SET(HDF5_LIBRARY "${HDF5_LIBRARY}" CACHE FILEPATH "Location of HDF5 library" FORCE)
    ENDIF("${HDF5_LIBRARY}" MATCHES "^ *$")
    IF("${HDF5_INCLUDE_DIR}" MATCHES "^ *$")
    ELSE("${HDF5_INCLUDE_DIR}" MATCHES "^ *$")
      SET(HDF5_INCLUDE_DIR "${HDF5_INCLUDE_DIR}" CACHE PATH "Location of HDF5 includes" FORCE)
    ENDIF("${HDF5_INCLUDE_DIR}" MATCHES "^ *$")
  ENDIF(XDMF_VTK_DIR)

  # if vtk was found include the use vtk file
  IF (VTK_FOUND)
    INCLUDE(${VTK_USE_FILE})
  ELSE(VTK_FOUND)
    SET(PARAVIEW_CAN_BUILD 0)
  ENDIF (VTK_FOUND)
ENDIF(ParaViewComplete_BINARY_DIR)

OPTION(PARAVIEW_USE_ICE_T "Build paraview using IceT" OFF)
IF(PARAVIEW_USE_ICE_T)
	INCLUDE( ${PARAVIEW_SOURCE_DIR}/FindIceT.cmake )
  INCLUDE(${ICET_USE_FILE})
ENDIF(PARAVIEW_USE_ICE_T)

IF (PARAVIEW_CAN_BUILD)
 IF ( NOT VTK_WRAP_TCL )
   MESSAGE("ParaView requires VTK to be built with Tcl bindings. Please rebuild VTK with Tcl bindings and re-run CMake")
   SET (PARAVIEW_CAN_BUILD 0)
 ENDIF ( NOT VTK_WRAP_TCL )

 IF (NOT VTK_USE_HYBRID)
   MESSAGE("ParaView requires VTK to be built with hybrid support. Please rebuild VTK with hybrid and re-run CMake")
   SET (PARAVIEW_CAN_BUILD 0)
 ENDIF (NOT VTK_USE_HYBRID)

 IF (NOT VTK_USE_RENDERING)
   MESSAGE("ParaView requires VTK to be built with rendering support. Please rebuild VTK with rendering and re-run CMake")
   SET (PARAVIEW_CAN_BUILD 0)
 ENDIF (NOT VTK_USE_RENDERING)

 IF (NOT VTK_USE_PARALLEL)
   MESSAGE("ParaView requires VTK to be built with parallel support. Please rebuild VTK with parallel and re-run CMake")
   SET (PARAVIEW_CAN_BUILD 0)
 ENDIF (NOT VTK_USE_PARALLEL)
ENDIF (PARAVIEW_CAN_BUILD)

IF(NOT PARAVIEW_NO_HDF5)
  IF(PARAVIEW_USE_SYSTEM_HDF5)
    FIND_LIBRARY(HDF5_LIBRARY
        hdf5
        /usr/lib
        /opt/lib
        /usr/local/lib)
    FIND_PATH(HDF5_INCLUDE_DIR
        H5Ipublic.h
        /usr/include
        /opt/include
        /usr/local/include)
    MESSAGE(STATUS "Using system HDF5")
    INCLUDE_DIRECTORIES(${HDF5_INCLUDE_DIR})
    SET(PARAVIEW_HDF5_LIBRARIES ${HDF5_LIBRARY})
  ELSE(PARAVIEW_USE_SYSTEM_HDF5)
    INCLUDE_DIRECTORIES(BEFORE ${PARAVIEW_SOURCE_DIR}/Utilities/hdf5
                               ${PARAVIEW_BINARY_DIR}/Utilities/hdf5)
    SET(PARAVIEW_HDF5_LIBRARIES vtkhdf5)
  ENDIF(PARAVIEW_USE_SYSTEM_HDF5)
ENDIF(NOT PARAVIEW_NO_HDF5)

IF (PARAVIEW_CAN_BUILD)

 SET (KWWIDGETS_SOURCE_DIR ${PARAVIEW_SOURCE_DIR}/Widgets)
 SET (KWWIDGETS_BINARY_DIR ${PARAVIEW_BINARY_DIR}/Widgets)

 SET (DO_NOT_BUILD_XML_RW 1)

 SUBDIRS(KWCommon Widgets Utilities ClientServer ParaView Demos)

 SUBDIRS(Packages)

 SUBDIR_DEPENDS(ParaView Widgets)
 SUBDIRS(Config)
 IF (BUILD_DOCUMENTATION)
  SUBDIRS(Documentation)
 ENDIF (BUILD_DOCUMENTATION)

 OPTION(BUILD_SHARED_LIBS "Build ParaView with shared libraries." OFF)
 OPTION(BUILD_EXAMPLES "Build VTK examples." OFF)

 IF (VTK_USE_MPI)
  IF (MPI_LIBRARY)
    LINK_LIBRARIES (${MPI_LIBRARY})
  ENDIF (MPI_LIBRARY)
  IF (MPI_INCLUDE_PATH)
    INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
  ENDIF (MPI_INCLUDE_PATH)
  IF (MPI_EXTRA_LIBRARY)
    LINK_LIBRARIES (${MPI_EXTRA_LIBRARY})
  ENDIF (MPI_EXTRA_LIBRARY)
 ENDIF (VTK_USE_MPI)

 IF (TK_INCLUDE_PATH)
   INCLUDE_DIRECTORIES(${TK_INCLUDE_PATH})
 ENDIF (TK_INCLUDE_PATH)
 IF(VTK_TK_INTERNAL_DIR)
   INCLUDE_DIRECTORIES(${VTK_TK_INTERNAL_DIR})
 ENDIF(VTK_TK_INTERNAL_DIR)

 INCLUDE_DIRECTORIES(${PARAVIEW_BINARY_DIR}
                     ${PARAVIEW_SOURCE_DIR}/KWCommon
                     ${PARAVIEW_SOURCE_DIR}/Widgets
                     ${PARAVIEW_SOURCE_DIR}/ClientServer
                     ${PARAVIEW_BINARY_DIR}/ClientServer
                     ${PARAVIEW_SOURCE_DIR}/ParaView)

 LINK_DIRECTORIES(${PARAVIEW_BINARY_DIR}/KWCommon
                  ${PARAVIEW_BINARY_DIR}/Widgets
                  ${PARAVIEW_BINARY_DIR}/ParaView)

 SET (PARAVIEW_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/paraview)

# Include the standard Dart testing module
 INCLUDE (${CMAKE_ROOT}/Modules/Dart.cmake)

# Add the testing directories
IF(BUILD_TESTING)
  OPTION (PARAVIEW_HAVE_XDMF "Perform testing of ParaView with XDMF" OFF)
  MARK_AS_ADVANCED(PARAVIEW_HAVE_XDMF)
  IF(PARAVIEW_HAVE_XDMF)
  ENDIF(PARAVIEW_HAVE_XDMF)

  ENABLE_TESTING()
  SUBDIRS(ParaView/Testing ClientServer/Testing)
  SUBDIR_DEPENDS(ParaView/Testing ParaView)
ENDIF(BUILD_TESTING)

 INCLUDE (${PARAVIEW_BINARY_DIR}/LocalUserOptions.cmake OPTIONAL)
 INCLUDE (${PARAVIEW_SOURCE_DIR}/LocalUserOptions.cmake OPTIONAL)

 INSTALL_FILES(/share/paraview-${PARAVIEW_VERSION}/Data .bin Data/Data/combq Data/Data/combxyz)
 INSTALL_FILES(/share/paraview-${PARAVIEW_VERSION}/Data .vtk Data/Data/blow)

ENDIF (PARAVIEW_CAN_BUILD)

# Tell the KW components where to install their libraries.
SET(KW_INSTALL_LIB_DIR /lib/paraview-${PARAVIEW_VERSION})
