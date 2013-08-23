vtk_module(vtkPVVTKExtensionsDefault
  DEPENDS
    vtkPVVTKExtensionsCore
    ${_dependencies}
  PRIVATE_DEPENDS
    vtksys
)
