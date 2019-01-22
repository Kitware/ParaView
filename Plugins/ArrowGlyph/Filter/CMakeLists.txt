set(classes
  vtkArrowGlyphFilter)

vtk_module_add_module(ArrowGlyph::ArrowGlyphFilter
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  vtkArrowGlyphFilter.xml)
