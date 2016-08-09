# This plugin demonstrates how to add a new representation to ParaView that
# employs hardware shaders. The shaders are employed by the
# vtkVisibleLinesPainter which is initialized by the
# vtkGeometryRepresentationWithHiddenLinesRemoval.
# This plugin add a new representation-type
# "Visible Wireframe" when showing polydata in a 3D view.

# Note that this plugin does not work in parallel since it relies on composited
# Z-buffer and currently there's not support to obtain a composited Z-buffer
# during the rendering stage.

cmake_minimum_required(VERSION 3.3)

if (NOT ParaView_BINARY_DIR)
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()

# Set a consistent MACOSX_RPATH default across all CMake versions.
# When CMake 2.8.12 is required, change this default to 1.
# When CMake 3.0.0 is required, remove this block (see CMP0042).
if(NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()

include(ParaViewPlugins)

if (VTK_RENDERING_BACKEND STREQUAL "OpenGL")

# Compile-in all GLSL files are strings.
# const char* with the names same as that of the file then become available for
# use.
encode_files_as_strings(ENCODED_STRING_FILES
  vtkPVLightingHelper_s.glsl
  vtkPVColorMaterialHelper_vs.glsl
  vtkVisibleLinesPainter_fs.glsl
  vtkVisibleLinesPainter_vs.glsl
  )

add_paraview_plugin(
  HiddenLinesRemoval "1.0"
  SERVER_MANAGER_XML
          HiddenLinesRemovalPlugin.xml

  SERVER_MANAGER_SOURCES
          vtkGeometryRepresentationWithHiddenLinesRemoval.cxx

  SOURCES vtkPVColorMaterialHelper.cxx
          vtkPVLightingHelper.cxx
          vtkVisibleLinesPainter.cxx
          ${ENCODED_STRING_FILES}
)

find_package(OpenGL REQUIRED)
include_directories(SYSTEM ${OPENGL_INCLUDE_DIR})
target_link_libraries(HiddenLinesRemoval LINK_PRIVATE ${OPENGL_LIBRARIES})

endif()
