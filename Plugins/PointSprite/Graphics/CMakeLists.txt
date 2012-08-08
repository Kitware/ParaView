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

set_source_files_properties(
  vtk1DTransferFunction.cxx
  ABSTRACT
)

# -----------------------------------------------------------------------------
# Create the library
# -----------------------------------------------------------------------------
vtk_module_library(${vtk-module} ${KIT_SRCS})
