CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

PROJECT(KWCommon)

# --------------------------------------------------------------------------
# To use this library in a larger project you will need to SUBDIR into this
# directory so that it gets built and then you will use the following variables
# in your CMakeLists files to get the proper include paths and libraries

SET(KWCommon_INCLUDE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}"
  CACHE INTERNAL "include paths for KWCommon"
  )

SET(KWCommon_LIBRARIES KWCommon CACHE INTERNAL "libraries for KWCommon")

# You will also need to define a value for the following variables:
# KWCommon_INSTALL_BIN_DIR          - binary dir (executables)
# KWCommon_INSTALL_LIB_DIR          - library dir (libs)
# KWCommon_INSTALL_DATA_DIR         - share dir (say, examples, data, etc)
# KWCommon_INSTALL_INCLUDE_DIR      - include dir (headers)
# KWCommon_INSTALL_PACKAGE_DIR      - package/export configuration files
# KWCommon_VTK_INSTALL_PACKAGE_DIR  - VTK package/export configuration files
# KWCommon_INSTALL_NO_DEVELOPMENT   - do not install development files
# KWCommon_INSTALL_NO_RUNTIME       - do not install runtime files
# KWCommon_INSTALL_NO_DOCUMENTATION - do not install documentation files
# --------------------------------------------------------------------------

# --------------------------------------------------------------------------
# Version

SET(KWCommon_MAJOR_VERSION 1)
SET(KWCommon_MINOR_VERSION 0)

# --------------------------------------------------------------------------
# We need VTK

IF(NOT VTK_SOURCE_DIR)
  FIND_PACKAGE(VTK REQUIRED)
  INCLUDE(${VTK_USE_FILE})
ENDIF(NOT VTK_SOURCE_DIR)

# --------------------------------------------------------------------------
# EXPORT() will be in cmake 2.6, add an empty macro so older cmake versions stil work
IF(NOT COMMAND EXPORT)
  MACRO(EXPORT)
  ENDMACRO(EXPORT)
ENDIF(NOT COMMAND EXPORT)

# --------------------------------------------------------------------------
# Output directories

