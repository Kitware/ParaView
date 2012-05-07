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

set(KIT_EXTRA_SRCS
  )
  
# -----------------------------------------------------------------------------
# List the source files that should not be wrapped.
# -----------------------------------------------------------------------------
set_source_files_properties(
  vtk1DTransferFunction.cxx
  ABSTRACT
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
    DEPENDS ${src}
    COMMAND ${VTK_ENCODESTRING_EXE}
    ARGS ${res} ${src} ${file}
    )
  set(KIT_EXTRA_SRCS ${KIT_EXTRA_SRCS} ${res})
endforeach(file)

# -----------------------------------------------------------------------------
# List the kits from VTK that are needed by this project
# -----------------------------------------------------------------------------
set(KIT_LIBS
  vtkFiltersHybrid
  vtkRenderingOpenGL
  vtkImagingCore
  vtkInteractionStyle
  vtkRenderingFreeTypeOpenGL
  ${OPENGL_gl_LIBRARY}
  )

if(VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)
  set(KIT_LIBS ${KIT_LIBS} ${OSMESA_LIBRARY} )
endif(VTK_OPENGL_HAS_OSMESA AND OSMESA_LIBRARY)

# -----------------------------------------------------------------------------
# Create the library
# -----------------------------------------------------------------------------
add_library(PointSprite_Rendering ${KIT_SRCS} ${KIT_EXTRA_SRCS})
target_link_libraries(PointSprite_Rendering ${KIT_LIBS})

if (PV_INSTALL_BIN_DIR)
  install(TARGETS PointSprite_Rendering
    RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
    LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
    ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development
    )
else (PV_INSTALL_BIN_DIR)
  install(TARGETS PointSprite_Rendering
    RUNTIME DESTINATION bin COMPONENT Runtime
    LIBRARY DESTINATION lib COMPONENT Runtime
    ARCHIVE DESTINATION lib COMPONENT Development
    )
endif (PV_INSTALL_BIN_DIR)

# -----------------------------------------------------------------------------
# This make it easy for other projects to get the list of files etc. in this
# kit.
# -----------------------------------------------------------------------------
# needed by vtkExportKit.cmake
include(${ParaView_CMAKE_DIR}/VTKMono/vtkExportKit.cmake)
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  STRING(REGEX REPLACE "^/" "" VTK_INSTALL_PACKAGE_DIR_CM24 "${VTK_INSTALL_PACKAGE_DIR}")
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

vtk_export_kit2(
  "PointSprite_Rendering" 
  "POINTSPRITE_RENDERING" 
  ${CMAKE_CURRENT_BINARY_DIR} 
  "${KIT_SRCS}"
)
