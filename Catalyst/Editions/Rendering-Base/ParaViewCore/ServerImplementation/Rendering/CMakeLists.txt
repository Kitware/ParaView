set(Module_SRCS)

file(GLOB cmake_files "${CMAKE_CURRENT_SOURCE_DIR}/*.catalyst.cmake")

foreach (cmake_file IN LISTS cmake_files)
  include("${cmake_file}")
endforeach ()

# Use a custom hints file for this module.
#set(${vtk-module}_WRAP_HINTS "${CMAKE_CURRENT_SOURCE_DIR}/hints")
vtk_module_library(vtkPVServerImplementationRendering ${Module_SRCS})
