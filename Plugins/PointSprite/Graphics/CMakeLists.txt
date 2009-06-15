# -----------------------------------------------------------------------------
# Set of basic source files (not the OpenGL or GLSL ones)
# -----------------------------------------------------------------------------
set(KIT_SRCS
  vtkCellPointsFilter.cxx
  vtk1DLookupTableTransferFunction.cxx
  vtk1DTransferFunction.cxx
  vtk1DTransferFunctionChooser.cxx
  vtk1DTransferFunctionFilter.cxx
  vtk1DGaussianTransferFunction.cxx
  )


# -----------------------------------------------------------------------------
# List the kits from VTK that are needed by this project
# -----------------------------------------------------------------------------
set(KIT_LIBS
  vtkGraphics
  )

# -----------------------------------------------------------------------------
# Create the library
# -----------------------------------------------------------------------------
add_library(CSCS_PointSprite_Graphics ${KIT_SRCS})
target_link_libraries(CSCS_PointSprite_Graphics ${KIT_LIBS})

# -----------------------------------------------------------------------------
# This make it easy for other projects to get the list of files etc. in this
# kit.
# -----------------------------------------------------------------------------
# needed by vtkExportKit.cmake
include(${VTK_CMAKE_DIR}/vtkExportKit.cmake)
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  STRING(REGEX REPLACE "^/" "" VTK_INSTALL_PACKAGE_DIR_CM24 "${VTK_INSTALL_PACKAGE_DIR}")
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

vtk_export_kit2(
  "CSCS_PointSprite_Graphics" 
  "POINTSPRITE_GRAPHICS" 
  ${CMAKE_CURRENT_BINARY_DIR} 
  "${KIT_SRCS}"
)
