set(classes
  vtkPVZSpaceView)

if (PARAVIEW_USE_PYTHON)
  list (APPEND classes 
    vtkPVZSpaceSettings)
endif ()

vtk_module_add_module(zSpace::vtkZSpaceView
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS ZSpaceView.xml)
