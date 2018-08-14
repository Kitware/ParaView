set (__dependencies)
set (__private_dependencies)
if (PARAVIEW_USE_PISTON)
  list (APPEND __dependencies vtkAcceleratorsPiston)
endif()
if(PARAVIEW_USE_VTKM)
  list(APPEND __private_dependencies
    vtkAcceleratorsVTKm
    vtkm
  )
endif()

vtk_module(vtkPVClientServerCoreRendering
  GROUPS
    ParaViewRendering
  DEPENDS
    #vtkDomainsChemistry
    #vtkFiltersAMR
    vtkPVClientServerCoreCore
    vtkPVVTKExtensionsDefault
    vtkPVVTKExtensionsRendering
    #vtkWebGLExporter
    vtkRenderingLabel
    #vtkRenderingVolumeAMR
    #vtkRenderingVolumeOpenGL
    vtkViewsContext2D
    vtkViewsCore
    vtkjsoncpp
    ${__dependencies}
  PRIVATE_DEPENDS
    vtksys
    vtkzlib
    ${__private_dependencies}
  TEST_LABELS
    PARAVIEW
  KIT
    vtkPVClientServer
)
