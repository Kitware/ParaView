# Generate the KWCommonConfig.cmake file in the build tree. Also configure
# one for installation. The file tells external projects how to use
# KWCommon.

# Help store a literal dollar in a string.  CMake 2.2 allows escaped
# dollars but we have to support CMake 2.0.
SET(DOLLAR "$")

#-----------------------------------------------------------------------------
# Settings shared between the build tree and install tree.

EXPORT_LIBRARY_DEPENDENCIES(
  ${KWCommon_BINARY_DIR}/KWCommonLibraryDepends.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/CMakeExportBuildSettings.cmake)
CMAKE_EXPORT_BUILD_SETTINGS(
  ${KWCommon_BINARY_DIR}/KWCommonBuildSettings.cmake)

IF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${KWCommon_INSTALL_PACKAGE_DIR} FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/UseKWCommon.cmake
    ${KWCommon_BINARY_DIR}/KWCommonLibraryDepends.cmake
    ${KWCommon_BINARY_DIR}/KWCommonBuildSettings.cmake
    )
ENDIF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)

#-----------------------------------------------------------------------------
# Settings specific to the build tree.

# The install-only section is empty for the build tree.
SET(KWCommon_CONFIG_INSTALL_ONLY)

# The "use" file.
SET(KWCommon_USE_FILE_CONFIG 
  ${CMAKE_CURRENT_SOURCE_DIR}/UseKWCommon.cmake)

# The build settings file.
SET(KWCommon_BUILD_SETTINGS_FILE_CONFIG 
  ${KWCommon_BINARY_DIR}/KWCommonBuildSettings.cmake)

# The library directories.
SET(KWCommon_LIBRARY_DIRS_CONFIG ${KWCommon_LIBRARY_DIRS})

# The runtime directories.
SET(KWCommon_RUNTIME_DIRS_CONFIG ${KWCommon_RUNTIME_DIRS})

# The include directories.
SET(KWCommon_INCLUDE_DIRS_CONFIG ${KWCommon_INCLUDE_PATH})

# The library dependencies file.
SET(KWCommon_LIBRARY_DEPENDS_FILE 
  ${KWCommon_BINARY_DIR}/KWCommonLibraryDepends.cmake)

# The CMake macros dir.
SET(KWCommon_CMAKE_DIR_CONFIG 
  ${KWCommon_CMAKE_DIR})

# The VTK options.
SET(KWCommon_VTK_DIR_CONFIG ${VTK_DIR})

# The build configuration information.
SET(KWCommon_CONFIGURATION_TYPES_CONFIG ${KWCommon_CONFIGURATION_TYPES})
SET(KWCommon_BUILD_TYPE_CONFIG ${CMAKE_BUILD_TYPE})

# Configure KWCommonConfig.cmake for the build tree.
CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/KWCommonConfig.cmake.in
  ${KWCommon_BINARY_DIR}/KWCommonConfig.cmake @ONLY IMMEDIATE)

#-----------------------------------------------------------------------------
# Settings specific to the install tree.

# The "use" file.
SET(KWCommon_USE_FILE_CONFIG 
  ${DOLLAR}{KWCommon_INSTALL_PREFIX}${KWCommon_INSTALL_PACKAGE_DIR}/UseKWCommon.cmake)

# The build settings file.
SET(KWCommon_BUILD_SETTINGS_FILE_CONFIG 
  ${DOLLAR}{KWCommon_INSTALL_PREFIX}${KWCommon_INSTALL_PACKAGE_DIR}/KWCommonBuildSettings.cmake)

# The library directories.
IF(CYGWIN AND KWCommon_BUILD_SHARED_LIBS)
  # In Cygwin programs directly link to the .dll files.
  SET(KWCommon_LIBRARY_DIRS_CONFIG 
    ${DOLLAR}{KWCommon_INSTALL_PREFIX}${KWCommon_INSTALL_BIN_DIR})
ELSE(CYGWIN AND KWCommon_BUILD_SHARED_LIBS)
  SET(KWCommon_LIBRARY_DIRS_CONFIG 
    ${DOLLAR}{KWCommon_INSTALL_PREFIX}${KWCommon_INSTALL_LIB_DIR})
ENDIF(CYGWIN AND KWCommon_BUILD_SHARED_LIBS)

# The runtime directories.
IF(WIN32)
  SET(KWCommon_RUNTIME_DIRS_CONFIG 
    ${DOLLAR}{KWCommon_INSTALL_PREFIX}${KWCommon_INSTALL_BIN_DIR})
ELSE(WIN32)
  SET(KWCommon_RUNTIME_DIRS_CONFIG 
    ${DOLLAR}{KWCommon_INSTALL_PREFIX}${KWCommon_INSTALL_LIB_DIR})
ENDIF(WIN32)

