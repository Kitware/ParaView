# Generate the KWWidgetsConfig.cmake file in the build tree. Also configure
# one for installation. The file tells external projects how to use
# KWWidgets.

# Help store a literal dollar in a string.  CMake 2.2 allows escaped
# dollars but we have to support CMake 2.0.
SET(DOLLAR "$")

#-----------------------------------------------------------------------------
# Settings shared between the build tree and install tree.

EXPORT_LIBRARY_DEPENDENCIES(
  ${KWWidgets_BINARY_DIR}/KWWidgetsLibraryDepends.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/CMakeExportBuildSettings.cmake)
CMAKE_EXPORT_BUILD_SETTINGS(
  ${KWWidgets_BINARY_DIR}/KWWidgetsBuildSettings.cmake)

IF(NOT KWWidgets_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${KWWidgets_INSTALL_PACKAGE_DIR} FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/UseKWWidgets.cmake
    ${KWWidgets_BINARY_DIR}/KWWidgetsLibraryDepends.cmake
    ${KWWidgets_BINARY_DIR}/KWWidgetsBuildSettings.cmake
    )
ENDIF(NOT KWWidgets_INSTALL_NO_DEVELOPMENT)

#-----------------------------------------------------------------------------
# Settings specific to the build tree.

# The install-only section is empty for the build tree.
SET(KWWidgets_CONFIG_INSTALL_ONLY)

# The "use" file.
SET(KWWidgets_USE_FILE_CONFIG 
  ${CMAKE_CURRENT_SOURCE_DIR}/UseKWWidgets.cmake)

# The build settings file.
SET(KWWidgets_BUILD_SETTINGS_FILE_CONFIG 
  ${KWWidgets_BINARY_DIR}/KWWidgetsBuildSettings.cmake)

# The library directories.
SET(KWWidgets_LIBRARY_DIRS_CONFIG ${KWWidgets_LIBRARY_DIRS})

# The runtime directories.
SET(KWWidgets_RUNTIME_DIRS_CONFIG ${KWWidgets_RUNTIME_DIRS})

# The include directories.
SET(KWWidgets_INCLUDE_DIRS_CONFIG ${KWWidgets_INCLUDE_PATH})

# The library dependencies file.
SET(KWWidgets_LIBRARY_DEPENDS_FILE 
  ${KWWidgets_BINARY_DIR}/KWWidgetsLibraryDepends.cmake)

# The examples dir.
SET(KWWidgets_EXAMPLES_DIR_CONFIG 
  ${KWWidgets_EXAMPLES_DIR})

# The templates dir.
SET(KWWidgets_TEMPLATES_DIR_CONFIG 
  ${KWWidgets_TEMPLATES_DIR})

# The resources dir.
SET(KWWidgets_RESOURCES_DIR_CONFIG 
  ${KWWidgets_RESOURCES_DIR})

# The CMake macros dir.
SET(KWWidgets_CMAKE_DIR_CONFIG 
  ${KWWidgets_CMAKE_DIR})

# The Tcl/Tk options.
SET(KWWidgets_TCL_PACKAGE_INDEX_DIR_CONFIG ${KWWidgets_TCL_PACKAGE_INDEX_DIR})

# The Python options.
SET(KWWidgets_PYTHON_MODULE_DIR_CONFIG ${KWWidgets_PYTHON_MODULE_DIR})

# The Doxygen options.
SET(KWWidgets_DOXYGEN_DIR_CONFIG 
  ${KWWidgets_SOURCE_DIR}/Utilities/Doxygen)

# The VTK options.
SET(KWWidgets_VTK_DIR_CONFIG ${VTK_DIR})

# The build configuration information.
SET(KWWidgets_CONFIGURATION_TYPES_CONFIG ${KWWidgets_CONFIGURATION_TYPES})
SET(KWWidgets_BUILD_TYPE_CONFIG ${CMAKE_BUILD_TYPE})

# Configure KWWidgetsConfig.cmake for the build tree.
CONFIGURE_FILE(
  ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgetsConfig.cmake.in
  ${KWWidgets_BINARY_DIR}/KWWidgetsConfig.cmake @ONLY IMMEDIATE)

#-----------------------------------------------------------------------------
# Settings specific to the install tree.

