set(Module_SRCS)

file(GLOB cmake_files "${CMAKE_CURRENT_SOURCE_DIR}/*.catalyst.cmake")

foreach (cmake_file IN LISTS cmake_files)
  include("${cmake_file}")
endforeach ()

set(shader_files vtkCompositeZPassFS.glsl)

foreach(file ${shader_files})
  get_filename_component(file_we ${file} NAME_WE)
  vtk_encode_string(
    INPUT         "${file}"
    NAME          "${file_we}"
    EXPORT_SYMBOL "VTKRENDERINGPARALLEL_EXPORT"
    EXPORT_HEADER "vtkRenderingParallelModule.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND Module_SRCS ${source})
endforeach()

add_definitions(-DVTK_OPENGL2)

include(vtkOpenGL)
vtk_module_library(vtkRenderingParallel ${Module_SRCS})
vtk_opengl_link(vtkRenderingParallel)
