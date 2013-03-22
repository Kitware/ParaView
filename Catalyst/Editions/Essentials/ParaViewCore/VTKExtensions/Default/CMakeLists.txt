set(Module_SRCS
  vtkFileSeriesWriter.cxx
  vtkPVArrayCalculator.cxx
)

# until VTK comes up with a cleaner mechanism, we set the hints like so.
set (old_VTK_WRAP_HINTS ${VTK_WRAP_HINTS})
set (VTK_WRAP_HINTS ${CMAKE_CURRENT_SOURCE_DIR}/hints)
vtk_module_library(vtkPVVTKExtensionsDefault ${Module_SRCS})
set (VTK_WRAP_HINTS ${VTK_WRAP_HINTS})
