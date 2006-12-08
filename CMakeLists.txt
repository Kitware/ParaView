PROJECT(ParaView)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

#########################################################################
# Warn about ParaQ
SET(PARAVIEW_WARN_ABOUT_PARAQ)
IF(EXISTS "${ParaView_SOURCE_DIR}/CVS/Root")
  FILE(READ "${ParaView_SOURCE_DIR}/CVS/Root" paraview_cvs_root)
  IF("${paraview_cvs_root}" MATCHES ParaQ)
    SET(PARAVIEW_WARN_ABOUT_PARAQ 1)
  ENDIF("${paraview_cvs_root}" MATCHES ParaQ)
ENDIF(EXISTS "${ParaView_SOURCE_DIR}/CVS/Root")
IF("${ParaView_SOURCE_DIR}" MATCHES "ParaQ")
  SET(PARAVIEW_WARN_ABOUT_PARAQ 1)
ENDIF("${ParaView_SOURCE_DIR}" MATCHES "ParaQ")
IF(PARAVIEW_WARN_ABOUT_PARAQ)
  MESSAGE(SEND_ERROR "ParaView 3 should be checked out from ParaView3 repository. ParaQ repository is deprecated and will be removed")
ENDIF(PARAVIEW_WARN_ABOUT_PARAQ)

#########################################################################
# Disallow in-source build
STRING(COMPARE EQUAL "${ParaView_SOURCE_DIR}" 
  "${ParaView_BINARY_DIR}" INSOURCE)
IF(INSOURCE)
  MESSAGE(FATAL_ERROR "ParaView requires an out of source Build. Please create a separate binary directory and run CMake there.")
ENDIF(INSOURCE)

#########################################################################
SET(PARAVIEW_VERSION_MAJOR 2)
SET(PARAVIEW_VERSION_MINOR 9)
SET(PARAVIEW_VERSION_PATCH 7)
SET(PARAVIEW_VERSION "${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}")
SET(PARAVIEW_VERSION_FULL "${PARAVIEW_VERSION}.${PARAVIEW_VERSION_PATCH}")

# See VTK/CMakeLists.txt for an explanation of this default selection.
IF(PARAVIEW_VERSION_MINOR MATCHES "[02468]$")
  # This is a release version.  Default to not use rpath.
  SET(VTK_USE_RPATH_DEFAULT OFF)
ELSE(PARAVIEW_VERSION_MINOR MATCHES "[02468]$")
  # This is a development version.  Default to use rpath.
  SET(VTK_USE_RPATH_DEFAULT ON)
ENDIF(PARAVIEW_VERSION_MINOR MATCHES "[02468]$")

#########################################################################
# build VTK/Qt support
OPTION(PARAVIEW_BUILD_QT_GUI "Build ParaView Qt Client. This requires Qt." ON)
MARK_AS_ADVANCED(PARAVIEW_BUILD_QT_GUI)
IF (PARAVIEW_BUILD_QT_GUI)
  SET(VTK_USE_GUISUPPORT ON CACHE BOOL "Build VTK with GUI Support" FORCE)
  SET(VTK_USE_QVTK ON CACHE BOOL "Build VTK with Qt Support" FORCE)
  SET(DESIRED_QT_VERSION 4 CACHE STRING "Use Qt4" FORCE)
  MARK_AS_ADVANCED(DESIRED_QT_VERSION VTK_USE_QVTK VTK_USE_GUISUPPORT)
ENDIF (PARAVIEW_BUILD_QT_GUI)


# Work around CMake 2.2.x FindPythonLibs bug.
# Do not find Python subdirectory as python debug library.
SET(PYTHON_DEBUG_LIBRARY "" CACHE FILEPATH "Purposely empty: work around CMake FindPythonLibs bug")
MARK_AS_ADVANCED(PYTHON_DEBUG_LIBRARY)


#########################################################################
# Force Infovis support on

SET(VTK_USE_INFOVIS ON CACHE BOOL "Build VTK with Infovis Support" FORCE)
MARK_AS_ADVANCED(VTK_USE_INFOVIS)

#########################################################################
# Configure Testing
OPTION(BUILD_TESTING "Build ParaView Testing" ON)
IF(BUILD_TESTING)
  SET(PARAVIEW_TEST_DIR    ${ParaView_BINARY_DIR}/Testing/Temporary)
  MAKE_DIRECTORY(${PARAVIEW_TEST_DIR})
  ENABLE_TESTING()
  INCLUDE (CTest)
ENDIF(BUILD_TESTING)

#########################################################################
# Configure data directory
IF(NOT PARAVIEW_DATA_ROOT AND PARAQ_DATA_ROOT)
  SET(PARAVIEW_DATA_ROOT "${PARAQ_DATA_ROOT}")
ENDIF(NOT PARAVIEW_DATA_ROOT AND PARAQ_DATA_ROOT)
FIND_PATH(PARAVIEW_DATA_ROOT ParaViewData.readme
  ${ParaView_SOURCE_DIR}/ParaViewData
  ${ParaView_SOURCE_DIR}/../ParaViewData
  ${ParaView_SOURCE_DIR}/ParaQData
  ${ParaView_SOURCE_DIR}/../ParaQData
  $ENV{PARAVIEW_DATA_ROOT}
  )

#########################################################################
# Configure Python
OPTION(PARAVIEW_EMBED_PYTHON "Embedded Python Interpreter" ON)

