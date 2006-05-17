#
# This module is provided as KWCommon_USE_FILE by KWCommonConfig.cmake.
# It can be INCLUDEd in a project to load the needed compiler and linker
# settings to use KWCommon:
#   FIND_PACKAGE(KWCommon REQUIRED)
#   INCLUDE(${KWCommon_USE_FILE})

IF(NOT KWCommon_USE_FILE_INCLUDED)
  SET(KWCommon_USE_FILE_INCLUDED 1)

  # Load the compiler settings used for KWCommon.
  IF(KWCommon_BUILD_SETTINGS_FILE)
    INCLUDE(${CMAKE_ROOT}/Modules/CMakeImportBuildSettings.cmake)
    CMAKE_IMPORT_BUILD_SETTINGS(${KWCommon_BUILD_SETTINGS_FILE})
  ENDIF(KWCommon_BUILD_SETTINGS_FILE)

  # Add compiler flags needed to use KWCommon.
  SET(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} ${KWCommon_REQUIRED_C_FLAGS}")
  SET(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} ${KWCommon_REQUIRED_CXX_FLAGS}")
  SET(CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} ${KWCommon_REQUIRED_EXE_LINKER_FLAGS}")
  SET(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} ${KWCommon_REQUIRED_SHARED_LINKER_FLAGS}")
  SET(CMAKE_MODULE_LINKER_FLAGS
    "${CMAKE_MODULE_LINKER_FLAGS} ${KWCommon_REQUIRED_MODULE_LINKER_FLAGS}")

  # Add include directories needed to use KWCommon.
  INCLUDE_DIRECTORIES(${KWCommon_INCLUDE_DIRS})

  # Add link directories needed to use KWCommon.
  LINK_DIRECTORIES(${KWCommon_LIBRARY_DIRS})

  # Add cmake module path.
  SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${KWCommon_CMAKE_DIR}")

  # Use VTK.
  IF(NOT KWCommon_NO_USE_VTK)
    SET(VTK_DIR ${KWCommon_VTK_DIR})
    FIND_PACKAGE(VTK)
    IF(VTK_FOUND)
      INCLUDE(${VTK_USE_FILE})
    ELSE(VTK_FOUND)
      MESSAGE("VTK not found in KWCommon_VTK_DIR=\"${KWCommon_VTK_DIR}\".")
    ENDIF(VTK_FOUND)
  ENDIF(NOT KWCommon_NO_USE_VTK)

ENDIF(NOT KWCommon_USE_FILE_INCLUDED)
