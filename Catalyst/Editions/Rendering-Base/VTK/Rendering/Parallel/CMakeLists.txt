set(Module_SRCS)

file(GLOB cmake_files "${CMAKE_CURRENT_SOURCE_DIR}/*.catalyst.cmake")

foreach (cmake_file IN LISTS cmake_files)
  include("${cmake_file}")
endforeach ()

include(vtkOpenGL)
vtk_module_library(vtkRenderingParallel ${Module_SRCS})
vtk_opengl_link(vtkRenderingParallel)
