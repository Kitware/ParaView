PROJECT(PVLocal)
INCLUDE_DIRECTORIES(${PVLocal_SOURCE_DIR})

# Import ParaView build settings.
FIND_PACKAGE(ParaView)
IF(NOT ParaView_FOUND)
  MESSAGE(FATAL_ERROR "ParaView is required.  Set ParaView_DIR.")
ENDIF(NOT ParaView_FOUND)
INCLUDE(${PARAVIEW_USE_FILE})

# Enforce library location.
SET(EXECUTABLE_OUTPUT_PATH "${PVLocal_BINARY_DIR}" CACHE INTERNAL "Executables")
SET(LIBRARY_OUTPUT_PATH "${PVLocal_BINARY_DIR}" CACHE INTERNAL "Libraries")

# List of classes for the plugin.
SET(PVLocal_SRCS vtkLocalConeSource.cxx)

# Create vtk client/server wrappers for the classes.
VTK_WRAP_ClientServer(PVLocal PVLocalCS_SRCS "${PVLocal_SRCS}")

# Build the package as a plugin for ParaView.
ADD_LIBRARY(PVLocal MODULE ${PVLocal_SRCS} ${PVLocalCS_SRCS})
TARGET_LINK_LIBRARIES(PVLocal vtkClientServer vtkGraphics)

# Place the package configuration file into the build tree.
CONFIGURE_FILE(${PVLocal_SOURCE_DIR}/PVLocal.xml.in
               ${PVLocal_BINARY_DIR}/PVLocal.xml @ONLY IMMEDIATE)