IF(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${KWCommon_BINARY_DIR}/bin)
ENDIF(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
SET(KWCommon_LIBRARY_DIRS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
SET(KWCommon_RUNTIME_DIRS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

IF(NOT EXECUTABLE_OUTPUT_PATH)
  SET(EXECUTABLE_OUTPUT_PATH ${KWCommon_BINARY_DIR}/bin CACHE INTERNAL
    "Single output directory for building all executables.")
ENDIF(NOT EXECUTABLE_OUTPUT_PATH)

# --------------------------------------------------------------------------
# Some flags

INCLUDE_REGULAR_EXPRESSION("^(vtk|kw|icons).*$")
SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
IF(CMAKE_COMPILER_2005)
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
ENDIF(CMAKE_COMPILER_2005)

MARK_AS_ADVANCED(CMAKE_BACKWARDS_COMPATIBILITY)

SET(KWCommon_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

SET(KWCommon_CMAKE_DIR "${KWCommon_SOURCE_DIR}/CMake")
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${KWCommon_CMAKE_DIR}")

SET(KWCommon_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES})

# --------------------------------------------------------------------------
# Install directories

IF(NOT KWCommon_INSTALL_BIN_DIR)
  SET(KWCommon_INSTALL_BIN_DIR "/bin")
ENDIF(NOT KWCommon_INSTALL_BIN_DIR)

IF(NOT KWCommon_INSTALL_LIB_DIR)
  SET(KWCommon_INSTALL_LIB_DIR "/lib/${PROJECT_NAME}")
ENDIF(NOT KWCommon_INSTALL_LIB_DIR)

IF(NOT KWCommon_INSTALL_DATA_DIR)
  SET(KWCommon_INSTALL_DATA_DIR "/share/${PROJECT_NAME}")
ENDIF(NOT KWCommon_INSTALL_DATA_DIR)

IF(NOT KWCommon_INSTALL_INCLUDE_DIR)
  SET(KWCommon_INSTALL_INCLUDE_DIR "/include/${PROJECT_NAME}")
ENDIF(NOT KWCommon_INSTALL_INCLUDE_DIR)

IF(NOT KWCommon_INSTALL_PACKAGE_DIR)
  SET(KWCommon_INSTALL_PACKAGE_DIR ${KWCommon_INSTALL_LIB_DIR}
    CACHE INTERNAL "")
ENDIF(NOT KWCommon_INSTALL_PACKAGE_DIR)

IF(NOT KWCommon_VTK_INSTALL_PACKAGE_DIR)
  SET(KWCommon_VTK_INSTALL_PACKAGE_DIR ${VTK_INSTALL_PACKAGE_DIR})
ENDIF(NOT KWCommon_VTK_INSTALL_PACKAGE_DIR)

IF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)
  SET(KWCommon_INSTALL_NO_DEVELOPMENT 0)
ENDIF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)

IF(NOT KWCommon_INSTALL_NO_RUNTIME)
  SET(KWCommon_INSTALL_NO_RUNTIME 0)
ENDIF(NOT KWCommon_INSTALL_NO_RUNTIME)

IF(NOT KWCommon_INSTALL_NO_DOCUMENTATION)
  SET(KWCommon_INSTALL_NO_DOCUMENTATION 0)
ENDIF(NOT KWCommon_INSTALL_NO_DOCUMENTATION)

SET(KWCommon_INSTALL_NO_LIBRARIES)
IF(KWCommon_BUILD_SHARED_LIBS)
  IF(KWCommon_INSTALL_NO_RUNTIME AND KWCommon_INSTALL_NO_DEVELOPMENT)
    SET(KWCommon_INSTALL_NO_LIBRARIES 1)
  ENDIF(KWCommon_INSTALL_NO_RUNTIME AND KWCommon_INSTALL_NO_DEVELOPMENT)
ELSE(KWCommon_BUILD_SHARED_LIBS)
  IF(KWCommon_INSTALL_NO_DEVELOPMENT)
    SET(KWCommon_INSTALL_NO_LIBRARIES 1)
  ENDIF(KWCommon_INSTALL_NO_DEVELOPMENT)
ENDIF(KWCommon_BUILD_SHARED_LIBS)

# --------------------------------------------------------------------------
# Sources

SET(KWCommon_SRCS
  vtkKWProcessStatistics.cxx
  vtkKWRemoteExecute.cxx
  )

# --------------------------------------------------------------------------
# Include dirs

IF(VTK_ZLIB_INCLUDE_DIRS)
  INCLUDE_DIRECTORIES(${VTK_ZLIB_INCLUDE_DIRS})
ENDIF(VTK_ZLIB_INCLUDE_DIRS)

INCLUDE_DIRECTORIES(
  ${KWCommon_BINARY_DIR}
  ${KWCommon_SOURCE_DIR}
  )

# --------------------------------------------------------------------------
# Build the library

PVVTK_ADD_LIBRARY(KWCommon ${KWCommon_SRCS})

TARGET_LINK_LIBRARIES(KWCommon
  vtkCommonCore
  vtksys
  )

IF(NOT KWCommon_INSTALL_NO_LIBRARIES)
  IF(PV_INSTALL_HAS_CMAKE_24)
    INSTALL(TARGETS ${KWCommon_LIBRARIES}
EXPORT ${PV_INSTALL_EXPORT_NAME}
      RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime
      LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime
      ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Development)
  ELSE(PV_INSTALL_HAS_CMAKE_24)
    INSTALL_TARGETS(${KWCommon_INSTALL_LIB_DIR}
      RUNTIME_DIRECTORY ${KWCommon_INSTALL_BIN_DIR}
      ${KWCommon_LIBRARIES})
  ENDIF(PV_INSTALL_HAS_CMAKE_24)
ENDIF(NOT KWCommon_INSTALL_NO_LIBRARIES)

# --------------------------------------------------------------------------
# Install the headers

FILE(GLOB templated_files "${CMAKE_CURRENT_SOURCE_DIR}/*.txx")

IF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${KWCommon_INSTALL_INCLUDE_DIR} .h
    ${KWCommon_SRCS}
    ${templated_files})
  INSTALL_FILES(${KWCommon_INSTALL_INCLUDE_DIR} .txx ${templated_files})
ENDIF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)

# --------------------------------------------------------------------------
# Testing

INCLUDE(Dart)
MARK_AS_ADVANCED(BUILD_TESTING DART_ROOT TCL_TCLSH)

SET(default_val 1)
IF(DEFINED KWCommon_BUILD_TESTING_DEFAULT)
  SET(default_val ${KWCommon_BUILD_TESTING_DEFAULT})
ENDIF(DEFINED KWCommon_BUILD_TESTING_DEFAULT)
OPTION(KWCommon_BUILD_TESTING
  "Enable testing in KWCommon (if BUILD_TESTING is ON as well)."
  ${default_val})
