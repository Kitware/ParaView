CMAKE_MINIMUM_REQUIRED (VERSION 2.0)
PROJECT(ParaView)


#########################################################################
# Disallow in-source build
STRING(COMPARE EQUAL "${ParaView_SOURCE_DIR}" 
  "${ParaView_BINARY_DIR}" INSOURCE)
IF(INSOURCE)
  MESSAGE(FATAL_ERROR "ParaView requires an out of source Build. Please create a separate binary directory and run CMake there.")
ENDIF(INSOURCE)


#########################################################################
# Configure TclTk
OPTION(PARAVIEW_BUILD_GUI
  "Build end user applications (ParaView, pvclient, pvbatch)" ON)
MARK_AS_ADVANCED(PARAVIEW_BUILD_GUI)
IF (PARAVIEW_BUILD_GUI)
  SET(APPLICATION_BINARY_DIR "${CMAKE_BINARY_DIR}")
  SET(VTK_TCL_TK_MACROS_MODULE 
    "${ParaView_SOURCE_DIR}/VTK/CMake/vtkTclTkMacros.cmake")
  SET(VTK_TCL_TK_INTERNALS_DIR 
    "${ParaView_SOURCE_DIR}/VTK/Utilities/TclTk/internals")
  SET(VTK_TCL_TK_WRAPPING_DIR "${ParaView_SOURCE_DIR}/VTK/Wrapping/Tcl")
  SET(TclTk_INSTALL_NO_RUNTIME ${PV_INSTALL_NO_RUNTIME})
  SET(TclTk_INSTALL_BIN_DIR ${PV_INSTALL_BIN_DIR})
  SET(TclTk_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
  INCLUDE(${VTK_TCL_TK_MACROS_MODULE})
  SUBDIRS(Utilities/TclTk)
ENDIF (PARAVIEW_BUILD_GUI)

# Find hints file for the tcl wrapping
FIND_FILE(VTK_WRAP_HINTS hints ${VTK_SOURCE_DIR}/Wrapping)
MARK_AS_ADVANCED(VTK_WRAP_HINTS)

SET(VTK_GENERATE_TK_INTERNALS 1)

#########################################################################
# Configure Tcl wrapping
IF (PARAVIEW_BUILD_GUI)
  SUBDIRS(Utilities/VTKTclWrapping)
  SET(VTKTclWrapping_INSTALL_NO_DEVELOPMENT ${PV_INSTALL_NO_DEVELOPMENT})
  SET(VTKTclWrapping_INSTALL_NO_RUNTIME ${PV_INSTALL_NO_RUNTIME})
  SET(VTKTclWrapping_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
  SET(VTKTclWrapping_INSTALL_BIN_DIR ${PV_INSTALL_BIN_DIR})
ENDIF (PARAVIEW_BUILD_GUI)

#########################################################################
# Include the file that most of the environment setup (except GUI).
INCLUDE(${ParaView_SOURCE_DIR}/CMake/ParaViewCommon.cmake)

#########################################################################
# Common settings
#

# See VTK/CMakeLists.txt for an explanation of this default selection.
IF(PARAVIEW_VERSION_MINOR MATCHES "[02468]$")
  # This is a release version.  Default to not use rpath.
  SET(VTK_USE_RPATH_DEFAULT OFF)
ELSE(PARAVIEW_VERSION_MINOR MATCHES "[02468]$")
  # This is a development version.  Default to use rpath.
  SET(VTK_USE_RPATH_DEFAULT ON)
ENDIF(PARAVIEW_VERSION_MINOR MATCHES "[02468]$")


OPTION(DISABLE_KWWIDGETS_TESTING "Disable the testing of KWWidgets." OFF)
MARK_AS_ADVANCED(DISABLE_KWWIDGETS_TESTING)

# Set the ServerManager test data dir,
SET(PVServerManagerTestData ${ParaView_SOURCE_DIR}/Data)

# KWWidgets should install only the components paraview does.

SET(KWWidgets_INSTALL_BIN_DIR ${PV_INSTALL_BIN_DIR})
SET(KWWidgets_INSTALL_LIB_DIR ${PV_INSTALL_LIB_DIR})
SET(KWWidgets_INSTALL_DATA_DIR ${PV_INSTALL_DATA_DIR})
SET(KWWidgets_INSTALL_INCLUDE_DIR ${PV_INSTALL_INCLUDE_DIR})
SET(KWWidgets_INSTALL_PACKAGE_DIR ${PV_INSTALL_LIB_DIR})
SET(KWWidgets_INSTALL_NO_DEVELOPMENT ${PV_INSTALL_NO_DEVELOPMENT})
SET(KWWidgets_INSTALL_NO_RUNTIME ${PV_INSTALL_NO_RUNTIME})
SET(KWWidgets_INSTALL_NO_DOCUMENTATION 1)
SET(KWWidgets_VTK_INSTALL_PACKAGE_DIR ${VTK_INSTALL_PACKAGE_DIR})

#########################################################################
# Setup release scripts.

# Search for system runtime libraries based on the platform.  This is
# not complete because it is used only for the release process by the
# developers.
IF(CMAKE_GENERATOR MATCHES "^Visual Studio 7 .NET 2003$")
  STRING(REGEX REPLACE "\\\\" "/" SYSTEMROOT "$ENV{SYSTEMROOT}")
  FOREACH(lib
      "${SYSTEMROOT}/system32/msvcp71.dll"
      "${SYSTEMROOT}/system32/msvcr71.dll"
      )
    IF(EXISTS ${lib})
      SET(PV_INSTALL_SYSTEM_RUNTIME_LIBS
        ${PV_INSTALL_SYSTEM_RUNTIME_LIBS} ${lib})
    ENDIF(EXISTS ${lib})
  ENDFOREACH(lib)
ENDIF(CMAKE_GENERATOR MATCHES "^Visual Studio 7 .NET 2003$")

# Include system runtime libraries in the installation if any are
# specified by PV_INSTALL_SYSTEM_RUNTIME_LIBS.
IF(PV_INSTALL_SYSTEM_RUNTIME_LIBS)
  IF(WIN32)
    INSTALL_PROGRAMS(${PV_INSTALL_BIN_DIR} ${PV_INSTALL_SYSTEM_RUNTIME_LIBS})
  ELSE(WIN32)
    INSTALL_PROGRAMS(${PV_INSTALL_LIB_DIR} ${PV_INSTALL_SYSTEM_RUNTIME_LIBS})
  ENDIF(WIN32)
ENDIF(PV_INSTALL_SYSTEM_RUNTIME_LIBS)

IF(WIN32)
  FIND_PROGRAM(NSIS_MAKENSIS NAMES makensis
    PATHS [HKEY_LOCAL_MACHINE\\SOFTWARE\\NSIS]
    DOC "Where is makensis.exe located"
    )
  MARK_AS_ADVANCED(NSIS_MAKENSIS)
  FIND_PROGRAM(WINZIP_WZZIP NAMES wzzip
    PATHS   "C:/Program Files/WinZip"
    DOC "Where is makensis.exe located"
    )
  MARK_AS_ADVANCED(WINZIP_WZZIP)
  STRING(REGEX REPLACE "/" "\\\\" PV_INSTALL_TOP "${CMAKE_INSTALL_PREFIX}")
  SET(NSIS_EXTRA_COMMANDS ";Include system runtime libraries.\n  SetOutPath \"$INSTDIR\\bin\"\n")
  FOREACH(lib ${PV_INSTALL_SYSTEM_RUNTIME_LIBS})
    STRING(REGEX REPLACE "/" "\\\\" LIB "${lib}")
    SET(NSIS_EXTRA_COMMANDS "${NSIS_EXTRA_COMMANDS}  File \"${LIB}\"\n")
  ENDFOREACH(lib)
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Utilities/Release/ParaView.nsi.in
    ${ParaView_BINARY_DIR}/Utilities/Release/ParaView.nsi IMMEDIATE @ONLY)
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Utilities/Release/Win32Release.sh.in
    ${ParaView_BINARY_DIR}/Utilities/Release/Win32Release.sh IMMEDIATE @ONLY)
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Utilities/Release/paraview_release.sh.in
    ${ParaView_BINARY_DIR}/Utilities/Release/paraview_release.sh IMMEDIATE @ONLY)
