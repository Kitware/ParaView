set(Module_SRCS)

file(GLOB cmake_files "${CMAKE_CURRENT_SOURCE_DIR}/*.catalyst.cmake")

foreach (cmake_file IN LISTS cmake_files)
  include("${cmake_file}")
endforeach ()

set_source_files_properties(
  vtkRenderState
  WRAP_EXCLUDE)

vtk_module_library(vtkRenderingCore ${Module_SRCS})