# The include directories.
SET(KWCommon_INCLUDE_DIRS_CONFIG
  ${DOLLAR}{KWCommon_INSTALL_PREFIX}${KWCommon_INSTALL_INCLUDE_DIR})

# The library dependencies file.
SET(KWCommon_LIBRARY_DEPENDS_FILE 
  ${DOLLAR}{KWCommon_INSTALL_PREFIX}${KWCommon_INSTALL_PACKAGE_DIR}/KWCommonLibraryDepends.cmake)

# The CMake macros dir.
SET(KWCommon_CMAKE_DIR_CONFIG 
  ${DOLLAR}{KWCommon_INSTALL_PREFIX}${KWCommon_INSTALL_PACKAGE_DIR}/CMake)

# The VTK options.
# If VTK_BINARY_DIR is set, then KWCommon is part of a larger parent project
# that also builds VTK. In that case, KWCommon_VTK_INSTALL_PACKAGE_DIR
# should be set to the value of VTK_INSTALL_PACKAGE_DIR. Otherwise, if not set,
# then we are using an outside VTK whose location will not change when we
# install.
IF(VTK_BINARY_DIR)
  IF(NOT KWCommon_VTK_INSTALL_PACKAGE_DIR)
    MESSAGE(
      "KWCommon_VTK_INSTALL_PACKAGE_DIR must be set by the parent project to the value of VTK_INSTALL_PACKAGE_DIR it uses to configure VTK.")
  ENDIF(NOT KWCommon_VTK_INSTALL_PACKAGE_DIR)
  SET(KWCommon_VTK_DIR_CONFIG ${KWCommon_VTK_INSTALL_PACKAGE_DIR})
ELSE(VTK_BINARY_DIR)
  SET(KWCommon_VTK_DIR_CONFIG ${VTK_DIR})
ENDIF(VTK_BINARY_DIR)

# The build configuration information.
# The install tree only has one configuration.
SET(KWCommon_CONFIGURATION_TYPES_CONFIG)

# Configure KWCommonConfig.cmake for the install tree.

# Construct the proper number of GET_FILENAME_COMPONENT(... PATH)
# calls to compute the installation prefix from KWCommon_DIR.
STRING(REGEX REPLACE "/" ";" KWCommon_INSTALL_PACKAGE_DIR_COUNT
  "${KWCommon_INSTALL_PACKAGE_DIR}")
SET(KWCommon_CONFIG_INSTALL_ONLY "
# Compute the installation prefix from KWCommon_DIR.
SET(KWCommon_INSTALL_PREFIX \"${DOLLAR}{KWCommon_DIR}\")
")
FOREACH(p ${KWCommon_INSTALL_PACKAGE_DIR_COUNT})
  SET(KWCommon_CONFIG_INSTALL_ONLY
    "${KWCommon_CONFIG_INSTALL_ONLY}GET_FILENAME_COMPONENT(KWCommon_INSTALL_PREFIX \"${DOLLAR}{KWCommon_INSTALL_PREFIX}\" PATH)\n"
    )
ENDFOREACH(p)

IF(CMAKE_CONFIGURATION_TYPES)
  # There are multiple build configurations.  Configure one
  # KWCommonConfig.cmake for each configuration.
  FOREACH(config ${CMAKE_CONFIGURATION_TYPES})
    SET(KWCommon_BUILD_TYPE_CONFIG ${config})
    CONFIGURE_FILE(
      ${CMAKE_CURRENT_SOURCE_DIR}/KWCommonConfig.cmake.in
      ${CMAKE_CURRENT_BINARY_DIR}/${config}/KWCommonConfig.cmake
      @ONLY IMMEDIATE)
  ENDFOREACH(config)

  # Install the config file corresponding to the build configuration
  # specified when building the install target.  The BUILD_TYPE variable
  # will be set while CMake is processing the install files.
  IF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)
    INSTALL_FILES(${KWCommon_INSTALL_PACKAGE_DIR} FILES
      ${CMAKE_CURRENT_BINARY_DIR}/${DOLLAR}{BUILD_TYPE}/KWCommonConfig.cmake)
  ENDIF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)
ELSE(CMAKE_CONFIGURATION_TYPES)
  # There is only one build configuration. Configure one KWCommonConfig.cmake.
  SET(KWCommon_BUILD_TYPE_CONFIG ${CMAKE_BUILD_TYPE})
  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/KWCommonConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KWCommonConfig.cmake @ONLY IMMEDIATE)

  # Setup an install rule for the config file.
  IF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)
    INSTALL_FILES(${KWCommon_INSTALL_PACKAGE_DIR} FILES
      ${CMAKE_CURRENT_BINARY_DIR}/KWCommonConfig.cmake)
  ENDIF(NOT KWCommon_INSTALL_NO_DEVELOPMENT)
ENDIF(CMAKE_CONFIGURATION_TYPES)
