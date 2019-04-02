set(classes
  vtkExplicitStructuredGridGeneratorSource
)

if(PARAVIEW_ENABLE_PYTHON)
  list(APPEND classes vtkExplicitStructuredGridPythonExtractor)
endif()

vtk_module_add_module(ExplicitStructuredGrid::vtkExplicitStructuredGrid
    CLASSES ${classes}
)
