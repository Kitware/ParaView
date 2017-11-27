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

foreach(file ${GLSL_SRCS})
  vtk_encode_string(
    INPUT         "${GLSL_RESOURCES_DIR}/${file}.glsl"
    NAME          "${file}"
    SOURCE_OUTPUT source)
  list(APPEND SRC_FILES ${source})
endforeach()

vtk_module_library(vtkEyeDomeLighting ${SRC_FILES})
include(vtkOpenGL)
vtk_opengl_link(vtkEyeDomeLighting)