# The "use" file.
SET(KWWidgets_USE_FILE_CONFIG 
  ${DOLLAR}{KWWidgets_INSTALL_PREFIX}${KWWidgets_INSTALL_PACKAGE_DIR}/UseKWWidgets.cmake)

# The build settings file.
SET(KWWidgets_BUILD_SETTINGS_FILE_CONFIG 
  ${DOLLAR}{KWWidgets_INSTALL_PREFIX}${KWWidgets_INSTALL_PACKAGE_DIR}/KWWidgetsBuildSettings.cmake)

# The library directories.
IF(CYGWIN AND KWWidgets_BUILD_SHARED_LIBS)
  # In Cygwin programs directly link to the .dll files.
  SET(KWWidgets_LIBRARY_DIRS_CONFIG 
    ${DOLLAR}{KWWidgets_INSTALL_PREFIX}${KWWidgets_INSTALL_BIN_DIR})
ELSE(CYGWIN AND KWWidgets_BUILD_SHARED_LIBS)
  SET(KWWidgets_LIBRARY_DIRS_CONFIG 
    ${DOLLAR}{KWWidgets_INSTALL_PREFIX}${KWWidgets_INSTALL_LIB_DIR})
ENDIF(CYGWIN AND KWWidgets_BUILD_SHARED_LIBS)

# The runtime directories.
IF(WIN32)
  SET(KWWidgets_RUNTIME_DIRS_CONFIG 
    ${DOLLAR}{KWWidgets_INSTALL_PREFIX}${KWWidgets_INSTALL_BIN_DIR})
ELSE(WIN32)
  SET(KWWidgets_RUNTIME_DIRS_CONFIG 
    ${DOLLAR}{KWWidgets_INSTALL_PREFIX}${KWWidgets_INSTALL_LIB_DIR})
ENDIF(WIN32)

# The include directories.
SET(KWWidgets_INCLUDE_DIRS_CONFIG
  ${DOLLAR}{KWWidgets_INSTALL_PREFIX}${KWWidgets_INSTALL_INCLUDE_DIR})

# The library dependencies file.
SET(KWWidgets_LIBRARY_DEPENDS_FILE 
  ${DOLLAR}{KWWidgets_INSTALL_PREFIX}${KWWidgets_INSTALL_PACKAGE_DIR}/KWWidgetsLibraryDepends.cmake)

# The examples dir.
SET(KWWidgets_EXAMPLES_DIR_CONFIG 
  ${DOLLAR}{KWWidgets_INSTALL_PREFIX}${KWWidgets_INSTALL_DATA_DIR}/Examples)

# The templates dir.
SET(KWWidgets_TEMPLATES_DIR_CONFIG 
 ${DOLLAR}{KWWidgets_INSTALL_PREFIX}${KWWidgets_INSTALL_INCLUDE_DIR}/Templates)

# The resources dir.
SET(KWWidgets_RESOURCES_DIR_CONFIG 
  ${DOLLAR}{KWWidgets_INSTALL_PREFIX}${KWWidgets_INSTALL_DATA_DIR}/Resources)

# The CMake macros dir.
SET(KWWidgets_CMAKE_DIR_CONFIG 
  ${DOLLAR}{KWWidgets_INSTALL_PREFIX}${KWWidgets_INSTALL_PACKAGE_DIR}/CMake)

# The Tcl/Tk options.
SET(KWWidgets_TCL_PACKAGE_INDEX_DIR_CONFIG 
  ${DOLLAR}{KWWidgets_INSTALL_PREFIX}${KWWidgets_INSTALL_LIB_DIR}/tcl)

# The Python options.
SET(KWWidgets_PYTHON_MODULE_DIR_CONFIG 
  ${DOLLAR}{KWWidgets_INSTALL_PREFIX}${KWWidgets_INSTALL_LIB_DIR}/../site-packages)

# The Doxygen options.
SET(KWWidgets_DOXYGEN_DIR_CONFIG 
  ${DOLLAR}{KWWidgets_INSTALL_PREFIX}${KWWidgets_INSTALL_DOXYGEN_DIR})

