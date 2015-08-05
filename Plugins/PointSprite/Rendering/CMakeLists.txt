# -----------------------------------------------------------------------------
# Set of basic source files (not the OpenGL or GLSL ones)
# -----------------------------------------------------------------------------
set(KIT_SRCS
  vtkDepthSortPainter.cxx
  vtkPointSpriteDefaultPainter.cxx
  vtkTwoScalarsToColorsPainter.cxx
  vtkPointSpriteProperty.cxx
  vtkImageSpriteSource.cxx
  vtkPointSpriteCoincidentTopologyResolutionPainter.cxx
  )

# -----------------------------------------------------------------------------
# Set of GLSL source files
# -----------------------------------------------------------------------------
set(KIT_GLSL_RESOURCES_DIR
    Resources/Shaders
    )
set(KIT_GLSL_SRCS
  Texture_vs
  Quadrics_fs
  Quadrics_vs
  AttributeRadiusHelper
  FixedRadiusHelper
  )

# -----------------------------------------------------------------------------
# Create custom commands to encode each glsl file into a C string literal
# in a header file
# -----------------------------------------------------------------------------
foreach(file ${KIT_GLSL_SRCS})
  set(src ${CMAKE_CURRENT_SOURCE_DIR}/${KIT_GLSL_RESOURCES_DIR}/${file}.glsl)
  set(res ${CMAKE_CURRENT_BINARY_DIR}/${file}.cxx)
  add_custom_command(
    OUTPUT ${res}
    DEPENDS ${src} vtkEncodeString
    COMMAND vtkEncodeString
    ARGS ${res} ${src} ${file}
    )
  set(KIT_EXTRA_SRCS ${KIT_EXTRA_SRCS} ${res})
endforeach()

# -----------------------------------------------------------------------------
# Create the library
# -----------------------------------------------------------------------------
set (${vtk-module}_HDRS ${KIT_EXTRA_SRCS})
vtk_module_library(${vtk-module} ${KIT_SRCS})
include(vtkOpenGL)
vtk_opengl_link(${vtk-module})
