set(Module_SRCS)

file(GLOB cmake_files "${CMAKE_CURRENT_SOURCE_DIR}/*.catalyst.cmake")

foreach (cmake_file IN LISTS cmake_files)
  include("${cmake_file}")
endforeach ()

#set (${vtk-module}_HDRS vtkStreamingPriorityQueue.h)

vtk_module_library(vtkPVClientServerCoreRendering ${Module_SRCS})
