set(modules
  Prism::vtkPrismServerManager
  )

set(module_files
  "${CMAKE_CURRENT_SOURCE_DIR}/ServerManager/vtk.module"
  )

paraview_add_plugin(Prism
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "2.0"
  MODULES ${modules}
  MODULE_FILES ${module_files}
  XML_DOCUMENTATION OFF
  )