ENDIF(WIN32)

#########################################################################
# Build Documentation
IF(BUILD_DOCUMENTATION)
  SUBDIRS(Utilities/Doxygen)
ENDIF(BUILD_DOCUMENTATION)

#########################################################################
# Configure GUI
IF (PARAVIEW_BUILD_GUI)
  SUBDIRS(GUI)
ENDIF (PARAVIEW_BUILD_GUI)

#########################################################################
# Configure ParaView Data
SUBDIRS(Data)

#-----------------------------------------------------------------------------
# Help outside projects build ParaView plugin packages.

EXPORT_LIBRARY_DEPENDENCIES(${ParaView_BINARY_DIR}/ParaViewLibraryDepends.cmake)

# For ParaView from build tree:
SET(PARAVIEW_VTK_DIR ${VTK_DIR})
SET(VTK_WRAP_ClientServer_EXE_CONFIG ${VTK_WRAP_ClientServer_EXE})
SET(VTKCS_SOURCE_DIR ${ParaView_SOURCE_DIR}/Utilities/VTKClientServer)
SET(PARAVIEW_CMAKE_EXTENSIONS
  "${ParaView_SOURCE_DIR}/VTK/CMake")
SET(PARAVIEW_CMAKE_CLIENT_SERVER_EXTENSION
  ${VTKCS_SOURCE_DIR}/CMake/vtkWrapClientServer.cmake)
