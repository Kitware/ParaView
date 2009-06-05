# -----------------------------------------------------------------------------
# Set the minimum required cmake version. Setting here to 2.4, but really 
# 2.7 is required for testing with bullseye coverage. Will change to 2.8 once
# it is officially released
# -----------------------------------------------------------------------------
project(PointSpritePlugin)

#CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

# -----------------------------------------------------------------------------
# Set cmake policy CMP0003 to NEW. This means cmake will not break up linked
# libraries into separate linker search paths and library names. 
# -----------------------------------------------------------------------------
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

# -----------------------------------------------------------------------------
# Disable deprecation warnings for standard C and STL functions in VS2005 and
# later (no, we don't need IF(CMAKE_COMPILER_2005) ... )
# -----------------------------------------------------------------------------
add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
add_definitions(-D_SCL_SECURE_NO_DEPRECATE)

#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
IF (ParaView_SOURCE_DIR)
  INCLUDE(${QT_USE_FILE}) 
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    ${PARAVIEW_KWSYS_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIR}
    ${MPI_INCLUDE_PATH}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)

# -----------------------------------------------------------------------------
# Add an option for building shared - default to the same choice made for VTK
# BUILD_SHARED_LIBS is a builtin cmake variable.
# -----------------------------------------------------------------------------
SET(BUILD_SHARED_LIBS ${PARAVIEW_BUILD_SHARED_LIBS})

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/Rendering
  ${CMAKE_CURRENT_SOURCE_DIR}/Graphics
  ${PROJECT_BINARY_DIR}
)

add_subdirectory(Rendering)
add_subdirectory(Graphics)

# -----------------------------------------------------------------------------
# Build the tests
# -----------------------------------------------------------------------------
set(PointSpritePlugin_BUILD_EXAMPLES ON CACHE BOOL "Build the example.")
if(PointSpritePlugin_BUILD_EXAMPLES)
    add_subdirectory(Examples)
endif(PointSpritePlugin_BUILD_EXAMPLES)

# -----------------------------------------------------------------------------
# Build the Paraview plugins
# -----------------------------------------------------------------------------
add_subdirectory(ParaViewPlugin)


