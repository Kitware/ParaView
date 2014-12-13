include_directories(
  ${VTK_INCLUDE_DIRS})

add_paraview_plugin(ArrowGlyph "1.0" 

  SERVER_MANAGER_XML 
    vtkArrowGlyphFilter.xml
  SERVER_MANAGER_SOURCES
    vtkArrowGlyphFilter.cxx
)