SET(PARAVIEW_USE_FILE ${ParaView_SOURCE_DIR}/ParaViewUse.cmake)
IF(PARAVIEW_BUILD_GUI)
  SET(PARAVIEW_INCLUDE_DIRS ${PARAVIEW_INCLUDE_DIRS}
    ${ParaView_SOURCE_DIR}/GUI/Widgets
    ${ParaView_SOURCE_DIR}/GUI/Client
    ${ParaView_BINARY_DIR}/GUI/Widgets
    ${ParaView_BINARY_DIR}/GUI/Client
    )
ENDIF(PARAVIEW_BUILD_GUI)
SET(PARAVIEW_GUI_INCLUDE_DIRS
  ${TCL_INCLUDE_PATH}
  ${TK_INCLUDE_PATH}
  ${KWCommon_INCLUDE_PATH}
  ${VTKZLIB_INCLUDE_DIR})
SET(PARAVIEW_LIBRARY_DIRS ${LIBRARY_OUTPUT_PATH} ${PARAVIEW_EXTRA_LINK_DIRECTORIES})
SET(PARAVIEW_LIBRARY_DEPENDS_FILE ${ParaView_BINARY_DIR}/ParaViewLibraryDepends.cmake)
SET(PARAVIEW_GUI_CLIENT_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/paraview)
SET(PARAVIEW_BATCH_EXECUTABLE ${EXECUTABLE_OUTPUT_PATH}/pvbatch)
SET(CFG_INIT "/${CMAKE_CFG_INTDIR}")
IF(MAKE_SYSTEM OR UNIX)
  SET(CFG_INIT "")
