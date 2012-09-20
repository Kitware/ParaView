set (__dependencies)
if (PARAVIEW_USE_PISTON)
  list (APPEND __dependencies vtkAcceleratorsPiston)
endif()

vtk_module(vtkPVClientServerCoreRendering
  GROUPS
    ParaViewRendering
  DEPENDS
    vtkDomainsChemistry
    vtkFiltersAMR
    vtkPVClientServerCoreCore
    vtkPVVTKExtensionsRendering
    vtkRenderingLabel
    vtkViewsContext2D
    vtkViewsCore
    vtkRenderingVolumeOpenGL
    ${__dependencies}
)
