set (__dependencies)

if (PARAVIEW_ENABLE_PYTHON)
  list(APPEND __dependencies vtkPVPythonSupport)
endif (PARAVIEW_ENABLE_PYTHON)

if (PARAVIEW_USE_ICE_T)
  list(APPEND __dependencies vtkicet)
endif()

vtk_module(vtkPVClientServerCore
  GROUPS
    ParaView
  DEPENDS
    vtkDomainsChemistry
    vtkPVVTKExtensions
    vtkRenderingLabel
    ${__dependencies}
)