IF(PARAVIEW_EMBED_PYTHON)
  SET(BUILD_SHARED_LIBS ON CACHE BOOL "Build ParaView using shared libraries" FORCE)
  SET(PARAVIEW_WRAP_PYTHON ON CACHE BOOL "Wrap ParaView server manager into Python" FORCE)
ELSE(PARAVIEW_EMBED_PYTHON)
  SET(PARAVIEW_WRAP_PYTHON OFF CACHE BOOL "Wrap ParaView server manager into Python" FORCE)
ENDIF(PARAVIEW_EMBED_PYTHON)

#########################################################################
IF(BUILD_SHARED_LIBS)
  ADD_DEFINITIONS(-DPARAVIEW_BUILD_SHARED_LIBS)
ENDIF(BUILD_SHARED_LIBS)


#########################################################################
# Set the ServerManager test data dir,
SET(PVServerManagerTestData ${PARAVIEW_DATA_ROOT})

#########################################################################
# Include the file that most of the environment setup (except GUI).
SET(ParaView_SOURCE_DIR ${ParaView_SOURCE_DIR})
SET(ParaView_BINARY_DIR ${ParaView_BINARY_DIR})
INCLUDE(${ParaView_SOURCE_DIR}/CMake/ParaViewCommon.cmake)

#########################################################################
IF(BUILD_DOCUMENTATION)
  SUBDIRS(Utilities/Doxygen)
ENDIF(BUILD_DOCUMENTATION)

#########################################################################
CONFIGURE_FILE(${ParaView_SOURCE_DIR}/vtkPQConfig.h.in
  ${ParaView_BINARY_DIR}/vtkPQConfig.h
  ESCAPE_QUOTES)

#########################################################################
IF (PARAVIEW_BUILD_QT_GUI)
  SUBDIRS(Qt)
ENDIF (PARAVIEW_BUILD_QT_GUI)

#########################################################################
IF (PARAVIEW_BUILD_QT_GUI)
SUBDIRS(Applications)
ENDIF (PARAVIEW_BUILD_QT_GUI)

# Find hints file for the client/server wrapping
FIND_FILE(VTK_WRAP_HINTS hints ${ParaView_SOURCE_DIR}/VTK/Wrapping)
MARK_AS_ADVANCED(VTK_WRAP_HINTS)

# If the cmake version includes cpack, use it
INCLUDE(InstallRequiredSystemLibraries)
SET(CPACK_INSTALL_CMAKE_PROJECTS "${ParaView_BINARY_DIR};ParaView;Runtime;/;${ParaView_BINARY_DIR};VTK Runtime Libs;RuntimeLibraries;/;${ParaView_BINARY_DIR};VTK Executables;RuntimeExecutables;/")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ParaView is a scientific visualization tool")
SET(CPACK_PACKAGE_VENDOR "Kitware Inc.")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/License_v1.1.txt")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License_v1.1.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR "${PARAVIEW_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${PARAVIEW_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${PARAVIEW_VERSION_PATCH}")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "ParaView ${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}.${PARAVIEW_VERSION_PATCH}")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME
  "paraview-${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}.${PARAVIEW_VERSION_PATCH}")
IF (CMAKE_SYSTEM_PROCESSOR MATCHES "unknown")
  SET (CMAKE_SYSTEM_PROCESSOR "x86")
ENDIF (CMAKE_SYSTEM_PROCESSOR MATCHES "unknown")
IF(NOT DEFINED CPACK_SYSTEM_NAME)
  SET(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
ENDIF(NOT DEFINED CPACK_SYSTEM_NAME)
IF(${CPACK_SYSTEM_NAME} MATCHES Windows)
  IF(CMAKE_CL_64)
    SET(CPACK_SYSTEM_NAME win64-${CMAKE_SYSTEM_PROCESSOR})
  ELSE(CMAKE_CL_64)
    SET(CPACK_SYSTEM_NAME win32-${CMAKE_SYSTEM_PROCESSOR})
  ENDIF(CMAKE_CL_64)
ENDIF(${CPACK_SYSTEM_NAME} MATCHES Windows)
IF(NOT DEFINED CPACK_PACKAGE_FILE_NAME)
  SET(CPACK_PACKAGE_FILE_NAME "${CPACK_SOURCE_PACKAGE_FILE_NAME}-${CPACK_SYSTEM_NAME}")
ENDIF(NOT DEFINED CPACK_PACKAGE_FILE_NAME)
SET(CPACK_PACKAGE_EXECUTABLES "paraview_alpha" "ParaView")
IF(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  SET(CPACK_PACKAGE_ICON "${ParaView_SOURCE_DIR}/Applications/Client\\\\ParaViewLogo.png")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\paraview_alpha.exe")
  SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} a cross-platform, open-source visualization system")
  SET(CPACK_NSIS_HELP_LINK "http://www.paraview.org")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http://www.kitware.com")
  SET(CPACK_NSIS_CONTACT "webmaster@paraview.org")
  #    SET(CPACK_NSIS_MODIFY_PATH ON)
ELSE(WIN32 AND NOT UNIX)
  SET(CPACK_STRIP_FILES "")
  SET(CPACK_SOURCE_STRIP_FILES "")
  IF (NOT APPLE)
    SET(CPACK_GENERATOR TGZ)
  ENDIF (NOT APPLE)
ENDIF(WIN32 AND NOT UNIX)
INCLUDE(CPack)
