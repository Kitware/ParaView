paraview_add_plugin(ArrowGlyph
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES ArrowGlyph::ArrowGlyphFilter
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Filter/vtk.module")
