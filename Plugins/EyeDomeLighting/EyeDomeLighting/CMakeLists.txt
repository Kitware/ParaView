SET( SRC_FILES
   vtkDepthImageProcessingPass.cxx
   vtkEDLShading.cxx
   )

# -----------------------------------------------------------------------------
set(GLSL_RESOURCES_DIR
    resources/Shaders
    )

set(GLSL_SRCS
  edl_compose
  edl_shade
  bilateral_filter
  depth_compose
  )

# -----------------------------------------------------------------------------
# Create custom commands to encode each glsl file into a C string literal
# in a header file
# -----------------------------------------------------------------------------

set (${vtk-module}_HDRS)
foreach(file ${GLSL_SRCS})
  set(src ${CMAKE_CURRENT_SOURCE_DIR}/${GLSL_RESOURCES_DIR}/${file}.glsl)
  set(res ${CMAKE_CURRENT_BINARY_DIR}/${file}.cxx)
  add_custom_command(
    OUTPUT ${res}
    DEPENDS ${src} vtkEncodeString
    COMMAND vtkEncodeString
    ARGS ${res} ${src} ${file}
    )
  list(APPEND ${vtk-module}_HDRS ${res})
endforeach()

vtk_module_library(vtkEyeDomeLighting ${SRC_FILES})
include(vtkOpenGL)
vtk_opengl_link(vtkEyeDomeLighting)
