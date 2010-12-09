cmake_minimum_required(VERSION 2.6)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake ${CMAKE_MODULE_PATH})

FIND_PACKAGE(VTK REQUIRED)
IF(NOT VTK_USE_RENDERING)
  MESSAGE(FATAL_ERROR "Example ${PROJECT_NAME} requires VTK_USE_RENDERING.")
ENDIF(NOT VTK_USE_RENDERING)
INCLUDE(${VTK_USE_FILE})

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
set(EXTRA_SRCS)

foreach(file ${GLSL_SRCS})
  set(src ${CMAKE_CURRENT_SOURCE_DIR}/${GLSL_RESOURCES_DIR}/${file}.glsl)
  set(res ${CMAKE_CURRENT_BINARY_DIR}/${file}.cxx)
  add_custom_command(
    OUTPUT ${res}
    DEPENDS ${src}
    COMMAND ${VTK_ENCODESTRING_EXE}
    ARGS ${res} ${src} ${file}
    )
  set(EXTRA_SRCS ${EXTRA_SRCS} ${res})
endforeach(file)

add_library(OffscreenShading SHARED ${SRC_FILES} ${EXTRA_SRCS})
target_link_libraries(OffscreenShading vtkRendering ${OPENGL_gl_LIBRARY})

install( TARGETS OffscreenShading
   RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR} COMPONENT Runtime
   LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
   ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Development
   )

include(${VTK_CMAKE_DIR}/vtkExportKit.cmake)
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  STRING(REGEX REPLACE "^/" "" VTK_INSTALL_PACKAGE_DIR_CM24 "${VTK_INSTALL_PACKAGE_DIR}")
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

vtk_export_kit2(
  "OffscreenShading"
  "OFFSCREEN_SHADING"
  ${CMAKE_CURRENT_BINARY_DIR}
  "${SRC_FILES}"
)
