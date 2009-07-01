# -----------------------------------------------------------------------------
# Set of basic source files (not the OpenGL or GLSL ones)
# -----------------------------------------------------------------------------
set(KIT_SRCS
  vtkDepthSortPainter.cxx
  vtkPointSpriteDefaultPainter.cxx
  vtkTwoScalarsToColorsPainter.cxx
  vtkPointSpriteProperty.cxx
  vtkImageSpriteSource.cxx
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
  vtkHybrid
  vtkRendering
  vtkImaging
  )

# -----------------------------------------------------------------------------
# Create the library
# -----------------------------------------------------------------------------
add_library(CSCS_PointSprite_Rendering ${KIT_SRCS} ${KIT_EXTRA_SRCS})
target_link_libraries(CSCS_PointSprite_Rendering ${KIT_LIBS})

if (PV_INSTALL_BIN_DIR)
  install(TARGETS CSCS_PointSprite_Rendering
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${PV_INSTALL_BIN_DIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${PV_INSTALL_LIB_DIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/${PV_INSTALL_LIB_DIR}
    )
else (PV_INSTALL_BIN_DIR)
  install(TARGETS CSCS_PointSprite_Rendering
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )
endif (PV_INSTALL_BIN_DIR)

# -----------------------------------------------------------------------------
# This make it easy for other projects to get the list of files etc. in this
# kit.
# -----------------------------------------------------------------------------
# needed by vtkExportKit.cmake
include(${VTK_CMAKE_DIR}/vtkExportKit.cmake)
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  STRING(REGEX REPLACE "^/" "" VTK_INSTALL_PACKAGE_DIR_CM24 "${VTK_INSTALL_PACKAGE_DIR}")
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

vtk_export_kit2(
  "CSCS_PointSprite_Rendering" 
  "POINTSPRITE_RENDERING" 
  ${CMAKE_CURRENT_BINARY_DIR} 
  "${KIT_SRCS}"
)
