set(sources
  vtkPVPythonInterpreterPath.cxx)

vtk_module_add_module(ParaView::PythonInterpreterPath
  SOURCES ${sources})

vtk_module_link(ParaView::PythonInterpreterPath
  PRIVATE
    ParaView::pvpythonmodules
    ParaView::pvincubatorpythonmodules
    VTK::vtkpythonmodules)
