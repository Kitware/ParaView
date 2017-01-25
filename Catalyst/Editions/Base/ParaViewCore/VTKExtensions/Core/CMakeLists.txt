set(Module_SRCS)

file(GLOB cmake_files "${CMAKE_CURRENT_SOURCE_DIR}/*.catalyst.cmake")

foreach (cmake_file IN LISTS cmake_files)
  include("${cmake_file}")
endforeach ()

# until VTK comes up with a cleaner mechanism, we set the hints like so.
set (old_VTK_WRAP_HINTS ${VTK_WRAP_HINTS})
set (VTK_WRAP_HINTS ${CMAKE_CURRENT_SOURCE_DIR}/hints)
vtk_module_library(vtkPVVTKExtensionsCore ${Module_SRCS})
set (VTK_WRAP_HINTS ${old_VTK_WRAP_HINTS})
