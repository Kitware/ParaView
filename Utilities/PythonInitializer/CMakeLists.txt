set(sources
  vtkPVPythonInitializer.cxx)

vtk_module_add_module(ParaView::PythonInitializer
  SOURCES ${sources})

# TODO: Frozen python support

vtk_module_link(ParaView::PythonInitializer
  PRIVATE
    ParaView::pvpythonmodules)
