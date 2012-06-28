set (__dependencies)

if (PARAVIEW_USE_MPI)
  list (APPEND __dependencies
    vtkParallelMPI)
endif()

if (PARAVIEW_ENABLE_PYTHON)
  list(APPEND __dependencies
      vtkPVPythonSupport
      vtkFiltersProgrammable)
endif (PARAVIEW_ENABLE_PYTHON)

vtk_module(vtkPVClientServerCoreCore
  GROUPS
    ParaViewCore
  DEPENDS
    vtkPVVTKExtensionsCore
    vtkRenderingCore # needed for vtkMapper in vtkProcessModule. 
                     # we should fix that.
    ${__dependencies}
)
