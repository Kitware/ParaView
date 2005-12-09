#
# This module is provided as KWWidgets_USE_FILE by KWWidgetsConfig.cmake.  
# It can be INCLUDEd in a project to load the needed compiler and linker
# settings to use KWWidgets:
#   FIND_PACKAGE(KWWidgets REQUIRED)
#   INCLUDE(${KWWidgets_USE_FILE})
#

IF(NOT KWWidgets_USE_FILE_INCLUDED)
  SET(KWWidgets_USE_FILE_INCLUDED 1)

  # Load the compiler settings used for KWWidgets.
  IF(KWWidgets_BUILD_SETTINGS_FILE)
    INCLUDE(${CMAKE_ROOT}/Modules/CMakeImportBuildSettings.cmake)
    CMAKE_IMPORT_BUILD_SETTINGS(${KWWidgets_BUILD_SETTINGS_FILE})
  ENDIF(KWWidgets_BUILD_SETTINGS_FILE)

  # Add compiler flags needed to use KWWidgets.
  SET(CMAKE_C_FLAGS 
    "${CMAKE_C_FLAGS} ${KWWidgets_REQUIRED_C_FLAGS}")
  SET(CMAKE_CXX_FLAGS 
    "${CMAKE_CXX_FLAGS} ${KWWidgets_REQUIRED_CXX_FLAGS}")
  SET(CMAKE_EXE_LINKER_FLAGS 
    "${CMAKE_EXE_LINKER_FLAGS} ${KWWidgets_REQUIRED_EXE_LINKER_FLAGS}")
  SET(CMAKE_SHARED_LINKER_FLAGS 
    "${CMAKE_SHARED_LINKER_FLAGS} ${KWWidgets_REQUIRED_SHARED_LINKER_FLAGS}")
  SET(CMAKE_MODULE_LINKER_FLAGS 
    "${CMAKE_MODULE_LINKER_FLAGS} ${KWWidgets_REQUIRED_MODULE_LINKER_FLAGS}")

  # Add include directories needed to use KWWidgets.
  INCLUDE_DIRECTORIES(${KWWidgets_INCLUDE_DIRS})

  # Add link directories needed to use KWWidgets.
  LINK_DIRECTORIES(${KWWidgets_LIBRARY_DIRS})

  # Use VTK.
  IF(NOT KWWidgets_NO_USE_VTK)
    SET(VTK_DIR ${KWWidgets_VTK_DIR})
    FIND_PACKAGE(VTK)
    IF(VTK_FOUND)
      INCLUDE(${VTK_USE_FILE})
    ELSE(VTK_FOUND)
      MESSAGE("VTK not found in KWWidgets_VTK_DIR=\"${KWWidgets_VTK_DIR}\".")
    ENDIF(VTK_FOUND)
  ENDIF(NOT KWWidgets_NO_USE_VTK)

ENDIF(NOT KWWidgets_USE_FILE_INCLUDED)