ENDIF(MAKE_SYSTEM OR UNIX)
SET(PARAVIEW_PROCESS_XML_EXECUTABLE "${EXECUTABLE_OUTPUT_PATH}${CFG_INIT}/kwProcessXML${EXE_EXT}")
SET(PARAVIEW_SERVER_MANAGER_RESOURCES_DIRECTORY ${ParaView_SOURCE_DIR}/Servers/ServerManager/Resources)
SET(PARAVIEW_KWSYS_INCLUDE_DIRS ${ParaView_BINARY_DIR}/VTK/Utilities)
CONFIGURE_FILE(${ParaView_SOURCE_DIR}/ParaViewConfig.cmake.in
               ${ParaView_BINARY_DIR}/ParaViewConfig.cmake @ONLY IMMEDIATE)

# For installed ParaView:
SET(PARAVIEW_VTK_DIR ${CMAKE_INSTALL_PREFIX}${VTK_INSTALL_LIB_DIR})
SET(VTK_WRAP_ClientServer_EXE_CONFIG ${CMAKE_INSTALL_PREFIX}${PV_INSTALL_LIB_DIR}/vtkWrapClientServer)
SET(PARAVIEW_CMAKE_EXTENSIONS
  "${CMAKE_INSTALL_PREFIX}${PV_INSTALL_LIB_DIR}/CMake")
SET(PARAVIEW_CMAKE_CLIENT_SERVER_EXTENSION ${CMAKE_INSTALL_PREFIX}${PV_INSTALL_LIB_DIR}/CMake/vtkWrapClientServer.cmake)
SET(PARAVIEW_USE_FILE ${CMAKE_INSTALL_PREFIX}${PV_INSTALL_LIB_DIR}/ParaViewUse.cmake)
SET(PARAVIEW_INCLUDE_DIRS ${CMAKE_INSTALL_PREFIX}/include/paraview-${PARAVIEW_VERSION})
SET(PARAVIEW_GUI_INCLUDE_DIRS ${PARAVIEW_INCLUDE_DIRS})
SET(PARAVIEW_KWSYS_INCLUDE_DIRS ${PARAVIEW_INCLUDE_DIRS})
SET(PARAVIEW_LIBRARY_DIRS ${CMAKE_INSTALL_PREFIX}${PV_INSTALL_LIB_DIR})
SET(PARAVIEW_LIBRARY_DEPENDS_FILE ${CMAKE_INSTALL_PREFIX}${PV_INSTALL_LIB_DIR}/ParaViewLibraryDepends.cmake)
SET(PARAVIEW_GUI_CLIENT_EXECUTABLE ${CMAKE_INSTALL_PREFIX}${PV_INSTALL_BIN_DIR}/paraview)
SET(PARAVIEW_BATCH_EXECUTABLE ${CMAKE_INSTALL_PREFIX}${PV_INSTALL_BIN_DIR}/pvbatch)
SET(PARAVIEW_PROCESS_XML_EXECUTABLE "${CMAKE_INSTALL_PREFIX}${PV_INSTALL_BIN_DIR}/kwProcessXML")
SET(PARAVIEW_SERVER_MANAGER_RESOURCES_DIRECTORY ${CMAKE_INSTALL_PREFIX}${PV_INSTALL_LIB_DIR}/ServerManager/Resources)
CONFIGURE_FILE(${ParaView_SOURCE_DIR}/ParaViewConfig.cmake.in
               ${ParaView_BINARY_DIR}/Utilities/ParaViewConfig.cmake @ONLY IMMEDIATE)

IF(NOT PV_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${PV_INSTALL_LIB_DIR}/CMake FILES
    ${VTKCS_SOURCE_DIR}/CMake/vtkWrapClientServer.cmake)
  INSTALL_FILES(${PV_INSTALL_LIB_DIR} FILES
    ${ParaView_SOURCE_DIR}/ParaViewUse.cmake
    ${ParaView_BINARY_DIR}/ParaViewLibraryDepends.cmake
    ${ParaView_BINARY_DIR}/Utilities/ParaViewConfig.cmake
    )
ENDIF(NOT PV_INSTALL_NO_DEVELOPMENT)

