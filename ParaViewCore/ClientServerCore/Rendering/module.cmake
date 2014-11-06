set (__dependencies)
if(PARAVIEW_USE_PISTON)
  list(APPEND __dependencies vtkAcceleratorsPiston)
endif()
if("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL")
  list(APPEND __dependencies vtkWebGLExporter vtkRenderingVolumeAMR)
endif()

vtk_module(vtkPVClientServerCoreRendering
  GROUPS
    ParaViewRendering
  DEPENDS
    vtkDomainsChemistry
    vtkFiltersAMR
    vtkPVClientServerCoreCore
    vtkPVVTKExtensionsDefault
    vtkPVVTKExtensionsRendering
    vtkRenderingLabel
    vtkRenderingVolume${VTK_RENDERING_BACKEND}
    vtkViewsContext2D
    vtkViewsCore
    ${__dependencies}
  PRIVATE_DEPENDS
    vtksys
    vtkzlib
  TEST_LABELS
    PARAVIEW
  KIT
    vtkPVClientServer
)