MARK_AS_ADVANCED(KWCommon_BUILD_TESTING)
IF(BUILD_TESTING AND KWCommon_BUILD_TESTING)
  #SET(BUILDNAME "${PROJECT_NAME}-${BUILDNAME}")
  ENABLE_TESTING()
  ADD_SUBDIRECTORY(Testing)
ENDIF(BUILD_TESTING AND KWCommon_BUILD_TESTING)

# --------------------------------------------------------------------------
# Configure the export configuration

ADD_SUBDIRECTORY(CMake/ExportConfiguration)

# --------------------------------------------------------------------------
# ProcessXML (is it still being used ?)

SET(KWCommon_EXE_SUFFIX)
SET(KWCommon_EXE_INSTALL ${KWCommon_INSTALL_BIN_DIR})
SET(Temp_Using_Shared_Forwarding OFF)
IF(KWCOMMON_BUILD_PROCESSXML)
  # For shared libraries on non-windows systems use the KWSys shared-forward
  # implementation to make sure shared libraries are found.
  IF(BUILD_SHARED_LIBS )
    IF(NOT WIN32)
      SET(KWCommon_FORWARD_DIR_BUILD ${EXECUTABLE_OUTPUT_PATH})
      SET(KWCommon_FORWARD_BIN_TO_LIB "..${KWCommon_INSTALL_LIB_DIR}")
      SET(KWCommon_EXE_SUFFIX -real)
      SET(KWCommon_FORWARD_EXE "kwProcessXML${KWCommon_EXE_SUFFIX}")
      SET(KWCommon_EXE_INSTALL ${KWCommon_INSTALL_LIB_DIR})
      CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/kw-forward.c.in
                     ${CMAKE_CURRENT_BINARY_DIR}/ProcessXML-forward.c
                     @ONLY IMMEDIATE)
      ADD_EXECUTABLE(kwProcessXML
                     ${CMAKE_CURRENT_BINARY_DIR}/ProcessXML-forward.c)
      SET(Temp_Using_Shared_Forwarding ON)
      EXPORT(TARGETS kwProcessXML FILE ${EXPORT_EXECUTABLES_FILE} NAMESPACE "${EXPORT_EXECUTABLES_NAMESPACE}" APPEND)
      IF(KWCOMMON_HOST_TOOLS_TARGET)
        ADD_DEPENDENCIES(${KWCOMMON_HOST_TOOLS_TARGET} kwProcessXML)
      ENDIF(KWCOMMON_HOST_TOOLS_TARGET)
      IF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)
        INSTALL(TARGETS kwProcessXML
          DESTINATION ${PV_INSTALL_BIN_DIR}
          COMPONENT Development)
      ENDIF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)
    ENDIF(NOT WIN32)
  ENDIF(BUILD_SHARED_LIBS )

  ADD_EXECUTABLE(kwProcessXML${KWCommon_EXE_SUFFIX} ProcessXML.cxx)
  TARGET_LINK_LIBRARIES(kwProcessXML${KWCommon_EXE_SUFFIX} vtkCommonCore vtksys)
  EXPORT(TARGETS kwProcessXML${KWCommon_EXE_SUFFIX} FILE ${EXPORT_EXECUTABLES_FILE} NAMESPACE "${EXPORT_EXECUTABLES_NAMESPACE}" APPEND)
  IF(KWCOMMON_HOST_TOOLS_TARGET)
    ADD_DEPENDENCIES(${KWCOMMON_HOST_TOOLS_TARGET} kwProcessXML${KWCommon_EXE_SUFFIX})
  ENDIF(KWCOMMON_HOST_TOOLS_TARGET)
  IF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)
    SET(kwProcessXML_real_INSTALL_DIR ${PV_INSTALL_BIN_DIR})
    IF(Temp_Using_Shared_Forwarding)
      SET(kwProcessXML_real_INSTALL_DIR ${PV_INSTALL_LIB_DIR})
    ENDIF(Temp_Using_Shared_Forwarding)
    INSTALL(TARGETS kwProcessXML${KWCommon_EXE_SUFFIX}
      DESTINATION ${kwProcessXML_real_INSTALL_DIR}
      COMPONENT Development)
  ENDIF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)
  IF(KWCommon_EXE_SUFFIX)
    ADD_DEPENDENCIES(kwProcessXML kwProcessXML${KWCommon_EXE_SUFFIX})
  ENDIF(KWCommon_EXE_SUFFIX)
ENDIF(KWCOMMON_BUILD_PROCESSXML)
