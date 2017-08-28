set (__dependencies)
if(PARAVIEW_USE_PISTON)
  list(APPEND __dependencies vtkAcceleratorsPiston)
endif()
if(PARAVIEW_USE_OSPRAY)
  list(APPEND __dependencies vtkRenderingOSPRay)
endif()
if(PARAVIEW_USE_OPENTURNS)
  list(APPEND __dependencies vtkFiltersOpenTurns)
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
    vtkRenderingVolumeOpenGL2
    vtkViewsContext2D
    vtkViewsCore
    ${__dependencies}
  PRIVATE_DEPENDS
    vtkAcceleratorsVTKm
    vtksys
    vtkzlib
  TEST_LABELS
    PARAVIEW
  KIT
    vtkPVClientServer
)