# The VTK options.
# If VTK_BINARY_DIR is set, then KWWidgets is part of a larger parent project
# that also builds VTK. In that case, KWWidgets_VTK_INSTALL_PACKAGE_DIR
# should be set to the value of VTK_INSTALL_PACKAGE_DIR. Otherwise, if not set,
# then we are using an outside VTK whose location will not change when we
# install.
IF(VTK_BINARY_DIR)
  IF(NOT KWWidgets_VTK_INSTALL_PACKAGE_DIR)
    MESSAGE(
      FATAL_ERROR 
      "KWWidgets_VTK_INSTALL_PACKAGE_DIR must be set by the parent project to the value of VTK_INSTALL_PACKAGE_DIR it uses to configure VTK.")
  ENDIF(NOT KWWidgets_VTK_INSTALL_PACKAGE_DIR)
  SET(KWWidgets_VTK_DIR_CONFIG ${KWWidgets_VTK_INSTALL_PACKAGE_DIR})
ELSE(VTK_BINARY_DIR)
  SET(KWWidgets_VTK_DIR_CONFIG ${VTK_DIR})
ENDIF(VTK_BINARY_DIR)

# The build configuration information.
# The install tree only has one configuration.
SET(KWWidgets_CONFIGURATION_TYPES_CONFIG)

# Configure KWWidgetsConfig.cmake for the install tree.

# Construct the proper number of GET_FILENAME_COMPONENT(... PATH)
# calls to compute the installation prefix from KWWidgets_DIR.
STRING(REGEX REPLACE "/" ";" KWWidgets_INSTALL_PACKAGE_DIR_COUNT
  "${KWWidgets_INSTALL_PACKAGE_DIR}")
SET(KWWidgets_CONFIG_INSTALL_ONLY "
# Compute the installation prefix from KWWidgets_DIR.
SET(KWWidgets_INSTALL_PREFIX \"${DOLLAR}{KWWidgets_DIR}\")
")
FOREACH(p ${KWWidgets_INSTALL_PACKAGE_DIR_COUNT})
  SET(KWWidgets_CONFIG_INSTALL_ONLY
    "${KWWidgets_CONFIG_INSTALL_ONLY}GET_FILENAME_COMPONENT(KWWidgets_INSTALL_PREFIX \"${DOLLAR}{KWWidgets_INSTALL_PREFIX}\" PATH)\n"
    )
ENDFOREACH(p)

IF(CMAKE_CONFIGURATION_TYPES)
  # There are multiple build configurations.  Configure one
  # KWWidgetsConfig.cmake for each configuration.
  FOREACH(config ${CMAKE_CONFIGURATION_TYPES})
    SET(KWWidgets_BUILD_TYPE_CONFIG ${config})
    CONFIGURE_FILE(
      ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgetsConfig.cmake.in
      ${CMAKE_CURRENT_BINARY_DIR}/${config}/KWWidgetsConfig.cmake
      @ONLY IMMEDIATE)
  ENDFOREACH(config)

  # Install the config file corresponding to the build configuration
  # specified when building the install target.  The BUILD_TYPE variable
  # will be set while CMake is processing the install files.
  IF(NOT KWWidgets_INSTALL_NO_DEVELOPMENT)
    INSTALL_FILES(${KWWidgets_INSTALL_PACKAGE_DIR} FILES
      ${CMAKE_CURRENT_BINARY_DIR}/${DOLLAR}{BUILD_TYPE}/KWWidgetsConfig.cmake)
  ENDIF(NOT KWWidgets_INSTALL_NO_DEVELOPMENT)
ELSE(CMAKE_CONFIGURATION_TYPES)
  # There is only one build configuration. Configure one KWWidgetsConfig.cmake.
  SET(KWWidgets_BUILD_TYPE_CONFIG ${CMAKE_BUILD_TYPE})
  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/KWWidgetsConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KWWidgetsConfig.cmake @ONLY IMMEDIATE)

  # Setup an install rule for the config file.
  IF(NOT KWWidgets_INSTALL_NO_DEVELOPMENT)
    INSTALL_FILES(${KWWidgets_INSTALL_PACKAGE_DIR} FILES
      ${CMAKE_CURRENT_BINARY_DIR}/KWWidgetsConfig.cmake)
  ENDIF(NOT KWWidgets_INSTALL_NO_DEVELOPMENT)
ENDIF(CMAKE_CONFIGURATION_TYPES)
