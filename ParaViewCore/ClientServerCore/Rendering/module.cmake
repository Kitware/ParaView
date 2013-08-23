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
    vtkPVVTKExtensionsWebGLExporter
    vtkRenderingLabel
    vtkRenderingVolumeAMR
    vtkRenderingVolumeOpenGL
    vtkViewsContext2D
    vtkViewsCore
    ${__dependencies}
  PRIVATE_DEPENDS
    vtksys
  TEST_LABELS
    PARAVIEW
)
